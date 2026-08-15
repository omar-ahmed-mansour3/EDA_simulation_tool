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

static std::string stateToString(LogicState s) {
    switch (s) {
        case LogicState::ZERO: return "0";
        case LogicState::ONE:  return "1";
        case LogicState::X:    return "x";
        case LogicState::Z:    return "z";
        default:               return "?";
    }
}

static void printUsage(const char* prog_name) {
    std::cout << "======================================================================\n";
    std::cout << "            EDA SIMULATION ENGINE - USAGE GUIDE                       \n";
    std::cout << "======================================================================\n";
    std::cout << "Usage:\n";
    std::cout << "  " << prog_name << " [options] [verilog_filepath]\n\n";
    std::cout << "Examples:\n";
    std::cout << "  " << prog_name << " simple_and_delay.v\n";
    std::cout << "  " << prog_name << " comb_logic.v\n";
    std::cout << "  " << prog_name << " full_adder.v --cli\n";
    std::cout << "  " << prog_name << " --test\n\n";
    std::cout << "Options:\n";
    std::cout << "  -h, --help       Show this help message and exit\n";
    std::cout << "  -i, --cli        Run in interactive Terminal CLI mode instead of GUI\n";
    std::cout << "  --test           Run internal CLI unit verification test suite\n";
    std::cout << "======================================================================\n";
}

static void runCliTestSuite(SimEngine& engine) {
    std::cout << "======================================================================\n";
    std::cout << "        PERSON 4: IOCONTROLLER CLI COMMAND TEST SUITE                 \n";
    std::cout << "======================================================================\n\n";

    std::cout << "[EXECUTING CLI COMMAND SEQUENCE]\n";
    std::cout << "----------------------------------------------------------------------\n";

    std::cout << "Command 1: 'set in1 1 at 0'\n   -> Actual Output: ";
    IOController::executeCommand("set in1 1 at 0", engine);

    std::cout << "Command 2: 'set in2 0 at 0'\n   -> Actual Output: ";
    IOController::executeCommand("set in2 0 at 0", engine);

    std::cout << "Command 3: 'run 5'\n   -> Actual Output: ";
    IOController::executeCommand("run 5", engine);

    std::cout << "Command 4: 'set in1 0 at 5'\n   -> Actual Output: ";
    IOController::executeCommand("set in1 0 at 5", engine);

    std::cout << "Command 5: 'run 5'\n   -> Actual Output: ";
    IOController::executeCommand("run 5", engine);

    std::cout << "Command 6: 'set bad_wire 1 at 10' (Invalid Wire Test)\n   -> Actual Output: ";
    IOController::executeCommand("set bad_wire 1 at 10", engine);

    std::cout << "Command 7: 'foobar 123' (Invalid Command Test)\n   -> Actual Output: ";
    IOController::executeCommand("foobar 123", engine);

    std::cout << "Command 8: 'set in1 1 at 3' (Past Time Rejection Test)\n   -> Actual Output: ";
    IOController::executeCommand("set in1 1 at 3", engine);

    std::cout << "----------------------------------------------------------------------\n\n";

    std::cout << "[VERIFYING WAVEFORM HISTORY AFTER CLI EXECUTION]\n";
    std::cout << std::left << std::setw(12) << "Timestamp" << std::setw(14) << "Wire" << "New State\n";
    std::cout << std::string(36, '-') << "\n";

    for (const auto& ev : engine.getHistory()) {
        std::cout << std::left
                  << std::setw(12) << ("t=" + std::to_string(ev.timestamp) + "ns")
                  << std::setw(14) << ev.wire->name
                  << stateToString(ev.new_state) << "\n";
    }

    bool check1 = false, check2 = false, check3 = false, check4 = false, check5 = false;

    for (const auto& ev : engine.getHistory()) {
        if (ev.timestamp == 0 && ev.wire->name == "in1" && ev.new_state == LogicState::ONE)  check1 = true;
        if (ev.timestamp == 0 && ev.wire->name == "in2" && ev.new_state == LogicState::ZERO) check2 = true;
        if (ev.timestamp == 1 && ev.wire->name == "out" && ev.new_state == LogicState::ONE)  check3 = true;
        if (ev.timestamp == 5 && ev.wire->name == "in1" && ev.new_state == LogicState::ZERO) check4 = true;
        if (ev.timestamp == 6 && ev.wire->name == "out" && ev.new_state == LogicState::ZERO) check5 = true;
    }

    std::cout << "\n[CLI EXPECTATIONS COMPARISON]\n";
    std::cout << "  1. t=0ns  : in1 = 1 injected                  -> " << (check1 ? "[MATCH ✅]" : "[FAIL ❌]") << "\n";
    std::cout << "  2. t=0ns  : in2 = 0 injected                  -> " << (check2 ? "[MATCH ✅]" : "[FAIL ❌]") << "\n";
    std::cout << "  3. t=1ns  : out = 1 (OR gate evaluated)       -> " << (check3 ? "[MATCH ✅]" : "[FAIL ❌]") << "\n";
    std::cout << "  4. t=5ns  : in1 = 0 injected                  -> " << (check4 ? "[MATCH ✅]" : "[FAIL ❌]") << "\n";
    std::cout << "  5. t=6ns  : out = 0 (OR gate re-evaluated)    -> " << (check5 ? "[MATCH ✅]" : "[FAIL ❌]") << "\n";

    bool all_passed = check1 && check2 && check3 && check4 && check5;

    std::cout << "\n======================================================================\n";
    if (all_passed) {
        std::cout << "  PERSON 4 (IOController CLI) PASSED ALL VERIFICATIONS! 🚀✅\n";
    } else {
        std::cout << "  CLI TEST FAILED! ❌\n";
    }
    std::cout << "======================================================================\n";
}

