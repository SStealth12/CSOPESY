#include "Screen.h"
#include "globals.h"
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

void Screen::setCustomInstructions(const std::vector<std::string>& customInstructions) {
	instructions_.clear();
	
	for (const std::string& instStr : customInstructions) {
		Instruction inst;
		std::istringstream iss(instStr);
		std::string command;
		iss >> command;
		
		// Convert command to uppercase for comparison
		std::transform(command.begin(), command.end(), command.begin(), ::toupper);
		
		if (command == "PRINT") {
			inst.type = Instruction::PRINT;
			// Extract the argument (everything after PRINT)
			std::string arg = instStr.substr(instStr.find("PRINT") + 5);
			arg.erase(0, arg.find_first_not_of(" \t")); // Trim leading whitespace
			if (arg.empty()) {
				arg = "\"Hello world from " + name_ + "!\"";
			}
			inst.args = { arg };
		}
		else if (command == "DECLARE") {
			inst.type = Instruction::DECLARE;
			std::string varName, value;
			iss >> varName >> value;
			inst.args = { varName, value };
		}
		else if (command == "ADD") {
			inst.type = Instruction::ADD;
			std::string var1, var2, var3;
			iss >> var1 >> var2 >> var3;
			inst.args = { var1, var2, var3 };
		}
		else if (command == "SUBTRACT") {
			inst.type = Instruction::SUBTRACT;
			std::string var1, var2, var3;
			iss >> var1 >> var2 >> var3;
			inst.args = { var1, var2, var3 };
		}
		else if (command == "SLEEP") {
			inst.type = Instruction::SLEEP;
			std::string ticks;
			iss >> ticks;
			inst.args = { ticks };
		}
		else if (command == "READ") {
			inst.type = Instruction::READ;
			std::string varName, address;
			iss >> varName >> address;
			inst.args = { varName, address };
		}
		else if (command == "WRITE") {
			inst.type = Instruction::WRITE;
			std::string address, value;
			iss >> address >> value;
			inst.args = { address, value };
		}
		else {
			// Default to PRINT for unknown commands
			inst.type = Instruction::PRINT;
			inst.args = { "\"Unknown command: " + instStr + "\"" };
		}
		
		instructions_.push_back(inst);
	}
	
	totalBurst_ = static_cast<int>(instructions_.size());
}

