#ifndef BTREE_INDEX_H
#define BTREE_INDEX_H

#include <map>
#include <string>
#include <vector>
#include "IndexPage.h"
#include "File.h"
#include "Catalog.h"

#define MAX_KEYS 8  // Maximum number of keys per node before splitting

class BTreeIndex {
private:
    File indexFile;
    std::map<int, IndexPage> index;
    int rootPageId;
    int nextPageId;
    Catalog* catalog;

    std::string indexFileName;
    std::string tableName;
    std::string attributeName;


    int ExtractKey(Record& rec);


    void Insert(int key, int pointer);
    bool InsertRecursive(int pageId, int key, int pointer, int& promotedKey, int& newPageId);
    bool SplitLeaf(IndexPage& node, int& promotedKey, int& newPageId);
    bool SplitInternal(IndexPage& node, int& promotedKey, int& newPageId);


public:
    BTreeIndex(Catalog* catalog);
    void Build(const std::string& indexName, const std::string& tableName, const std::string& attributeName);
    bool Find(int key, int& dataPagePointer);
    void Write();
    void Read();
    void PrintIndexStructure() const;
    void PrintPageStats() const;
    void ExportDOT(const std::string& filename) const;
    void ValidateTree();
};

#endif // BTREE_INDEX_H
