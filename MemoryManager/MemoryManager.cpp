#include "MemoryManager.h"
#include <cstdint>
#include <functional>
#include <sstream>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <vector>
#include <climits>

MemoryManager::MemoryManager(unsigned int wordSize, std::function<int(int, void*)> allocator) : 
    wordSize(wordSize), 
    allocator(allocator), 
    head(nullptr), 
    memoryStart(nullptr), 
    totalWords(0),
    initialized(false) 
{ }

MemoryManager::~MemoryManager() {
    shutdown();
}

void MemoryManager::initialize(size_t sizeInWords) {
    if (initialized) shutdown();
    unsigned int memorySize = sizeInWords * wordSize;
    memoryStart = static_cast<uint8_t*>(malloc(memorySize));
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
    ::free(memoryStart);
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

void* MemoryManager::allocate(size_t sizeInBytes) {
    if (!initialized) return nullptr;

    unsigned int sizeInWords = (sizeInBytes + wordSize - 1) / wordSize;
    void* holesVoid = getList();
    if (holesVoid == nullptr) return nullptr;
    uint16_t* holes = static_cast<uint16_t*>(holesVoid);

    int offset = allocator(sizeInWords, holesVoid);
    if (offset == -1) {
        delete[] holes;
        return nullptr;
    }

    Block* current = head;
    /* Traverse through the linked list to find the first free block that starts at the offset. */
    while (current != nullptr) {
        if (current -> offset == offset && current -> isFree) break;
        current = current -> next;
    }
    if (current == nullptr) {
        delete[] holes;
        return nullptr;
    }

    unsigned int originalSize = current->size;

    if (current -> size > sizeInWords) {
        current -> size = sizeInWords;
        current -> isFree = false;

        Block* remainingHole = new Block(current -> offset + sizeInWords, originalSize - sizeInWords, true);

        remainingHole -> prev = current;
        remainingHole -> next = current -> next;

        if (current -> next != nullptr) current -> next -> prev = remainingHole;
        current -> next = remainingHole;
    }
    else if (current -> size == sizeInWords) current -> isFree = false;
    else {
        delete[] holes;
        return nullptr;
    }
    delete[] holes;

    return memoryStart + offset * wordSize;
}


void MemoryManager::free(void* address) {
    if (!initialized || address == nullptr) return;

    uint8_t* addr = static_cast<uint8_t*>(address);
    uint8_t* memStart = memoryStart;
    uint8_t* memEnd = memStart + totalWords * wordSize;

    if (addr < memStart || addr >= memEnd) return;
    unsigned int offset = (addr - memStart) / wordSize;
    
    Block* current = head;
    while (current != nullptr) {
        if (current -> offset == offset && !current -> isFree) break;
        current = current -> next;
    }
    if (current == nullptr) return;

    current -> isFree = true;

    /* Merge our free block with previous free block */
    if (current -> prev != nullptr && current -> prev -> isFree) {
        Block* prevBlock = current -> prev;
        prevBlock -> size += current -> size;
        prevBlock -> next = current -> next;
        if (current -> next != nullptr) current -> next -> prev = prevBlock;
        delete current;
        current = prevBlock;
    }
    /* Merge our free block with next free block */
    if (current -> next != nullptr && current -> next -> isFree) {
        Block* nextBlock = current -> next;
        current -> size += nextBlock -> size;
        current -> next = nextBlock -> next;
        if (nextBlock -> next != nullptr) nextBlock -> next -> prev = current;
        delete nextBlock;
    }
}

 void MemoryManager::setAllocator(std::function<int(int, void *)> allocator) {
    this -> allocator = allocator;
}

int MemoryManager::dumpMemoryMap(char* filename) {
    if (!initialized) return -1;
    uint16_t* holes = static_cast<uint16_t*>(getList());
    if (holes == nullptr) return -1;
    
    int fd = open(filename, O_RDWR | O_CREAT | O_TRUNC, 0777);
    if (fd == -1) {
        std::cerr << "Could not open file" << std::endl;
        delete[] holes;
        return -1;
    }
    std::ostringstream oss;
    int numHoles = holes[0];
    for (int i = 0; i < numHoles; i++) oss << "[" << holes[i * 2 + 1] << ", " << holes[i * 2 + 2] << "] - ";
    std::string output = oss.str();
    
    if (!output.empty()) output.erase(output.size() - 3);

    if (!write(fd, output.c_str(), output.size())) std::cerr << "Could not write to file" << std::endl;
    else if (close(fd) == -1) std::cerr << "Could not close file" << std::endl;
    
    delete[] holes;
    return 0;
}

void* MemoryManager::getList() {
    std::vector<std::pair<unsigned int, unsigned int>> holePairs;

    Block* current = head;
    while (current != nullptr) {
        if (current -> isFree) holePairs.emplace_back(current -> offset, current -> size);
        current = current -> next;
    }
    if (holePairs.empty()) return nullptr;

    unsigned int size = holePairs.size();
    uint16_t* holes = new uint16_t[size * 2 + 1];
    holes[0] = holePairs.size();

    for (unsigned int i = 0; i < size; i++) {
        holes[i * 2 + 1] = holePairs[i].first;
        holes[i * 2 + 2] = holePairs[i].second;
    }

    return holes;
}

void* MemoryManager::getBitmap() {
    if (!initialized) return nullptr;
    
    unsigned int size = (totalWords + 7) / 8;
    uint8_t* bitmap = new uint8_t[size + 2]();
    
    bitmap[0] = size & 0xFF;
    bitmap[1] = (size >> 8) & 0xFF;
    
    Block* current = head;
    while (current != nullptr) {
        if (!current->isFree) {
            for (unsigned int i = 0; i < current -> size; i++) {
                unsigned int bitIndex = current -> offset + i;
                unsigned int byteIndex = (bitIndex / 8) + 2;
                unsigned int bitPosition = bitIndex % 8;
                bitmap[byteIndex] |= (1 << bitPosition);
            }
        }
        current = current->next;
    }
    
    return bitmap;
}

unsigned int MemoryManager::getWordSize() const {
    return wordSize;
}

void* MemoryManager::getMemoryStart() const {
    return memoryStart;
}

unsigned int MemoryManager::getMemoryLimit() const {
    if (!initialized) return 0;
    return totalWords * wordSize;
}

int bestFit(int sizeInWords, void* list) {
    if (!list) return -1;
    uint16_t* holes = static_cast<uint16_t*>(list);
    int numHoles = holes[0];
    int optimalOffset = -1;
    int smallest = INT_MAX;

    for (int i = 0; i < numHoles; i++) {
        uint16_t offset = holes[i * 2 + 1];
        uint16_t size = holes[i * 2 + 2];
        
        if (size >= sizeInWords && size < smallest) {
            smallest = size;
            optimalOffset = offset;
        }
    }

    return optimalOffset;
}

int worstFit(int sizeInWords, void* list) {
    if (!list) return -1;
    uint16_t* holes = static_cast<uint16_t*>(list);
    int numHoles = holes[0];
    int optimalOffset = -1;
    int largest = -1;

    for (int i = 0; i < numHoles; i++) {
        uint16_t offset = holes[i * 2 + 1];
        uint16_t size = holes[i * 2 + 2];
        
        if (size >= sizeInWords && size > largest) {
            largest = size;
            optimalOffset = offset;
        }
    }
    
    return optimalOffset;
}
