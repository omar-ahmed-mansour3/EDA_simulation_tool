#pragma once
#include "SimEngine.hpp"
#include "IOController.hpp"
#include <string>
#include <vector>

struct LogEntry {
    std::string text;
    bool is_error;
};

class WaveformGUI {
public:
    // Application entry point for running GUI render loop & handling CLI box input
    static void runApplication(SimEngine* engine);
};
