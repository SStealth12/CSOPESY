// Group 13: Miguel Estanol, Aaron Nicolas
// Section: S19

#include <iostream>
#include <string>
#include <map>
#include <sstream>
#include <vector>
#include <ctime>
#include <cstdlib>

#include "Screen.h"
#include "Process.h"
#include "CLIDesigns.h"
#include "Subcommand.h"
#include "Marquee.h"

#ifdef _WIN32
#include <cstdlib>
#define CLEAR_COMMAND "cls"
#else
#include <unistd.h>
#define CLEAR_COMMAND "clear"
#endif

int main() {
    // Initialize Screens Map
    std::map<std::string, Screen> screens;

    // TEMP: Seed for PID number generation
	srand(static_cast<unsigned int>(time(0))); 

    // TEMP: Create Process Names
    std::vector<std::string> processNames = {
        "C:\\Program Files\\NVIDIA Corporation\\NVIDIA GeForce Experience\\NVIDIA app\\CEF\\NVIDIA Overlay.exe",
        "C:\\Program Files\\Google\\Drive File Stream\\109.0.3.0\\GoogleDriveFS.exe",
        "C:\\Program Files (x86)\\Google\\Chrome\\Application\\chrome.exe",
        "C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\Common7\\IDE\\devenv.exe",
        "C:\\Users\\YourUser\\AppData\\Local\\Discord\\app-1.0.9193\\Discord.exe"
	};

	// Intialize Processes Map
    std::map<int, Process> processes;

    for (const auto& name : processNames) {
        int pid = std::rand() % 9000 + 1000;
        processes.emplace(pid, Process{pid, name});
    }

    printHeader();

    while (true) {
        std::string command, dashOpt, name;
		std::getline(std::cin, command);
        std::istringstream iss(command);
		iss >> command >> dashOpt >> name;

        if (command == "marquee") {
            marqueeConsole();
			system(CLEAR_COMMAND); // Clear the console after marquee
			printHeader(); // Print the header again after marquee
        }
        else if (command == "nvidia-smi") {
            nvidiasmi(processes);
        }
        else if (command == "screen") {
            screenCommand(screens, dashOpt, name);
        }        
        else if (command == "initialize") {
            std::cout << "initialize command recognized. Doing something." << std::endl;
        }
        else if (command == "screen") {
            std::cout << "screen command recognized. Doing something." << std::endl;
        }
        else if (command == "scheduler-test") {
            std::cout << "scheduler-test command recognized. Doing something." << std::endl;
        }
        else if (command == "scheduler-stop") {
            std::cout << "scheduler-stop command recognized. Doing something." << std::endl;
        }
        else if (command == "report-util") {
            std::cout << "report-util command recognized. Doing something." << std::endl;
        }
        else if (command == "clear") {
            std::cout << "clear command recognized. Doing something." << std::endl;
            system(CLEAR_COMMAND);
            printHeader();
            continue;  // Skip printing prompt
        }
        else if (command == "exit") {
            std::cout << "exit command recognized. Doing something." << std::endl;
            exit(0); // Instantly closes the app
        }
        else {
            std::cout << "Unknown command." << std::endl;
        }

        std::cout << "Enter a command: " << std::flush;
    }

    return 0;
}