void Screen::generateInstructions() {
	std::random_device rd;
	std::mt19937 gen(rd());
	instructions_.clear();
	int instructionsGenerated = 0;
	std::stack<LoopContext> loopStack;

	// Distributions - Favor READ/WRITE instructions for Test Case 4
	std::uniform_int_distribution<> typeDist(0, 9); // Increased range
	std::uniform_int_distribution<> loopChanceDist(0, 9);
	std::uniform_int_distribution<> iterDist(2, 5);
	std::uniform_int_distribution<> varDist(0, 25);
	std::uniform_int_distribution<> valueDist(0, 99);
	std::uniform_int_distribution<> sleepDist(1, 5);
	std::uniform_int_distribution<> memAddrDist(0x1000, 0x2000); // Memory address range

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

		case 5:
		case 6:
		case 7: {// READ - Higher probability
			inst.type = Instruction::READ;
			char var = 'a' + varDist(gen);
			std::stringstream ss;
			ss << "0x" << std::hex << memAddrDist(gen);
			inst.args = { std::string(1, var), ss.str() };
			break;
		}

		case 8:
		case 9: {// WRITE - Higher probability
			inst.type = Instruction::WRITE;
			std::stringstream ss;
			ss << "0x" << std::hex << memAddrDist(gen);
			uint16_t value = valueDist(gen);
			inst.args = { ss.str(), std::to_string(value) };
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
		
		// Handle variable concatenation in PRINT statements
		// Look for patterns like "text" + varName
		size_t plusPos = msg.find(" + ");
		if (plusPos != std::string::npos) {
			std::string beforePlus = msg.substr(0, plusPos);
			std::string afterPlus = msg.substr(plusPos + 3);
			
			// Remove quotes from the string part
			if (beforePlus.front() == '"' && beforePlus.back() == '"') {
				beforePlus = beforePlus.substr(1, beforePlus.length() - 2);
			}
			
			// Get variable value
			uint16_t varValue = getVariableValue(trim(afterPlus));
			msg = beforePlus + std::to_string(varValue);
		} else {
			// Remove quotes if present
			if (msg.front() == '"' && msg.back() == '"') {
				msg = msg.substr(1, msg.length() - 2);
			}
		}

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
			// Check if we have space for more variables (max 32 variables in 64-byte symbol table)
			if (variables_.size() >= 32) {
				logMessage = "DECLARE: Symbol table full, ignoring " + varName;
				pc_++;
				break;
			}
			
			uint16_t value = static_cast<uint16_t>(parseValue(inst.args[1]));
			variables_[varName] = value;
			
			// Calculate memory address in symbol table segment (0x0000 - 0x003F)
			size_t variableIndex = variables_.size() - 1;
			size_t symbolTableAddress = variableIndex * 2; // 2 bytes per uint16
			
			// Write variable to memory through memory manager
			if (globalMemoryManager) {
				try {
					globalMemoryManager->writeMemory(name_, symbolTableAddress, value);
					logMessage = "DECLARE: " + varName + " = " + std::to_string(value) + 
								" (stored at 0x" + std::to_string(symbolTableAddress) + ")";
				} catch (const std::exception&) {
					logMessage = "DECLARE: " + varName + " = " + std::to_string(value) + " (memory write failed)";
				}
			} else {
				logMessage = "DECLARE: " + varName + " = " + std::to_string(value) + " (no memory manager)";
			}
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
		
		// Check if we have space for more variables (max 32 variables in 64-byte symbol table)
		if (variables_.find(var1) == variables_.end() && variables_.size() >= 32) {
			logMessage = "ADD: Symbol table full, ignoring " + var1;
			pc_++;
			break;
		}
		
		variables_[var1] = result;
		
		// Write result to memory if it's a new variable or update existing one
		if (globalMemoryManager) {
			try {
				// Find variable index for memory address calculation
				size_t variableIndex = 0;
				for (const auto& pair : variables_) {
					if (pair.first == var1) break;
					variableIndex++;
				}
				size_t symbolTableAddress = variableIndex * 2; // 2 bytes per uint16
				
				globalMemoryManager->writeMemory(name_, symbolTableAddress, result);
				logMessage = "ADD: " + var1 + " = " + std::to_string(val2) + " + " + std::to_string(val3) + 
							" (stored at 0x" + std::to_string(symbolTableAddress) + ")";
			} catch (const std::exception&) {
				logMessage = "ADD: " + var1 + " = " + std::to_string(val2) + " + " + std::to_string(val3) + " (memory write failed)";
			}
		} else {
			logMessage = "ADD: " + var1 + " = " + std::to_string(val2) + " + " + std::to_string(val3);
		}
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
		
		// Check if we have space for more variables (max 32 variables in 64-byte symbol table)
		if (variables_.find(var1) == variables_.end() && variables_.size() >= 32) {
			logMessage = "SUBTRACT: Symbol table full, ignoring " + var1;
			pc_++;
			break;
		}
		
		variables_[var1] = result;
		
		// Write result to memory if it's a new variable or update existing one
		if (globalMemoryManager) {
			try {
				// Find variable index for memory address calculation
				size_t variableIndex = 0;
				for (const auto& pair : variables_) {
					if (pair.first == var1) break;
					variableIndex++;
				}
				size_t symbolTableAddress = variableIndex * 2; // 2 bytes per uint16
				
				globalMemoryManager->writeMemory(name_, symbolTableAddress, result);
				logMessage = "SUBTRACT: " + var1 + " = " + std::to_string(val2) + " - " + std::to_string(val3) + 
							" (stored at 0x" + std::to_string(symbolTableAddress) + ")";
			} catch (const std::exception&) {
				logMessage = "SUBTRACT: " + var1 + " = " + std::to_string(val2) + " - " + std::to_string(val3) + " (memory write failed)";
			}
		} else {
			logMessage = "SUBTRACT: " + var1 + " = " + std::to_string(val2) + " - " + std::to_string(val3);
		}
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

	case Instruction::READ: {
		std::string varName = trim(inst.args[0]);
		std::string addrStr = trim(inst.args[1]);
		
		// Parse hex address
		size_t address = 0;
		try {
			if (addrStr.substr(0, 2) == "0x" || addrStr.substr(0, 2) == "0X") {
				address = static_cast<size_t>(std::stoul(addrStr, nullptr, 16));
			} else {
				address = static_cast<size_t>(std::stoul(addrStr));
			}
		} catch (const std::exception&) {
			logMessage = "READ: Invalid address format " + addrStr;
			pc_++;
			break;
		}
		
		// Access memory through memory manager using process name
		if (globalMemoryManager) {
			try {
				uint16_t value = globalMemoryManager->readMemory(name_, address);
				variables_[varName] = value;
				logMessage = "READ: " + varName + " = " + std::to_string(value) + " from " + addrStr;
			} catch (const std::exception&) {
				logMessage = "READ: Memory access violation at " + addrStr;
				// Set memory access violation
				setMemoryAccessViolation(getCurrentTimeStamp(), static_cast<uint32_t>(address));
			}
		} else {
			logMessage = "READ: Memory manager not available";
		}
		pc_++;
		break;
	}

	case Instruction::WRITE: {
		std::string addrStr = trim(inst.args[0]);
		uint16_t value = static_cast<uint16_t>(parseValue(inst.args[1]));
		
		// Parse hex address
		size_t address = 0;
		try {
			if (addrStr.substr(0, 2) == "0x" || addrStr.substr(0, 2) == "0X") {
				address = static_cast<size_t>(std::stoul(addrStr, nullptr, 16));
			} else {
				address = static_cast<size_t>(std::stoul(addrStr));
			}
		} catch (const std::exception&) {
			logMessage = "WRITE: Invalid address format " + addrStr;
			pc_++;
			break;
		}
		
		// Access memory through memory manager using process name
		if (globalMemoryManager) {
			try {
				globalMemoryManager->writeMemory(name_, address, value);
				logMessage = "WRITE: " + std::to_string(value) + " to " + addrStr;
			} catch (const std::exception&) {
				logMessage = "WRITE: Memory access violation at " + addrStr;
				// Set memory access violation
				setMemoryAccessViolation(getCurrentTimeStamp(), static_cast<uint32_t>(address));
			}
		} else {
			logMessage = "WRITE: Memory manager not available";
		}
		pc_++;
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
	for (size_t i = 0; i < static_cast<size_t>(currentBurst_) && i < logEntries_.size(); ++i) {
		file << logEntries_[i].message << "\n";
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
		for (size_t i = 0; i < static_cast<size_t>(currentBurst_) && i < logEntries_.size(); ++i) {
			std::cout << logEntries_[i].message << "\n";
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
