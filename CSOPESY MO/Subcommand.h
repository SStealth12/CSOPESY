#ifndef SUBCOMMAND_H
#define SUBCOMMAND_H

#include <iostream>
#include <map>
#include <string>

#include "Screen.h"
#include "CLIDesigns.h"   // for printHeader()

#ifdef _WIN32
#include <cstdlib>
#define CLEAR_COMMAND "cls"
#else
#include <unistd.h>
#define CLEAR_COMMAND "clear"
#endif

inline void screenCommand(std::map<std::string, Screen>& screens,
    const std::string& dashOpt,
    const std::string& name)
{
    if (dashOpt == "-s" && !name.empty()) {
        // Create a new screen if it doesn’t already exist
        if (screens.count(name)) {
            std::cout << "Error: screen '" << name << "' already exists.\n";
        }
        else {
            screens.emplace(name, Screen{ name });
            std::cout << "Screen '" << name << "' created.\n";
        }
    }
    else if (dashOpt == "-r" && !name.empty()) {
        // Redraw an existing screen if found
        auto it = screens.find(name);
        if (it == screens.end()) {
            std::cout << "Error: no screen '" << name << "'\n";
        }
        else {
            Screen& scr = it->second;
            scr.draw();
            // Stay in this screen until user types “exit”
            while (true) {
                std::string sub;
                std::getline(std::cin, sub);
                if (sub == "exit") {
                    system(CLEAR_COMMAND);
                    printHeader();
                    break;
                }
                else {
                    std::cout << "Unknown sub-command.\n> " << std::flush;
                }
            }
        }
    }
    else {
        // If dashOpt wasn’t “-s” or “-r”, print usage
        std::cout << "Usage:\n"
            << "  screen -s <name>   create screen\n"
            << "  screen -r <name>   redraw screen\n";
    }
}

#endif // SUBCOMMAND_H
