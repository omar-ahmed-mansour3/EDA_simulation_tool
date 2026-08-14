//
// Created by omara on 8/4/2026.
//
#include "Parser.hpp"
#include "ASTNode.h"
#include "FileReader.h"
#include "Tokenizer.h"
#include <string>
#include <stdexcept>
#include <iostream>
using namespace std;

ParsedModule VerilogParser::parseFile(const string& filepath) {
    string content = FileReader::readFile(filepath);
    return parseString(content);
}

ParsedModule VerilogParser::parseString(const string& verilog_code) {
    vector<Token> tokens = Tokenizer::tokenize(verilog_code);
    return parseFromTokens(tokens);
}

ParsedModule VerilogParser::parseFromTokens(const vector<Token>& tokens) {
    ParsedModule module;

    int current = 0;

    // Make sure that the file starts with "module"
    if (tokens[current].type != TokenType::Keyword || tokens[current].text != "module") {
        parserError(tokens[current],"Expected 'module'");
    }

    current++;

    if (tokens[current].type != TokenType::Identifier) {
        parserError(tokens[current],"Expected module name");
    }

    module.module_name = tokens[current].text;
    current++;

    // Expect '('
    if (tokens[current].type != TokenType::LParen) {
        parserError(tokens[current],"Expected '(' after module name");
    }

    current++;

    // Parse ANSI port declarations
    while (tokens[current].type != TokenType::RParen) {

        if (tokens[current].type != TokenType::Keyword)
            parserError(tokens[current],"Expected 'input' or 'output'");

        bool isInput;

        if (tokens[current].text == "input")
            isInput = true;
        else if (tokens[current].text == "output")
            isInput = false;
        else
            parserError(tokens[current],"Only input/output are allowed in module header");

        current++;

        // Optional wire keyword
        if (tokens[current].type == TokenType::Keyword && tokens[current].text == "wire") {
            current++;
        }

        while (true) {
            if (tokens[current].type != TokenType::Identifier)
                parserError(tokens[current],"Expected port name");

            if (isInput)
                module.input_pins.push_back(tokens[current].text);
            else
                module.output_pins.push_back(tokens[current].text);

            current++;

            if (tokens[current].type == TokenType::Comma)
            {
                current++;

                // Next declaration?
                if (tokens[current].type == TokenType::Keyword)
                    break;

                continue;
            }

            break;
        }
    }

    // Consume ')'
    current++;

    if (tokens[current].type != TokenType::Semicolon) {
        parserError(tokens[current],"Expected ';' after module header");
    }

    current++;

    int wireCounter = 0;
    int gateCounter = 0;
    // Continue parsing until endmodule
    while (current < tokens.size()) {
        Token token = tokens[current];

        if (token.type == TokenType::Keyword) {
            ///////////////////////////////////////////////////////////////////////////////////////
            bool isGate =
                    (token.text == "and"  || token.text == "or"   || token.text == "not"  ||
                     token.text == "nand" || token.text == "nor"  || token.text == "xor"  ||
                     token.text == "xnor" || token.text == "buf");
            ///////////////////////////////////////////////////////////////////////////////////////

            if (token.text == "endmodule") {
                return module;
            }
            if (token.text == "wire") {
                current++;

                while (true)
                {
                    if (tokens[current].type != TokenType::Identifier)
                        parserError(tokens[current],"Expected wire identifier");


                    module.internal_wires.push_back(tokens[current].text);
                    current++;

                    if (tokens[current].type == TokenType::Comma)
                    {
                        current++;
                        continue;
                    }

                    if (tokens[current].type == TokenType::Semicolon)
                    {
                        current++;
                        break;
                    }
                    parserError(tokens[current],"Expected ',' or ';' after wire declaration");
                }
            }
            else if (token.text == "assign") {
                current++;

                // LHS
                if (tokens[current].type != TokenType::Identifier)
                    parserError(tokens[current],"Expected identifier after 'assign'");


                Token lhs = tokens[current];
                current++;

                // =
                if (tokens[current].type != TokenType::Operator || tokens[current].text != "=") {
                    parserError(tokens[current],"Expected '=' in assign statement");
                }

                current++;

                // Collect RHS tokens until ';'
                vector<Token> expression;

                while (tokens[current].type != TokenType::Semicolon)
                {
                    if (tokens[current].type == TokenType::EndOfFile)
                        parserError(tokens[current],"Missing ';' after assign statement");

                    expression.push_back(tokens[current]);
                    current++;
                }

                // Build AST of the RHS
                ASTNode* rhs = ASTNode::buildAST(expression);

                // Build assign node
                Token assignToken;
                assignToken.type = TokenType::Keyword;
                assignToken.text = "assign";

                ASTNode* lhsNode = new ASTNode(lhs);
                ASTNode* assignRoot = new ASTNode(assignToken, lhsNode, rhs);

                // Generate primitive gates
                evaluate(assignRoot,module.gates,module.internal_wires,wireCounter,gateCounter);

                current++;
            }

            else if (isGate) {
                ParsedGate gate;

                gate.type = stringToGateType(tokens[current].text);

                //default delay
                gate.delay = 1;

                current++;

                // Optional delay
                if (tokens[current].type == TokenType::Hash) {
                    current++;

                    if (tokens[current].type != TokenType::LParen)
                        parserError(tokens[current],"Expected '(' after '#'");

                    current++;

                    if (tokens[current].type != TokenType::Number)
                        parserError(tokens[current],"Expected delay number");

                    gate.delay = stoi(tokens[current].text);

                    current++;

                    if (tokens[current].type != TokenType::RParen)
                        parserError(tokens[current],"Expected ')' after delay");

                    current++;
                }

                if (tokens[current].type != TokenType::Identifier)
                    parserError(tokens[current],"Expected gate instance name");

                gate.instance_name = tokens[current].text;
                current++;

                // (
                if (tokens[current].type != TokenType::LParen)
                    parserError(tokens[current],"Expected '(' after gate instance name");

                current++;

                // Output
                if (tokens[current].type != TokenType::Identifier)
                    parserError(tokens[current],"Expected output signal");

                gate.output = tokens[current].text;
                current++;

                // At least one input
                if (tokens[current].type != TokenType::Comma)
                    parserError(tokens[current],"Expected ',' after output");

                current++;

                if (tokens[current].type != TokenType::Identifier)
                    parserError(tokens[current],"Expected input signal");

                while (true) {
                    if (tokens[current].type != TokenType::Identifier)
                        parserError(tokens[current],"Expected input signal");

                    gate.inputs.push_back(tokens[current].text);
                    current++;

                    if (tokens[current].type == TokenType::Comma)
                    {
                        current++;
                        continue;
                    }

                    if (tokens[current].type == TokenType::RParen)
                        break;

                    parserError(tokens[current],"Expected ',' or ')'");
                }
                current++;

                if (tokens[current].type != TokenType::Semicolon)
                    parserError(tokens[current],"Expected ';' after gate instantiation");

                current++;

                module.gates.push_back(gate);

                }
            else
                parserError(tokens[current],"Unknown Keyword: " + tokens[current].text);
            }

        }

         throw runtime_error("Parser error: Missing 'endmodule'");
    }

