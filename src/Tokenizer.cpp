//
// Created by omara on 8/5/2026.
//
#include "Tokenizer.h"
#include <stdexcept>

vector<Token> Tokenizer::tokenize(const string& verilog_code)
{
    vector<Token> tokens;
    int line = 1;
    int col = 1;
    int i = 0;

    while (i < verilog_code.size())
    {
        char c = verilog_code[i];

        // White space
        if (c == ' ' || c == '\t' || c == '\n') {
            if (c == '\n') {
                line++;
                col = 1;
            }
            else if (c == '\t') {
                col = ((col - 1) / 4 + 1) * 4 + 1;
            }
            else {
                col++;
            }

            i++;
            continue;
        }

        // Line comments
        if (c == '/' && (i + 1 < verilog_code.size() && verilog_code[i + 1] == '/') ) {
            while (i < verilog_code.size() && verilog_code[i] != '\n') {
                i++;
            }
            continue;
        }

        // Block comments
        if (c == '/' && i + 1 < verilog_code.size() && verilog_code[i + 1] == '*') {

            i += 2;
            col += 2;

            while (i < verilog_code.size() && !(verilog_code[i] == '*' && i + 1 < verilog_code.size() &&
                verilog_code[i + 1] == '/')) {

                if (verilog_code[i] == '\n') {
                    line++;
                    col = 1;
                }
                else if (verilog_code[i] == '\t') {
                    col = ((col - 1) / 4 + 1) * 4 + 1;
                }
                else {
                    col++;
                }
                i++;
            }

            // skip */
            i += 2;
            col += 2;

            // check if file is ended
            continue;
        }

        int startLine = line;
        int startCol = col;

        // Identifier / Keyword

        if (isAlphA(c) || c == '_') {
            string text;

            while (i < verilog_code.size()) {
                char ch = verilog_code[i];

                if ( isAlphA(ch) || isDigit(ch) || ch == '_') {
                    text += ch;
                    i++;
                    col++;
                }
                else {
                    break;
                }
            }

            Token tok;
            tok.text = text;
            tok.line = startLine;
            tok.column = startCol;

            if (text == "module" || text == "endmodule" || text == "input" || text == "output" ||
                text == "wire" || text == "assign" || text == "and" || text == "or" || text == "not" ||
                text == "nand" || text == "nor" || text == "xor" || text == "xnor" || text == "buf") {

                tok.type = TokenType::Keyword;
            }
            else {
                tok.type = TokenType::Identifier;
            }

            tokens.push_back(tok);

            // check if file ended
            continue;
        }

        // Number
        if (isDigit(c)) {
            string text;

            while (i < verilog_code.size() && isDigit(verilog_code[i]) ) {
                text += verilog_code[i];
                i++;
                col++;
            }

            Token tok;
            tok.type = TokenType::Number;
            tok.text = text;
            tok.line = startLine;
            tok.column = startCol;

            tokens.push_back(tok);
            continue;
        }

        // Punctuation / Operators
        Token tok;
        tok.line = startLine;
        tok.column = startCol;

        // Two-character operators
        if (i + 1 < verilog_code.size()) {
            string op = verilog_code.substr(i, 2);
            // logical
            if (op == "&&" || op == "||" || op == "==" || op == "!=" ) {
                tok.type = TokenType::Operator;
                tok.text = op;

                tokens.push_back(tok);

                i += 2;
                col += 2;
                continue;
            }
        }

        // Single character operators
        if (c == '(') {
            tok.type = TokenType::LParen;
            tok.text = "(";
        }
        else if (c == ')') {
            tok.type = TokenType::RParen;
            tok.text = ")";
        }
        else if (c == ',') {
            tok.type = TokenType::Comma;
            tok.text = ",";
        }
        else if (c == ';') {
            tok.type = TokenType::Semicolon;
            tok.text = ";";
        }
        else if (c == '#') {
            tok.type = TokenType::Hash;
            tok.text = "#";
        }

        // Single-character operators
        else if (c == '=' || c == '&' || c == '|' || c == '^' || c == '~' || c == '!') {
            tok.type = TokenType::Operator;
            tok.text = string(1, c);
        }
        else {
            throw runtime_error(
                "Unexpected character '" + string(1, c) + "' at line " +
                to_string(line) + ", column " + to_string(col) );
        }

        // Token for Single character operators
        tokens.push_back(tok);
        i++;
        col++;

    }

    Token eofTok;
    eofTok.type = TokenType::EndOfFile;
    eofTok.text = "";
    eofTok.line = line;
    eofTok.column = col;

    tokens.push_back(eofTok);

    return tokens;
}


bool Tokenizer::isAlphA(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

bool Tokenizer::isDigit(char c) {
    return (c >= '0' && c <= '9');
}