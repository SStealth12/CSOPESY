#include "osloop.h"
#include "globals.h"
#include "Screen.h"


void printHeader() {
    std::cout << R"(
   _____  _____  ____  _____  ______  _______     __
  / ____|/ ____|/ __ \|  __ \|  ____|/ ____\ \   / /
 | |    | (___ | |  | | |__) | |__  | (___  \ \_/ / 
 | |     \___ \| |  | |  ___/|  __|  \___ \  \   /  
 | |____ ____) | |__| | |    | |____ ____) |  | |   
  \_____|_____/ \____/|_|    |______|_____/   |_|   

)" << std::endl;

    std::cout << "\033[1;32mHello, Welcome to CSOPESY commandline!\033[0m" << std::endl;
    std::cout << "\033[1;33mType 'exit' to quit, 'clear' to clear the screen\033[0m" << std::endl;
}

void nvidiasmi(const std::map<int, Process>& processes) {
    std::cout << "+---------------------------------------------------------------------------------------+" << std::endl;
    std::cout << "| NVIDIA-SMI 576.02                 Driver Version: 576.02       CUDA Version: 12.9     |" << std::endl;
    std::cout << "|-----------------------------------------+----------------------+----------------------+" << std::endl;
    std::cout << "| GPU  Name                  Driver-Model | Bus-Id        Disp.A | Volatile Uncorr. ECC |" << std::endl;
    std::cout << "| Fan  Temp  Perf            Pwr:Usage/Cap|         Memory-Usage | GPU-Util  Compute M. |" << std::endl;
    std::cout << "|                                         |                      |               MIG M. |" << std::endl;
    std::cout << "|=========================================+======================+======================|" << std::endl;
    std::cout << "|   0  NVIDIA GeForce GTX 1650     WDDM   | 00000000:01:00.0  On |                  N/A |" << std::endl;
    std::cout << "| N/A   48C    P8              5W / 50W   | 458MiB /  4096MiB    |      2%      Default |" << std::endl;
    std::cout << "|                                         |                      |                  N/A |" << std::endl;
    std::cout << "+-----------------------------------------+----------------------+----------------------+" << std::endl;

    std::cout << "                                                                                         " << std::endl;
    std::cout << "+---------------------------------------------------------------------------------------+" << std::endl;
    std::cout << "| Processes:                                                        GPU Memory          |" << std::endl;
    std::cout << "|  GPU   PID     Type      Process name                             Usage               |" << std::endl;
    std::cout << "|=======================================================================================|" << std::endl;

    for (auto it = processes.begin(); it != processes.end(); ++it) {
        int pid = it->first;
        const Process& proc = it->second;

        std::string pname = proc.getFormattedName();
        std::string gpuUsage = proc.getFormattedGpuUsage();
        const std::string& ptype = proc.getType();

        std::cout << "|   0  " << std::setw(6) << pid << "   " << std::setw(4) << ptype << "  "
            << std::setw(37) << pname << "  " << std::setw(11) << gpuUsage << "                |" << std::endl;

    }

    std::cout << "+---------------------------------------------------------------------------------------+" << std::endl;
    std::cout << std::endl;
}


