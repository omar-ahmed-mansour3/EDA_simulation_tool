#ifndef IOCONTROLLER_H
#define IOCONTROLLER_H

#include <string>
#include <vector>
#include <map>
#include "Common.h"


class SimEngine; 
class Netlist;   

class IOController {
public:
    // Workflow A. 
    // Takes what the user types (like "set b 1 at 0") and feeds it to the simulation engine.
    static void executeCommand(const std::string& command, SimEngine& engine);

    // Workflow B.
    // Grabs the whole simulation history and spits out an IEEE 1364 VCD file so we can view the waveforms.
    static void exportVCD(const std::string& filename, const Netlist& netlist, const std::vector<Event>& history);

private:
    //  helpers to make converting states back and forth easier
    static LogicState charToLogicState(char c);
    static char logicStateToChar(LogicState state);
    
    // Generates required ASCII symbols for the VCD file variables (!, ", #, etc.)
    static std::string generateVcdId(int index);
};

#endif 
