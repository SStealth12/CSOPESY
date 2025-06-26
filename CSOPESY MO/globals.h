#pragma once
#include <vector>
#include <memory>
#include <queue>
#include <map>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <thread>
#include <filesystem>

#include "Scheduler.h"
#include "Screen.h"
#include "Process.h"


// File management globals
namespace fs = std::filesystem;

// Automatic process creation gobals
extern std::atomic<bool> automaticThreadJoined;
extern std::atomic<bool> automaticCreationEnabled;
extern std::thread automaticCreationThread;
extern std::mutex creationMutex;
extern std::condition_variable creationCV;


// Process (for NVIDIA-SMI)
extern std::vector<std::string> PROCESS_NAMES;
extern std::map<int, Process> processes;

// Screen management globals
extern std::vector<std::shared_ptr<Screen>> createdProcesses; 
extern std::vector<std::shared_ptr<Screen>> readyQueue;
extern std::vector<std::shared_ptr<Screen>> finishedProcesses;

// Scheduler
extern std::unique_ptr<Scheduler> globalScheduler;

// Program configuration
extern int coresUsed;
extern bool isEvaluationMode;
extern std::string schedulingAlgorithm;
extern int minInstructions;
extern int maxInstructions;
extern int quantumCycles;
extern int batchProcessFreq;
extern int delaysPerExec;

// Mutexes for thread safety
extern std::mutex createdMutex;
extern std::mutex readyMutex;
extern std::mutex finishedMutex;