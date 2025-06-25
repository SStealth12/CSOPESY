#pragma once
#include <vector>
#include <queue>
#include <memory>

#include <fstream>
#include <sstream>
#include <random>
#include <ctime>
#include <map>
#include <iostream>
#include <thread>
#include <chrono>
#include <iomanip>
#include <condition_variable>
#include <filesystem>

#include "Marquee.h"       
#include "Screen.h"        
#include "Scheduler.h"     
#include "FCFSScheduler.h" 
#include "globals.h"   

#ifdef _WIN32
#include <cstdlib>
#include <direct.h>
#define CLEAR_COMMAND "cls"
#define GET_CURRENT_DIR _getcwd
#else
#include <unistd.h>
#define CLEAR_COMMAND "clear"
#define GET_CURRENT_DIR getcwd
#endif


void printHeader();
void nvidiasmi(const std::map<int, Process>& processes);
void screenCommand(const std::string& dashOpt, const std::string& name);
void bootstrap(const std::string& configFile = "config.txt");
void OSLoop();