void automaticProcessCreation() {
    int createdCount = 0;
    int cycleCount = 0;
    int nextId = 1; // Default starting ID

    // Initial ID determination (thread-safe)
    {
        std::lock_guard<std::mutex> guard(creationMutex);
        if (!createdProcesses.empty()) {
            nextId = createdProcesses.back()->getId() + 1;
        }
    }

    while (true) {
        // Wait with timeout at start of each cycle
        std::unique_lock<std::mutex> lock(creationMutex);
        bool stillOn = creationCV.wait_for(lock,
            std::chrono::milliseconds(delaysPerExec),
            [] { return !automaticCreationEnabled.load(); });

        if (!automaticCreationEnabled) break;
        lock.unlock();
        

        cycleCount++;

        // Process creation block
        if (cycleCount % batchProcessFreq == 0) {
            std::string name;
            int totalBurst = 0;
            std::shared_ptr<Screen> screen;

            {
                std::ostringstream nameStream;
                nameStream << "screen_" << std::setw(2) << std::setfill('0') << nextId;
                name = nameStream.str();
                totalBurst = minInstructions + (rand() % (maxInstructions - minInstructions + 1));
            }

            // Create and register the process (locked section)
            {
                std::lock_guard<std::mutex> guard(creationMutex);
                screen = std::make_shared<Screen>(nextId, name, totalBurst);
                
                // Allocate memory for the process
                if (globalMemoryManager) {
                    int memorySize = minMemoryPerProcess + (rand() % (maxMemoryPerProcess - minMemoryPerProcess + 1));
                    if (globalMemoryManager->allocateMemory(name, memorySize)) {
                        screen->setMemorySize(memorySize);
                        screen->setStatus("READY");
                    } else {
                        screen->setStatus("WAITING"); // Waiting for memory
                    }
                } else {
                    screen->setStatus("READY");
                }
                
                createdProcesses.push_back(screen);
                nextId++;
            }

            // Schedule the process (no lock needed)
            if (globalScheduler && screen->getStatus() == "READY") {
                globalScheduler->addProcess(screen);
            }

            createdCount++;

            // Check evaluation mode
            if (isEvaluationMode && createdCount >= 10) {
                automaticCreationEnabled = false;
                automaticThreadJoined = true;
                creationCV.notify_all(); // Wake up any waiting threads
                break;
            }
        }
    }
}

void enterScreen(std::shared_ptr<Screen> screen) {
    if (!screen) return;

    /*if (screen->isFinished()) {
        std::cout << "Process " << screen->getName() << " has finished execution.\n";
        return;
    }*/

    screen->draw();
    while (true) {
        std::string sub;
        std::cout << ">>";
        std::getline(std::cin, sub);

        if (sub == "exit") {
            system(CLEAR_COMMAND);
            printHeader();
            break;
        }
        else if (sub == "process-smi") {
            screen->printLogs();
        }
        else if (sub == "report-util") {
            screen->exportLogs();
            std::cout << "Report generated as: " << screen->getName() << ".txt\n";
        }
        else if (sub == "execute") {
            if (!screen->isFinished()) {
                screen->executeInstruction(-1);
                std::cout << "Executed one instruction\n";
            }
            else {
                std::cout << "Process has finished execution\n";
            }
        }
        else {
            std::cout << "Unknown sub-command.\n";
        }
    }
}

