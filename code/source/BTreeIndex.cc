


#include "../headers/BTreeIndex.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

using namespace std;

BTreeIndex::BTreeIndex(Catalog* catalog)
    : rootPageId(0), nextPageId(0), catalog(catalog) {
    IndexPage root;
    root.pageId = nextPageId++;
    root.pageType = IndexPage::LEAF;
    root.numKeys = 0;
    index[root.pageId] = root;
}

void BTreeIndex::Build(const string& indexName,
                       const string& tableName,
                       const string& attributeName) {
    this->indexFileName = indexName;
    this->tableName = tableName;
    this->attributeName = attributeName;

    cout << "DEBUG: Building index from table: " << tableName
         << " on attribute: " << attributeName << endl;

    File dataFile;
    Page page;
    Record rec;

    string tablePath = "../data/" + tableName + ".heap";
    dataFile.Open(1, const_cast<char*>(tablePath.c_str()));
    off_t numPages = dataFile.GetLength();
    int recordCounter = 0;

    for (off_t i = 0; i < numPages; ++i) {
        dataFile.GetPage(page, i);
        while (page.GetFirst(rec)) {
            int key = ExtractKey(rec);
            Insert(key, recordCounter++);
        }
    }

    PrintPageStats();
    Write();
    PrintIndexStructure();
    ExportDOT("btree.dot");
    dataFile.Close();
}

int BTreeIndex::ExtractKey(Record& rec) {
    Schema schema;
    SString sTable(tableName);
    SString sAttr(attributeName);
    if (!catalog->GetSchema(sTable, schema)) {
        cerr << "ERROR: Table not found in catalog: " << tableName << endl;
        exit(1);
    }
    int attIndex = schema.Index(sAttr);
    if (attIndex < 0) {
        cerr << "ERROR: Attribute not found: " << attributeName << endl;
        exit(1);
    }
    Type attType = schema.FindType(sAttr);
    char* columnPtr = rec.GetColumn(attIndex);
    if (attType == Integer) {
        int key;
        memcpy(&key, columnPtr, sizeof(int));
        return key;
    } else {
        cerr << "ERROR: Only INTEGER keys are supported in B+ tree index." << endl;
        exit(1);
    }
}

void BTreeIndex::Insert(int key, int pointer) {
    cout << "DEBUG: Inserting key = " << key
         << " pointing to record = " << pointer << endl;

    int promotedKey = -1;
    int newPageId = -1;
    bool split = InsertRecursive(rootPageId, key, pointer,
                                 promotedKey, newPageId);

    if (split) {
        IndexPage newRoot;
        newRoot.pageType = IndexPage::INTERNAL;
        newRoot.pageId = nextPageId++;
        newRoot.numKeys = 1;
        newRoot.keys.push_back(promotedKey);
        newRoot.pointers.push_back(rootPageId);
        newRoot.pointers.push_back(newPageId);
        index[newRoot.pageId] = newRoot;
        rootPageId = newRoot.pageId;
        cout << "DEBUG: Root split. New root pageId = " << rootPageId
             << ", promoted key = " << promotedKey << endl;
    }

    ValidateTree();
}

bool BTreeIndex::InsertRecursive(int pageId,
                                 int key,
                                 int pointer,
                                 int& promotedKey,
                                 int& newPageId) {
    IndexPage& node = index[pageId];
    cout << "DEBUG: Processing page " << pageId
         << " (type="
         << (node.pageType == IndexPage::LEAF ? "LEAF" : "INTERNAL")
         << ", keys=" << node.numKeys << ")" << endl;

    if (node.pageType == IndexPage::LEAF) {
        auto it = lower_bound(node.keys.begin(), node.keys.end(), key);
        int pos = it - node.keys.begin();
        node.keys.insert(it, key);
        node.pointers.insert(node.pointers.begin() + pos, pointer);
        node.numKeys = node.keys.size();

        if (node.numKeys < MAX_KEYS) return false;

        IndexPage newLeaf;
        newLeaf.pageType = IndexPage::LEAF;
        newLeaf.pageId = nextPageId++;

        int mid = node.numKeys / 2;
        newLeaf.keys = vector<int>(node.keys.begin() + mid, node.keys.end());
        newLeaf.pointers = vector<int>(node.pointers.begin() + mid,
                                        node.pointers.end());
        newLeaf.numKeys = newLeaf.keys.size();

        node.keys.resize(mid);
        node.pointers.resize(mid);
        node.numKeys = node.keys.size();

        // Maintain sibling leaf links
        newLeaf.lastPointer = node.lastPointer;  // Inherit existing
        node.lastPointer = newLeaf.pageId;       // Point to new right sibling
        

        promotedKey = newLeaf.keys.front();
        newPageId = newLeaf.pageId;
        index[newLeaf.pageId] = newLeaf;
        return true;
    }

    int i = 0;
    while (i < node.numKeys && key >= node.keys[i]) ++i;
    int childId = node.pointers[i];

    int childPromoted = -1;
    int childNewPage = -1;
    bool split = InsertRecursive(childId, key, pointer,
                                 childPromoted, childNewPage);
    if (!split) return false;

    auto it = upper_bound(node.keys.begin(), node.keys.end(), childPromoted);
    int pos = it - node.keys.begin();
    node.keys.insert(it, childPromoted);
    node.pointers.insert(node.pointers.begin() + pos + 1, childNewPage);
    node.numKeys = node.keys.size();

    if (node.numKeys < MAX_KEYS) return false;

    IndexPage newInternal;
    newInternal.pageType = IndexPage::INTERNAL;
    newInternal.pageId = nextPageId++;

    int mid = node.numKeys / 2;
    promotedKey = node.keys[mid];

    newInternal.keys = vector<int>(node.keys.begin() + mid + 1,
                                    node.keys.end());
    newInternal.pointers = vector<int>(node.pointers.begin() + mid + 1,
                                       node.pointers.end());
    newInternal.numKeys = newInternal.keys.size();

    node.keys.resize(mid);
    node.pointers.resize(mid + 1);
    node.numKeys = node.keys.size();

    newPageId = newInternal.pageId;
    index[newInternal.pageId] = newInternal;
    return true;
}

