#pragma once
#include <queue>
#include <vector>
#include <memory>

#include "Scheduler.h"
#include "Screen.h"

class FCFSScheduler : public Scheduler {
private:
	int delaysPerExec_;
	std::queue<std::shared_ptr<Screen>> readyQueue;
	std::vector<std::shared_ptr<Screen>> finishedProcesses;
public:
	FCFSScheduler(int cores, int delaysPerExec);
	virtual ~FCFSScheduler();

	void addProcess(std::shared_ptr<Screen> process) override;
	void printStatus() override;
	bool allProcessesFinished() override;
	void writeFinishedProcessLogs();
protected:
	void cpuWorker(int coreId) override;
	void scheduler() override;
};