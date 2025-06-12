#ifndef PROCESS_H
#define PROCESS_H


#include <string>

class Process
{
private:
	int pid_;
	std::string type_;
	std::string name_;
	float gpuMemoryUsage_;

	std::string formatProcessName() const;
	std::string formatGPUMemoryUsage() const;

public:
	Process(int pid, const std::string& name, const std::string& type = "C+G", float gpuMemoryUsage = 0.0f);

	int getPid() const;
	const std::string& getType() const;
	std::string getFormattedName() const;
	std::string getFormattedGpuUsage() const;
};


#endif