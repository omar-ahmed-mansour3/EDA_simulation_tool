#include "Netlist.hpp"
#include <stdexcept>
#include <utility>

namespace {

bool isKnown(LogicState s) {
    return s == LogicState::ZERO || s == LogicState::ONE;
}

LogicState invert(LogicState s) {
    if (s == LogicState::ZERO) return LogicState::ONE;
    if (s == LogicState::ONE) return LogicState::ZERO;
    return LogicState::X;
}

// a single 0 forces the result to 0, even if the other inputs are unknown
LogicState andOf(const std::vector<Wire*>& inputs) {
    bool unknown = false;
    for (Wire* w : inputs) {
        LogicState s = w->current_state;
        if (s == LogicState::ZERO) return LogicState::ZERO;
        if (!isKnown(s)) unknown = true;
    }
    return unknown ? LogicState::X : LogicState::ONE;
}

// a single 1 forces the result to 1
LogicState orOf(const std::vector<Wire*>& inputs) {
    bool unknown = false;
    for (Wire* w : inputs) {
        LogicState s = w->current_state;
        if (s == LogicState::ONE) return LogicState::ONE;
        if (!isKnown(s)) unknown = true;
    }
    return unknown ? LogicState::X : LogicState::ZERO;
}

// xor has no dominating value, so one unknown input ruins the whole result
LogicState xorOf(const std::vector<Wire*>& inputs) {
    int ones = 0;
    for (Wire* w : inputs) {
        if (!isKnown(w->current_state)) return LogicState::X;
        if (w->current_state == LogicState::ONE) ones++;
    }
    return (ones % 2 == 1) ? LogicState::ONE : LogicState::ZERO;
}

bool drivesItsOwnInput(const Gate* g) {
    for (const Wire* in : g->inputs) {
        if (in == g->output) return true;
    }
    return false;
}

// walks the gates depth first looking for a loop with no gate in between.
// person 3 would never finish simulating one of these.
bool hasCombinationalLoop(const Netlist& n) {
    std::unordered_map<const Gate*, int> colour;   // 0 new, 1 on the stack, 2 done
    for (const auto& g : n.gates) colour[g.get()] = 0;

    for (const auto& start : n.gates) {
        if (colour[start.get()] != 0) continue;

        std::vector<std::pair<const Gate*, std::size_t>> stack;
        colour[start.get()] = 1;
        stack.emplace_back(start.get(), 0);

        while (!stack.empty()) {
            const Gate* g = stack.back().first;
            std::size_t next_index = stack.back().second;

            if (g->output != nullptr && next_index < g->output->fanout_gates.size()) {
                stack.back().second = next_index + 1;
                const Gate* next = g->output->fanout_gates[next_index];

                if (colour[next] == 1) return true;
                if (colour[next] == 0) {
                    colour[next] = 1;
                    stack.emplace_back(next, 0);
                }
            } else {
                colour[g] = 2;
                stack.pop_back();
            }
        }
    }
    return false;
}

void validateGraph(const Netlist& n) {
    for (const auto& w : n.wires) {
        if (w->name.empty())
            throw std::runtime_error("netlist: a wire was given an empty name");
    }

    for (const auto& g : n.gates) {
        if (g->inputs.empty())
            throw std::runtime_error("netlist: gate driving '" + g->output->name +
                                     "' has no inputs");
        if (drivesItsOwnInput(g.get()))
            throw std::runtime_error("netlist: gate driving '" + g->output->name +
                                     "' also reads it, that is a loop");
    }

    std::unordered_map<const Wire*, int> driver_count;
    for (const auto& g : n.gates) {
        driver_count[g->output]++;
        if (driver_count[g->output] > 1)
            throw std::runtime_error("netlist: wire '" + g->output->name +
                                     "' is driven by more than one gate");
    }

    if (hasCombinationalLoop(n))
        throw std::runtime_error("netlist: the design contains a combinational loop");
}

} // namespace

LogicState Gate::evaluate() const {
    if (inputs.empty()) return LogicState::X;

    switch (type) {
        case GateType::AND:  return andOf(inputs);
        case GateType::NAND: return invert(andOf(inputs));
        case GateType::OR:   return orOf(inputs);
        case GateType::NOR:  return invert(orOf(inputs));
        case GateType::XOR:  return xorOf(inputs);
        case GateType::XNOR: return invert(xorOf(inputs));
        case GateType::NOT:  return invert(inputs[0]->current_state);
        case GateType::BUF:  return isKnown(inputs[0]->current_state)
                                        ? inputs[0]->current_state
                                        : LogicState::X;
    }
    return LogicState::X;
}

Wire* Netlist::getOrCreateWire(const std::string& name) {
    auto it = wire_map.find(name);
    if (it != wire_map.end()) return it->second;

    wires.push_back(std::make_unique<Wire>());
    Wire* w = wires.back().get();
    w->name = name;
    wire_map[name] = w;
    return w;
}

Wire* Netlist::findWire(const std::string& name) const {
    auto it = wire_map.find(name);
    if (it == wire_map.end()) return nullptr;
    return it->second;
}

void Netlist::buildGraph(const ParsedModule& parsed_data) {
    // start clean so calling this twice rebuilds instead of doubling everything
    wires.clear();
    gates.clear();
    wire_map.clear();

    for (const auto& name : parsed_data.input_pins) {
        getOrCreateWire(name);
    }
    for (const auto& name : parsed_data.output_pins) {
        getOrCreateWire(name);
    }
    for (const auto& name : parsed_data.internal_wires) {
        getOrCreateWire(name);
    }

    for (const auto& parsed_gate : parsed_data.gates) {
        auto gate = std::make_unique<Gate>();
        gate->type = parsed_gate.type;
        gate->delay = parsed_gate.delay;

        for (const auto& input_name : parsed_gate.inputs) {
            gate->inputs.push_back(getOrCreateWire(input_name));
        }
        gate->output = getOrCreateWire(parsed_gate.output);

        // take the pointer before the move, gate is empty afterwards
        Gate* raw = gate.get();
        gates.push_back(std::move(gate));

        for (Wire* w : raw->inputs) {
            w->fanout_gates.push_back(raw);
        }
    }

    validateGraph(*this);
}
