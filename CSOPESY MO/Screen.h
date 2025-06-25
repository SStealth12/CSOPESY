#pragma once
#ifndef SCREEN_H
#define SCREEN_H

#include <string>
#include <vector>
#include <queue>
#include <chrono>
#include <memory>
#include <iomanip>
#include <sstream>
#include <ctime>
#include <fstream>

class Screen {
public:
	struct LogEntry {
		std::string timestamp;
		int coreId;
		std::string message;
	};

	Screen(int id, const std::string& name, int totalBurst);

	void draw() const;
	void addLogEntry(int coreId, const std::string& message);
	void exportLogs() const;
	void printLogs() const;

	// Getters and Setters
	const std::string& getName() const { return name_; }
	int getId() const { return id_; }
	int getCurrentBurst() const { return currentBurst_; }
	int getTotalBurst() const { return totalBurst_; }
	const std::string& getStatus() const { return status_; }
	const std::string& getCreateTimestamp() const { return createTimestamp_; }
	void setStatus(const std::string& status) { status_ = status; }
	void incrementCurrentBurst() { currentBurst_++; }

private:
	std::string name_;
	int id_;
	std::string status_;
	int currentBurst_;
	int totalBurst_;
	std::string createTimestamp_;
	std::vector<LogEntry> logEntries_;

	std::string getCurrentTimeStamp();

};

#endif // SCREEN_H