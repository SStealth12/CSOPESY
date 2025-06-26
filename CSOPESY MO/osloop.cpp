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
	int nextId = createdProcesses.empty() ? 1 : (createdProcesses.back()->getId() + 1);

    while (automaticCreationEnabled) {
        cycleCount++;

        
        if (cycleCount % batchProcessFreq == 0) {
            std::ostringstream nameStream;

            nameStream << "screen_" << std::setw(2) << std::setfill('0') << nextId;
            std::string name = nameStream.str();

            int totalBurst = minInstructions + (rand() % (maxInstructions - minInstructions + 1));

            auto screen = std::make_shared<Screen>(nextId, name, totalBurst);

            {
                std::lock_guard<std::mutex> guard(createdMutex);
                createdProcesses.push_back(screen);
            }

            if (globalScheduler) {
                screen->setStatus("READY");
                globalScheduler->addProcess(screen);
            }

            // std::cout << "Screen '" << name << "' created (Burst: " << totalBurst << ").\n";
            nextId++;
            createdCount++;
        }
		// Sleep given delaysPerExec config parameter
        std::this_thread::sleep_for(std::chrono::milliseconds(delaysPerExec));

        if (isEvaluationMode && createdCount >= 10) {
            automaticCreationEnabled = false;
            automaticThreadJoined = true;
            //std::cout << "Evaluation mode: Created 10 processes. Automatic creation stopped.\n";
        }
    }
}

void screenCommand(const std::string& dashOpt, const std::string& name) {
    if (dashOpt == "-s" && !name.empty()) {
        std::lock_guard<std::mutex> guard(createdMutex);

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
            std::lock_guard<std::mutex> guard(createdMutex);
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
    printHeader();

    while (true) {
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
                if (schedulingAlgorithm == "FCFS") {
                    globalScheduler = std::make_unique<FCFSScheduler>(coresUsed, delaysPerExec);
                    std::cout << "FCFS scheduler initialized\n";
                }
                // Add RR Scheduler here
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
                automaticCreationEnabled = false;
                if (automaticCreationThread.joinable() && !automaticThreadJoined) {
                    automaticCreationThread.join();
					automaticThreadJoined = true;
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
            // Stop automatic process creation
            if (automaticCreationEnabled) {
                automaticCreationEnabled = false;
                if (automaticCreationThread.joinable()) {
                    automaticCreationThread.join();
                }
            }

            // Stop scheduler if running
            if (globalScheduler && schedulerRunning) {
                globalScheduler->stop();

                // Export logs for finished processes
                {
                    std::lock_guard<std::mutex> guard(finishedMutex);
                    for (const auto& process : finishedProcesses) {
                        process->exportLogs();
                    }
                }
            }
            exit(0);
        }
        else {
            std::cout << "Unknown command.\n";
        }
    }
}