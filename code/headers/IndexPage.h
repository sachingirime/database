#ifndef INDEX_PAGE_H
#define INDEX_PAGE_H

#include <vector>
#include <cstdint>
#include <cstring>
#include <iostream>

#define PAGE_SIZE 131072       // 128 KB per page
#define HEADER_SIZE 16         // Fixed header size
#define PTR_SIZE 4             // 4 bytes per pointer (page ID)
#define KEY_SIZE 4             // 4 bytes per integer key
#define MAX_KEYS ((PAGE_SIZE - HEADER_SIZE) / (KEY_SIZE + PTR_SIZE))

class IndexPage {
public:
    enum PageType { LEAF = 0, INTERNAL = 1 };

    int pageType;              // 0 for leaf, 1 for internal
    int pageId;                // Page ID
    int lastPointer;           // Pointer after the last key
    int numKeys;               // Number of keys currently stored

    std::vector<int> keys;     // Keys (integers)
    std::vector<int> pointers; // Corresponding pointers (page IDs)

    IndexPage();

    // Serialize to binary buffer
    void ToBinary(char* buf) const;

    // Deserialize from binary buffer
    void FromBinary(const char* buf);

    void Print() const;
};

#endif // INDEX_PAGE_H
