#include "IndexPage.h"
#include <cstring>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>

using namespace std;

IndexPage::IndexPage() : pageType(LEAF), pageId(-1), lastPointer(-1), numKeys(0) {}

// void IndexPage::ToBinary(char* bits) const {
//     // 1) Zero out the entire PAGE_SIZE buffer so no byte is ever left uninitialized
//     std::memset(bits, 0, PAGE_SIZE);

//     // 2) Sanity check: pointers.size() must == numKeys
//     if (static_cast<int>(pointers.size()) != numKeys) {
//         std::cerr << "ERROR: pointers count mismatch in ToBinary for page "
//                   << pageId << " (got " << pointers.size()
//                   << ", expected " << numKeys << ")\n";
//         std::exit(EXIT_FAILURE);
//     }

//     // 3) Walk the buffer as an int array
//     int* out = reinterpret_cast<int*>(bits);
//     size_t pos = 0;

//     // A) Header
//     out[pos++] = pageType;
//     out[pos++] = pageId;
//     out[pos++] = numKeys;

//     // B) Each (key, pointer) pair
//     for (int i = 0; i < numKeys; ++i) {
//         out[pos++] = keys[i];
//         out[pos++] = pointers[i];
//     }

//     // C) The one extra pointer (last child for INTERNAL, sibling for LEAF)
//     out[pos++] = lastPointer;

//     // Everything past 'pos' remains zero from the original memset
// }
void IndexPage::ToBinary(char* bits) const {
    // 1) Wipe the whole buffer to zero
    std::memset(bits, 0, PAGE_SIZE);

    // 2) We'll write everything as 32‑bit ints
    static_assert(sizeof(int)==4, "This on‑disk format assumes 4‑byte ints");
    size_t offset = 0;

    // helper to copy a single int into bits[]
    auto writeInt = [&](int32_t v) {
        std::memcpy(bits + offset, &v, sizeof(v));
        offset += sizeof(v);
    };

    // A) header
    writeInt(static_cast<int32_t>(pageType));
    writeInt(static_cast<int32_t>(pageId));
    writeInt(static_cast<int32_t>(numKeys));

    // B) each (key, pointer)
    for (int i = 0; i < numKeys; ++i) {
        writeInt(static_cast<int32_t>(keys[i]));
        writeInt(static_cast<int32_t>(pointers[i]));
    }

    // C) the extra pointer (last child or sibling)
    writeInt(static_cast<int32_t>(lastPointer));

    // sanity check: we never overflowed
    assert(offset <= PAGE_SIZE);
    // any bytes in [offset .. PAGE_SIZE) remain zero from the initial memset
}
void IndexPage::FromBinary(const char* bits) {
    int offset = 0;
    memcpy(&pageType, bits + offset, sizeof(int)); offset += sizeof(int);
    memcpy(&pageId,   bits + offset, sizeof(int)); offset += sizeof(int);
    memcpy(&numKeys,  bits + offset, sizeof(int)); offset += sizeof(int);

    keys.clear();
    pointers.clear();

    // 1) read numKeys pairs of (key, pointer)
    for (int i = 0; i < numKeys; ++i) {
        int k, p;
        memcpy(&k, bits + offset,         sizeof(int)); offset += sizeof(int);
        memcpy(&p, bits + offset,         sizeof(int)); offset += sizeof(int);
        keys.push_back(k);
        pointers.push_back(p);
    }

    // 2) read the final pointer (child or sibling)
    int extra;
    memcpy(&extra, bits + offset, sizeof(int));
    lastPointer = extra;

    // 3) sanity check
    if ((int)pointers.size() != numKeys) {
        cerr << "ERROR: pointers count corrupt in FromBinary for page "
             << pageId << endl;
        exit(1);
    }
}




void IndexPage::Print() const {
    cout << "IndexPage [pageId=" << pageId << ", pageType=" << (pageType == 0 ? "Leaf" : "Internal")
         << ", numKeys=" << numKeys << "]\n";
    for (int i = 0; i < numKeys; ++i) {
        cout << "  Key: " << keys[i] << " -> Page: " << pointers[i] << "\n";
    }
    cout << "  LastPointer: " << lastPointer << endl;
}
