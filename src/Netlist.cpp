#include "Netlist.hpp"

LogicState Gate::evaluate() const {
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
}
