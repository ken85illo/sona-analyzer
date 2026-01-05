#pragma once

#include "Error.h"
#include "Expr.h"
#include "ParseResult.h"
#include "Token.h"

class Parser {
public:
    class ParseError : public std::runtime_error {
        using std::runtime_error::runtime_error;
    };

    Parser(const TokenVec &tokens)
    : m_tokens(tokens) {}

    CST parse() {
        try {
            return expression();
        }
        catch (ParseError) {
            synchronize();
            return nullptr;
        }
    }

private:
    const TokenVec &m_tokens;
    size_t current = 0;

    /* ================= Grammar ================= */

    // General Expression
    CST expression() {
        auto node = NonTerminalNode::make("EXPRESSION");
        node->add(logicalLevel());
        return node;
    }

    // Expression Precedence: Parentheses > Arithmetic > Relational > Logical
    CST logicalLevel() {
        auto node = NonTerminalNode::make("LOGICAL_LEVEL");
        node->add(relLevel());

        while (auto op = logOp()) {
            node->add(op);
            node->add(relLevel());
        }
        return node;
    }

    CST relLevel() {
        auto node = NonTerminalNode::make("REL_LEVEL");
        node->add(arithLevel());

        while (auto op = relOp()) {
            node->add(op);
            node->add(arithLevel());
        }
        return node;
    }

    CST arithLevel() {
        auto node = NonTerminalNode::make("ARITH_LEVEL");
        node->add(term());

        while (auto op = addOp()) {
            node->add(op);
            node->add(term());
        }
        return node;
    }

    CST term() {
        auto node = NonTerminalNode::make("TERM");
        node->add(factor());

        while (auto op = multOp()) {
            node->add(op);
            node->add(factor());
        }
        return node;
    }

    CST factor() {
        auto node = NonTerminalNode::make("FACTOR");
        if (auto op = operand()) {
            node->add(op);
        }
        else if (auto leftParen = match(LEFT_PAREN_DELIM)) {
            node->add(TerminalNode::make(leftParen));
            node->add(expression());
            auto rightParen = consume(RIGHT_PAREN_DELIM, "Expect ')' after expression ");
            node->add(TerminalNode::make(rightParen));
        }

        return node;
    }

    // Operators
    CST logOp() {
        if (auto op = match(AND_LOG_OP, OR_LOG_OP)) {
            auto node = NonTerminalNode::make("LOG_OP");
            node->add(TerminalNode::make(op));
            return node;
        }
        return nullptr;
    }

    CST relOp() {
        if (auto op = match(
                EQUAL_REL_OP, NOT_EQUAL_REL_OP, LESS_REL_OP, GREATER_REL_OP, LESS_EQUAL_REL_OP, GREATER_EQUAL_REL_OP
            )) {
            auto node = NonTerminalNode::make("REL_OP");
            node->add(TerminalNode::make(op));
            return node;
        }
        return nullptr;
    }

    CST addOp() {
        if (auto op = match(ADD_OP, SUBTRACT_OP)) {
            auto node = NonTerminalNode::make("ADD_OP");
            node->add(TerminalNode::make(op));
            return node;
        }
        return nullptr;
    }

    CST multOp() {
        if (auto op = match(MULTIPLY_OP, DIVIDE_OP, MODULO_OP)) {
            auto node = NonTerminalNode::make("MULT_OP");
            node->add(TerminalNode::make(op));
            return node;
        }
        return nullptr;
    }

    // Expression operands
    CST operand() {
        auto node = NonTerminalNode::make("OPERAND");
        if (auto op = number()) {
            node->add(op);
            return node;
        }
        else if (auto op = realNum()) {
            node->add(op);
            return node;
        }
        else if (auto op = boolG()) {
            node->add(op);
            return node;
        }
        throw error(peek(), "Expect NUMBER, REAL_NUMBER, or BOOL literals inside expression");

        return nullptr;
    }

    // Construccting Numbers
    CST number() {
        auto node = NonTerminalNode::make("NUMBER");
        if (auto op = match(INT_LITERAL)) {
            auto test = TerminalNode::make(op);
            node->add(test);
            return node;
        }
        else if (auto op = match(POSITIVE_OP)) {
            node->add(TerminalNode::make(op));
            auto operand = consume(INT_LITERAL, "Expect a number after POSITIVE_OP");
            node->add(TerminalNode::make(operand));
            return node;
        }
        else if (auto op = match(NEGATIVE_OP)) {
            node->add(TerminalNode::make(op));
            auto operand = consume(INT_LITERAL, "Expect a number after NEGATIVE_OP");
            node->add(TerminalNode::make(operand));
            return node;
        }
        return nullptr;
    }

