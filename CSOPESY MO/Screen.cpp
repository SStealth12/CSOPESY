#include "Screen.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <ctime>
#include <cstdlib>
#include <algorithm>

#ifdef _WIN32
#define CLEAR_COMMAND "cls"
#else
#include <unistd.h>
#define CLEAR_COMMAND "clear"
#endif


Screen::Screen(int id, const std::string& name, int totalBurst)
	: id_(id), name_(name), status_("CREATED"), currentBurst_(0), totalBurst_(totalBurst) {
	createTimestamp_ = getCurrentTimeStamp();
}

std::string Screen::getCurrentTimeStamp() {
	auto now = std::chrono::system_clock::now();
	auto in_time_t = std::chrono::system_clock::to_time_t(now);
	std::tm tm;
#ifdef _WIN32
	localtime_s(&tm, &in_time_t);
#else
	localtime_r(&in_time_t, &tm);
#endif
	std::stringstream ss;
	ss << std::put_time(&tm, "%m/%d/%Y %I:%M:%S%p");
	return ss.str();
}

void Screen::addLogEntry(int coreId, const std::string& message) {
	logEntries_.push_back({ getCurrentTimeStamp(), coreId, message });
}

void Screen::exportLogs() const {
	std::ofstream file(name_ + ".txt");
	file << "Process name: " << name_ << "\n";
	file << "ID: " << id_ << "\n";
	file << "Logs:\n";
	for (const auto& log : logEntries_) {
		file << log.message << "\n";
	}

	file << "\nCurrent instruction line: " << currentBurst_ << "\n";
	file << "Lines of code: " << totalBurst_ << "\n";

	if (currentBurst_ >= totalBurst_) {
		file << "Finished!\n";
	}
}

void Screen::printLogs() const {
	std::cout << "\nProcess name: " << name_ << "\n";
	std::cout << "ID: " << id_ << "\n";
	std::cout << "Logs:\n";

	if (logEntries_.empty()) {
		std::cout << "  No logs available\n";
	}
	else {
		for (const auto& log : logEntries_) {
			std::cout << log.message << "\n";
		}
	}

	std::cout << "\nCurrent instruction line: " << currentBurst_ << "\n";
	std::cout << "Lines of code: " << totalBurst_ << "\n";
	
	if (currentBurst_ >= totalBurst_) {
		std::cout << "Finished!\n";
	}
}

void Screen::draw() const {
	system(CLEAR_COMMAND);

	std::cout << "==============================		SCREEN		========================\n\n";
	std::cout << "Process name:			" << name_ << "\n";
	std::cout << "Instruction line:		" << currentBurst_ << " / " << totalBurst_ << "\n";
	std::cout << "Created at:			" << createTimestamp_ << "\n\n";
	std::cout << "================================================================================\n";
	std::cout << "Type 'exit' to return to main menu\n";
	std::cout << "Type 'process-smi' to view logs\n";
	std::cout << "Type 'report-util' to export detailed report\n";
}