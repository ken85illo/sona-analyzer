#pragma once

#include "Error.h"
#include "Expr.h"
#include "Token.h"

class Parser {
public:
    class ParseError : public std::runtime_error {
        using std::runtime_error::runtime_error;
    };

    Parser(const TokenVec &tokens)
    : m_tokens(tokens) {}

    Ref<Expr> parse() {
        try {
            return expression();
        }
        catch (ParseError error) {
            return nullptr;
        }
    }

private:
    const TokenVec &m_tokens;
    size_t current = 0;

    Ref<Expr> expression() {
        return equality();
    }

    Ref<Expr> equality() {
        Ref<Expr> expr = comparison();

        while (match(NOT_EQUAL_REL_OP, EQUAL_REL_OP)) {
            Ref<Token> op = previous();
            Ref<Expr> right = comparison();
            expr = MakeRef<BinaryExpr>(expr, op, right);
        }
        return expr;
    }

    Ref<Expr> comparison() {
        Ref<Expr> expr = term();

        while (match(GREATER_REL_OP, GREATER_EQUAL_REL_OP, LESS_REL_OP, LESS_EQUAL_REL_OP)) {
            Ref<Token> op = previous();
            Ref<Expr> right = term();
            expr = MakeRef<BinaryExpr>(expr, op, right);
        }

        return expr;
    }

    Ref<Expr> term() {
        Ref<Expr> expr = factor();

        while (match(SUBTRACT_OP, ADD_OP)) {
            Ref<Token> op = previous();
            Ref<Expr> right = factor();

            expr = MakeRef<BinaryExpr>(expr, op, right);
        }

        return expr;
    }

    Ref<Expr> factor() {
        Ref<Expr> expr = unary();

        while (match(DIVIDE_OP, MULTIPLY_OP)) {
            Ref<Token> op = previous();
            Ref<Expr> right = unary();
            expr = MakeRef<BinaryExpr>(expr, op, right);
        }

        return expr;
    }

    Ref<Expr> unary() {
        if (match(NOT_LOG_OP, NEGATIVE_OP, POSITIVE_OP)) {
            Ref<Token> op = previous();
            Ref<Expr> right = unary();
            return MakeRef<UnaryExpr>(op, right);
        }

        return primary();
    }

    Ref<Expr> primary() {
        if (match(FALSE_LITERAL)) {
            return MakeRef<LiteralExpr>(MakeRef<std::string>("false"));
        }

        if (match(TRUE_LITERAL)) {
            return MakeRef<LiteralExpr>(MakeRef<std::string>("true"));
        }

        if (match(INT_LITERAL, FLT_LITERAL, STR_LITERAL)) {
            return MakeRef<LiteralExpr>(MakeRef<std::string>(previous()->lexeme()));
        }

        if (match(LEFT_PAREN_DELIM)) {
            Ref<Expr> expr = expression();
            consume(RIGHT_PAREN_DELIM, "Expect ')' after expression.");
            return MakeRef<GroupingExpr>(expr);
        }

        throw error(peek(), "Expect expression.");
    }

    Ref<Token> consume(TokenType type, const std::string &message) {
        if (check(type)) {
            return advance();
        }

        throw error(peek(), message);
    }

    ParseError error(Ref<Token> token, const std::string &message) {
        ::error(token, message);
        return ParseError("");
    }

    template <typename... TokenType>
    bool match(TokenType... types) {
        return (tryMatch(types) || ...);
    }

    bool tryMatch(TokenType type) {
        if (check(type)) {
            advance();
            return true;
        }
        return false;
    }

    bool check(TokenType type) {
        if (isAtEnd()) {
            return false;
        }
        return peek()->type() == type;
    }

    Ref<Token> advance() {
        if (!isAtEnd()) {
            current++;
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
