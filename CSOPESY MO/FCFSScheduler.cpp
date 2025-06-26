#include "FCFSScheduler.h"
#include "Screen.h"
#include <iostream>
#include <fstream>
#include <chrono>
#include <iomanip>

FCFSScheduler::FCFSScheduler(int cores, int delaysPerExec)
	: Scheduler(cores), delaysPerExec_(delaysPerExec) {
}


FCFSScheduler::~FCFSScheduler() {
	stop();
}

void FCFSScheduler::cpuWorker(int coreId) {
	while (cpuCores[coreId].running) {
		std::unique_lock<std::mutex> lock(queueMutex);
		schedulerCV.wait(lock, [this, coreId] {
			return !cpuCores[coreId].running ||
				cpuCores[coreId].currentProcess != nullptr;
			});

		if (!cpuCores[coreId].running) break;

		auto& process = cpuCores[coreId].currentProcess;
		lock.unlock();

		if (process) {
			while (process->getCurrentBurst() < process->getTotalBurst() &&
				cpuCores[coreId].running) {
				process->executeInstruction(coreId);

				std::this_thread::sleep_for(std::chrono::milliseconds(delaysPerExec_));
				if (!cpuCores[coreId].running) break;
			}

			{
				std::lock_guard<std::mutex> finishedLock(finishedMutex);
				finishedProcesses.push_back(process);
				process->setStatus("FINISHED");
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

void FCFSScheduler::scheduler() {
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

void FCFSScheduler::stop() {
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

void FCFSScheduler::addProcess(std::shared_ptr<Screen> process) {
	std::lock_guard<std::mutex> lock(queueMutex);
	process->setStatus("READY");
	readyQueue.push(process);
	schedulerCV.notify_all();
}

void FCFSScheduler::printStatus() {
	std::lock_guard<std::mutex> printLock(printMutex);
	std::lock_guard<std::mutex> queueLock(queueMutex);
	std::lock_guard<std::mutex> finishedLock(finishedMutex);

	std::cout << "CPU utilization: 100%\n";
	std::cout << "Cores used: " << cpuCores.size() << "\n";
	std::cout << "Cores available: 0\n";
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
			<< process->getTotalBurst() << " / " << process->getTotalBurst() << "\n";
	}
	std::cout << "--------------------------------------\n";
}

bool FCFSScheduler::allProcessesFinished() {
	std::lock_guard<std::mutex> queueLock(queueMutex);
	std::lock_guard<std::mutex> finishedLock(finishedMutex);

	for (const auto& core : cpuCores) {
		if (core.isBusy) {
			return false;
		}
	}

	return readyQueue.empty();
}

void FCFSScheduler::writeFinishedProcessLogs() {
	std::lock_guard<std::mutex> finishedLock(finishedMutex);
	for (const auto& process : finishedProcesses) {
		process->exportLogs();
	}
}