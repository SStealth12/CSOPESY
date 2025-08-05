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
#include <map>
#include <stack>
#include <cctype>
#include <random>
#include <cstdint>

class Screen {
public:
	struct LogEntry {
		std::string timestamp;
		int coreId;
		std::string message;
	};

	struct Instruction {
		enum Type {
			PRINT,
			DECLARE,
			ADD,
			SUBTRACT,
			SLEEP,
			FOR,
			ENDLOOP,
			READ,
			WRITE
		} type;
		std::vector<std::string> args;
	};

	struct LoopContext {
		int iterations;
		int depth;
		int currentIteration;
		size_t startIndex; 
		size_t loopBodySize;
	};

	Screen(int id, const std::string& name, int totalBurst);

	void draw() const;
	void addLogEntry(int coreId, const std::string& message);
	void exportLogs() const;
	void printLogs() const;
	void executeInstruction(int coreId);
	void generateInstructions();
	void setCustomInstructions(const std::vector<std::string>& customInstructions);
	uint16_t getVariableValue(const std::string& varName);
	void setVariableValue(const std::string& varName, uint16_t value);

	// Memory management methods
	void setMemorySize(int memorySize) { memorySize_ = memorySize; }
	int getMemorySize() const { return memorySize_; }
	void setMemoryValue(uint32_t address, uint16_t value) { memoryData_[address] = value; }
	const std::map<uint32_t, uint16_t>& getMemoryData() const { return memoryData_; }
	
	// Memory access violation handling
	void setMemoryAccessViolation(const std::string& timestamp, uint32_t address) {
		hasMemoryViolation_ = true;
		violationTimestamp_ = timestamp;
		violationAddress_ = address;
	}
	bool hasMemoryAccessViolation() const { return hasMemoryViolation_; }
	const std::string& getViolationTimestamp() const { return violationTimestamp_; }
	uint32_t getViolationAddress() const { return violationAddress_; }

	// Getters and Setters
	const std::string& getName() const { return name_; }
	int getId() const { return id_; }
	int getCurrentBurst() const { return currentBurst_; }
	int getTotalBurst() const { return totalBurst_; }
	const std::string& getStatus() const { return status_; }
	const std::string& getCreateTimestamp() const { return createTimestamp_; }
	void setStatus(const std::string& status) { status_ = status; }
	void incrementCurrentBurst() { currentBurst_++; }
	bool isFinished() const {
		return pc_ >= instructions_.size() && sleepTicksRemaining_ == 0;
	}

private:
	std::string name_;
	int id_;
	std::string status_;
	int currentBurst_;
	int totalBurst_;
	std::string createTimestamp_;
	std::vector<LogEntry> logEntries_;
	std::vector<Instruction> instructions_;

	std::map<std::string, uint16_t> variables_;
	std::stack<LoopContext> loopStack_;
	
	int sleepTicksRemaining_ = 0;
	
	bool skippingTooDeepLoop_ = false;
	int skipDepth_ = 0;

	size_t pc_ = 0; // Program counter

	// Memory management members
	int memorySize_ = 0;
	std::map<uint32_t, uint16_t> memoryData_;
	
	// Memory access violation members
	bool hasMemoryViolation_ = false;
	std::string violationTimestamp_;
	uint32_t violationAddress_ = 0;


	std::string getCurrentTimeStamp();

	uint16_t parseValue(const std::string& str);

	void generateInstructionsRecursive(int& instructionsGenerated, int depth, std::mt19937& gen);
	Instruction generateRandomNonLoopInstruction(std::mt19937& gen);
};

#endif // SCREEN_H
