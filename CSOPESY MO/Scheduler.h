#pragma once
#include <vector>
#include <memory>
#include <queue>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <iomanip>
#include <sstream>

#include "Screen.h"

struct CPUCore {
	int coreId;
	bool isBusy;
	std::shared_ptr<Screen> currentProcess;
	std::thread workerThread;
	std::atomic<bool> running;

	CPUCore(const CPUCore&) = delete;
	CPUCore& operator=(const CPUCore&) = delete;

	// custom move ctor
	CPUCore(CPUCore&& o) noexcept
		: coreId(o.coreId),
		isBusy(o.isBusy),
		currentProcess(std::move(o.currentProcess)),
		workerThread(std::move(o.workerThread)),
		running(o.running.load()){}

	// custom move assignment
	CPUCore& operator=(CPUCore&& o) noexcept {
		if (this != &o) {
			coreId = o.coreId;
			isBusy = o.isBusy;
			currentProcess = std::move(o.currentProcess);
			workerThread = std::move(o.workerThread);
			running.store(o.running.load());
		}
		return *this;
	}

	CPUCore(int id)
		: coreId(id),
		isBusy(false),
		currentProcess(nullptr),
		running(true)
	{}
};


class Scheduler {
protected:
	std::vector<CPUCore> cpuCores;
	std::thread schedulerThread;
	std::atomic<bool> running;

	std::mutex queueMutex;
	std::mutex finishedMutex;
	std::mutex printMutex;
	std::condition_variable schedulerCV;

	int prcessCounter = 1;

	virtual void cpuWorker(int coreId) = 0;
	virtual void scheduler() = 0;

public:
	virtual ~Scheduler() = default;

	Scheduler(int cores) : running(true) {
		cpuCores.reserve(cores);
		for (int i = 0; i < cores; ++i) {
			cpuCores.emplace_back(i);
		}
	}

	virtual void start() {
		running = true;
		for (auto& core : cpuCores) {
			core.workerThread = std::thread(&Scheduler::cpuWorker, this, core.coreId);
		}

		schedulerThread = std::thread(&Scheduler::scheduler, this);
	}

	virtual void stop() {
		running = false;
		schedulerCV.notify_all();

		for (auto& core : cpuCores) {
			core.running = false;
			if (core.workerThread.joinable()) {
				core.workerThread.join();
			}
		}

		if (schedulerThread.joinable()) {
			schedulerThread.join();
		}
	}

	virtual void addProcess(std::shared_ptr<Screen> process) = 0;
	virtual void printStatus() = 0;
	virtual bool allProcessesFinished() = 0;

	std::string getCurrentTimeString() {
		auto now = std::chrono::system_clock::now();
		auto time_t = std::chrono::system_clock::to_time_t(now);
		std::tm timeInfo;

#ifdef _WIN32
		localtime_s(&timeInfo, &time_t);
#else
		localtime_r(&time_t, &timeInfo);
#endif // _WIN32
		
		std::stringstream ss;
		ss << std::put_time(&timeInfo, "%m/%d/%Y %I:%M:%S");
		ss << (timeInfo.tm_hour >= 12 ? "PM" : "AM");
		return ss.str();

	}
};