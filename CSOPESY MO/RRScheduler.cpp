#include "RRScheduler.h"
#include "Screen.h"
#include <iostream>
#include <fstream>
#include <chrono>
#include <iomanip>

RRScheduler::RRScheduler(int cores, int delaysPerExec, int quantumCycles)
    : Scheduler(cores), delaysPerExec_(delaysPerExec), quantumCycles_(quantumCycles) {
}

RRScheduler::~RRScheduler() {
    stop();
}

void RRScheduler::cpuWorker(int coreId) {
    while (cpuCores[coreId].running) {
        std::unique_lock<std::mutex> lock(queueMutex);
        schedulerCV.wait(lock, [this, coreId] {
            return !cpuCores[coreId].running || cpuCores[coreId].currentProcess != nullptr;
            });

        if (!cpuCores[coreId].running) break;

        auto process = cpuCores[coreId].currentProcess;
        lock.unlock();

        if (process) {
            int executedInQuantum = 0;
            bool processFinished = false;

            while (executedInQuantum < quantumCycles_ && cpuCores[coreId].running) {
                if (process->getCurrentBurst() >= process->getTotalBurst()) {
                    processFinished = true;
                    break;
                }

                process->executeInstruction(coreId);
                executedInQuantum++;

                std::this_thread::sleep_for(std::chrono::milliseconds(delaysPerExec_));
            }

            if (processFinished || process->getCurrentBurst() >= process->getTotalBurst()) {
                {
                    std::lock_guard<std::mutex> finishedLock(finishedMutex);
                    process->setStatus("FINISHED");
                    finishedProcesses.push_back(process);
                }
            }
            else {
                std::lock_guard<std::mutex> queueLock(queueMutex);
                process->setStatus("READY");
                readyQueue.push(process);
            }

            {
                std::lock_guard<std::mutex> coreLock(queueMutex);
                cpuCores[coreId].isBusy = false;
                cpuCores[coreId].currentProcess = nullptr;
            }
            schedulerCV.notify_all();
        }
    }
}

void RRScheduler::scheduler() {
    while (running) {
        std::unique_lock<std::mutex> lock(queueMutex);

        for (int i = 0; i < cpuCores.size(); i++) {
            if (!cpuCores[i].isBusy && !readyQueue.empty()) {
                auto process = std::move(readyQueue.front());
                readyQueue.pop();

                cpuCores[i].isBusy = true;
                cpuCores[i].currentProcess = process;
                process->setStatus("RUNNING");
                schedulerCV.notify_all();
            }
        }

        lock.unlock();
        std::this_thread::sleep_for(std::chrono::milliseconds(delaysPerExec_));
    }
}

void RRScheduler::stop() {
    if (!running) return;

    running = false;
    schedulerCV.notify_all();

    if (schedulerThread.joinable()) {
        schedulerThread.join();
    }

    for (auto& core : cpuCores) {
        core.running = false;
    }

    schedulerCV.notify_all();

    for (auto& core : cpuCores) {
        if (core.workerThread.joinable()) {
            core.workerThread.join();
        }
    }

    writeFinishedProcessLogs();
}

void RRScheduler::addProcess(std::shared_ptr<Screen> process) {
    std::lock_guard<std::mutex> lock(queueMutex);
    process->setStatus("READY");
    readyQueue.push(process);
    schedulerCV.notify_all();
}

void RRScheduler::printStatus() {
    std::lock_guard<std::mutex> printLock(printMutex);
    std::lock_guard<std::mutex> queueLock(queueMutex);
    std::lock_guard<std::mutex> finishedLock(finishedMutex);

    // Calculate busy cores
    int busyCores = 0;
    for (const auto& core : cpuCores) {
        if (core.isBusy) {
            busyCores++;
        }
    }

    // Calculate CPU utilization
    int utilization = (busyCores / cpuCores.size()) * 100;

    // Print dynamic status
    std::cout << "\nCPU utilization: "
        << utilization << "%\n";
    std::cout << "Cores used: " << busyCores << "\n";
    std::cout << "Cores available: " << (cpuCores.size() - busyCores) << "\n";
    std::cout << "--------------------------------------\n";
    std::cout << "\nRunning processes:\n";
    for (int i = 0; i < cpuCores.size(); i++) {
        if (cpuCores[i].isBusy && cpuCores[i].currentProcess) {
            const auto& process = cpuCores[i].currentProcess;
            std::string timeStr = getCurrentTimeString();
            std::cout << process->getName() << "\t(" << process->getCreateTimestamp() << ")\tCore: " << i
                << "\t" << process->getCurrentBurst() << " / " << process->getTotalBurst() << "\n";
        }
    }

    std::cout << "\nFinished processes:\n";
    for (const auto& process : finishedProcesses) {
        std::string timeStr = getCurrentTimeString();
        std::cout << process->getName() << "\t(" << process->getCreateTimestamp() << ")\t"
            << "Finished  " << process->getTotalBurst() << " / " << process->getTotalBurst() << "\n";
    }
    std::cout << "--------------------------------------\n";
}

bool RRScheduler::allProcessesFinished() {
    std::lock_guard<std::mutex> queueLock(queueMutex);
    std::lock_guard<std::mutex> finishedLock(finishedMutex);

    for (const auto& core : cpuCores) {
        if (core.isBusy) {
            return false;
        }
    }

    return readyQueue.empty();
}

void RRScheduler::writeFinishedProcessLogs() {
    std::lock_guard<std::mutex> finishedLock(finishedMutex);
    for (const auto& process : finishedProcesses) {
        process->exportLogs();
    }
}