    CST realNum() {
        auto node = NonTerminalNode::make("REAL_NUM");
        if (auto op = match(FLT_LITERAL)) {
            node->add(TerminalNode::make(op));
            return node;
        }
        else if (auto op = match(POSITIVE_OP)) {
            node->add(TerminalNode::make(op));
            auto operand = consume(FLT_LITERAL, "Expect a number after POSITIVE_OP");
            node->add(TerminalNode::make(operand));
            return node;
        }
        else if (auto op = match(NEGATIVE_OP)) {
            node->add(TerminalNode::make(op));
            auto operand = consume(FLT_LITERAL, "Expect a number after NEGATIVE_OP");
            node->add(TerminalNode::make(operand));
            return node;
        }
        return nullptr;
    }

    // Constants
    CST boolG() {
        auto node = NonTerminalNode::make("BOOL");
        if (auto op = match(TRUE_LITERAL)) {
            node->add(TerminalNode::make(op));
            return node;
        }
        else if (auto op = match(FALSE_LITERAL)) {
            node->add(TerminalNode::make(op));
            return node;
        }

        return nullptr;
    }

    /* ================= Helpers ================= */

    Ref<Token> consume(TokenType type, const std::string &message) {
        if (!isAtEnd()) {
            validateToken(peek());
        }
        if (check(type)) {
            return advance();
        }

        throw error(peek(), message);
    }

    ParseError error(Ref<Token> token, const std::string &message) {
        ::error(token, message);
        synchronize();
        return ParseError("");
    }

    template <typename... TokenType>
    Ref<Token> match(TokenType... types) {
        if (!isAtEnd()) {
            validateToken(peek());
        }
        if ((check(types) || ...)) {
            return advance();
        }
        return nullptr;
    }

    bool check(TokenType type) {
        if (isAtEnd()) {
            return false;
        }
        return peek()->type() == type;
    }

    Ref<Token> advance() {
        if (!isAtEnd()) {
            ++current;
        }
        return previous();
    }

    bool isAtEnd() {
        return current == m_tokens.size();
    }

    Ref<Token> peek() {
        return m_tokens[current];
    }

    Ref<Token> previous() {
        return m_tokens[current - 1];
    }

    void validateToken(Ref<Token> token) {
        if (token->type() != UNKNOWN) {
            return;
        }

        // Handle lexical unknown tokens
        if (token->lexeme() == "++") {
            throw error(token, "Invalid use of INCREMENT_OP");
        }
        else if (token->lexeme() == "--") {
            throw error(token, "Invalid use of DECREMENT_OP");
        }
        else if (token->lexeme() == "-") {
            throw error(token, "Invalid use of NEGATIVE_OP or SUBTRACT_OP");
        }
        else if (token->lexeme() == "+") {
            throw error(token, "Invalid use of POSITIVE_OP or ADD_OP");
        }
        else if (token->lexeme() == "*") {
            throw error(token, "Invalid use of MULTIPLY_OP");
        }
        else if (token->lexeme() == "/") {
            throw error(token, "Invalid use of DIVIDE_OP");
        }
        else if (token->lexeme() == "%") {
            throw error(token, "Invalid use of MODULO_OP");
        }
        else if (token->lexeme() == "+=") {
            throw error(token, "Invalid use of ADD_ASS_OP");
        }
        else if (token->lexeme() == "-=") {
            throw error(token, "Invalid use of SUBTRACT_ASS_OP");
        }
        else if (token->lexeme() == "*=") {
            throw error(token, "Invalid use of MULTIPLY_ASS_OP");
        }
        else if (token->lexeme() == "/=") {
            throw error(token, "Invalid use of DIVIDE_ASS_OP");
        }
        else if (token->lexeme() == "%=") {
            throw error(token, "Invalid use of MODULO_ASS_OP");
        }
        else {
            throw error(token, "Undefined identifier or keyword");
        }
    }

    void synchronize() {
        advance();

        while (!isAtEnd()) {
            if (previous()->type() == SEMICOLON_DELIM) {
                return;
            }

            switch (peek()->type()) {
            case MACHINE_TYPE_RESW:
            case STRUCT_TYPE_RESW:
            case INT_TYPE_RESW:
            case FLOAT_TYPE_RESW:
            case DOUBLE_TYPE_RESW:
            case STRING_TYPE_RESW:
            case BOOL_TYPE_RESW:
            case CHAR_TYPE_RESW:
            case VOID_TYPE_RESW:
            case STATIC_RESW:
            case UNSIGNED_RESW:
            case CONST_RESW:
            case MAC_CONTEXT_RESW:
            case MAC_FINAL_RESW:
            case MAC_FINAL_STATE_RESW:
            case MAC_START_RESW:
            case MAC_STATE_RESW:
            case MAC_STATES_RESW:
            case MAC_TRANSITIONS_RESW:
            case FOR_KEYW:
            case IF_RESW:
            case WHILE_KEYW:
            case RETURN_RESW:
                return;
                break;
            default:
                break;
            }

            advance();
        }
    }
};
