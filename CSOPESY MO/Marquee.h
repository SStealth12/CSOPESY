#pragma once
#define NOMINMAX

#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <chrono>
#include <algorithm>

#include <conio.h>
#include <windows.h>
#define CLEAR_SCREEN() system("cls")

static std::chrono::milliseconds MARQUEE_REFRESH_RATE(144);
static std::chrono::milliseconds INPUT_POLLING_RATE(7);

inline void marqueeConsole() {
    const std::string headerTop = "****************************************";
    const std::string headerMid = "* Displaying a marquee console!       *";
    const std::string headerBottom = "****************************************";
    const std::string marqueeText = "Hello world in marquee!";

    const int consoleWidth = 120;
    const int headerLines = 3;
    const int promptLine = 25;    // row where prompt always sits

    int marqueeHeight = promptLine - headerLines - 1;

    int x = 0, y = 0;
    int dx = 1, dy = 1;
    int maxX = consoleWidth - static_cast<int>(marqueeText.size());
    int maxY = marqueeHeight - 1;

    std::atomic<bool> running{ true };
    std::string input;
    std::vector<std::string> history;
    size_t maxInputLen = 0;

    // --- Input thread ---
    std::thread inputThread([&]() {
        while (running) {
            if (_kbhit()) {
                char c = _getch();
                if (c == '\r') {
                    history.push_back("Command processed in MARQUEE_CONSOLE: " + input);
                    if (input == "exit") running = false;
                    input.clear();
                }
                else if (c == '\b') {  // backspace
                    if (!input.empty()) input.pop_back();
                }
                else {
                    input.push_back(c);
                }
            }
            std::this_thread::sleep_for(INPUT_POLLING_RATE);
        }
        });

    while (running) {
        CLEAR_SCREEN();

        // 1) Header
        std::cout
            << headerTop << "\n"
            << headerMid << "\n"
            << headerBottom << "\n\n";


        for (int row = 0; row < marqueeHeight; ++row) {
            if (row == y) {
                std::cout << std::string(x, ' ') << marqueeText << "\n";
            }
            else {
                std::cout << "\n";
            }
        }

        // 3) User Input
        std::cout << "Enter a command for MARQUEE_CONSOLE: " << input;
        if (input.size() < maxInputLen) {
            std::cout << std::string(maxInputLen - input.size(), ' ');
        }
        std::cout << std::flush;
        maxInputLen = std::max(maxInputLen, input.size());

        // 4) Command History
        for (const auto& line : history) {
            std::cout << "\n" << line;
        }

        // 5) Bounce update
        x += dx; if (x <= 0 || x >= maxX) dx = -dx;
        y += dy; if (y <= 0 || y >= maxY) dy = -dy;

        std::this_thread::sleep_for(MARQUEE_REFRESH_RATE);
    }

    inputThread.join();
}
