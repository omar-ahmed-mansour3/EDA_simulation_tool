// main.cpp
// Entry point for EDA Simulation Engine & Waveform GUI

#include "Parser.hpp"
#include "Netlist.hpp"
#include "SimEngine.hpp"
#include "IOController.hpp"
#include "WaveformGUI.hpp"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>
#include <vector>
#include <algorithm>

static void printUsage(const char* prog_name) {
    std::cout << "======================================================================\n";
    std::cout << "            EDA SIMULATION ENGINE - USAGE GUIDE                       \n";
    std::cout << "======================================================================\n";
    std::cout << "Usage:\n";
    std::cout << "  " << prog_name << " [options] [verilog_filepath]\n\n";
    std::cout << "Examples:\n";
    std::cout << "  " << prog_name << " simple_and_delay.v\n";
    std::cout << "  " << prog_name << " comb_logic.v\n";
    std::cout << "  " << prog_name << " full_adder.v --cli\n\n";
    std::cout << "Options:\n";
    std::cout << "  -h, --help       Show this help message and exit\n";
    std::cout << "  -i, --cli        Run in interactive Terminal CLI mode instead of GUI\n";
    std::cout << "======================================================================\n";
}

int main(int argc, char* argv[]) {
    std::string verilog_filepath = "";
    bool interactive_cli = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            printUsage(argv[0]);
            return 0;
        } else if (arg == "--cli" || arg == "-i" || arg == "--test") {
            interactive_cli = true;
        } else if (arg.rfind("-", 0) != 0 && verilog_filepath.empty()) {
            verilog_filepath = arg;
        }
    }

    ParsedModule parsed;
    if (verilog_filepath.empty()) {
        std::cout << "======================================================================\n";
        std::cout << "        EDA SIMULATION ENGINE - DYNAMIC FILE SELECTION               \n";
        std::cout << "======================================================================\n";
        std::cout << "Enter Verilog file path (or press Enter for default 'simple_and_delay.v'): ";
        std::string user_input;
        if (std::getline(std::cin, user_input) && !user_input.empty()) {
            // Trim whitespace
            user_input.erase(0, user_input.find_first_not_of(" \t\r\n"));
            user_input.erase(user_input.find_last_not_of(" \t\r\n") + 1);
            if (!user_input.empty()) {
                verilog_filepath = user_input;
            }
        }
        if (verilog_filepath.empty()) {
            verilog_filepath = "simple_and_delay.v";
        }
    }

    try {
        std::cout << "[INFO] Loading Verilog file: '" << verilog_filepath << "'...\n";
        parsed = VerilogParser::parseFile(verilog_filepath);
    } catch (const std::exception& ex) {
        std::cout << "[INFO] File load failed (" << ex.what() << "), falling back to embedded sample module...\n";
        std::string verilog_input = 
            "module simple_and_delay(input a, input b, output out1);\n"
            "    and # (2) o1 (out1, a, b);\n"
            "endmodule\n";
        parsed = VerilogParser::parseString(verilog_input);
    }

    Netlist netlist;
    netlist.buildGraph(parsed);
    SimEngine engine(&netlist);

    if (interactive_cli) {
        std::cout << "======================================================================\n";
        std::cout << "        EDA SIMULATION ENGINE INTERACTIVE CLI MODE                    \n";
        std::cout << "======================================================================\n";
        std::cout << "Module : " << parsed.module_name << "\n";
        std::cout << "Inputs : ";
        for (const auto& pin : parsed.input_pins) std::cout << pin << " ";
        std::cout << "\nOutputs: ";
        for (const auto& pin : parsed.output_pins) std::cout << pin << " ";
        std::cout << "\nType 'help' for command usage or 'exit' / 'quit' to exit.\n";
        std::cout << "----------------------------------------------------------------------\n";

        std::string line;
        while (true) {
            std::cout << "EDA-SimEngine> ";
            if (!std::getline(std::cin, line)) break;
            if (line == "exit" || line == "quit") break;
            if (line.empty()) continue;
            IOController::executeCommand(line, engine);
        }
        return 0;
    }

    std::cout << "======================================================================\n";
    std::cout << "        EDA SIMULATION ENGINE - WAVEFORM GUI MODE                     \n";
    std::cout << "======================================================================\n";
    std::cout << "Loaded Module: " << parsed.module_name << "\n";
    std::cout << "Inputs : ";
    for (const auto& pin : parsed.input_pins) std::cout << pin << " ";
    std::cout << "\nOutputs: ";
    for (const auto& pin : parsed.output_pins) std::cout << pin << " ";
    std::cout << "\n----------------------------------------------------------------------\n";

    std::cout << "[RUNTIME CONTROL INSTRUCTIONS]\n";
    std::cout << "  Type commands directly using the GUI control box or terminal!\n";
    std::cout << "  Example commands:\n";
    for (const auto& pin : parsed.input_pins) {
        std::cout << "    set " << pin << " 1 at 10\n";
    }
    std::cout << "    run 10\n";
    std::cout << "======================================================================\n\n";

    std::cout << "Launching EDA Waveform GUI...\n";
    WaveformGUI::runApplication(&engine);

    return 0;
}