void screenCommandWithInstructions(const std::string& name, const std::string& memorySize, const std::string& instructions) {
    if (name.empty()) {
        std::cout << "Process name cannot be empty.\n";
        return;
    }
    
    // Validate instruction count (1-50)
    std::vector<std::string> instructionList;
    std::istringstream iss(instructions);
    std::string instruction;
    
    while (std::getline(iss, instruction, ';')) {
        // Trim whitespace
        instruction.erase(0, instruction.find_first_not_of(" \t"));
        instruction.erase(instruction.find_last_not_of(" \t") + 1);
        if (!instruction.empty()) {
            instructionList.push_back(instruction);
        }
    }
    
    if (instructionList.size() < 1 || instructionList.size() > 50) {
        std::cout << "Invalid command. Instruction count must be between 1 and 50.\n";
        return;
    }
    
    int memSize = 0;
    try {
        memSize = std::stoi(memorySize);
        // Validate memory size (power of 2, between 8 and 65536)
        if (memSize < 8 || memSize > 65536 || (memSize & (memSize - 1)) != 0) {
            std::cout << "Invalid memory allocation. Memory size must be a power of 2 between 8 and 65536 bytes.\n";
            return;
        }
    } catch (const std::exception&) {
        std::cout << "Invalid memory size format.\n";
        return;
    }
    
    std::shared_ptr<Screen> newScreen = nullptr;
    {
        std::lock_guard<std::mutex> guard(creationMutex);

        // Check if screen already exists
        bool exists = false;
        for (const auto& screen : createdProcesses) {
            if (screen->getName() == name) {
                exists = true;
                break;
            }
        }
        if (exists) {
            std::cout << "Error: screen '" << name << "' already exists.\n";
            return;
        }

        int nextId = createdProcesses.empty() ? 1 : (createdProcesses.back()->getId() + 1);

        // Create screen with custom instructions
        newScreen = std::make_shared<Screen>(nextId, name, static_cast<int>(instructionList.size()));
        
        // Set custom instructions
        newScreen->setCustomInstructions(instructionList);
        
        // Allocate memory for the process
        if (globalMemoryManager) {
            if (globalMemoryManager->allocateMemory(name, memSize)) {
                newScreen->setMemorySize(memSize);
                newScreen->setStatus("READY");
                std::cout << "Process '" << name << "' created with " << memSize << " bytes of memory and " 
                          << instructionList.size() << " custom instructions.\n";
            } else {
                std::cout << "Failed to allocate " << memSize << " bytes of memory for process '" << name << "'.\n";
                return;
            }
        } else {
            newScreen->setStatus("READY");
        }
        
        createdProcesses.push_back(newScreen);
    }

    // Add to scheduler
    if (globalScheduler && newScreen->getStatus() == "READY") {
        globalScheduler->addProcess(newScreen);

        // Start scheduler if not already running
        if (!schedulerRunning) {
            globalScheduler->start();
            schedulerRunning = true;
            std::cout << "Scheduler started\n";
        }
    }
}

void screenCommand(const std::string& dashOpt, const std::string& name, const std::string& memorySize) {
    if (dashOpt == "-s" && !name.empty()) {
        int memSize = 0;
        
        // Parse memory size if provided
        if (!memorySize.empty()) {
            try {
                memSize = std::stoi(memorySize);
                // Validate memory size (power of 2, between 8 and 65536)
                if (memSize < 8 || memSize > 65536 || (memSize & (memSize - 1)) != 0) {
                    std::cout << "Invalid memory allocation. Memory size must be a power of 2 between 8 and 65536 bytes.\n";
                    return;
                }
            } catch (const std::exception&) {
                std::cout << "Invalid memory size format.\n";
                return;
            }
        } else {
            // Use default memory size if not specified
            memSize = minMemoryPerProcess;
        }
        
        std::shared_ptr<Screen> newScreen = nullptr;
        {
            std::lock_guard<std::mutex> guard(creationMutex);

            // Check if screen already exists
            bool exists = false;
            for (const auto& screen : createdProcesses) {
                if (screen->getName() == name) {
                    exists = true;
                    break;
                }
            }
            if (exists) {
                std::cout << "Error: screen '" << name << "' already exists.\n";
                return;
            }

            int nextId = createdProcesses.empty() ? 1 : (createdProcesses.back()->getId() + 1);
            int totalBurst = minInstructions + (rand() % (maxInstructions - minInstructions + 1));

            newScreen = std::make_shared<Screen>(nextId, name, totalBurst);
            
            // Allocate memory for the process
            if (globalMemoryManager) {
                if (globalMemoryManager->allocateMemory(name, memSize)) {
                    newScreen->setMemorySize(memSize);
                    newScreen->setStatus("READY");
                    std::cout << "Process '" << name << "' created with " << memSize << " bytes of memory.\n";
                } else {
                    std::cout << "Failed to allocate " << memSize << " bytes of memory for process '" << name << "'.\n";
                    return;
                }
            } else {
                newScreen->setStatus("READY");
            }
            
            createdProcesses.push_back(newScreen);
        }

        // Add to scheduler
        if (globalScheduler && newScreen->getStatus() == "READY") {
            globalScheduler->addProcess(newScreen);

            // Start scheduler if not already running
            if (!schedulerRunning) {
                globalScheduler->start();
                schedulerRunning = true;
                std::cout << "Scheduler started\n";
            }
        }

        // Enter the new screen
        //enterScreen(newScreen);
    }
    else if (dashOpt == "-r" && !name.empty()) {
        std::shared_ptr<Screen> targetScreen = nullptr;

        {
            std::lock_guard<std::mutex> guard(creationMutex);
            for (const auto& screen : createdProcesses) {
                if (screen->getName() == name) {
                    targetScreen = screen;
                    break;
                }
            }
        }

        if (!targetScreen) {
            std::cout << "Process " << name << " not found.\n";
        }
        else if (targetScreen->hasMemoryAccessViolation()) {
            std::cout << "Process " << name << " shut down due to memory access violation error that occurred at " 
                      << targetScreen->getViolationTimestamp() << ". 0x" 
                      << std::hex << std::uppercase << targetScreen->getViolationAddress() 
                      << std::dec << " invalid.\n";
        }
        /*else if (targetScreen->isFinished()) {
            std::cout << "Process " << name << " not found.\n";
        }*/
        else {
            enterScreen(targetScreen);
        }
    }
    else if (dashOpt == "-ls") {
        if (globalScheduler) {
            globalScheduler->printStatus();
        }
        else {
            std::cout << "Scheduler is not running\n";
        }
    }
    else {
        std::cout << "Usage:\n"
            << "  screen -s <name>   create screen\n"
            << "  screen -r <name>   redraw screen\n"
            << "  screen -ls         list running/finished processes\n";
    }
}