int main(int argc, char* argv[]) {
    std::string verilog_filepath = "";
    bool run_unit_test = false;
    bool interactive_cli = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            printUsage(argv[0]);
            return 0;
        } else if (arg == "--test") {
            run_unit_test = true;
        } else if (arg == "--cli" || arg == "-i") {
            interactive_cli = true;
        } else if (arg.rfind("-", 0) != 0 && verilog_filepath.empty()) {
            verilog_filepath = arg;
        }
    }

    ParsedModule parsed;
    if (run_unit_test) {
        std::string verilog_input = 
            "module cli_test(input in1, input in2, output out);\n"
            "    assign out = in1 | in2;\n"
            "endmodule\n";
        parsed = VerilogParser::parseString(verilog_input);
    } else {
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
    }

    Netlist netlist;
    netlist.buildGraph(parsed);
    SimEngine engine(&netlist);

    if (run_unit_test) {
        runCliTestSuite(engine);
        return 0;
    }

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
    for (const auto& pin : parsed.input_pins) {
        std::cout << pin << " ";
        // Initialize all input pins dynamically to ZERO at t=0
        IOController::executeCommand("set " + pin + " 0 at 0", engine);
    }
    std::cout << "\nOutputs: ";
    for (const auto& pin : parsed.output_pins) std::cout << pin << " ";
    std::cout << "\n----------------------------------------------------------------------\n";

    // Dynamic pre-population logic: only run demo if pins 'a' and 'b' exist
    bool has_a = std::find(parsed.input_pins.begin(), parsed.input_pins.end(), "a") != parsed.input_pins.end();
    bool has_b = std::find(parsed.input_pins.begin(), parsed.input_pins.end(), "b") != parsed.input_pins.end();

    if (has_a && has_b) {
        std::cout << "[PRE-POPULATING PROPAGATION DELAY DEMO SEQUENCE]\n";
        IOController::executeCommand("set a 1 at 0", engine);
        IOController::executeCommand("set b 1 at 0", engine);
        IOController::executeCommand("run 5", engine);
        IOController::executeCommand("set a 0 at 5", engine);
        IOController::executeCommand("run 5", engine);
        std::cout << "----------------------------------------------------------------------\n";
    } else {
        // General initial run for any loaded module
        IOController::executeCommand("run 10", engine);
    }

    std::cout << "[RUNTIME CONTROL INSTRUCTIONS]\n";
    std::cout << "  Type commands directly using the GUI control box!\n";
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