void VerilogParser::evaluate(ASTNode* node, vector<ParsedGate>& gates, vector<string>& wires, int& wireCounter, int& gateCounter) {
    if (node == nullptr)
        return;

    // assign
    if (node->token.text == "assign")
    {
        node->right->outputWire = node->left->token.text;
        evaluate(node->right, gates, wires, wireCounter, gateCounter);
        return;
    }

    // leaf
    if (node->isLeaf())
    {
        node->outputWire = node->token.text;
        return;
    }

    // -------- Flatten associative gates --------
    if (node->token.text == "&" ||
        node->token.text == "|" ||
        node->token.text == "^")
    {
        vector<ASTNode*> operands;
        collectOperands(node, node->token.text, operands);

        ParsedGate gate;
        gate.type = stringToGateType(node->token.text);

        string gateName;
        switch (gate.type)
        {
            case GateType::AND:  gateName = "and"; break;
            case GateType::OR:   gateName = "or";  break;
            case GateType::XOR:  gateName = "xor"; break;
            default: break;
        }

        gate.instance_name = gateName + to_string(gateCounter++);
        gate.delay = 1;

        // Evaluate every operand
        for (ASTNode* operand : operands)
        {
            evaluate(operand, gates, wires, wireCounter, gateCounter);
            gate.inputs.push_back(operand->outputWire);
        }

        // Output wire
        if (node->outputWire.empty())
        {
            node->outputWire = "_net_" + to_string(wireCounter++);
            wires.push_back(node->outputWire);
        }

        gate.output = node->outputWire;
        gates.push_back(gate);

        return;
    }

    // -------- Normal binary evaluation --------

    evaluate(node->left, gates, wires, wireCounter, gateCounter);
    evaluate(node->right, gates, wires, wireCounter, gateCounter);

    ParsedGate gate;
    gate.type = stringToGateType(node->token.text);
    gate.delay = 1;

    string gateName;

    switch (gate.type)
    {
        case GateType::AND:  gateName = "and"; break;
        case GateType::OR:   gateName = "or"; break;
        case GateType::NOT:  gateName = "not"; break;
        case GateType::NAND: gateName = "nand"; break;
        case GateType::NOR:  gateName = "nor"; break;
        case GateType::XOR:  gateName = "xor"; break;
        case GateType::XNOR: gateName = "xnor"; break;
        case GateType::BUF:  gateName = "buf"; break;
        default:             gateName = "gate"; break;
    }

    gate.instance_name = gateName + to_string(gateCounter++);
    gate.delay = 1;

    if (node->outputWire.empty())
    {
        node->outputWire = "_net_" + to_string(wireCounter++);
        wires.push_back(node->outputWire);
    }

    gate.output = node->outputWire;

    if (node->token.text == "~")
    {
        gate.inputs.push_back(node->right->outputWire);
    }
    else
    {
        gate.inputs.push_back(node->left->outputWire);
        gate.inputs.push_back(node->right->outputWire);
    }

    gates.push_back(gate);
}

GateType VerilogParser::stringToGateType(const string& gate)
{
    if (gate == "&" || gate == "and")
        return GateType::AND;

    if (gate == "|" || gate == "or")
        return GateType::OR;

    if (gate == "~" || gate == "not")
        return GateType::NOT;

    if (gate == "^" || gate == "xor" || gate == "!=")
        return GateType::XOR;

    if (gate == "~^" || gate == "^~" || gate == "xnor" || gate == "==")
        return GateType::XNOR;

    if (gate == "nand")
        return GateType::NAND;

    if (gate == "nor")
        return GateType::NOR;

    if (gate == "buf")
        return GateType::BUF;

    throw runtime_error("Unknown gate type: " + gate);
}

void VerilogParser::collectOperands(ASTNode* node, const string& op, vector<ASTNode*>& operands) {
    if (node->token.text == op) {
        collectOperands(node->left, op, operands);
        collectOperands(node->right, op, operands);
    }
    else {
        operands.push_back(node);
    }
}

void VerilogParser::parserError(const Token& token, const string& message) {
    throw runtime_error( "Parser error at line " + to_string(token.line) +
    ", column " + to_string(token.column) +": " +message);
}