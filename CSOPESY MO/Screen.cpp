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
#include <random>

// Helper function to trim whitespace from both ends of a string
std::string trim(const std::string& str) {
	size_t first = str.find_first_not_of(' ');
	if (first == std::string::npos) return ""; // No non-whitespace characters
	size_t last = str.find_last_not_of(' ');
	return str.substr(first, (last - first + 1));
}

Screen::Screen(int id, const std::string& name, int totalBurst)
	: id_(id), name_(name), status_("CREATED"),
	currentBurst_(0), totalBurst_(totalBurst)
{	
	createTimestamp_ = getCurrentTimeStamp();
	generateInstructions();
}

void Screen::generateInstructions() {
	std::random_device rd;
	std::mt19937 gen(rd());
	instructions_.clear();
	int instructionsGenerated = 0;
	std::stack<LoopContext> loopStack;

	// Distributions
	std::uniform_int_distribution<> typeDist(0, 5);
	std::uniform_int_distribution<> loopChanceDist(0, 9);
	std::uniform_int_distribution<> iterDist(2, 5);
	std::uniform_int_distribution<> varDist(0, 25);
	std::uniform_int_distribution<> valueDist(0, 99);
	std::uniform_int_distribution<> sleepDist(1, 5);

	while (instructionsGenerated < totalBurst_) {
		// Handle loop closing first
		if (!loopStack.empty() && loopStack.top().loopBodySize > 0 &&
			instructions_.size() == loopStack.top().startIndex + loopStack.top().loopBodySize) {

			LoopContext context = loopStack.top();
			loopStack.pop();

			Instruction endLoop;
			endLoop.type = Instruction::ENDLOOP;
			instructions_.push_back(endLoop);
			instructionsGenerated++;

			// Account for loop iterations
			int remainingInstructions = totalBurst_ - instructionsGenerated;
			int iterationsPossible = remainingInstructions / context.loopBodySize;

			if (iterationsPossible < context.iterations) {
				context.iterations = iterationsPossible;
			}

			// Store updated context
			loopStack.push(context);
			continue;
		}

		// Start new loop if possible
		if (loopStack.size() < 3 &&
			(totalBurst_ - instructionsGenerated) > 2 &&
			loopChanceDist(gen) == 0) {

			LoopContext newLoop;
			newLoop.iterations = iterDist(gen);
			newLoop.currentIteration = 0;
			newLoop.startIndex = instructions_.size();
			newLoop.loopBodySize = 0;  // Will be calculated later

			Instruction forInst;
			forInst.type = Instruction::FOR;
			forInst.args = { std::to_string(newLoop.iterations) };
			instructions_.push_back(forInst);
			instructionsGenerated++;

			loopStack.push(newLoop);
			continue;
		}

		// Generate regular instruction
		Instruction inst;
		int instType = typeDist(gen);

		switch (instType) {
			
		case 0: {// PRINT
			inst.type = Instruction::PRINT;
			inst.args = { "\"Hello world from " + name_ + "!\"" };
			break;
			}

		
		case 1: {// DECLARE
			inst.type = Instruction::DECLARE;
			char var = 'a' + varDist(gen);
			uint16_t value = valueDist(gen);
			inst.args = { std::string(1, var), std::to_string(value) };
			break;
		}

			
		case 2: {// ADD
			inst.type = Instruction::ADD;
			char var1 = 'a' + varDist(gen);
			char var2 = 'a' + varDist(gen);
			char var3 = 'a' + varDist(gen);
			inst.args = { std::string(1, var1), std::string(1, var2), std::string(1, var3) };
			break;
			}

			
		case 3: {// SUBTRACT
			inst.type = Instruction::SUBTRACT;
			char var1 = 'a' + varDist(gen);
			char var2 = 'a' + varDist(gen);
			char var3 = 'a' + varDist(gen);
			inst.args = { std::string(1, var1), std::string(1, var2), std::string(1, var3) };
			break;
			}

		
		case 4: {// SLEEP
			inst.type = Instruction::SLEEP;
			uint8_t ticks = sleepDist(gen);
			inst.args = { std::to_string(ticks) };
			break;
		}

		
		default: {// PRINT (fallback)
			inst.type = Instruction::PRINT;
			inst.args = { "\"Hello world from " + name_ + "!\"" };
			break;
		}
		}

		instructions_.push_back(inst);
		instructionsGenerated++;

		// Update loop body size if in loop
		if (!loopStack.empty() && loopStack.top().loopBodySize == 0) {
			loopStack.top().loopBodySize = instructions_.size() - loopStack.top().startIndex;
		}
	}

	// Close any remaining loops
	while (!loopStack.empty() && instructionsGenerated < totalBurst_) {
		Instruction endLoop;
		endLoop.type = Instruction::ENDLOOP;
		instructions_.push_back(endLoop);
		instructionsGenerated++;
		loopStack.pop();
	}

	// Final clamp to totalBurst
	if (instructions_.size() > static_cast<size_t>(totalBurst_)) {
		instructions_.resize(totalBurst_);
	}
	totalBurst_ = instructions_.size();
}

