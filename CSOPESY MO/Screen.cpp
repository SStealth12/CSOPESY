#include "Screen.h"
#include <iostream>
#include <chrono>
#include <ctime>
#include <iomanip>
#ifdef _WIN32
#include <cstdlib>
#define CLEAR_COMMAND "cls"
#else
#include <unistd.h>
#define CLEAR_COMMAND "clear"
#endif
#include <sstream>


Screen::Screen(const std::string& name, size_t totalLines)
	: name_(name), currentLine_(1), totalLines_(totalLines), createTimestamp_(getCurrentTimeStamp()) 
{}

std::string Screen::getCurrentTimeStamp() {
	using namespace std::chrono;
	auto now = system_clock::now();
	std::time_t t = system_clock::to_time_t(now);
	std::tm local_tm;

#ifdef _WIN32
	localtime_s(&local_tm, &t);
#else
	localtime_r(&t, &local_tm);
#endif

	std::ostringstream oss;
	oss << std::put_time(&local_tm, "%Y-%m-%d %H:%M:%S");
	return oss.str();
}

void Screen::draw() const {
	system(CLEAR_COMMAND);

	std::cout << "==============================		SCREEN		========================\n\n";
	std::cout << "Process name:			" << name_ << "\n";
	std::cout << "Instruction line:		" << currentLine_ << " / " << totalLines_ << "\n";
	std::cout << "Created at:			" << createTimestamp_ << "\n\n";
	std::cout << "================================================================================\n";
	std::cout << "Type 'exit' to return to main menu.\n\n"
			  <<"> " << std::flush;
}