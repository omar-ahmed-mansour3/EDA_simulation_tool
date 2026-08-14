#include "IOController.hpp"
#include "SimEngine.hpp"
#include "Netlist.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <algorithm>

// Helper Methods

LogicState IOController::charToLogicState(char c) {
    // Translating raw user input chars into our shared enum
    switch (c) {
        case '0': return LogicState::ZERO;
        case '1': return LogicState::ONE;
        case 'z': case 'Z': return LogicState::Z;
        case 'x': case 'X': default: return LogicState::X;
    }
}

char IOController::logicStateToChar(LogicState state) {
    // Need this for writing out to the VCD text file later
    switch (state) {
        case LogicState::ZERO: return '0';
        case LogicState::ONE:  return '1';
        case LogicState::Z:    return 'z';
        case LogicState::X:    return 'x';
        default:               return 'x';
    }
}

std::string IOController::generateVcdId(int index) {
    // VCD specs need printable ASCII characters starting from '!' (ASCII 33). 
    std::string id = "";
    do {
        id += static_cast<char>('!' + (index % 94));
        index /= 94;
    } while (index > 0);
    return id;
}

// Workflow A: CLI Command Parsing

void IOController::executeCommand(const std::string& command, SimEngine& engine) {
    std::istringstream iss(command);
    std::string token;
    
    if (!(iss >> token)) return; 

    if (token == "set") {
        std::string wire_name;
        char val;
        std::string at_keyword;
        uint64_t time;

        // Parsing "set <wire> <val> at <time>"
        if (iss >> wire_name >> val >> at_keyword >> time && at_keyword == "at") {
            LogicState state = charToLogicState(val);
            engine.injectEventByName(time, wire_name, state);
            std::cout << "Event injected: " << wire_name << " = " << val << " at " << time << "ns\n";
        } else {
            std::cerr << "Format error. Usage: set <wire> <value> at <time>\n";
        }
    } else {
        std::cerr << "Command not recognized: '" << token << "'\n";
    }
}

// Workflow B: Exporting Waveform VCD File

void IOController::exportVCD(const std::string& filename, const Netlist& netlist, const std::vector<Event>& history) {
    std::ofstream vcd_file(filename);
    if (!vcd_file.is_open()) {
        std::cerr << "Couldn't open " << filename << " to write VCD file.\n";
        return;
    }

    std::time_t t = std::time(nullptr);
    char time_str[100];
    std::strftime(time_str, sizeof(time_str), "%Y-%m-%d", std::localtime(&t));

    vcd_file << "$date " << time_str << " $end\n";
    vcd_file << "$version EDA Simulator $end\n";
    vcd_file << "$timescale 1ns $end\n";
    vcd_file << "$scope module top $end\n";

    // Mapping wires to ASCII identifiers
    std::map<Wire*, std::string> wire_to_id;
    int id_counter = 0;

    for (const auto& wire_ptr : netlist.wires) {
        std::string vcd_id = generateVcdId(id_counter++);
        wire_to_id[wire_ptr.get()] = vcd_id;
        vcd_file << "$var wire 1 " << vcd_id << " " << wire_ptr->name << " $end\n";
    }

    vcd_file << "$upscope $end\n";
    vcd_file << "$enddefinitions $end\n";

    // Initialize time 0
    vcd_file << "#0\n";
    vcd_file << "$dumpvars\n";

    std::map<Wire*, LogicState> current_states;
    std::map<uint64_t, std::vector<Event>> events_by_time;
    for (const auto& ev : history) {
        events_by_time[ev.timestamp].push_back(ev);
    }

    if (events_by_time.find(0) != events_by_time.end()) {
        for (const auto& ev : events_by_time[0]) {
            current_states[ev.wire] = ev.new_state;
        }
    }

    for (const auto& pair : wire_to_id) {
        Wire* w = pair.first;
        std::string id = pair.second;
        
        LogicState state = LogicState::X;
        if (current_states.find(w) != current_states.end()) {
            state = current_states[w];
        }
        vcd_file << logicStateToChar(state) << id << "\n";
    }
    vcd_file << "$end\n";

    for (auto const& [timestamp, time_events] : events_by_time) {
        if (timestamp == 0) continue;

        vcd_file << "#" << timestamp << "\n";
        for (const auto& ev : time_events) {
            if (current_states[ev.wire] != ev.new_state) {
                vcd_file << logicStateToChar(ev.new_state) << wire_to_id[ev.wire] << "\n";
                current_states[ev.wire] = ev.new_state;
            }
        }
    }

    vcd_file.close();
    std::cout << "Done! Exported waveform to " << filename << "\n";
}
