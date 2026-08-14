// Netlist.cpp
// Implements:
//   - Gate::evaluate()  : 4-state logic truth tables (0, 1, X, Z)
//   - Netlist::getOrCreateWire() : Wire allocation and wire_map registration
//   - Netlist::buildGraph()      : Wires and Gates construction from ParsedModule
//   - Netlist::findWire()        : Wire lookup by name

#include "../include/Netlist.hpp"
#include <stdexcept>

// ---------------------------------------------------------------------------
// Helper: Evaluate a single logic state through NOT
// ---------------------------------------------------------------------------
static LogicState logicNot(LogicState s) {
    switch (s) {
        case LogicState::ZERO: return LogicState::ONE;
        case LogicState::ONE:  return LogicState::ZERO;
        default:               return LogicState::X; // NOT(X) = X, NOT(Z) = X
    }
}

// ---------------------------------------------------------------------------
// Gate::evaluate()
//
// Implements 4-state IEEE logic for AND, OR, NOT, NAND, NOR, XOR, XNOR, BUF.
//
// Key rules:
//   AND: Dominant 0  -> 0 & anything = 0
//   OR:  Dominant 1  -> 1 | anything = 1
//   NOT: Inverts 0/1, returns X for X/Z inputs
//   Z is treated as X for all standard gate inputs (no tri-state logic here)
// ---------------------------------------------------------------------------
LogicState Gate::evaluate() const {
    if (inputs.empty()) return LogicState::X;

    switch (type) {

        // ---------------------------------------------------------------
        case GateType::BUF:
            return inputs[0]->current_state; // Buffer passes state directly

        // ---------------------------------------------------------------
        case GateType::NOT:
            return logicNot(inputs[0]->current_state);

        // ---------------------------------------------------------------
        case GateType::AND: {
            bool has_unknown = false;
            for (Wire* w : inputs) {
                LogicState s = w->current_state;
                if (s == LogicState::ZERO)            return LogicState::ZERO; // Dominant 0
                if (s == LogicState::X || s == LogicState::Z) has_unknown = true;
            }
            return has_unknown ? LogicState::X : LogicState::ONE;
        }

        // ---------------------------------------------------------------
        case GateType::OR: {
            bool has_unknown = false;
            for (Wire* w : inputs) {
                LogicState s = w->current_state;
                if (s == LogicState::ONE)             return LogicState::ONE; // Dominant 1
                if (s == LogicState::X || s == LogicState::Z) has_unknown = true;
            }
            return has_unknown ? LogicState::X : LogicState::ZERO;
        }

        // ---------------------------------------------------------------
        case GateType::NAND: {
            // NAND = NOT(AND)
            bool has_unknown = false;
            for (Wire* w : inputs) {
                LogicState s = w->current_state;
                if (s == LogicState::ZERO)            return LogicState::ONE; // NOT(dominant 0) = 1
                if (s == LogicState::X || s == LogicState::Z) has_unknown = true;
            }
            return has_unknown ? LogicState::X : LogicState::ZERO;
        }

        // ---------------------------------------------------------------
        case GateType::NOR: {
            // NOR = NOT(OR)
            bool has_unknown = false;
            for (Wire* w : inputs) {
                LogicState s = w->current_state;
                if (s == LogicState::ONE)             return LogicState::ZERO; // NOT(dominant 1) = 0
                if (s == LogicState::X || s == LogicState::Z) has_unknown = true;
            }
            return has_unknown ? LogicState::X : LogicState::ONE;
        }

        // ---------------------------------------------------------------
        case GateType::XOR: {
            // XOR: any X/Z input -> output is X
            // Otherwise: count number of ONE inputs; result is ONE if odd
            int one_count = 0;
            for (Wire* w : inputs) {
                LogicState s = w->current_state;
                if (s == LogicState::X || s == LogicState::Z) return LogicState::X;
                if (s == LogicState::ONE) one_count++;
            }
            return (one_count % 2 == 1) ? LogicState::ONE : LogicState::ZERO;
        }

        // ---------------------------------------------------------------
        case GateType::XNOR: {
            // XNOR = NOT(XOR)
            int one_count = 0;
            for (Wire* w : inputs) {
                LogicState s = w->current_state;
                if (s == LogicState::X || s == LogicState::Z) return LogicState::X;
                if (s == LogicState::ONE) one_count++;
            }
            return (one_count % 2 == 0) ? LogicState::ONE : LogicState::ZERO;
        }

        // ---------------------------------------------------------------
        case GateType::EQ: { // a == b (logical equality)
            if (inputs.size() < 2) return LogicState::X;
            LogicState s1 = inputs[0]->current_state;
            LogicState s2 = inputs[1]->current_state;
            if (s1 == LogicState::X || s1 == LogicState::Z ||
                s2 == LogicState::X || s2 == LogicState::Z) {
                return LogicState::X;
            }
            return (s1 == s2) ? LogicState::ONE : LogicState::ZERO;
        }

        // ---------------------------------------------------------------
        case GateType::CASE_EQ: { // a === b (case equality: exact state match)
            if (inputs.size() < 2) return LogicState::X;
            LogicState s1 = inputs[0]->current_state;
            LogicState s2 = inputs[1]->current_state;
            return (s1 == s2) ? LogicState::ONE : LogicState::ZERO;
        }

        // ---------------------------------------------------------------
        case GateType::NE: { // a != b (logical inequality)
            if (inputs.size() < 2) return LogicState::X;
            LogicState s1 = inputs[0]->current_state;
            LogicState s2 = inputs[1]->current_state;
            if (s1 == LogicState::X || s1 == LogicState::Z ||
                s2 == LogicState::X || s2 == LogicState::Z) {
                return LogicState::X;
            }
            return (s1 != s2) ? LogicState::ONE : LogicState::ZERO;
        }

        // ---------------------------------------------------------------
        case GateType::CASE_NE: { // a !== b (case inequality: exact state mismatch)
            if (inputs.size() < 2) return LogicState::X;
            LogicState s1 = inputs[0]->current_state;
            LogicState s2 = inputs[1]->current_state;
            return (s1 != s2) ? LogicState::ONE : LogicState::ZERO;
        }

        // ---------------------------------------------------------------
        case GateType::LOGICAL_AND: { // a && b
            bool has_unknown = false;
            for (Wire* w : inputs) {
                LogicState s = w->current_state;
                if (s == LogicState::ZERO)            return LogicState::ZERO; // Dominant 0
                if (s == LogicState::X || s == LogicState::Z) has_unknown = true;
            }
            return has_unknown ? LogicState::X : LogicState::ONE;
        }

        // ---------------------------------------------------------------
        case GateType::LOGICAL_OR: { // a || b
            bool has_unknown = false;
            for (Wire* w : inputs) {
                LogicState s = w->current_state;
                if (s == LogicState::ONE)             return LogicState::ONE; // Dominant 1
                if (s == LogicState::X || s == LogicState::Z) has_unknown = true;
            }
            return has_unknown ? LogicState::X : LogicState::ZERO;
        }

        // ---------------------------------------------------------------
        default:
            return LogicState::X;
    }
}

