#pragma once
#include <queue>
#include <vector>
#include <memory>

#include "Scheduler.h"
#include "Screen.h"

class RRScheduler : public Scheduler {
private:
    int delaysPerExec_;
    int quantumCycles_;
    std::queue<std::shared_ptr<Screen>> readyQueue;
    std::vector<std::shared_ptr<Screen>> finishedProcesses;

public:
    RRScheduler(int cores, int delaysPerExec, int quantumCycles);
    virtual ~RRScheduler();

    void addProcess(std::shared_ptr<Screen> process) override;
    void printStatus() override;
    bool allProcessesFinished() override;
    void writeFinishedProcessLogs();
    void stop() override;

protected:
    void cpuWorker(int coreId) override;
    void scheduler() override;
};