#pragma once

#include "Token.h"
#include "TokenType.h"
#include "TokenUtils.h"

class Validator {
    using Indices = std::stack<size_t>;

public:
    Validator(const TokenVec &tokens)
    : m_tokens(tokens) {}

    void pushBinaryOp() {
        m_binaryOps.push(m_tokens.size() - 1);
    }

    void pushSingleUnaryOp() {
        m_sUnaryOps.push(m_tokens.size() - 1);
    }

    void pushDoubleUnaryOp() {
        m_dUnaryOps.push(m_tokens.size() - 1);
    }

    void pushAssignmentOp() {
        m_assOps.push(m_tokens.size() - 1);
    }

    void finalCheck() {
        // Backtrack checking

        // Check if op is surounded by value "1 + 1"
        checkOps(m_binaryOps, [&](TokenType first, TokenType last) {
            return TokenUtils::isValue(first) && TokenUtils::isValue(last);
        });

        // Checkk if op is followed by value "+1"
        checkOps(m_sUnaryOps, [&](TokenType, TokenType next) {
            return TokenUtils::isValue(next);
        });

        // Check if identifier is preceeded or proceeded by op "++x or x++"
        checkOps(m_dUnaryOps, [&](TokenType first, TokenType last) {
            return TokenUtils::isIdentifier(first) || TokenUtils::isIdentifier(last);
        });

        // Check if if op is surrounded by identifier and value "x = 1"
        checkOps(m_assOps, [&](TokenType first, TokenType last) {
            return TokenUtils::isIdentifier(first) && TokenUtils::isValue(last);
        });
    }

private:
    const TokenVec &m_tokens;
    Indices m_binaryOps, m_sUnaryOps, m_dUnaryOps, m_assOps;

    void checkOps(Indices &stack, const std::function<bool(TokenType first, TokenType last)> &condition) {
        auto getToken = [&](size_t index) {
            return m_tokens[index];
        };

        while (!stack.empty()) {
            size_t index = stack.top();
            auto current = getToken(index);

            if (index - 1 < 0 || index + 1 >= m_tokens.size()) {
                current->setType(UNKNOWN);
                stack.pop();
                continue;
            }

            auto first = getToken(index - 1);
            auto last = getToken(index + 1);

            if (!condition(first->type(), last->type())) {
                current->setType(UNKNOWN);
            }
            stack.pop();
        }
    }
};