// ---------------------------------------------------------------------------
// Netlist::getOrCreateWire()
//
// Returns an existing wire if it already exists in wire_map.
// Otherwise allocates a new Wire, stores it in the wires vector (ownership),
// registers it in wire_map (fast lookup), and returns the raw pointer.
// ---------------------------------------------------------------------------
Wire* Netlist::getOrCreateWire(const std::string& name) {
    auto it = wire_map.find(name);
    if (it != wire_map.end()) {
        return it->second;
    }

    auto new_wire       = std::make_unique<Wire>();
    new_wire->name      = name;
    new_wire->current_state = LogicState::X; // All wires start as Unknown

    Wire* raw_ptr       = new_wire.get();
    wire_map[name]      = raw_ptr;
    wires.push_back(std::move(new_wire));

    return raw_ptr;
}

// ---------------------------------------------------------------------------
// Netlist::buildGraph()
//
// Converts a ParsedModule (plain data from the Parser) into the live pointer
// graph that the SimEngine will walk during simulation.
//
// Step 1: Create a Wire for every pin and internal wire declared in the module.
// Step 2: For each ParsedGate, allocate a Gate, connect input/output Wire*,
//         and register the gate in each input wire's fanout list.
// ---------------------------------------------------------------------------
void Netlist::buildGraph(const ParsedModule& parsed_data) {
    // Step 1 - Allocate all declared wires upfront so gate pointers are valid
    for (const auto& name : parsed_data.input_pins)     getOrCreateWire(name);
    for (const auto& name : parsed_data.output_pins)    getOrCreateWire(name);
    for (const auto& name : parsed_data.internal_wires) getOrCreateWire(name);

    // Step 2 - Build each gate and wire up the pointers
    for (const ParsedGate& pg : parsed_data.gates) {
        auto gate_ptr    = std::make_unique<Gate>();
        gate_ptr->type   = pg.type;
        gate_ptr->delay  = pg.delay;
        gate_ptr->output = getOrCreateWire(pg.output);

        // Connect input wires and register this gate in each wire's fanout list
        for (const std::string& in_name : pg.inputs) {
            Wire* in_wire = getOrCreateWire(in_name);
            gate_ptr->inputs.push_back(in_wire);
            in_wire->fanout_gates.push_back(gate_ptr.get()); // raw pointer is safe here
        }

        gates.push_back(std::move(gate_ptr));
    }
}

// ---------------------------------------------------------------------------
// Netlist::findWire()
//
// Returns the Wire* for a given name, or nullptr if not found.
// Used by SimEngine::injectEventByName() and the IOController.
// ---------------------------------------------------------------------------
Wire* Netlist::findWire(const std::string& name) const {
    auto it = wire_map.find(name);
    return (it != wire_map.end()) ? it->second : nullptr;
}
