#pragma once

#include "CST.h"
#include "Error.h"
#include "Token.h"

class Parser {
public:
    class ParseError : public std::runtime_error {
        using std::runtime_error::runtime_error;
    };

    Parser(const TokenVec &tokens)
    : m_tokens(tokens) {}

    CST parse() {
        auto node = NonTerminalNode::make("_BODY");
        while (!isAtEnd()) {
            if (auto stmt = statements()) {
                node->add(stmt);
            }
        }
        return node;
    }

    const TokenVec &m_tokens;

private:
    int64_t m_current = 0;

    /* ================= Grammar ================= */

    // Global + Data Type
    CST mod() {
        auto bt = m_current;
        auto node = NonTerminalNode::make("MOD");

        if (auto kw = match(CONST_RESW)) {
            if (auto kwm = match(STATIC_RESW)) {
                node->add(TerminalNode::make(kw));
                node->add(TerminalNode::make(kwm));
            }
            m_current = bt;
        }
        else if (auto kw = match(CONST_RESW)) {
            node->add(TerminalNode::make(kw));
        }
        else if (auto kw = match(STATIC_RESW)) {
            node->add(TerminalNode::make(kw));
        }
        else {
            node->add(EpsilonNode::make());
        }
        return node;
    }

    CST decSign() {
        auto node = NonTerminalNode::make("DEC_SIGN");
        node->add(mod());

        if (auto type = dt()) {
            node->add(type);
            node->add(dtSuffix());
            return node;
        }
        return nullptr;
    }

    CST dtSuffix() {
        if (auto stmt = decStmnt()) {
            auto node = NonTerminalNode::make("DT_SUFFIX");
            node->add(stmt);
            auto sc = consume(SEMICOLON_DELIM, "Expected a semicolon after.");
            node->add(TerminalNode::make(sc));
            return node;
        }
        throw error(previous(), "Expected a variable or function declaration statement");
    }

    // List of statements
    CST statements() {
        auto node = NonTerminalNode::make("_STATEMENTS");
        try {
            if (auto stmt = decSign()) {
                node->add(stmt);
                return node;
            }
            else if (auto stmt = assStmnt()) {
                node->add(stmt);
                auto sc = consume(SEMICOLON_DELIM, "Expected a semicolon after.");
                node->add(TerminalNode::make(sc));
                return node;
            }
            throw error(previous(), "Invalid statement.");
        }
        catch (ParseError) {
            node->add(ErrorNode::make(advance(), synchronize()));
            return node;
        }
    }

    // Naming Identifiers
    CST id() {
        if (auto ident = match(IDENTIFIER)) {
            auto node = NonTerminalNode::make("ID");
            return node;
        }
        return nullptr;
    }

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

