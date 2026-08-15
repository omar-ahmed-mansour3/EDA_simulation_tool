#pragma once
#include "Netlist.hpp"
#include <queue>
#include <vector>
#include <unordered_map>

class SimEngine {
private:
    Netlist* netlist;
    uint64_t current_time = 0;
    uint64_t event_counter = 0; // Monotonic counter for deterministic tie-breaking

    std::priority_queue<Event, std::vector<Event>, std::greater<Event>> time_wheel;
    std::vector<Event> simulation_history;

public:
    explicit SimEngine(Netlist* nl) : netlist(nl) {}

    // Inject runtime events into the priority queue
    void injectEvent(uint64_t time, Wire* wire, LogicState state);
    bool injectEventByName(uint64_t time, const std::string& wire_name, LogicState state);

    // Processes the time_wheel queue up to target_time with max event loop guard
    bool stepTo(uint64_t target_time, size_t max_events = 50000);

    const std::vector<Event>& getHistory() const { return simulation_history; }
    uint64_t getCurrentTime() const { return current_time; }
    Netlist* getNetlist() const { return netlist; }
    void reset();
};
