#include "Process.h"
#include <iostream>
#include <iomanip>
#include <sstream>

Process::Process(int pid, const std::string& name, const std::string& type, float gpuMemoryUsage)
	:pid_(pid), type_(type), name_(name), gpuMemoryUsage_(gpuMemoryUsage) {}

std::string Process::formatGPUMemoryUsage() const {
	if (gpuMemoryUsage_ == 0.0f) return "N/A";

	std::ostringstream oss;
	oss << std::fixed << std::setprecision(2) << gpuMemoryUsage_;
	return oss.str();
}

std::string Process::formatProcessName() const {
	const size_t maxLength = 32;

	if (name_.length() <= maxLength) { 
		return name_; 
	}
	
	return "..." + name_.substr(name_.length() - (maxLength - 3));
}

int Process::getPid() const {
	return pid_;
}

const std::string& Process::getType() const {
	return type_;
}

std::string Process::getFormattedName() const {
	return formatProcessName();
}

std::string Process::getFormattedGpuUsage() const {
	return formatGPUMemoryUsage();
}