void bootstrap(const std::string& configFile) {
    std::ifstream file(configFile);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open config file: " << configFile << "\n";
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string key;
        if (std::getline(iss, key, '=')) {
            std::string value;
            if (std::getline(iss, value)) {
                // Trim whitespace
                key.erase(key.find_last_not_of(" \t") + 1);
                value.erase(0, value.find_first_not_of(" \t"));
                value.erase(value.find_last_not_of(" \t") + 1);

                if (key == "num-cpu") coresUsed = std::stoi(value);
                else if (key == "scheduler") schedulingAlgorithm = value;
                else if (key == "quantum-cycles") quantumCycles = std::stoi(value);
                else if (key == "batch-process-freq") batchProcessFreq = std::stoi(value);
                else if (key == "min-ins") minInstructions = std::stoi(value);
                else if (key == "max-ins") maxInstructions = std::stoi(value);
                else if (key == "delay-per-exec") delaysPerExec = std::stoi(value);
                else if (key == "max-overall-mem") maxOverallMemory = std::stoi(value);
                else if (key == "mem-per-frame") memoryPerFrame = std::stoi(value);
                else if (key == "min-mem-per-proc") minMemoryPerProcess = std::stoi(value);
                else if (key == "max-mem-per-proc") maxMemoryPerProcess = std::stoi(value);
                else if (key == "is-evaluation-mode") {
                    if (value == "true") {
                        isEvaluationMode = true;
                    }
                    else {
						isEvaluationMode = false;
                    }
                }
            }
        }
    }

    // Initialize random seed
    srand(static_cast<unsigned int>(time(nullptr)));
}

void exportSchedulerReport() {
	char buffer[1024];

    if (GET_CURRENT_DIR(buffer, sizeof(buffer))) {
		fs::path filePath = fs::path(buffer) / "csopesy_log.txt";

        std::ofstream outFile(filePath);
        if (outFile) {
			auto oldCoutBuffer = std::cout.rdbuf();
			std::cout.rdbuf(outFile.rdbuf());

            if (globalScheduler) {
                globalScheduler->printStatus();
            }
            else {
                std::cout << "Scheduler is not running.\n";
            }

			std::cout.rdbuf(oldCoutBuffer);
            std::cout << "Report generated at: " << filePath << "\n";
        }
        else {
            std::cerr << "Error: Could not create report file\n";
        }
    }
    else {
        std::cerr << "Error: Could not get current directory\n";
    }
}

