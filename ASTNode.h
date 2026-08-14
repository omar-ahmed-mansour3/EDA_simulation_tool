//
// Created by omara on 8/5/2026.
//

#ifndef EDA_PROJECT_ASTNODE_H
#define EDA_PROJECT_ASTNODE_H

#include <vector>
#include "Tokenizer.h"
using namespace std;
class ASTNode {

    public:
        Token token;
        ASTNode* left ;
        ASTNode* right ;
        string outputWire;
        ASTNode(Token tkn, ASTNode* l, ASTNode* r);
        ASTNode(Token tkn);
        bool isLeaf();
        static bool isOperator(const Token& token);
        static int precedence(const Token& token);
        static bool isUnary(const Token& token);
        static ASTNode* buildAST(const vector<Token>& tokens);

};
#endif //EDA_PROJECT_ASTNODE_H