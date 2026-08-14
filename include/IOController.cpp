#include "IOController.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <algorithm>

// uncomment these once your files are pushed to the repo!
// #include "SimEngine.h"
// #include "Netlist.h"


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
    // Wrote this loop to generate them dynamically based on the wire index count.
    std::string id = "";
    do {
        id += static_cast<char>('!' + (index % 94));
        index /= 94;
    } while (index > 0);
    return id;
}

// Workflow A: CLI Stuff

void IOController::executeCommand(const std::string& command, SimEngine& engine) {
    std::istringstream iss(command);
    std::string token;
    
    
    if (!(iss >> token)) return; 

    if (token == "set") {
        std::string wire_name;
        char val;
        std::string at_keyword;
        uint64_t time;

        // Parsing the exact "set <wire> <val> at <time>" format requested
        if (iss >> wire_name >> val >> at_keyword >> time && at_keyword == "at") {
            LogicState state = charToLogicState(val);
            
            // TODO (Person 3): confirm this exact method signature is implemented in SimEngine!
            // engine.injectEventByName(time, wire_name, state);
            
            std::cout << "Event injected: " << wire_name << " = " << val << " at " << time << "ns\n";
        } else {
            std::cerr << "Whoops, invalid format. Make sure it's: set <wire> <value> at <time>\n";
        }
    } else {
        std::cerr << "Command not recognized: '" << token << "'\n";
    }
}

// Workflow B: Exporting the Waveform

void IOController::exportVCD(const std::string& filename, const Netlist& netlist, const std::vector<Event>& history) {
    std::ofstream vcd_file(filename);
    if (!vcd_file.is_open()) {
        std::cerr << "Couldn't open " << filename << " to write the VCD. Check folder permissions maybe?\n";
        return;
    }

    // Setting up the VCD header dynamically so waveform viewers don't complain
    std::time_t t = std::time(nullptr);
    char time_str[100];
    std::strftime(time_str, sizeof(time_str), "%Y-%m-%d", std::localtime(&t));

    vcd_file << "$date " << time_str << " $end\n";
    vcd_file << "$version EDA Simulator $end\n";
    vcd_file << "$timescale 1ns $end\n";
    vcd_file << "$scope module top $end\n";

    // Mapping wires to their ASCII identifiers
    std::map<Wire*, std::string> wire_to_id;
    int id_counter = 0;

    // TODO (Person 2): needs a way to get all the wires from your netlist!
    // uncomment this loop after person 2 is done
    /*
    for (const auto& wire_ptr : netlist.getAllWires()) {
        std::string vcd_id = generateVcdId(id_counter++);
        wire_to_id[wire_ptr] = vcd_id;
        vcd_file << "$var wire 1 " << vcd_id << " " << wire_ptr->name << " $end\n";
    }
    */
   
    // Temporary: Hardcoded the a, b, c wires (to test the build)
    // REMOVE THIS BLOCK once the real getter is ready
    Wire mock_b{"b", LogicState::X};
    Wire mock_c{"c", LogicState::X};
    Wire mock_a{"a", LogicState::X};
    wire_to_id[&mock_b] = generateVcdId(0); // This becomes '!'
    wire_to_id[&mock_c] = generateVcdId(1); // This becomes '"'
    wire_to_id[&mock_a] = generateVcdId(2); // This becomes '#'
    
    vcd_file << "$var wire 1 ! b $end\n";
    vcd_file << "$var wire 1 \" c $end\n";
    vcd_file << "$var wire 1 # a $end\n";

    vcd_file << "$upscope $end\n";
    vcd_file << "$enddefinitions $end\n";

    // Initialize time 0
    vcd_file << "#0\n";
    vcd_file << "$dumpvars\n";

    // Keeping track of states so we only log changes (keeps the VCD file size down)
    std::map<Wire*, LogicState> current_states;
    
    // Throwing events into a map sorted by time makes writing the file way easier
    std::map<uint64_t, std::vector<Event>> events_by_time;
    for (const auto& ev : history) {
        events_by_time[ev.timestamp].push_back(ev);
    }

    // Grab the initial states at t=0
    if (events_by_time.find(0) != events_by_time.end()) {
        for (const auto& ev : events_by_time[0]) {
            current_states[ev.wire] = ev.new_state;
        }
    }

    // initial variables. If a wire doesn't have a t=0 event, default it to 'x'
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

    // Loop through the rest of the simulation timeline
    for (auto const& [timestamp, time_events] : events_by_time) {
        if (timestamp == 0) continue; // Already handled t=0 above

        vcd_file << "#" << timestamp << "\n";
        for (const auto& ev : time_events) {
            // Good practice: only log it to the VCD if the state actually changed
            if (current_states[ev.wire] != ev.new_state) {
                vcd_file << logicStateToChar(ev.new_state) << wire_to_id[ev.wire] << "\n";
                current_states[ev.wire] = ev.new_state;
            }
        }
    }

    vcd_file.close();
    std::cout << "Done! Exported the waveform to " << filename << "\n";
}
