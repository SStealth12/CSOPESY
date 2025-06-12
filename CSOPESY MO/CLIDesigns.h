#ifndef SCREEN_HEADER_H
#define SCREEN_HEADER_H

#include <iostream>
#include <iomanip>
#include <map>
#include "Process.h"

inline void printHeader() {
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
    std::cout << "Enter a command: " << std::flush;
}


inline void nvidiasmi (const std::map<int, Process>& processes) {
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


#endif // !SCREEN_HEADER_H