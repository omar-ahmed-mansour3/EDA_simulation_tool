// main.cpp
// Automated Test Harness for Person 4: IOController CLI Commands

#include "Parser.hpp"
#include "Netlist.hpp"
#include "SimEngine.hpp"
#include "IOController.hpp"
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>

static std::string stateToString(LogicState s) {
    switch (s) {
        case LogicState::ZERO: return "0";
        case LogicState::ONE:  return "1";
        case LogicState::X:    return "x";
        case LogicState::Z:    return "z";
        default:               return "?";
    }
}

int main() {
    std::cout << "======================================================================\n";
    std::cout << "        PERSON 4: IOCONTROLLER CLI COMMAND TEST SUITE                 \n";
    std::cout << "======================================================================\n\n";

    std::string verilog_input = 
        "module cli_test(input in1, input in2, output out);\n"
        "    assign out = in1 | in2;\n"
        "endmodule\n";

    // Setup Circuit
    ParsedModule parsed = VerilogParser::parseString(verilog_input);
    Netlist netlist;
    netlist.buildGraph(parsed);
    SimEngine engine(&netlist);

    std::cout << "[EXECUTING CLI COMMAND SEQUENCE]\n";
    std::cout << "----------------------------------------------------------------------\n";

    // Command 1
    std::cout << "Command 1: 'set in1 1 at 0'\n   -> Actual Output: ";
    IOController::executeCommand("set in1 1 at 0", engine);

    // Command 2
    std::cout << "Command 2: 'set in2 0 at 0'\n   -> Actual Output: ";
    IOController::executeCommand("set in2 0 at 0", engine);

    // Command 3
    std::cout << "Command 3: 'run 5'\n   -> Actual Output: ";
    IOController::executeCommand("run 5", engine);

    // Command 4
    std::cout << "Command 4: 'set in1 0 at 5'\n   -> Actual Output: ";
    IOController::executeCommand("set in1 0 at 5", engine);

    // Command 5
    std::cout << "Command 5: 'run 5'\n   -> Actual Output: ";
    IOController::executeCommand("run 5", engine);

    // Command 6 (Error case: invalid wire)
    std::cout << "Command 6: 'set bad_wire 1 at 10' (Invalid Wire Test)\n   -> Actual Output: ";
    IOController::executeCommand("set bad_wire 1 at 10", engine);

    // Command 7 (Error case: invalid command)
    std::cout << "Command 7: 'foobar 123' (Invalid Command Test)\n   -> Actual Output: ";
    IOController::executeCommand("foobar 123", engine);

    std::cout << "----------------------------------------------------------------------\n\n";

    // Compare Logged History Against Expectations
    std::cout << "[VERIFYING WAVEFORM HISTORY AFTER CLI EXECUTION]\n";
    std::cout << std::left << std::setw(12) << "Timestamp" << std::setw(14) << "Wire" << "New State\n";
    std::cout << std::string(36, '-') << "\n";

    for (const auto& ev : engine.getHistory()) {
        std::cout << std::left
                  << std::setw(12) << ("t=" + std::to_string(ev.timestamp) + "ns")
                  << std::setw(14) << ev.wire->name
                  << stateToString(ev.new_state) << "\n";
    }

    // Checking expectations
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

    return 0;
}
