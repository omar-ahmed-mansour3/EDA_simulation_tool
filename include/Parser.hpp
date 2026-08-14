#pragma once
#include "Common.hpp"
#include "Tokenizer.h"
#include <string>
#include <vector>

class ASTNode;

enum class GateType {
    AND,
    OR,
    NOT,
    NAND,
    NOR,
    XOR,
    XNOR,
    BUF,
    EQ,          // a == b (logical equality)
    CASE_EQ,     // a === b (case equality)
    NE,          // a != b (logical inequality)
    CASE_NE,     // a !== b (case inequality)
    LOGICAL_AND, // a && b
    LOGICAL_OR   // a || b
};

struct ParsedGate {
    GateType type;
    std::string type_name;            // Primitive name (e.g., "and", "assign")
    std::string instance_name;        // Gate instance identifier (e.g., "g1")
    int delay = 1;                    // Default gate delay
    std::vector<std::string> inputs;  // Input wire names
    std::string output;               // Output wire name
};

struct ParsedModule {
    std::string module_name;
    std::vector<std::string> input_pins;
    std::vector<std::string> output_pins;
    std::vector<std::string> internal_wires;
    std::vector<ParsedGate> gates;
};

class VerilogParser {
public:
    static ParsedModule parseFile(const std::string& filepath);
    static ParsedModule parseString(const std::string& verilog_code);
    static ParsedModule parseFromTokens(const std::vector<Token>& tokens);

private:
    static void evaluate(ASTNode* node, std::vector<ParsedGate>& gates, std::vector<std::string>& wires, int& wireCounter, int& gateCounter);
    static GateType stringToGateType(const std::string& gate);
    static void collectOperands(ASTNode* node, const std::string& op, std::vector<ASTNode*>& operands);
    static void parserError(const Token& token, const std::string& message);
};