void Screen::generateInstructionsRecursive(int& instructionsGenerated, int depth, std::mt19937& gen) {
	std::uniform_int_distribution<> typeDist(0, 6);
	std::uniform_int_distribution<> loopChanceDist(0, 9);
	std::uniform_int_distribution<> iterDist(2, 5);
	std::uniform_int_distribution<> varDist(0, 25);
	std::uniform_int_distribution<> valueDist(0, 99);
	std::uniform_int_distribution<> sleepDist(1, 5);

	while (instructionsGenerated < totalBurst_) {
		// 10% chance to start a loop if we have space and depth < 3
		if (depth < 3 && (totalBurst_ - instructionsGenerated) >= 2 &&
			loopChanceDist(gen) == 0) {

			// Generate FOR instruction
			Instruction forInst;
			forInst.type = Instruction::FOR;
			int iterations = iterDist(gen);
			forInst.args = { std::to_string(iterations) };
			instructions_.push_back(forInst);
			instructionsGenerated++;

			// Recursively generate loop body
			generateInstructionsRecursive(instructionsGenerated, depth + 1, gen);

			// Generate ENDLOOP instruction
			Instruction endInst;
			endInst.type = Instruction::ENDLOOP;
			instructions_.push_back(endInst);
			instructionsGenerated++;
		}
		else {
			// Generate non-loop instruction
			instructions_.push_back(generateRandomNonLoopInstruction(gen));
			instructionsGenerated++;
		}
	}
}


Screen::Instruction Screen::generateRandomNonLoopInstruction(std::mt19937& gen) {  
	std::uniform_int_distribution<> typeDist(0, 5);  
	std::uniform_int_distribution<> varDist(0, 25);  
	std::uniform_int_distribution<> valueDist(0, 99);  
	std::uniform_int_distribution<> sleepDist(1, 5);  

	Screen::Instruction inst;  
	int type = typeDist(gen);  

	switch (type) {  
		
	case 0: {// PRINT  
		inst.type = Screen::Instruction::PRINT;
		inst.args = { "\"Hello world from " + name_ + "!\"" };
		break;
	}
	case 1: {// DECLARE  
		inst.type = Screen::Instruction::DECLARE;
		char var = 'a' + varDist(gen);
		uint16_t value = valueDist(gen);
		inst.args = { std::string(1, var), std::to_string(value) };
		break;
	}

	
	case 2: {// ADD  
		inst.type = Screen::Instruction::ADD;
		char var1 = 'a' + varDist(gen);
		char var2 = 'a' + varDist(gen);
		char var3 = 'a' + varDist(gen);
		inst.args = { std::string(1, var1), std::string(1, var2), std::string(1, var3) };
		break;
	}

	
	case 3: {// SUBTRACT  
		inst.type = Screen::Instruction::SUBTRACT;
		char var1 = 'a' + varDist(gen);
		char var2 = 'a' + varDist(gen);
		char var3 = 'a' + varDist(gen);
		inst.args = { std::string(1, var1), std::string(1, var2), std::string(1, var3) };
		break;
	}

	
	case 4: {// SLEEP  
		inst.type = Screen::Instruction::SLEEP;
		uint8_t ticks = sleepDist(gen);
		inst.args = { std::to_string(ticks) };
		break;
	}
	default: {// Fallback to PRINT  
		inst.type = Screen::Instruction::PRINT;
		inst.args = { "\"Hello world from " + name_ + "!\"" };
		break;
	}
	}  

	return inst;  
}


