// SimEngine.cpp
// Implements:
//   - SimEngine::injectEvent()       : Push a pre-formed Event onto the time_wheel
//   - SimEngine::injectEventByName() : Lookup wire by name, then push event
//   - SimEngine::stepTo()            : Core discrete-event simulation loop
//   - SimEngine::reset()             : Clear all state for a fresh run

#include "../include/SimEngine.hpp"
#include <iostream>

// ---------------------------------------------------------------------------
// SimEngine::injectEvent()
//
// Creates an Event with a guaranteed-unique sequence_id for FIFO tie-breaking
// when multiple events share the same timestamp, then pushes it onto the
// min-heap priority queue (time_wheel).
//
// Called internally when a gate output changes (future-scheduled events),
// and externally by IOController for user stimulus injection.
// ---------------------------------------------------------------------------
void SimEngine::injectEvent(uint64_t time, Wire* wire, LogicState state) {
    if (!wire) return; // Defensive null check

    Event ev;
    ev.timestamp   = time;
    ev.sequence_id = ++event_counter; // increasing to preserve FIFO order
    ev.wire        = wire;
    ev.new_state   = state;

    time_wheel.push(ev);
}

// ---------------------------------------------------------------------------
// SimEngine::injectEventByName()
//
// Convenience wrapper used by IOController and the test harness main.cpp.
// Looks up the wire by its string name in the Netlist, then calls injectEvent.
// Prints a warning if the wire name is not found (catches typos in CLI commands).
// ---------------------------------------------------------------------------
void SimEngine::injectEventByName(uint64_t time, const std::string& wire_name, LogicState state) {
    Wire* wire = netlist->findWire(wire_name);
    if (wire) {
        injectEvent(time, wire, state);
    } else {
        std::cerr << "[SimEngine] Warning: Wire \"" << wire_name
                  << "\" not found in netlist. Event ignored.\n";
    }
}

// ---------------------------------------------------------------------------
// SimEngine::stepTo()
//
// The core discrete-event simulation loop. Processes all events in the
// time_wheel whose timestamp <= target_time, then yields back to the caller.
//
// Algorithm (per event iteration):
//   1. Peek at the earliest event in the min-heap.
//   2. If its timestamp > target_time, stop and return — simulation is paused.
//   3. Pop the event and update current_time.
//   4. INERTIAL DELAY CHECK: If the wire is already in the requested state,
//      discard this event (cancelled out by a later contradicting input).
//   5. Apply the state change to the wire and record in simulation_history.
//   6. Walk the wire's fanout_gates list; re-evaluate each gate.
//   7. If a gate's new output differs from its current output wire state,
//      schedule a new future Event at (current_time + gate->delay).
//
// Safety:
//   - max_events cap prevents infinite loops caused by 0-delay feedback.
//   - Returns false if the cap is hit (GUI can display a warning).
// ---------------------------------------------------------------------------
bool SimEngine::stepTo(uint64_t target_time, size_t max_events) {
    size_t processed = 0;

    while (!time_wheel.empty()) {
        // Peek at the earliest event — do NOT pop yet
        const Event& next_ev = time_wheel.top();

        // Stop if the next event is beyond our simulation window
        if (next_ev.timestamp > target_time) break;

        // Safety guard against 0-delay combinational loops
        if (processed >= max_events) {
            std::cerr << "[SimEngine] ERROR: max_events (" << max_events
                      << ") exceeded at t=" << current_time
                      << "ns. Possible 0-delay feedback loop detected.\n";
            return false;
        }

        // Pop the event from the queue
        Event current_ev = time_wheel.top();
        time_wheel.pop();
        processed++;

        // Advance simulation clock to this event's time
        current_time = current_ev.timestamp;
        Wire* target_wire = current_ev.wire;

        // Inertial delay check:
        // If wire is already at the target state, this event is outdated — skip it.
        // (Happens when a later input contradicted this scheduled transition.)
        if (target_wire->current_state == current_ev.new_state) {
            continue;
        }

        // Apply the state change to the wire
        target_wire->current_state = current_ev.new_state;

        // Record this transition permanently in the waveform history
        simulation_history.push_back(current_ev);

        // Propagate through all downstream gates driven by this wire
        for (Gate* gate : target_wire->fanout_gates) {
            if (!gate->output) continue; // check for unconnected gate output

            // Re-evaluate this gate with the updated wire state
            LogicState new_output = gate->evaluate();

            // Only schedule a future event if the output actually changed
            // (avoids redundant events and unnecessary gate re-evaluations)
            if (new_output != gate->output->current_state) {
                uint64_t scheduled_time = current_time + static_cast<uint64_t>(gate->delay);
                injectEvent(scheduled_time, gate->output, new_output);
            }
        }
    }

    // Advance clock to target_time even if no events were processed in this window
    current_time = target_time;
    return true;
}

// ---------------------------------------------------------------------------
// SimEngine::reset()
//
// Resets the simulation to a clean initial state:
//   - Clears the time_wheel priority queue
//   - Clears simulation_history
//   - Resets clock and event counter to 0
//
// Note: Does NOT reset wire states in the Netlist. Caller should re-run
// Netlist::buildGraph() or manually set wires back to LogicState::X if needed.
// ---------------------------------------------------------------------------
void SimEngine::reset() {
    // Drain the priority queue (no clear() on std::priority_queue)
    while (!time_wheel.empty()) {
        time_wheel.pop();
    }

    simulation_history.clear();
    current_time  = 0;
    event_counter = 0;
}
