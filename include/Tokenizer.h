//
// Created by omara on 8/5/2026.
//
#pragma once
#ifndef EDA_PROJECT_TOKENIZER_H
#define EDA_PROJECT_TOKENIZER_H

#include <string>
#include <vector>
using namespace std;

enum class TokenType {
    Keyword,
    Identifier,
    Number,
    Operator,
    LParen,
    RParen,
    Comma,
    Semicolon,
    Hash,
    EndOfFile
};


struct Token
{
    TokenType type;
    string text;
    int line;
    int column;
};

class Tokenizer {
    public:
        static vector<Token> tokenize(const string& verilog_code);

    private:
        static bool isDigit(char c);
        static bool isAlphA(char c);
};
#endif //EDA_PROJECT_TOKENIZER_H