void Screen::executeInstruction(int coreId) {
	// Handle sleep ticks
	if (sleepTicksRemaining_ > 0) {
		sleepTicksRemaining_--;
		std::string timeStr = getCurrentTimeStamp();
		std::string entry = "(" + timeStr + ")\tCore: " + std::to_string(coreId) +
			"\tSLEEP: " + std::to_string(sleepTicksRemaining_) + " ticks remaining";
		addLogEntry(coreId, entry);
		return;
	}

	if (pc_ >= instructions_.size()) return;

	Instruction& inst = instructions_[pc_];
	std::string logMessage;

	switch (inst.type) {
	case Instruction::PRINT: {
		std::string msg = inst.args[0];

		logMessage = "PRINT: " + msg;
		pc_++;
		break;
	}

	case Instruction::DECLARE: {
		std::string varName = trim(inst.args[0]);
		if (variables_.find(varName) != variables_.end()) {
			logMessage = "REDECLARE: " + varName;
		}
		else {
			uint16_t value = static_cast<uint16_t>(parseValue(inst.args[1]));
			variables_[varName] = value;
			logMessage = "DECLARE: " + varName + " = " + std::to_string(value);
		}
		pc_++;
		break;
	}
	case Instruction::ADD: {
		std::string var1 = trim(inst.args[0]);
		std::string arg2 = trim(inst.args[1]);
		std::string arg3 = trim(inst.args[2]);
		uint16_t val2 = parseValue(arg2);
		uint16_t val3 = parseValue(arg3);
		uint16_t result = val2 + val3;
		variables_[var1] = result;
		logMessage = "ADD: " + var1 + " = " + std::to_string(val2) + " + " + std::to_string(val3);
		pc_++;
		break;
	}
	case Instruction::SUBTRACT: {
		std::string var1 = trim(inst.args[0]);
		std::string arg2 = trim(inst.args[1]);
		std::string arg3 = trim(inst.args[2]);
		uint16_t val2 = parseValue(arg2);
		uint16_t val3 = parseValue(arg3);
		uint16_t result = (val2 > val3) ? (val2 - val3) : 0;
		variables_[var1] = result;
		logMessage = "SUBTRACT: " + var1 + " = " + std::to_string(val2) + " - " + std::to_string(val3);
		pc_++;
		break;
	}
	case Instruction::SLEEP: {
		uint8_t ticks = static_cast<uint8_t>(parseValue(inst.args[0]));
		sleepTicksRemaining_ = ticks;
		pc_++;  // Always advance to next instruction

		if (ticks > 0) {
			logMessage = "SLEEP: " + std::to_string(ticks) + " ticks started";
		}
		else {
			logMessage = "SLEEP: Zero ticks - no op";
		}
		break;
	}
	case Instruction::FOR: {
		int depth = static_cast<int>(loopStack_.size()) + 1;
		if (depth > 3) {
			logMessage = "FOR loop skipped (max depth exceeded)";
			pc_++;
			break;
		}

		LoopContext context;
		context.iterations = parseValue(inst.args[0]);
		context.currentIteration = 1;  // Start at first iteration
		context.startIndex = pc_ + 1;
		context.depth = depth;
		loopStack_.push(context);

		logMessage = "[D" + std::to_string(depth) + "] FOR started (" +
			std::to_string(context.iterations) + " iterations)";
		pc_ = context.startIndex;
		break;
	}

	case Instruction::ENDLOOP: {
		if (loopStack_.empty()) {
			logMessage = "ERROR: ENDLOOP without matching FOR";
			pc_++;
			break;
		}

		LoopContext& context = loopStack_.top();
		context.currentIteration++;

		if (context.currentIteration <= context.iterations) {
			logMessage = "[D" + std::to_string(context.depth) + "] Iteration " +
				std::to_string(context.currentIteration) + "/" +
				std::to_string(context.iterations);
			pc_ = context.startIndex;
		}
		else {
			logMessage = "[D" + std::to_string(context.depth) + "] FOR completed";
			loopStack_.pop();
			pc_++;
		}
		break;
	}
	default:
		logMessage = "UNKNOWN INSTRUCTION";
		break;
	}

	// Add log entry if not sleeping (sleep logs are handled separately)
	if (inst.type != Instruction::SLEEP || sleepTicksRemaining_ == 0) {
		std::string timeStr = getCurrentTimeStamp();
		std::string entry = "(" + timeStr + ")\tCore: " +
			std::to_string(coreId) + "\t" + logMessage;
		addLogEntry(coreId, entry);
	}

	currentBurst_++;
}

uint16_t Screen::parseValue(const std::string& str) {
	std::string s = trim(str);
	if (std::all_of(s.begin(), s.end(), ::isdigit)) {
		return static_cast<uint16_t>(std::stoi(s));
	}
	return getVariableValue(s);
}

uint16_t Screen::getVariableValue(const std::string& varName) {
	auto it = variables_.find(varName);
	return (it != variables_.end()) ? it->second : 0;
}

void Screen::setVariableValue(const std::string& varName, uint16_t value) {
	variables_[varName] = value;
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