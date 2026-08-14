//
// Created by omara on 8/5/2026.
//
#include "ASTNode.h"
#include <stack>
#include <stdexcept>

ASTNode::ASTNode(Token tkn, ASTNode* l, ASTNode* r) {
    this->token = tkn;
    this->left = l;
    this->right = r;
}

ASTNode::ASTNode(Token tkn) {
    this->token = tkn;
    this->left = nullptr;
    this->right = nullptr;
}

bool ASTNode::isLeaf() {
    return (left == nullptr && right == nullptr);
}

bool ASTNode::isOperator(const Token& token) {
    return token.type == TokenType::Operator;
}

int ASTNode::precedence(const Token& token) {
    if (token.text == "~")
        return 7;

    if (token.text == "&")
        return 6;

    if (token.text == "^" || token.text == "~^" || token.text == "^~")
        return 5;

    if (token.text == "|")
        return 4;

    if (token.text == "==" || token.text == "!=")
        return 3;

    if (token.text == "&&")
        return 2;

    if (token.text == "||")
        return 1;

    return 0;
}

bool ASTNode::isUnary(const Token& token) {
    return token.text == "~";
}

ASTNode* ASTNode::buildAST(const vector<Token>& tokens) {
    stack<Token> operators;
    stack<ASTNode*> nodes;

    for (const Token& token : tokens)
    {
        // Operand
        if (token.type == TokenType::Identifier ||
            token.type == TokenType::Number)
        {
            nodes.push(new ASTNode(token));
        }

        // (
        else if (token.type == TokenType::LParen)
        {
            operators.push(token);
        }

        // )
        else if (token.type == TokenType::RParen)
        {
            while (!operators.empty() &&
                   operators.top().type != TokenType::LParen)
            {
                Token op = operators.top();
                operators.pop();

                ASTNode* node = new ASTNode(op);

                if (isUnary(op))
                {
                    node->right = nodes.top();
                    nodes.pop();
                }
                else
                {
                    ASTNode* right = nodes.top();
                    nodes.pop();

                    ASTNode* left = nodes.top();
                    nodes.pop();

                    node->left = left;
                    node->right = right;
                }

                nodes.push(node);
            }

            if (operators.empty())
                throw runtime_error("Missing '('");

            operators.pop();
        }

        // Operator
        else if (isOperator(token))
        {
            while (!operators.empty() &&
                   operators.top().type != TokenType::LParen &&
                   precedence(operators.top()) >= precedence(token))
            {
                Token op = operators.top();
                operators.pop();

                ASTNode* node = new ASTNode(op);

                if (isUnary(op))
                {
                    node->right = nodes.top();
                    nodes.pop();
                }
                else
                {
                    ASTNode* right = nodes.top();
                    nodes.pop();

                    ASTNode* left = nodes.top();
                    nodes.pop();

                    node->left = left;
                    node->right = right;
                }

                nodes.push(node);
            }

            operators.push(token);
        }
    }

    while (!operators.empty())
    {
        if (operators.top().type == TokenType::LParen)
            throw runtime_error("Missing ')'");

        Token op = operators.top();
        operators.pop();

        ASTNode* node = new ASTNode(op);

        if (isUnary(op))
        {
            node->right = nodes.top();
            nodes.pop();
        }
        else
        {
            ASTNode* right = nodes.top();
            nodes.pop();

            ASTNode* left = nodes.top();
            nodes.pop();

            node->left = left;
            node->right = right;
        }

        nodes.push(node);
    }

    if (nodes.size() != 1)
        throw runtime_error("Invalid expression");

    return nodes.top();
}
