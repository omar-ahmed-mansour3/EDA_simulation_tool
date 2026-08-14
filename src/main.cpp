// main.cpp
// Standalone test harness for the SimEngine and Netlist modules.
// Verifies 4-state IEEE Verilog logic against the full 16-row Truth Table.

#include "../include/SimEngine.hpp"
#include <iostream>
#include <iomanip>
#include <string>
#include <cassert>

static std::string stateToString(LogicState s) {
    switch (s) {
        case LogicState::ZERO: return "0";
        case LogicState::ONE:  return "1";
        case LogicState::X:    return "x";
        case LogicState::Z:    return "z";
        default:               return "?";
    }
}

// Struct representing a row in the Verilog truth table
struct TruthTableRow {
    LogicState a;
    LogicState b;
    LogicState eq;      // a == b
    LogicState case_eq; // a === b
    LogicState ne;      // a != b
    LogicState case_ne; // a !== b
    LogicState bit_and; // a & b
    LogicState log_and; // a && b
    LogicState bit_or;  // a | b
    LogicState log_or;  // a || b
    LogicState bit_xor; // a ^ b
};

void runTruthTableVerification() {
    using LS = LogicState;

    // The complete 16-row IEEE Verilog 4-state truth table from the specification:
    const std::vector<TruthTableRow> expected_table = {
        //  a      b      a==b   a===b  a!=b   a!==b  a&b    a&&b   a|b    a||b   a^b
        { LS::ZERO, LS::ZERO, LS::ONE,  LS::ONE,  LS::ZERO, LS::ZERO, LS::ZERO, LS::ZERO, LS::ZERO, LS::ZERO, LS::ZERO },
        { LS::ZERO, LS::ONE,  LS::ZERO, LS::ZERO, LS::ONE,  LS::ONE,  LS::ZERO, LS::ZERO, LS::ONE,  LS::ONE,  LS::ONE  },
        { LS::ZERO, LS::X,    LS::X,    LS::ZERO, LS::X,    LS::ONE,  LS::ZERO, LS::ZERO, LS::X,    LS::X,    LS::X    },
        { LS::ZERO, LS::Z,    LS::X,    LS::ZERO, LS::X,    LS::ONE,  LS::ZERO, LS::ZERO, LS::X,    LS::X,    LS::X    },

        { LS::ONE,  LS::ZERO, LS::ZERO, LS::ZERO, LS::ONE,  LS::ONE,  LS::ZERO, LS::ZERO, LS::ONE,  LS::ONE,  LS::ONE  },
        { LS::ONE,  LS::ONE,  LS::ONE,  LS::ONE,  LS::ZERO, LS::ZERO, LS::ONE,  LS::ONE,  LS::ONE,  LS::ONE,  LS::ZERO },
        { LS::ONE,  LS::X,    LS::X,    LS::ZERO, LS::X,    LS::ONE,  LS::X,    LS::X,    LS::ONE,  LS::ONE,  LS::X    },
        { LS::ONE,  LS::Z,    LS::X,    LS::ZERO, LS::X,    LS::ONE,  LS::X,    LS::X,    LS::ONE,  LS::ONE,  LS::X    },

        { LS::X,    LS::ZERO, LS::X,    LS::ZERO, LS::X,    LS::ONE,  LS::ZERO, LS::ZERO, LS::X,    LS::X,    LS::X    },
        { LS::X,    LS::ONE,  LS::X,    LS::ZERO, LS::X,    LS::ONE,  LS::X,    LS::X,    LS::ONE,  LS::ONE,  LS::X    },
        { LS::X,    LS::X,    LS::X,    LS::ONE,  LS::X,    LS::ZERO, LS::X,    LS::X,    LS::X,    LS::X,    LS::X    },
        { LS::X,    LS::Z,    LS::X,    LS::ZERO, LS::X,    LS::ONE,  LS::X,    LS::X,    LS::X,    LS::X,    LS::X    },

        { LS::Z,    LS::ZERO, LS::X,    LS::ZERO, LS::X,    LS::ONE,  LS::ZERO, LS::ZERO, LS::X,    LS::X,    LS::X    },
        { LS::Z,    LS::ONE,  LS::X,    LS::ZERO, LS::X,    LS::ONE,  LS::X,    LS::X,    LS::ONE,  LS::ONE,  LS::X    },
        { LS::Z,    LS::X,    LS::X,    LS::ZERO, LS::X,    LS::ONE,  LS::X,    LS::X,    LS::X,    LS::X,    LS::X    },
        { LS::Z,    LS::Z,    LS::X,    LS::ONE,  LS::X,    LS::ZERO, LS::X,    LS::X,    LS::X,    LS::X,    LS::X    }
    };

    std::cout << "=== Verifying 4-State Logic Against Truth Table ===\n\n";
    std::cout << std::left
              << std::setw(5)  << "a b"
              << std::setw(8)  << "a==b"
              << std::setw(8)  << "a===b"
              << std::setw(8)  << "a!=b"
              << std::setw(8)  << "a!==b"
              << std::setw(7)  << "a&b"
              << std::setw(8)  << "a&&b"
              << std::setw(7)  << "a|b"
              << std::setw(8)  << "a||b"
              << std::setw(7)  << "a^b"
              << "Status\n";
    std::cout << std::string(75, '-') << "\n";

    bool all_passed = true;

    for (const auto& row : expected_table) {
        Wire wire_a{ "a", row.a, {} };
        Wire wire_b{ "b", row.b, {} };

        Gate gate_eq      { GateType::EQ,          1, { &wire_a, &wire_b }, nullptr };
        Gate gate_case_eq { GateType::CASE_EQ,     1, { &wire_a, &wire_b }, nullptr };
        Gate gate_ne      { GateType::NE,          1, { &wire_a, &wire_b }, nullptr };
        Gate gate_case_ne { GateType::CASE_NE,     1, { &wire_a, &wire_b }, nullptr };
        Gate gate_bit_and { GateType::AND,         1, { &wire_a, &wire_b }, nullptr };
        Gate gate_log_and { GateType::LOGICAL_AND, 1, { &wire_a, &wire_b }, nullptr };
        Gate gate_bit_or  { GateType::OR,          1, { &wire_a, &wire_b }, nullptr };
        Gate gate_log_or  { GateType::LOGICAL_OR,  1, { &wire_a, &wire_b }, nullptr };
        Gate gate_bit_xor { GateType::XOR,         1, { &wire_a, &wire_b }, nullptr };

        LogicState res_eq      = gate_eq.evaluate();
        LogicState res_case_eq = gate_case_eq.evaluate();
        LogicState res_ne      = gate_ne.evaluate();
        LogicState res_case_ne = gate_case_ne.evaluate();
        LogicState res_bit_and = gate_bit_and.evaluate();
        LogicState res_log_and = gate_log_and.evaluate();
        LogicState res_bit_or  = gate_bit_or.evaluate();
        LogicState res_log_or  = gate_log_or.evaluate();
        LogicState res_bit_xor = gate_bit_xor.evaluate();

        bool match = (res_eq      == row.eq)      &&
                     (res_case_eq == row.case_eq) &&
                     (res_ne      == row.ne)      &&
                     (res_case_ne == row.case_ne) &&
                     (res_bit_and == row.bit_and) &&
                     (res_log_and == row.log_and) &&
                     (res_bit_or  == row.bit_or)  &&
                     (res_log_or  == row.log_or)  &&
                     (res_bit_xor == row.bit_xor);

        if (!match) all_passed = false;

        std::string pair = stateToString(row.a) + " " + stateToString(row.b);
        std::cout << std::left
                  << std::setw(5)  << pair
                  << std::setw(8)  << stateToString(res_eq)
                  << std::setw(8)  << stateToString(res_case_eq)
                  << std::setw(8)  << stateToString(res_ne)
                  << std::setw(8)  << stateToString(res_case_ne)
                  << std::setw(7)  << stateToString(res_bit_and)
                  << std::setw(8)  << stateToString(res_log_and)
                  << std::setw(7)  << stateToString(res_bit_or)
                  << std::setw(8)  << stateToString(res_log_or)
                  << std::setw(7)  << stateToString(res_bit_xor)
                  << (match ? "[OK]" : "[FAIL]") << "\n";
    }

    std::cout << std::string(75, '-') << "\n";
    if (all_passed) {
        std::cout << "SUCCESS: All 16 truth table rows verified perfectly! ✅\n\n";
    } else {
        std::cout << "FAILURE: One or more truth table rows mismatched! ❌\n\n";
    }
}

int main() {
    runTruthTableVerification();
    return 0;
}
