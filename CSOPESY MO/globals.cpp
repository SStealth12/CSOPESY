#include "globals.h"
#include "Process.h"
#include "Screen.h"

// Automatic process creation globals
std::atomic <bool> automaticThreadJoined(false);
std::atomic<bool> automaticCreationEnabled(false);
std::thread automaticCreationThread;
std::mutex creationMutex;
std::condition_variable creationCV;

// Initialize process names
std::vector<std::string> PROCESS_NAMES = {
    "C:\\Program Files\\NVIDIA Corporation\\NVIDIA GeForce Experience\\NVIDIA app\\CEF\\NVIDIA Overlay.exe",
    "C:\\Program Files\\Google\\Drive File Stream\\109.0.3.0\\GoogleDriveFS.exe",
    "C:\\Program Files (x86)\\Google\\Chrome\\Application\\chrome.exe",
    "C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\Common7\\IDE\\devenv.exe",
    "C:\\Users\\YourUser\\AppData\\Local\\Discord\\app-1.0.9193\\Discord.exe"
};

// Initialize process map
std::map<int, Process> processes = {
    // you can even seed it here with Process constructors:
    { 1001, Process(1001, PROCESS_NAMES[0]) },
    { 1002, Process(1002, PROCESS_NAMES[1]) },
    { 1003, Process(1003, PROCESS_NAMES[2]) },
    { 1004, Process(1004, PROCESS_NAMES[3]) },
    { 1005, Process(1005, PROCESS_NAMES[4]) }
};

// Screen management globals
std::vector<std::shared_ptr<Screen>> createdProcesses;
std::vector<std::shared_ptr<Screen>> readyQueue;
std::vector<std::shared_ptr<Screen>> finishedProcesses;

// Scheduler
std::unique_ptr<Scheduler> globalScheduler;

// Configs
int coresUsed = 0;
bool isEvaluationMode = false;
std::string schedulingAlgorithm = "FCFS"; 
int minInstructions = 0;
int maxInstructions = 0;
int quantumCycles = 0;
int batchProcessFreq = 0;
int delaysPerExec = 0;

// Mutexes
std::mutex createdMutex;
std::mutex readyMutex;
std::mutex finishedMutex;