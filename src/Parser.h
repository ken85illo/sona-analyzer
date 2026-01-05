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
        auto node = NonTerminalNode::make("expression");

        auto left = equality();
        node->add(left->cst);

        return ParseResult::make(left->ast, node);
    }

    Ref<ParseResult> equality() {
        auto node = NonTerminalNode::make("equality");

        auto left = comparison();
        node->add(left->cst);

        while (auto op = match(NOT_EQUAL_REL_OP, EQUAL_REL_OP)) {
            auto right = comparison();

            node->add(TerminalNode::make(op));
            node->add(right->cst);

            left->ast = BinaryExpr::make(left->ast, op, right->ast);
        }
        return ParseResult::make(left->ast, node);
    }

    Ref<ParseResult> comparison() {
        auto node = NonTerminalNode::make("comparison");

        auto left = term();
        node->add(left->cst);

        while (auto op = match(GREATER_REL_OP, GREATER_EQUAL_REL_OP, LESS_REL_OP, LESS_EQUAL_REL_OP)) {
            auto right = term();

            node->add(TerminalNode::make(op));
            node->add(right->cst);

            left->ast = BinaryExpr::make(left->ast, op, right->ast);
        }
        return ParseResult::make(left->ast, node);
    }

    Ref<ParseResult> term() {
        auto node = NonTerminalNode::make("term");

        auto left = factor();
        node->add(left->cst);

        while (auto op = match(SUBTRACT_OP, ADD_OP)) {
            auto right = factor();

            node->add(TerminalNode::make(op));
            node->add(right->cst);

            left->ast = BinaryExpr::make(left->ast, op, right->ast);
        }
        return ParseResult::make(left->ast, node);
    }

    Ref<ParseResult> factor() {
        auto node = NonTerminalNode::make("factor");

        auto left = unary();
        node->add(left->cst);

        while (auto op = match(DIVIDE_OP, MULTIPLY_OP)) {
            auto right = unary();

            node->add(TerminalNode::make(op));
            node->add(right->cst);

            left->ast = BinaryExpr::make(left->ast, op, right->ast);
        }
        return ParseResult::make(left->ast, node);
    }

    Ref<ParseResult> unary() {
        auto node = NonTerminalNode::make("unary");

        if (auto op = match(NOT_LOG_OP, NEGATIVE_OP, POSITIVE_OP)) {
            auto right = primary();

            node->add(TerminalNode::make(op));
            node->add(right->cst);

            return ParseResult::make(UnaryExpr::make(op, right->ast), node);
        }

        return primary();
    }

    Ref<ParseResult> primary() {
        auto node = NonTerminalNode::make("primary");

        if (auto token = match(INT_LITERAL, FLT_LITERAL, TRUE_LITERAL, FALSE_LITERAL, STR_LITERAL)) {
            node->add(TerminalNode::make(token));
            return ParseResult::make(LiteralExpr::make(token), node);
        }

        if (auto leftParen = match(LEFT_PAREN_DELIM)) {
            node->add(TerminalNode::make(leftParen));

            auto expr = expression();
            node->add(expr->cst);

            auto rightParen = consume(RIGHT_PAREN_DELIM, "Expect ')' after expression.");
            node->add(TerminalNode::make(rightParen));

            return ParseResult::make(GroupingExpr::make(expr->ast), node);
        }

        validateToken(peek());
        throw error(
            peek(),
            "[SYNTAX] Expect STR_LITERAL, INT_LITERAL, FLT_LITERAL, FALSE_LITERAL, or TRUE_LITERAL in expression."
        );
    }

    /* ================= Helpers ================= */

    Ref<Token> consume(TokenType type, const std::string &message) {
        if (check(type)) {
            validateToken(peek());
            return advance();
        }

        throw error(peek(), message);
    }

    ParseError error(Ref<Token> token, const std::string &message) {
        ::error(token, message);
        return ParseError("");
    }

    template <typename... TokenType>
    Ref<Token> match(TokenType... types) {
        if ((check(types) || ...)) {
            validateToken(peek());
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
            throw error(peek(), "Invalid use of INCREMENT_OP");
        }
        else if (token->lexeme() == "--") {
            throw error(peek(), "Invalid use of DECREMENT_OP");
        }
        else if (token->lexeme() == "-") {
            throw error(peek(), "Invalid use of NEGATIVE_OP or SUBTRACT_OP");
        }
        else if (token->lexeme() == "+") {
            throw error(peek(), "Invalid use of POSITIVE_OP or ADD_OP");
        }
        else if (token->lexeme() == "*") {
            throw error(peek(), "Invalid use of MULTIPLY_OP");
        }
        else if (token->lexeme() == "/") {
            throw error(peek(), "Invalid use of DIVIDE_OP");
        }
        else if (token->lexeme() == "%") {
            throw error(peek(), "Invalid use of MODULO_OP");
        }
        else if (token->lexeme() == "+=") {
            throw error(peek(), "Invalid use of ADD_ASS_OP");
        }
        else if (token->lexeme() == "-=") {
            throw error(peek(), "Invalid use of SUBTRACT_ASS_OP");
        }
        else if (token->lexeme() == "*=") {
            throw error(peek(), "Invalid use of MULTIPLY_ASS_OP");
        }
        else if (token->lexeme() == "/=") {
            throw error(peek(), "Invalid use of DIVIDE_ASS_OP");
        }
        else if (token->lexeme() == "%=") {
            throw error(peek(), "Invalid use of MODULO_ASS_OP");
        }
        else {
            throw error(peek(), "Undefined identifier or keyword");
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