void OSLoop() {
    bool shouldExit = false;
    printHeader();

    while (!shouldExit) {
        std::string fullCommand;
        std::cout << "Enter a command: ";
        std::getline(std::cin, fullCommand);
        
        std::istringstream iss(fullCommand);
        std::string command, dashOpt, name, memorySize;
        iss >> command >> dashOpt >> name >> memorySize;

        if (command == "marquee") {
            marqueeConsole();
            system(CLEAR_COMMAND);
            printHeader();
        }
        else if (command == "nvidia-smi") {
            nvidiasmi(processes);
        }
        else if (command == "screen") {
            // Handle screen -c command specially
            if (dashOpt == "-c") {
                // Parse: screen -c <process_name> <memory_size> "<instructions>"
                std::istringstream fullIss(fullCommand);
                std::string cmd, opt, procName, memSize;
                fullIss >> cmd >> opt >> procName >> memSize;
                
                // Find the start of the instruction string (enclosed in quotes)
                size_t quoteStart = fullCommand.find('"');
                size_t quoteEnd = fullCommand.rfind('"');
                
                if (quoteStart != std::string::npos && quoteEnd != std::string::npos && quoteStart < quoteEnd) {
                    std::string instructions = fullCommand.substr(quoteStart + 1, quoteEnd - quoteStart - 1);
                    screenCommandWithInstructions(procName, memSize, instructions);
                } else {
                    std::cout << "Invalid command format. Use: screen -c <process_name> <memory_size> \"<instructions>\"\n";
                }
            } else {
                screenCommand(dashOpt, name, memorySize);
            }
        }
        else if (command == "initialize") {
            // Check if a specific config file is mentioned
            std::istringstream configIss(fullCommand);
            std::string initCmd, configFile;
            configIss >> initCmd >> configFile;
            
            if (configFile.empty()) {
                configFile = "config.txt";
            }
            
            bootstrap(configFile);
            std::cout << "System initialized with configuration:\n";
            std::cout << "  Number of Cores: " << coresUsed << "\n";
            std::cout << "  Scheduling Algorithm: " << schedulingAlgorithm << "\n";
            if (schedulingAlgorithm == "RR") {
				std::cout << "  Quantum Cycles: " << quantumCycles << "\n";
            }
			std::cout << "  Batch Process Frequency: " << batchProcessFreq << "\n";
			std::cout << "  Minimum Instructions: " << minInstructions << "\n";
			std::cout << "  Maximum Instructions: " << maxInstructions << "\n";
            std::cout << "  Delays per Execution: " << delaysPerExec << "\n";
            std::cout << "  Max Overall Memory: " << maxOverallMemory << " bytes\n";
            std::cout << "  Memory per Frame: " << memoryPerFrame << " bytes\n";
            std::cout << "  Min Memory per Process: " << minMemoryPerProcess << " bytes\n";
            std::cout << "  Max Memory per Process: " << maxMemoryPerProcess << " bytes\n";
            std::cout << "  Evaluation Mode: " << (isEvaluationMode ? "true" : "false") << "\n";

            // Initialize memory manager
            if (!globalMemoryManager) {
                globalMemoryManager = std::make_unique<MemoryManager>(maxOverallMemory, memoryPerFrame);
                std::cout << "Memory manager initialized\n";
            }

            // Initialize scheduler based on configuration
            if (!globalScheduler) {
                // FCFS Scheduler
                if (schedulingAlgorithm == "FCFS") {
                    globalScheduler = std::make_unique<FCFSScheduler>(coresUsed, delaysPerExec);
                    std::cout << "FCFS scheduler initialized\n";
                }
                // RR Scheduler here
                else if (schedulingAlgorithm == "RR") {
                    globalScheduler = std::make_unique<RRScheduler>(coresUsed, delaysPerExec, quantumCycles);
                    std::cout << "RR scheduler initialized (Quantum: " << quantumCycles << ")\n";
                }
            }
        }
        else if (command == "scheduler-start") {
            if (!globalScheduler) {
                std::cout << "Error: Scheduler not initialized. Run 'initialize' first.\n";
                continue;
            }

            if (!schedulerRunning) {
                // Start scheduler
                globalScheduler->start();
                schedulerRunning = true;
                std::cout << "Scheduler started\n";
            }

            if (!automaticCreationEnabled) {
                automaticCreationEnabled = true;
                automaticCreationThread = std::thread(automaticProcessCreation);
                std::cout << "Automatic process creation started\n";

                if (isEvaluationMode) {
                    std::cout << "Evaluation mode: Will create 10 processes\n";
                }
            }
            else {
                std::cout << "Automatic creation already running\n";
            }
        }
        else if (command == "scheduler-stop") {
            if (automaticCreationEnabled) {
                {
                    std::lock_guard<std::mutex> lk(creationMutex);
                    automaticCreationEnabled = false;
                }
                creationCV.notify_all();
                if (automaticCreationThread.joinable()) {
                    automaticCreationThread.join();
                }
                std::cout << "Automatic process creation stopped\n";
            }
            else {
                std::cout << "Automatic creation not running\n";
            }
        }
        else if (command == "process-smi") {
            if (globalMemoryManager) {
                globalMemoryManager->printMemoryStatus();
                globalMemoryManager->printProcessMemoryUsage();
            } else {
                std::cout << "Memory manager not initialized. Run 'initialize' first.\n";
            }
        }
        else if (command == "vmstat") {
            if (globalMemoryManager) {
                MemoryStats stats = globalMemoryManager->getStats();
                std::cout << "=========================================================================\n";
                std::cout << "Memory Statistics:\n";
                std::cout << "Total Memory: " << stats.totalMemory << " bytes\n";
                std::cout << "Used Memory: " << stats.usedMemory << " bytes\n";
                std::cout << "Free Memory: " << stats.freeMemory << " bytes\n";
                std::cout << "Idle CPU Ticks: " << stats.idleCpuTicks << "\n";
                std::cout << "Active CPU Ticks: " << stats.activeCpuTicks << "\n";
                std::cout << "Total CPU Ticks: " << stats.totalCpuTicks << "\n";
                std::cout << "Num Paged In: " << stats.numPagedIn << "\n";
                std::cout << "Num Paged Out: " << stats.numPagedOut << "\n";
                std::cout << "=========================================================================\n";
            } else {
                std::cout << "Memory manager not initialized. Run 'initialize' first.\n";
            }
        }
        else if (command == "report-util") {
            exportSchedulerReport();
        }
        else if (command == "clear") {
            system(CLEAR_COMMAND);
            printHeader();
        }
        else if (command == "exit") {
            if (automaticCreationEnabled) {
                {
                    std::lock_guard<std::mutex> lk(creationMutex);
                    automaticCreationEnabled = false;
                }
                creationCV.notify_all();
                if (automaticCreationThread.joinable()) {
                    automaticCreationThread.join();
                }
                std::cout << "Automatic process creation stopped\n";
            }

            // Stop scheduler if running
            if (globalScheduler && schedulerRunning) {
                schedulerRunning = false;
                globalScheduler->stop();
                globalScheduler.reset();
                std::cout << "Scheduler stopped and destroyed\n";


                // Export logs for finished processes
                {
                    std::lock_guard<std::mutex> guard(finishedMutex);
                    for (auto& process : finishedProcesses) {
                        process->exportLogs();
                    }
                }
            }
            shouldExit = true;
            }
        else {
            std::cout << "Unknown command.\n";
        }
    }
}
