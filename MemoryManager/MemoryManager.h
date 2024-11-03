#pragma once

#include <functional>
#include <cstddef>
#include <cstdint>

/* Returns word offset of hole selected by the best fit memory allocation algorithm, and -1 if there is no fit */
int bestFit(int sizeInWords, void *list);
/* Returns word offset of hole selected by the worst fit memory allocation algorithm, and -1 if there is no fit */
int worstFit(int sizeInWords, void *list);

/* Comments are taken from the specifications in "Memory Management & Layering (Fall 2024).pdf"

    The Memory Manager will handle the allocation/deallocation of memory and provide details of its state.
    How the class keeps track of allocation/deallocation is implementation dependent and is left for the student
    to decide. MemoryManager.h and MemoryManager.cpp will contain declaration and definition, respectively.
*/
class MemoryManager {
public:
    /* Constructor; sets native word size (in bytes, for alignment) and default allocator for finding a memory hole. */
    MemoryManager(unsigned wordSize, std::function<int(int, void *)> allocator);
    /* Releases all memory allocated by this object without leaking memory */
    ~MemoryManager();

    /* Instantiates block of requested size, no larger than 65536 words; cleans up previous block if applicable. */
    void initialize(size_t sizeInWords);
    /* Releases memory block acquired during initialization, if any. This should only include memory created for 
        long term use not those that returned such as getList() or getBitmap() as whatever is calling those should
        delete it instead. */
    void shutdown();

    /* Allocates a memory using the allocator function. If no memory is available or size is invalid, returns nullptr. */ 
    void *allocate(size_t sizeInBytes);
    /* Frees the memory block within the memory manager so that it can be reused. */
    void free(void* address);

    /* Changes the allocation algorithm to identifying the memory hole to use for allocation. */
    void setAllocator(std::function<int(int, void *)> allocator);

    /* Uses standard POSIX calls to write hole list to filename as text, returning -1 on error and 0 if successful.
        Format: "[START, LENGTH] - [START, LENGTH] …", e.g., "[0, 10] - [12, 2] - [20, 6]" */
    int dumpMemoryMap(char* filename);
    /* Returns an array of information (in decimal) about holes for use by the allocator function (little-Endian).
        Offset and length are in words. If no memory has been allocated, the function should return a NULL pointer.
        Format:
        NUMBER OF HOLES HOLE 0 OFFSET HOLE 0 LENGTH HOLE 1 OFFSET HOLE 1 LENGTH ...
        2B              2B            2B            2B            2B            ...

        Example:
        [3, 0, 10, 12, 2, 20, 6] */
    void *getList();
    /* Returns a bit-stream of bits in terms of an array representing whether words are used (1) or free (0). The
        first two bytes are the size of the bitmap (little-Endian); the rest is the bitmap, word-wise.
        Note : In the following example B0, B2, and B4 are holes, B1 and B3 are allocated memory.
        Hole-0 Hole-1 Hole-2 ┌─B4─┐ ┌ B2┐ ┌───B0 ──┐ ┌─Size (4)─┐┌This is Bitmap in Hex┐
        Example: [0,10]-[12,2]-[20,6] [00 00001111 11001100 00000000] [0x04,0x00,0x00,0xCC,0x0F,0x00]
        ┕─B3─┙ ┕B1┙
        Returned Array: [0x04,0x00,0x00,0xCC,0x0F,0x00] or [4,0,0,204,15,0] */
    void *getBitmap();
    
    /* Returns the word size used for alignment. */
    unsigned getWordSize() const;
    /* Returns the byte-wise memory address of the beginning of the memory block. */
    void* getMemoryStart() const;
    /* Returns the byte limit of the current memory block */
    unsigned getMemoryLimit() const;

    struct Hole {
        unsigned int offset; // Starting word index of the hole
        unsigned int size;   // Size of the hole in words

        // Constructor for convenience
        Hole(unsigned int offset, unsigned int size) : offset(offset), size(size) {}
    };

private:
    // Internal data structures
    // Using a doubly linked list to keep track of memory blocks
    
    struct Block {
        unsigned int offset; // Offset from the start of the memory block
        unsigned int size;   // Size of the block in words
        bool isFree;         // True if the block is a hole (free), false if allocated
        Block* next;         // Pointer to the next block
        Block* prev;         // Pointer to the previous block
        Block(int start, int blockSize, bool free)
        : offset(start), size(blockSize), isFree(free), next(nullptr), prev(nullptr) {}
    };
    Block* head; // Head of the block list

    // Member variables
    unsigned int wordSize;                       // Word size in bytes
    unsigned int totalWords;                     // Total number of words in the memory block
    uint8_t* memoryStart;                           // Pointer to the start of the memory block
    std::function<int(int, void*)> allocator;    // Allocation algorithm function pointer
    bool initialized; // Flag indicating if the memory manager is initialized


};
