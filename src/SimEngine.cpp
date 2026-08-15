#include "../include/SimEngine.hpp"
#include <iostream>


void SimEngine::injectEvent(uint64_t time, Wire* wire, LogicState state) {
    if (!wire) return; // Defensive null check
    if (time < current_time) return; // Cannot schedule in the past

    Event ev;
    ev.timestamp   = time;
    ev.sequence_id = ++event_counter; 
    ev.wire        = wire;
    ev.new_state   = state;

    time_wheel.push(ev);
}

bool SimEngine::injectEventByName(uint64_t time, const std::string& wire_name, LogicState state) {
    if (time < current_time) {
        std::cerr << "[SimEngine] Error: Cannot schedule event at " << time
                  << "ns. Current simulation time is " << current_time << "ns (time has already passed).\n";
        return false;
    }
    Wire* wire = netlist ? netlist->findWire(wire_name) : nullptr;
    if (wire) {
        injectEvent(time, wire, state);
        return true;
    } else {
        std::cerr << "[SimEngine] Warning: Wire \"" << wire_name
                  << "\" not found in netlist. Event ignored.\n";
        return false;
    }
}



bool SimEngine::stepTo(uint64_t target_time, size_t max_events) {
    size_t processed = 0;

    while (!time_wheel.empty()) {
        const Event& next_ev = time_wheel.top();

        if (next_ev.timestamp > target_time) break;

        if (processed >= max_events) {
            std::cerr << "[SimEngine] ERROR: max_events (" << max_events
                      << ") exceeded at t=" << current_time
                      << "ns. Possible 0-delay feedback loop detected.\n";
            return false;
        }

        Event current_ev = time_wheel.top();
        time_wheel.pop();
        processed++;

        current_time = current_ev.timestamp;
        Wire* target_wire = current_ev.wire;


        // This check correctly discards redundant events when they are processed,
        // which prevents infinite loops and handles glitch masking properly.
        if (target_wire->current_state == current_ev.new_state) {
            continue;
        }

        target_wire->current_state = current_ev.new_state;

        simulation_history.push_back(current_ev);

      
        for (Gate* gate : target_wire->fanout_gates) {
            if (!gate->output) continue; 
            
            LogicState new_output = gate->evaluate();

            uint64_t scheduled_time = current_time + static_cast<uint64_t>(gate->delay);
            injectEvent(scheduled_time, gate->output, new_output);
        }
    }

    current_time = target_time;
    return true;
}


void SimEngine::reset() {

    while (!time_wheel.empty()) {
        time_wheel.pop();
    }

    simulation_history.clear();
    current_time  = 0;
    event_counter = 0;
}