void BTreeIndex::Write() {
    cout << "DEBUG: Writing index to file: " << indexFileName << endl;
    File f;
    f.Open(0, const_cast<char*>(indexFileName.c_str()));
    for (auto& [pid, page] : index) {
        f.AddPage(page, pid);
    }
    f.Close();
}

void BTreeIndex::Read(const string& fileName) {
    indexFileName = fileName;  // store for future use
    File f;
    f.Open(0, const_cast<char*>(fileName.c_str()));
    off_t len = f.GetLength();
    for (off_t i = 0; i < len; ++i) {
        IndexPage page;
        f.GetPage(page, i);
        index[i] = page;
    }
    f.Close();
}


bool BTreeIndex::Find(int key, int& pageNumberOut) {
    int current = rootPageId;
    while (true) {
        IndexPage& node = index[current];
        if (node.pageType == IndexPage::LEAF) {
            for (int i = 0; i < node.numKeys; ++i) {
                if (node.keys[i] == key) {
                    pageNumberOut = node.pointers[i];
                    return true;
                }
            }
            return false;
        }
        int i = 0;
        while (i < node.numKeys && key >= node.keys[i]) ++i;
        current = node.pointers[i];
    }
}

void BTreeIndex::ValidateTree() {
    cout << "DEBUG: Validating tree structure..." << endl;
    if (index.find(rootPageId) == index.end()) {
        cerr << "ERROR: Root page " << rootPageId
             << " does not exist!" << endl;
        return;
    }
    for (auto& [pid, page] : index) {
        if (page.pageType == IndexPage::INTERNAL) {
            size_t exp = page.numKeys + 1;
            if (page.pointers.size() != exp) {
                cerr << "ERROR: Page " << pid
                     << " pointer count " << page.pointers.size()
                     << ", expected " << exp << endl;
            }
            for (int ptr : page.pointers) {
                if (index.find(ptr) == index.end()) {
                    cerr << "ERROR: Page " << pid
                         << " references missing child " << ptr
                         << endl;
                }
            }
        }
    }
    cout << "DEBUG: Tree validation complete." << endl;
}

void BTreeIndex::PrintPageStats() const {
    cout << "DEBUG: B+ Tree Stats:" << endl;
    cout << "  Total pages: " << index.size() << endl;
    cout << "  Root page: " << rootPageId << endl;
    cout << "  Next page ID: " << nextPageId << endl;
    int leafCount = 0, internalCount = 0;
    for (auto& [pid, page] : index) {
        if (page.pageType == IndexPage::LEAF) ++leafCount;
        else ++internalCount;
    }
    cout << "  Leaf pages: " << leafCount << endl;
    cout << "  Internal pages: " << internalCount << endl;
}

void BTreeIndex::PrintIndexStructure() const {
    cout << "\nDEBUG: B+ Tree Structure (Visualized):\n";
    for (auto& [pid, page] : index) {
        cout << "[Page " << pid << "] "
             << (page.pageType == IndexPage::LEAF ? "LEAF" : "INTERNAL")
             << "\n  Keys: ";
        for (int k : page.keys) cout << k << " | ";
        cout << "\n  Pointers: ";
        for (int p : page.pointers) cout << p << " -> ";
        cout << "\n";
        if (page.pageType == IndexPage::LEAF) {
            cout << "  LastPointer (sibling leaf): " << page.lastPointer << "\n";
        }
    }
}


void BTreeIndex::ExportDOT(const string& filename) const {
    ofstream out(filename);
    out << "digraph BPlusTree {\n"
           "  graph [rankdir=TB, splines=ortho];\n"
           "  node [shape=record, fontsize=10];\n"
           "  edge [arrowsize=0.7];\n\n";
    for (auto& [pid, page] : index) {
        string label;
        if (page.pageType == IndexPage::LEAF) {
            // Leaf: simple record of keys
            label = "{Leaf " + to_string(pid) + "|";
            for (int i = 0; i < page.numKeys; ++i) {
                label += to_string(page.keys[i]);
                if (i + 1 < page.numKeys) label += " | ";
            }
            label += "}";
            out << "  node" << pid << " [label=\""
                << label << "\"];\n";
        } else {
            // Internal: interleave pointer ports and key fields
            label = "{Internal " + to_string(pid) + "|";
            // p0 port
            label += "<p0> ";
            for (int i = 0; i < page.numKeys; ++i) {
                label += "|";
                label += to_string(page.keys[i]);
                label += "|<p" + to_string(i+1) + ">";
            }
            label += "}";
            out << "  node" << pid << " [label=\""
                << label << "\"];\n";
        }
    }
    // Emit edges
    for (auto& [pid, page] : index) {
        if (page.pageType == IndexPage::INTERNAL) {
            for (int i = 0; i < (int)page.pointers.size(); ++i) {
                out << "  node" << pid << ":p" << i
                    << " -> node" << page.pointers[i]
                    << ";\n";
            }
        }
    }
    out << "}\n";
    out.close();
}