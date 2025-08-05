#pragma once
#ifndef MEMORY_MANAGER_H
#define MEMORY_MANAGER_H

#include <vector>
#include <map>
#include <mutex>
#include <fstream>
#include <string>
#include <memory>
#include <queue>
#include <cstdint>

// Forward declaration
class Screen;

// Constants for demand paging
static const size_t PAGE_SIZE = 32;  // 32 bytes per page (16 uint16 values)
static const size_t FRAME_SIZE = 32; // Same as page size

struct Frame {
    size_t frameId;
    bool isOccupied;
    std::string processName;
    size_t pageNumber;
    std::vector<uint16_t> data;  // Actual frame data
    
    Frame(size_t id) : frameId(id), isOccupied(false), pageNumber(static_cast<size_t>(-1)) {
        data.resize(FRAME_SIZE / 2, 0);  // 16 uint16 values per frame
    }
};

struct Page {
    size_t pageNumber;
    std::string processName;
    size_t frameNumber;
    bool isInMemory;
    std::vector<uint16_t> data;  // Page data when not in memory
    
    Page(size_t pageNum, const std::string& procName) 
        : pageNumber(pageNum), processName(procName), frameNumber(static_cast<size_t>(-1)), isInMemory(false) {
        data.resize(PAGE_SIZE / 2, 0);  // 16 uint16 values per page
    }
};

struct MemoryStats {
    int totalMemory;
    int usedMemory;
    int freeMemory;
    int totalFrames;
    int usedFrames;
    int freeFrames;
    int numPagedIn;
    int numPagedOut;
    int idleCpuTicks;
    int activeCpuTicks;
    int totalCpuTicks;
    
    MemoryStats() : totalMemory(0), usedMemory(0), freeMemory(0), 
                   totalFrames(0), usedFrames(0), freeFrames(0),
                   numPagedIn(0), numPagedOut(0), idleCpuTicks(0),
                   activeCpuTicks(0), totalCpuTicks(0) {}
};

class MemoryManager {
private:
    // Core data structures for demand paging
    std::vector<Frame> frames;
    std::map<std::string, std::vector<Page>> processPages;
    std::queue<std::pair<std::string, size_t>> loadedPagesQueue;  // FIFO queue for page replacement
    
    // Memory configuration
    size_t totalMemory;
    size_t numFrames;
    std::string backingStoreFile;
    
    // Statistics tracking
    size_t pagesIn = 0;
    size_t pagesOut = 0;
    size_t pageFaults = 0;
    size_t idleCpuTicks = 0;
    size_t activeCpuTicks = 0;
    size_t totalCpuTicks = 0;
    
    // Thread safety
    mutable std::mutex memoryMutex;
    
    // Helper methods for demand paging
    void createProcessPages(const std::string& processName, size_t totalSize);
    size_t virtualToPageNumber(size_t virtualAddress) const;
    size_t virtualToPageOffset(size_t virtualAddress) const;
    size_t findFreeFrame();
    size_t selectVictimFrame();
    void pageOut(const std::string& processName, size_t pageNumber);
    void pageIn(const std::string& processName, size_t pageNumber, size_t frameNumber);
    bool handlePageFault(const std::string& processName, size_t virtualAddress);
    bool isValidVirtualAddress(const std::string& processName, size_t virtualAddress);
    
    // Statistics helpers
    void incrementPagesIn() { pagesIn++; }
    void incrementPagesOut() { pagesOut++; }
    void incrementPageFaults() { pageFaults++; }
    
    // Legacy compatibility methods
    std::vector<Frame> frames_;
    std::map<std::shared_ptr<Screen>, std::vector<Page>> pageTables_;
    std::queue<int> freeFrames_;
    std::queue<int> fifoQueue_;
    int maxOverallMemory_;
    int memoryPerFrame_;
    int totalFrames_;
    MemoryStats stats_;
    
    // Legacy helper methods
    int findVictimFrame();
    void pageOut(int frameId);
    void pageIn(std::shared_ptr<Screen> process, int pageNumber, int frameId);
    void writeToBackingStore(std::shared_ptr<Screen> process, int pageNumber, const std::vector<uint8_t>& data);
    std::vector<uint8_t> readFromBackingStore(std::shared_ptr<Screen> process, int pageNumber);
    
public:
    MemoryManager(size_t totalMem);
    MemoryManager(int maxOverallMemory, int memoryPerFrame);  // Legacy constructor
    ~MemoryManager();
    
    // New demand paging interface
    uint16_t readMemory(const std::string& processName, size_t virtualAddress);
    void writeMemory(const std::string& processName, size_t virtualAddress, uint16_t value);
    bool allocateMemory(const std::string& processName, size_t size);
    void deallocateMemory(const std::string& processName);
    
    // Legacy interface for compatibility
    bool allocateMemory(std::shared_ptr<Screen> process, int memorySize);
    void deallocateMemory(std::shared_ptr<Screen> process);
    bool accessMemory(std::shared_ptr<Screen> process, uint32_t virtualAddress, bool isWrite = false);
    bool readMemory(std::shared_ptr<Screen> process, uint32_t virtualAddress, uint16_t& value);
    bool writeMemory(std::shared_ptr<Screen> process, uint32_t virtualAddress, uint16_t value);
    bool handlePageFault(std::shared_ptr<Screen> process, int pageNumber);
    
    // Memory validation
    static bool isValidMemorySize(size_t size);
    
    // Statistics and monitoring
    struct MemorySnapshot {
        size_t totalMemory;
        size_t usedMemory;
        size_t availableMemory;
        size_t fragmentationCount;
        size_t largestFreeBlock;
        size_t activeProcesses;
        size_t inactiveProcesses;
        std::vector<std::pair<std::string, size_t>> processMemory;
        size_t pagesIn;
        size_t pagesOut;
        size_t pageFaults;
        size_t idleCpuTicks;
        size_t activeCpuTicks;
        size_t totalCpuTicks;
    };
    
    MemorySnapshot getMemorySnapshot() const;
    MemoryStats getStats() const;  // Legacy compatibility
    void updateCpuTicks(bool isActive);
    
    // Debugging and visualization
    void printMemoryStatus() const;
    void printProcessMemoryUsage() const;
    
    // Backing store operations
    void initializeBackingStore();
    void cleanupBackingStore();
    
    // Helper methods for address translation (legacy)
    int getPageNumber(uint32_t virtualAddress) const;
    int getOffset(uint32_t virtualAddress) const;
    uint32_t getPhysicalAddress(int frameId, int offset) const;
};

#endif // MEMORY_MANAGER_H
