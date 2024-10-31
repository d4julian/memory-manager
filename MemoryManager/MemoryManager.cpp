// MemoryManager.cpp

#include "MemoryManager.h"
#include <cstdint>
#include <functional>
#include <iostream>

// Constructor
MemoryManager::MemoryManager(unsigned int wordSize, std::function<int(int, void*)> allocator) : 
    wordSize(wordSize), 
    allocator(allocator), 
    head(nullptr), 
    memoryStart(nullptr), 
    totalWords(0),
    initialized(false) 
{
    // Constructor implementation
}

// Destructor
MemoryManager::~MemoryManager() {
    // Destructor implementation
}

void MemoryManager::initialize(unsigned int sizeInWords) {
    if (initialized) shutdown();
    unsigned int memorySize = sizeInWords * wordSize;
    memoryStart = malloc(memorySize);
    if (memoryStart == nullptr) {
        std::cerr << "Could not allocate memory :(" << std::endl;
        return;
    }
    initialized = true;
    totalWords = sizeInWords;
    head = new Block(0, sizeInWords, true);
}

void MemoryManager::shutdown() {
    if (!initialized) return;
    free(memoryStart);
    memoryStart = nullptr;
    while (head != nullptr) {
        Block* temp = head;
        head = head->next;
        delete temp;
    }
    head = nullptr;
    totalWords = 0;
    initialized = false;
}

void* MemoryManager::allocate(unsigned int sizeInBytes) {
    if (!initialized) return nullptr;

    unsigned int sizeInWords = (sizeInBytes + wordSize - 1) / wordSize;
    void* holeList = getList();

    int offset = allocator(sizeInWords, holeList);
    if (offset == -1) return nullptr;


    Block* current = head;
    /* Traverse through the linked list to find the first free block that starts at the offset. */
    while (current != nullptr) {
        if (current -> offset == offset && current -> isFree) break;
        current = current->next;
    }
    if (current == nullptr) return nullptr;

    unsigned int originalSize = current->size;

    if (current -> size > sizeInWords) {
        current -> size = sizeInWords;
        current -> isFree = false;

        Block* remainingHole = new Block(current->offset + sizeInWords, originalSize - sizeInWords, true);

        remainingHole -> prev = current;
        remainingHole -> next = current->next;

        if (current -> next != nullptr) {
            current -> next -> prev = remainingHole;
        }
        current -> next = remainingHole;
    }
    else if (current -> size == sizeInWords) {
        current -> isFree = false;
    }
    else return nullptr;

    return static_cast<void*>(static_cast<char*>(memoryStart) + offset * wordSize);
}


// Free allocated memory
void MemoryManager::free(void* address) {
    // Free implementation
}

 void MemoryManager::setAllocator(std::function<int(int, void *)> allocator) {
    MemoryManager::allocator = allocator;
}

// Dump memory map to file
int MemoryManager::dumpMemoryMap(char* filename) {
    // Dump memory map implementation
}

// Get list of holes
void* MemoryManager::getList() {
    // Get list implementation
    return nullptr;
}

// Get bitmap representation
void* MemoryManager::getBitmap() {
    // Get bitmap implementation
    return nullptr;
}

// Get word size
unsigned int MemoryManager::getWordSize() const {
    // Get word size implementation
    return 0;
}

// Get start of memory block
void* MemoryManager::getMemoryStart() const {
    // Get memory start implementation
    return nullptr;
}

// Get memory limit (total bytes)
unsigned int MemoryManager::getMemoryLimit() const {
    // Get memory limit implementation
    return 0;
}

// Best fit allocator function (outside class)
int bestFit(unsigned int sizeInWords, void* list) {
    // Best fit implementation
    return -1;
}

// Worst fit allocator function (outside class)
int worstFit(unsigned int sizeInWords, void* list) {
    // Worst fit implementation
    return -1;
}
