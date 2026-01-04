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

    Ref<ParseResult> parse() {
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

    /* ================= Grammar ================= */

    Ref<ParseResult> expression() {
        return equality();
    }

    Ref<ParseResult> equality() {
        auto node = MakeRef<NonTerminalNode>("equality");

        auto left = comparison();
        node->children.push_back(left->cst);

        while (match(NOT_EQUAL_REL_OP, EQUAL_REL_OP)) {
            Ref<Token> op = previous();
            validateToken(op);
            auto right = comparison();

            node->children.push_back(MakeRef<TerminalNode>(op));
            node->children.push_back(right->cst);

            left->ast = MakeRef<BinaryExpr>(left->ast, op, right->ast);
        }
        return MakeRef<ParseResult>(left->ast, node);
    }

    Ref<ParseResult> comparison() {
        auto node = MakeRef<NonTerminalNode>("comparison");

        auto left = term();
        node->children.push_back(left->cst);

        while (match(GREATER_REL_OP, GREATER_EQUAL_REL_OP, LESS_REL_OP, LESS_EQUAL_REL_OP)) {
            Ref<Token> op = previous();
            validateToken(op);
            auto right = term();

            node->children.push_back(MakeRef<TerminalNode>(op));
            node->children.push_back(right->cst);

            left->ast = MakeRef<BinaryExpr>(left->ast, op, right->ast);
        }
        return MakeRef<ParseResult>(left->ast, node);
    }

    Ref<ParseResult> term() {
        auto node = MakeRef<NonTerminalNode>("term");

        auto left = factor();
        node->children.push_back(left->cst);

        while (match(SUBTRACT_OP, ADD_OP)) {
            Ref<Token> op = previous();
            validateToken(op);
            auto right = factor();

            node->children.push_back(MakeRef<TerminalNode>(op));
            node->children.push_back(right->cst);

            left->ast = MakeRef<BinaryExpr>(left->ast, op, right->ast);
        }
        return MakeRef<ParseResult>(left->ast, node);
    }

    Ref<ParseResult> factor() {
        auto node = MakeRef<NonTerminalNode>("factor");

        auto left = unary();
        node->children.push_back(left->cst);

        while (match(DIVIDE_OP, MULTIPLY_OP)) {
            Ref<Token> op = previous();
            validateToken(op);
            auto right = unary();

            node->children.push_back(MakeRef<TerminalNode>(op));
            node->children.push_back(right->cst);

            left->ast = MakeRef<BinaryExpr>(left->ast, op, right->ast);
        }
        return MakeRef<ParseResult>(left->ast, node);
    }

    Ref<ParseResult> unary() {
        auto node = MakeRef<NonTerminalNode>("unary");

        if (match(NOT_LOG_OP, NEGATIVE_OP, POSITIVE_OP)) {
            Ref<Token> op = previous();
            validateToken(op);

            auto right = unary();
            return MakeRef<ParseResult>(MakeRef<UnaryExpr>(op, right->ast), node);
        }

        return primary();
    }

    Ref<ParseResult> primary() {
        auto node = MakeRef<NonTerminalNode>("primary");
        auto token = peek();

        if (match(FALSE_LITERAL)) {
            node->children.push_back(MakeRef<TerminalNode>(token));
            return MakeRef<ParseResult>(MakeRef<LiteralExpr>(token), node);
        }

        if (match(TRUE_LITERAL)) {
            node->children.push_back(MakeRef<TerminalNode>(token));
            return MakeRef<ParseResult>(MakeRef<LiteralExpr>(token), node);
        }

        if (match(INT_LITERAL, FLT_LITERAL, STR_LITERAL)) {
            node->children.push_back(MakeRef<TerminalNode>(token));
            return MakeRef<ParseResult>(MakeRef<LiteralExpr>(token), node);
        }

        if (match(LEFT_PAREN_DELIM)) {
            node->children.push_back(MakeRef<TerminalNode>(token));

            auto expr = expression();
            node->children.push_back(expr->cst);

            consume(RIGHT_PAREN_DELIM, "Expect ')' after expression.");
            node->children.push_back(MakeRef<TerminalNode>(previous()));

            return MakeRef<ParseResult>(MakeRef<GroupingExpr>(expr->ast), node);
        }

        validateToken(peek());
        throw error(peek(), "Expect expression.");
    }

    /* ================= Helpers ================= */

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
        if ((check(types) || ...)) {
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

    void validateToken(Ref<Token> token) {
        if (token->type() == UNKNOWN) {
            throw error(peek(), "Lexer defined this as UNKNOWN token!");
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