        while (auto nt = logOp()) {
            node->add(nt);
            node->add(relLevel());
        }
        return node;
    }

    CST relLevel() {
        auto node = NonTerminalNode::make("REL_LEVEL");
        node->add(arithLevel());

        while (auto nt = relOp()) {
            node->add(nt);
            node->add(arithLevel());
        }
        return node;
    }

    CST arithLevel() {
        auto node = NonTerminalNode::make("ARITH_LEVEL");
        node->add(term());

        while (auto nt = addOp()) {
            node->add(nt);
            node->add(term());
        }
        return node;
    }

    CST term() {
        auto node = NonTerminalNode::make("TERM");
        node->add(factor());

        while (auto nt = multOp()) {
            node->add(nt);
            node->add(factor());
        }
        return node;
    }

    CST factor() {
        if (auto nt = operand()) {
            auto node = NonTerminalNode::make("FACTOR");
            node->add(nt);
            return node;
        }
        else if (auto leftParen = match(LEFT_PAREN_DELIM)) {
            auto node = NonTerminalNode::make("FACTOR");

            node->add(TerminalNode::make(leftParen));
            node->add(expression());
            auto rightParen = consume(RIGHT_PAREN_DELIM, "Expect ')' after expression ");
            node->add(TerminalNode::make(rightParen));
            return node;
        }

        throw error(previous(), "Expected an expression after.");
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

    CST unaryAssOp() {
        if (auto op = match(PRE_INCRMNT_OP, PRE_DECRMNT_OP, POST_INCRMNT_OP, POST_DECRMNT_OP)) {
            auto node = NonTerminalNode::make("UNARY_ASS_OP");
            node->add(TerminalNode::make(op));
            return node;
        }
        return nullptr;
    }

    CST assOp() {
        if (auto op = match(ADD_ASS_OP, SUBTRCT_ASS_OP, DIVIDE_ASS_OP, MODULO_ASS_OP, MULTPLY_ASS_OP, EQUAL_ASS_OP)) {
            auto node = NonTerminalNode::make("ASS_OP");
            node->add(TerminalNode::make(op));
            return node;
        }
        return nullptr;
    }

    // Expression operands
    CST operand() {
        if (auto nt = operandLiteral()) {
            auto node = NonTerminalNode::make("OPERAND");
            node->add(nt);
            return node;
        }
        else if (auto op = match(NOT_LOG_OP)) {
            auto node = NonTerminalNode::make("OPERAND");
            node->add(TerminalNode::make(op));
            node->add(operandList());
            return node;
        }
        else if (auto nt = assSingle()) {
            auto node = NonTerminalNode::make("OPERAND");
            node->add(nt);
            return node;
        }
        return nullptr;
    }

    // Variable operand cases
    CST operandList() {
        if (auto ident = operandId()) {
            auto node = NonTerminalNode::make("OPERAND_LIST");
            node->add(ident);
            return node;
        }
        else if (auto literal = operandLiteral()) {
            auto node = NonTerminalNode::make("OPERAND_LIST");
            node->add(literal);
            return node;
        }

        return nullptr;
    }

    CST operandLiteral() {
        if (auto nt = number()) {
            auto node = NonTerminalNode::make("OPERAND_LITERAL");
            node->add(nt);
            return node;
        }
        else if (auto nt = realNum()) {
            auto node = NonTerminalNode::make("OPERAND_LITERAL");
            node->add(nt);
            return node;
        }
        else if (auto nt = boolG()) {
            auto node = NonTerminalNode::make("OPERAND_LITERAL");
            node->add(nt);
            return node;
        }
        else if (auto c = match(CHAR_LITERAL)) {
            auto node = NonTerminalNode::make("OPERAND_LITERAL");
            node->add(TerminalNode::make(c));
            return node;
        }

        return nullptr;
    }

    CST operandId() {
        if (auto ident = id()) {
            auto node = NonTerminalNode::make("OPERAND_ID");
            node->add(ident);
            node->add(idSuffix());
            return node;
        }
        return nullptr;
    }

    CST idSuffix() {
        auto node = NonTerminalNode::make("ID_SUFFIX");
        if (auto leftSquare = match(LEFT_SQUARE_DELIM)) {
            node->add(TerminalNode::make(leftSquare));
            node->add(expression());
            auto rightSquare = consume(RIGHT_SQUARE_DELIM, "Expected ']' after expression");
            node->add(TerminalNode::make(rightSquare));
        }
        else {
            node->add(EpsilonNode::make());
        }
        return node;
    }

    // Data types
    CST dt() {
        if (auto dataType = match(
                INT_TYPE_RESW, CHAR_TYPE_RESW, STRING_TYPE_RESW, FLOAT_TYPE_RESW, DOUBLE_TYPE_RESW, VOID_TYPE_RESW
            )) {
            auto node = NonTerminalNode::make("DT");
            node->add(TerminalNode::make(dataType));
            return node;
        }
        else if (auto mod = match(UNSIGNED_RESW)) {
            if (auto dataType = match(INT_TYPE_RESW)) {
                auto node = NonTerminalNode::make("DT");
                node->add(TerminalNode::make(mod));
                node->add(TerminalNode::make(dataType));
                return node;
            }
        }
        return nullptr;
    }

    // Constructing Numbers
    CST number() {
        if (auto op = match(INT_LITERAL)) {
            auto node = NonTerminalNode::make("NUMBER");

            auto test = TerminalNode::make(op);
            node->add(test);
            return node;
        }
        else if (auto op = match(POSITIVE_OP)) {
            auto node = NonTerminalNode::make("NUMBER");

            node->add(TerminalNode::make(op));
            auto operand = match(INT_LITERAL);
            node->add(TerminalNode::make(operand));
            return node;
        }
        else if (auto op = match(NEGATIVE_OP)) {
            auto node = NonTerminalNode::make("NUMBER");

            node->add(TerminalNode::make(op));
            auto operand = match(INT_LITERAL);
            node->add(TerminalNode::make(operand));
            return node;
        }
        return nullptr;
    }

    CST realNum() {
        if (auto op = match(FLT_LITERAL)) {
            auto node = NonTerminalNode::make("REAL_NUM");

            node->add(TerminalNode::make(op));
            return node;
        }
        else if (auto op = match(POSITIVE_OP)) {
            auto node = NonTerminalNode::make("REAL_NUM");

            node->add(TerminalNode::make(op));
            auto operand = match(FLT_LITERAL);
            node->add(TerminalNode::make(operand));
            return node;
        }
        else if (auto op = match(NEGATIVE_OP)) {
            auto node = NonTerminalNode::make("REAL_NUM");

            node->add(TerminalNode::make(op));
            auto operand = match(FLT_LITERAL);
            node->add(TerminalNode::make(operand));
            return node;
        }
        return nullptr;
    }

    // Constants
    CST boolG() {
        if (auto op = match(TRUE_LITERAL)) {
            auto node = NonTerminalNode::make("BOOL");

            node->add(TerminalNode::make(op));
            return node;
        }
        else if (auto op = match(FALSE_LITERAL)) {
            auto node = NonTerminalNode::make("BOOL");

            node->add(TerminalNode::make(op));
            return node;
        }

        return nullptr;
    }

    // [[ ASSIGNMENT PRODUCTION RULE ]]
    // Basic Assignment Structure
    CST assStmnt() {
        if (auto ass = assSingle()) {
            auto node = NonTerminalNode::make("ASS_STMNT");
            node->add(ass);

            while (auto comma = match(COMMA_OP)) {
                node->add(TerminalNode::make(comma));
                node->add(checkAdd(assSingle(), previous(), "Expected another assignment expression after comma."));
            }

            return node;
        }
        return nullptr;
    }

    CST assSingle() {
        if (auto op = operandId()) {
            auto node = NonTerminalNode::make("ASS_SINGLE");
            node->add(op);
            node->add(assTail());
            return node;
        }
        else if (auto op = unaryAssOp()) {
            auto node = NonTerminalNode::make("ASS_SINGLE");
            node->add(op);
            node->add(operandId());
            return node;
        }
        return nullptr;
    }

    CST assTail() {
        auto node = NonTerminalNode::make("ASS_TAIL");
        if (auto op = assOp()) {
            node->add(op);
            node->add(expression());
        }
        else if (auto op = unaryAssOp()) {
            node->add(op);
        }
        else {
            node->add(EpsilonNode::make());
        }
        return node;
    }

    // [[ DECLARATION PRODUCTION RULE ]]
    CST decStmnt() {
        if (auto ident = id()) {
            auto node = NonTerminalNode::make("DEC_STMNT");
            node->add(ident);
            node->add(decTail());

            while (auto comma = match(COMMA_OP)) {
                node->add(TerminalNode::make(comma));
                node->add(checkAdd(id(), previous(), "Expected another assignment expression after comma."));
            }

            return node;
        }
        return nullptr;
    }

    CST decTail() {
        auto node = NonTerminalNode::make("DEC_TAIL");
        if (auto tail = arrSuffix()) {
            node->add(tail);
        }
        else {
            node->add(varTail());
        }
        return node;
    }

    CST varTail() {
        auto node = NonTerminalNode::make("VAR_TAIL");
        if (auto ass = match(EQUAL_ASS_OP)) {
            node->add(TerminalNode::make(ass));
            node->add(expression());
        }
        else {
            node->add(EpsilonNode::make());
        }
        return node;
    }

    CST arrSuffix() {
        if (auto leftSquare = match(LEFT_SQUARE_DELIM)) {
            auto node = NonTerminalNode::make("ARR_SUFFIX");
            node->add(TerminalNode::make(leftSquare));
            node->add(expression());
            auto rightSquare = consume(RIGHT_SQUARE_DELIM, "Expected a closing square bracket ']'.");
            node->add(TerminalNode::make(rightSquare));
            node->add(arrTail());
            return node;
        }
        return nullptr;
    }

    CST arrTail() {
        auto node = NonTerminalNode::make("ARR_TAIL");
        if (auto equal = match(EQUAL_ASS_OP)) {
            node->add(TerminalNode::make(equal));

            auto leftCurly = consume(LEFT_CURLY_DELIM, "Expected an opening curly brace '{' when declaring arrays.");
            node->add(TerminalNode::make(leftCurly));

            node->add(TerminalNode::make(leftCurly));
            node->add(checkAdd(operand(), previous(), "Expected an operand after left curly brace '{'."));

            while (auto comma = match(COMMA_OP)) {
                node->add(TerminalNode::make(comma));
                node->add(checkAdd(operand(), previous(), "Expected an operand after comma."));
            }

            auto rightCurly = consume(RIGHT_CURLY_DELIM, "Expected a closing curly brace '}'.");
            node->add(TerminalNode::make(rightCurly));
        }
        else {
            node->add(EpsilonNode::make());
        }
        return node;
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

    ParseError error(
        const Ref<Token> &token, const std::string &message,
        const std::optional<std::string> &tokenString = std::nullopt
    ) {
        ::error(token, message, tokenString);
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

    CST checkAdd(const CST &cst, const Ref<Token> &token, const std::string &message) {
        if (!cst) {
            throw error(token, message);
        }
        return cst;
    }

    Ref<Token> advance() {
        if (!isAtEnd()) {
            ++m_current;
        }
        return previous();
    }

    bool isAtEnd() {
        return m_current == m_tokens.size();
    }

    Ref<Token> peek() {
        if (isAtEnd()) {
            return previous();
        }

        return m_tokens[m_current];
    }

    Ref<Token> previous() {
        if (m_current - 1 < 0) {
            return m_tokens[0];
        }

        return m_tokens[m_current - 1];
    }

    void validateToken(const Ref<Token> &token) {
        if (token->type() != UNKNOWN) {
            return;
        }

        // Handle lexical unknown tokens
        if (token->lexeme() == "++") {
            throw error(
                token, "Invalid use of increment operator (it should be with a defined identifier).", "INCREMNT_OP"
            );
        }
        if (token->lexeme() == "--") {
            throw error(
                token, "Invalid use of decrement operator (it should be with a defined identifier).", "DECREMNT_OP"
            );
        }
        if (token->lexeme() == "-") {
            throw error(token, "Invalid use of negation or subtract(binary) operator.", "MINUS_OP");
        }
        if (token->lexeme() == "+") {
            throw error(token, "Invalid use of positive or addition(binary) operator.", "PLUS_OP");
        }
        if (token->lexeme() == "*") {
            throw error(token, "Invalid use of multiply(binary) operator.", "MULTIPLY_OP");
        }
        if (token->lexeme() == "/") {
            throw error(token, "Invalid use of divide(binary) operator.", "DIVIDE_OP");
        }
        if (token->lexeme() == "%") {
            throw error(token, "Invalid use of modulo(binary) operator.", "MODULO_OP");
        }
        if (token->lexeme() == "+=") {
            throw error(token, "Invalid use of addition assignment operator (Example use: x += 1)", "ADD_ASS_OP");
        }
        if (token->lexeme() == "-=") {
            throw error(
                token,
                "Invalid use of subtract assignment operator (Example use: x += 2)"
                "to assign).",
                "SUBTRCT_ASS_OP"
            );
        }
        if (token->lexeme() == "*=") {
            throw error(
                token,
                "Invalid use of multiply assignment operator (it should be used with a defined identifier and  a value "
                "to assign).",
                "MULTPLY_ASS_OP"
            );
        }
        if (token->lexeme() == "/=") {
            throw error(
                token,
                "Invalid use of divide assignment operator (it should be used with a defined identifier and  a value "
                "to assign).",
                "DIVIDE_ASS_OP"
            );
        }
        if (token->lexeme() == "%=") {
            throw error(
                token,
                "Invalid use of modulo assignment operator (it should be used with a defined identifier and  a value "
                "to assign).",
                "MODULO_ASS_OP"
            );
        }
        if (token->lexeme()[0] == '\'') {
            throw error(
                token, "Non terminated use of ' in a character literal (Example use: 'x' or '1')", "CHAR_LITERAL"
            );
        }

        throw error(token, "Undefined identifier or keyword");
    }

    int64_t synchronize() {
        advance();

        while (!isAtEnd()) {
            if (previous()->type() == SEMICOLON_DELIM) {
                return m_current;
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
                return m_current;
                break;
            default:
                break;
            }

            advance();
        }

        return -1;
    }
};
