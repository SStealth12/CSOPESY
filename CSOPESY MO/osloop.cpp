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

            // Generate process details (no lock needed here)
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
                createdProcesses.push_back(screen);
                nextId++;
            }

            // Schedule the process (no lock needed)
            if (globalScheduler) {
                screen->setStatus("READY");
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

void screenCommand(const std::string& dashOpt, const std::string& name) {
    if (dashOpt == "-s" && !name.empty()) {
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
        }
        else {
			int nextId = createdProcesses.empty() ? 1 : (createdProcesses.back()->getId() + 1);

            int totalBurst = minInstructions + (rand() % (maxInstructions - minInstructions + 1));

			auto screen = std::make_shared<Screen>(nextId, name, totalBurst);
			createdProcesses.push_back(screen);
            std::cout << "Screen '" << name << "' created (Burst: " << totalBurst << ").\n";
        }
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
            std::cout << "Error: no screen '" << name << "'\n";
        }
        else if (targetScreen->isFinished()) {
            std::cout << "Process " << name << " not found.\n";
        }
        else {
            targetScreen->draw();

            // Sub-command loop
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
                    targetScreen->printLogs();
                }
                else if (sub == "report-util") {
                    targetScreen->exportLogs();
                    std::cout << "Report generated as: " << targetScreen->getName() << ".txt\n";
                }
                else if (sub == "execute") {
                    if (!targetScreen->isFinished()) {
						targetScreen->executeInstruction(-1); // -1 for manual execution
                        std::cout << "Executed one instuction\n";
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
    // Initialize local variables
    bool schedulerRunning = false;
    bool shouldExit = false;
    printHeader();

    while (!shouldExit) {
        std::string command, dashOpt, name;
        std::cout << "Enter a command: ";
        std::getline(std::cin, command);
        std::istringstream iss(command);
        iss >> command >> dashOpt >> name;

        if (command == "marquee") {
            marqueeConsole();
            system(CLEAR_COMMAND);
            printHeader();
        }
        else if (command == "nvidia-smi") {
            nvidiasmi(processes);
        }
        else if (command == "screen") {
            screenCommand(dashOpt, name);
        }
        else if (command == "initialize") {
            bootstrap();
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
            std::cout << "  Evaluation Mode: " << (isEvaluationMode ? "true" : "false") << "\n";

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