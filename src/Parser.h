#pragma once

#include "CST.h"
#include "Error.h"
#include "Token.h"

class Parser {
public:
    class ParseError : public std::runtime_error {
    public:
        ParseError(const Ref<Token> &token)
        : std::runtime_error(""), token(token), node(nullptr) {}

        ParseError(const CST &node)
        : std::runtime_error(""), token(nullptr), node(node) {}

        Ref<Token> token;
        CST node;
    };

    Parser(const TokenVec &tokens)
    : m_tokens(tokens) {}

    CST parse() {
        return body();
    }

private:
    const TokenVec &m_tokens;
    int64_t m_current = 0;

    /* ================= Grammar ================= */

    // Global + Data Type
    CST mod() {
        return tryParse("MOD", [&](auto node) {
            auto bt = m_current;

            if (auto kw = match(CONST_RESW)) {
                if (auto kwm = match(STATIC_RESW)) {
                    node->add(TerminalNode::make(kw));
                    node->add(TerminalNode::make(kwm));
                }
                else {
                    m_current = bt;
                }
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
            return true;
        });
    }

    CST decSign() {
        return tryParse("DEC_SIGN", [&](auto node) {
            node->add(mod());

            if (auto type = dt()) {
                node->add(type);
                node->add(dtSuffix());
                return true;
            }
            return false;
        });
    }

    CST dtSuffix() {
        return tryParse("DT_SUFFIX", [&](auto node) {
            if (auto stmt = decStmnt()) {
                node->add(stmt);
                auto sc = consume(SEMICOLON_DELIM, "Expected a semicolon after.");
                node->add(TerminalNode::make(sc));
                return true;
            }
            throw error(previous(), "Expected a variable or function declaration statement");
        });
    }

    // Body
    CST body() {
        try {
            return tryParse("_BODY", [&](auto node) {
                while (auto stmt = statements()) {
                    node->add(stmt);

                    if (isAtEnd()) {
                        break;
                    }
                }

                if (node->empty()) {
                    node->add(EpsilonNode::make());
                }
                return true;
            });
        }
        catch (ParseError &error) {
            return error.node;
        }
    }

    // List of statements
    CST statements() {
        return tryParse("_STATEMENTS", [&](auto node) {
            // Skip all line comment and multiline comments
            while (check(LINE_COMNT) || check(MULTILINE_COMNT)) {
                advance();
            }

            if (auto stmt = decSign()) {
                node->add(stmt);
                return true;
            }
            else if(auto stmt = structStmnt()) {
                node->add(stmt);
                return true;
            }
            else if(auto stmt = structDec()) {
                node->add(stmt);
                return true;
            }
            else if (auto stmt = funcCall()) { // Must be called before assignment stmnt (both <id> first)
                node->add(stmt);
                return true;
            }
            else if (auto stmt = assStmnt()) {
                node->add(stmt);
                auto sc = consume(SEMICOLON_DELIM, "Expected a semicolon after.");
                node->add(TerminalNode::make(sc));
                return true;
            }
            else if (auto stmt = condStmnt()) {
                node->add(stmt);
                return true;
            }
            else if (auto stmt = iterativeStmt()) {
                node->add(stmt);
                return true;
            }
            return false;
        });
    }

    // Naming Identifiers
    CST id(TokenType type = IDENTIFIER) {
        return tryParse("ID", [&](auto node) {
            if (auto ident = match(type)) {
                node->add(TerminalNode::make(ident));
                return true;
            }
            return false;
        });
    }

    // General Expression
    CST expression() {
        return tryParse("EXPRESSION", [&](auto node) {
            node->add(logicalLevel());
            return true;
        });
    }

    // Expression Precedence: Parentheses > Arithmetic > Relational > Logical
    CST logicalLevel() {
        return tryParse("LOGICAL_LEVEL", [&](auto node) {
            node->add(relLevel());

            while (auto nt = logOp()) {
                node->add(nt);
                node->add(relLevel());
            }
            return true;
        });
    }

    CST relLevel() {
        return tryParse("REL_LEVEL", [&](auto node) {
            node->add(arithLevel());

            while (auto nt = relOp()) {
                node->add(nt);
                node->add(arithLevel());
            }
            return true;
        });
    }

    CST arithLevel() {
        return tryParse("ARITH_LEVEL", [&](auto node) {
            node->add(term());

            while (auto nt = addOp()) {
                node->add(nt);
                node->add(term());
            }
            return true;
        });
    }

    CST term() {
        return tryParse("TERM", [&](auto node) {
            node->add(factor());

            while (auto nt = multOp()) {
                node->add(nt);
                node->add(factor());
            }
            return true;
        });
    }

    CST factor() {
        return tryParse("FACTOR", [&](auto node) {
            if (auto nt = operand()) {
                node->add(nt);
                return true;
            }
            else if (auto leftParen = match(LEFT_PAREN_DELIM)) {
                node->add(TerminalNode::make(leftParen));
                node->add(expression());
                auto rightParen = consume(RIGHT_PAREN_DELIM, "Expect ')' after expression ");
                node->add(TerminalNode::make(rightParen));
                return true;
            }
            return false;
        });
    }

    // Operators
    CST logOp() {
        return tryParse("LOG_OP", [&](auto node) {
            if (auto op = match(AND_LOG_OP, OR_LOG_OP)) {
                node->add(TerminalNode::make(op));
                return true;
            }
            return false;
        });
    }

    CST relOp() {
        return tryParse("REL_OP", [&](auto node) {
            if (auto op = match(
                    EQUAL_REL_OP, NOT_EQUAL_REL_OP, LESS_REL_OP, GREATER_REL_OP, LESS_EQUAL_REL_OP, GREATER_EQUAL_REL_OP
                )) {
                node->add(TerminalNode::make(op));
                return true;
            }
            return false;
        });
    }

    CST addOp() {
        return tryParse("ADD_OP", [&](auto node) {
            if (auto op = match(ADD_OP, SUBTRACT_OP)) {
                node->add(TerminalNode::make(op));
                return true;
            }
            return false;
        });
    }

    CST multOp() {
        return tryParse("MULT_OP", [&](auto node) {
            if (auto op = match(MULTIPLY_OP, DIVIDE_OP, MODULO_OP)) {
                node->add(TerminalNode::make(op));
                return true;
            }
            return false;
        });
    }

    CST unaryAssOp() {
        return tryParse("UNARY_ASS_OP", [&](auto node) {
            if (auto op = match(PRE_INCRMNT_OP, PRE_DECRMNT_OP, POST_INCRMNT_OP, POST_DECRMNT_OP)) {
                node->add(TerminalNode::make(op));
                return true;
            }
            return false;
        });
    }

    CST assOp() {
        return tryParse("ASS_OP", [&](auto node) {
            if (auto op =
                    match(ADD_ASS_OP, SUBTRCT_ASS_OP, DIVIDE_ASS_OP, MODULO_ASS_OP, MULTPLY_ASS_OP, EQUAL_ASS_OP)) {
                node->add(TerminalNode::make(op));
                return true;
            }
            return false;
        });
    }

    // Expression operands
    CST operand() {
        return tryParse("OPERAND", [&](auto node) {
            if (auto nt = operandLiteral()) {
                node->add(nt);
                return true;
            }
            else if (auto op = match(NOT_LOG_OP)) {
                node->add(TerminalNode::make(op));
                node->add(operandList());
                return true;
            }
            else if (auto nt = assSingle()) {
                node->add(nt);
                return true;
            }
            return false;
        });
    }

    // Variable operand cases
    CST operandList() {
        return tryParse("OPERAND_LIST", [&](auto node) {
            if (auto ident = operandId()) {
                node->add(ident);
                return true;
            }
            else if (auto literal = operandLiteral()) {
                node->add(literal);
                return true;
            }

            return false;
        });
    }

    CST operandLiteral() {
        return tryParse("OPERAND_LITERAL", [&](auto node) {
            if (auto nt = number()) {
                node->add(nt);
                return true;
            }
            else if (auto nt = realNum()) {
                node->add(nt);
                return true;
            }
            else if (auto nt = boolG()) {
                node->add(nt);
                return true;
            }
            else if (auto c = match(CHAR_LITERAL)) {
                node->add(TerminalNode::make(c));
                return true;
            }

            return false;
        });
    }

    CST operandId() {
        return tryParse("OPERAND_ID", [&](auto node) {
            if (auto ident = id()) {
                node->add(ident);
                node->add(idSuffix());
                return true;
            }
            return false;
        });
    }

    CST idSuffix() {
        return tryParse("ID_SUFFIX", [&](auto node) {
            if (auto leftSquare = match(LEFT_SQUARE_DELIM)) {
                node->add(TerminalNode::make(leftSquare));
                node->add(checkAdd(expression(), previous(), "Expected an expression after."));
                auto rightSquare = consume(RIGHT_SQUARE_DELIM, "Expected ']' after expression");
                node->add(TerminalNode::make(rightSquare));
            }
            else {
                node->add(EpsilonNode::make());
            }
            return true;
        });
    }

    // Data types
    CST dt() {
        return tryParse("DT", [&](auto node) {
            if (auto dataType = match(
                    INT_TYPE_RESW, CHAR_TYPE_RESW, STRING_TYPE_RESW, FLOAT_TYPE_RESW, DOUBLE_TYPE_RESW, VOID_TYPE_RESW
                )) {
                node->add(TerminalNode::make(dataType));
                return true;
            }
            else if (auto mod = match(UNSIGNED_RESW)) {
                if (auto dataType = match(INT_TYPE_RESW)) {
                    node->add(TerminalNode::make(mod));
                    node->add(TerminalNode::make(dataType));
                    return true;
                }
            }
            return false;
        });
    }

    // Constructing Numbers
    CST number() {
        return tryParse("NUMBER", [&](auto node) {
            if (auto op = match(INT_LITERAL)) {
                auto test = TerminalNode::make(op);
                node->add(test);
                return true;
            }
            else if (auto op = match(POSITIVE_OP)) {
                node->add(TerminalNode::make(op));
                auto operand = match(INT_LITERAL);
                node->add(TerminalNode::make(operand));
                return true;
            }
            else if (auto op = match(NEGATIVE_OP)) {
                node->add(TerminalNode::make(op));
                auto operand = match(INT_LITERAL);
                node->add(TerminalNode::make(operand));
                return true;
            }
            return false;
        });
    }

    CST realNum() {
        return tryParse("REAL_NUM", [&](auto node) {
            if (auto op = match(FLT_LITERAL)) {
                node->add(TerminalNode::make(op));
                return true;
            }
            else if (auto op = match(POSITIVE_OP)) {
                node->add(TerminalNode::make(op));
                auto operand = match(FLT_LITERAL);
                node->add(TerminalNode::make(operand));
                return true;
            }
            else if (auto op = match(NEGATIVE_OP)) {
                node->add(TerminalNode::make(op));
                auto operand = match(FLT_LITERAL);
                node->add(TerminalNode::make(operand));
                return true;
            }
            return false;
        });
    }

    // Constants
    CST boolG() {
        return tryParse("BOOL", [&](auto node) {
            if (auto op = match(TRUE_LITERAL)) {
                node->add(TerminalNode::make(op));
                return true;
            }
            else if (auto op = match(FALSE_LITERAL)) {
                node->add(TerminalNode::make(op));
                return true;
            }

            return false;
        });
    }

    // [[ ASSIGNMENT PRODUCTION RULE ]]
    // Basic Assignment Structure
    CST assStmnt() {
        return tryParse("ASS_STMNT", [&](auto node) {
            if (auto ass = assSingle()) {
                node->add(ass);

                while (auto comma = match(COMMA_OP)) {
                    node->add(TerminalNode::make(comma));
                    node->add(checkAdd(assSingle(), previous(), "Expected another assignment expression after comma."));
                }

                return true;
            }
            return false;
        });
    }

    CST assSingle() {
        return tryParse("ASS_SINGLE", [&](auto node) {
            if (auto op = operandId()) {
                node->add(op);
                node->add(assTail());
                return true;
            }
            else if (auto op = unaryAssOp()) {
                node->add(op);
                node->add(operandId());
                return true;
            }
            return false;
        });
    }

    CST assTail() {
        return tryParse("ASS_TAIL", [&](auto node) {
            if (auto op = assOp()) {
                node->add(op);
                node->add(checkAdd(expression(), previous(), "Expected an expression after."));
            }
            else if (auto op = unaryAssOp()) {
                node->add(op);
            }
            else {
                node->add(EpsilonNode::make());
            }

            return true;
        });
    }

    // [[ DECLARATION PRODUCTION RULE ]]
    CST decStmnt() {
        return tryParse("DEC_STMNT", [&](auto node) {
            if (auto ident = id()) {
                node->add(ident);
                node->add(decTail());

                while (auto comma = match(COMMA_OP)) {
                    node->add(TerminalNode::make(comma));
                    node->add(checkAdd(id(), previous(), "Expected another assignment expression after comma."));
                }

                return true;
            }

            return false;
        });
    }

    CST decTail() {
        return tryParse("DEC_TAIL", [&](auto node) {
            if (auto tail = arrSuffix()) {
                node->add(tail);
            }
            else {
                node->add(varTail());
            }
            return true;
        });
    }

    CST varTail() {
        return tryParse("VAR_TAIL", [&](auto node) {
            if (auto ass = match(EQUAL_ASS_OP)) {
                node->add(TerminalNode::make(ass));
                node->add(checkAdd(expression(), previous(), "Expected an expression after."));
            }
            else {
                node->add(EpsilonNode::make());
            }
            return true;
        });
    }

    CST arrSuffix() {
        return tryParse("ARR_SUFFIX", [&](auto node) {
            if (auto leftSquare = match(LEFT_SQUARE_DELIM)) {
                node->add(TerminalNode::make(leftSquare));
                node->add(expression());
                auto rightSquare = consume(RIGHT_SQUARE_DELIM, "Expected a closing square bracket ']'.");
                node->add(TerminalNode::make(rightSquare));
                node->add(arrTail());
                return true;
            }
            return false;
        });
    }

    CST arrTail() {
        return tryParse("ARR_TAIL", [&](auto node) {
            if (auto equal = match(EQUAL_ASS_OP)) {
                node->add(TerminalNode::make(equal));

                auto leftCurly =
                    consume(LEFT_CURLY_DELIM, "Expected an opening curly brace '{' when declaring arrays.");
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
            return true;
        });
    }

    // [[ CONDITIONAL PRODUCTION RULE ]]

    // Conditional Statements Production Rule
    CST condStmnt() {
        return tryParse("COND_STMNT", [&](auto node) {
            if (auto nt = ifStmnt()) {
                node->add(nt);
                return true;
            }
            return false;
        });
    }

    CST ifStmnt() {
        return tryParse("IF_STMNT", [&](auto node) {
            if (auto ifkw = match(IF_RESW)) {
                node->add(TerminalNode::make(ifkw));

                auto leftParen = consume(
                    LEFT_PAREN_DELIM,
                    "Expected an opening parenthesis '(' after 'if' keyword as a start of conditional expression."
                );
                node->add(TerminalNode::make(leftParen));
                node->add(checkAdd(expression(), previous(), "Expected an expression after."));

                auto rightParen =
                    consume(RIGHT_PAREN_DELIM, "Expected a closing parenthesis ')' after 'if' expression.");
                node->add(TerminalNode::make(rightParen));

                auto leftCurly =
                    consume(LEFT_CURLY_DELIM, "Expected an opening curly brace '{' before 'if' body statement.");
                node->add(TerminalNode::make(leftCurly));

                node->add(body());

                auto rightCurly =
                    consume(RIGHT_CURLY_DELIM, "Expected a closing curly brace '}' after 'if' body statement.");
                node->add(TerminalNode::make(rightCurly));

                node->add(elseStmnt());
                return true;
            }
            return false;
        });
    }

    // Else / Elif Statements
    CST elseStmnt() {
        return tryParse("ELSE_STMNT", [&](auto node) {
            if (auto elifRw = match(ELIF_RESW)) {
                node->add(TerminalNode::make(elifRw));

                auto leftParen = consume(
                    LEFT_PAREN_DELIM,
                    "Expected an opening parenthesis '(' after 'elif' keyword as a start of conditional expression."
                );
                node->add(TerminalNode::make(leftParen));
                node->add(checkAdd(expression(), previous(), "Expected an expression after."));

                auto rightParen =
                    consume(RIGHT_PAREN_DELIM, "Expected a closing parenthesis ')' after 'elif' expression.");
                node->add(TerminalNode::make(rightParen));

                auto leftCurly =
                    consume(LEFT_CURLY_DELIM, "Expected an opening curly brace '{' before 'elif' body statement.");
                node->add(TerminalNode::make(leftCurly));

                node->add(body());

                auto rightCurly =
                    consume(RIGHT_CURLY_DELIM, "Expected a closing curly brace '}' after 'elif' body statement.");
                node->add(TerminalNode::make(rightCurly));

                node->add(elseStmnt());
            }
            else if (auto elseRw = match(ELSE_RESW)) {
                auto leftCurly =
                    consume(LEFT_CURLY_DELIM, "Expected an opening curly brace '{' before 'else' body statement.");
                node->add(TerminalNode::make(leftCurly));

                node->add(body());

                auto rightCurly =
                    consume(RIGHT_CURLY_DELIM, "Expected a closing curly brace '}' after 'else' body statement.");
                node->add(TerminalNode::make(rightCurly));

                node->add(elseStmnt());
            }
            else {
                node->add(EpsilonNode::make());
            }
            return true;
        });
    }

    // [[ Iterative Production Rule ]]

    // Iterative Loop Statement Production Rule
    CST iterativeStmt() {
        return tryParse("ITERATIVE_STMT", [&](auto node) {
            if (auto stmt = whileStmt()) {
                node->add(stmt);
                return true;
            }
            else if (auto stmt = doWhileStmt()) {
                node->add(stmt);
                return true;
            }
            else if (auto stmt = forStmt()) {
                node->add(stmt);
                return true;
            }

            return false;
        });
    }

    // While
    CST whileStmt() {
        return tryParse("WHILE_STMT", [&](auto node) {
            if (auto whileKw = match(WHILE_KEYW)) {
                node->add(TerminalNode::make(whileKw));

                auto leftParen = consume(
                    LEFT_PAREN_DELIM,
                    "Expected an opening parenthesis '(' after 'while' keyword as a start of conditional expression."
                );
                node->add(TerminalNode::make(leftParen));
                node->add(checkAdd(expression(), previous(), "Expected an expression after."));

                auto rightParen = consume(
                    RIGHT_PAREN_DELIM, "Expected a closing parenthesis ')' after 'while' conditional expression."
                );
                node->add(TerminalNode::make(rightParen));

                auto leftCurly =
                    consume(LEFT_CURLY_DELIM, "Expected an opening curly brace '{' before 'while' body statement.");
                node->add(TerminalNode::make(leftCurly));

                node->add(body());

                auto rightCurly =
                    consume(RIGHT_CURLY_DELIM, "Expected a closing curly brace '}' after 'while' body statement.");
                node->add(TerminalNode::make(rightCurly));
                return true;
            }
            return false;
        });
    }

    // Do-While
    CST doWhileStmt() {
        return tryParse("DO_WHILE_STMT", [&](auto node) {
            if (auto doKw = match(DO_KEYW)) {
                node->add(TerminalNode::make(doKw));

                auto leftCurly =
                    consume(LEFT_CURLY_DELIM, "Expected an opening curly brace '{' before 'do while' body statement.");
                node->add(TerminalNode::make(leftCurly));

                node->add(body());

                auto rightCurly =
                    consume(RIGHT_CURLY_DELIM, "Expected a closing curly brace '}' after 'do while' body statement.");

                node->add(TerminalNode::make(rightCurly));

                auto leftParen = consume(
                    LEFT_PAREN_DELIM,
                    "Expected an opening parenthesis '(' after 'do while' as a start of conditional expression."
                );

                node->add(TerminalNode::make(leftParen));
                node->add(checkAdd(expression(), previous(), "Expected an expression after."));

                auto rightParen = consume(
                    RIGHT_PAREN_DELIM, "Expected a closing parenthesis ')' after 'do while' conditional expression."
                );
                node->add(TerminalNode::make(rightParen));

                auto sc = consume(SEMICOLON_DELIM, "Expected a semicolon after a 'do while' body statement");
                node->add(TerminalNode::make(sc));

                return true;
            }
            return false;
        });
    }

    // For
    CST forStmt() {
        return tryParse("FOR_STMT", [&](auto node) {
            if (auto forKw = match(FOR_KEYW)) {
                node->add(TerminalNode::make(forKw));

                auto leftParen = consume(
                    LEFT_PAREN_DELIM,
                    "Expected an opening parenthesis '(' after 'for' keyword as a start of for statement expression."
                );
                node->add(TerminalNode::make(leftParen));

                node->add(forInit());
                auto scFirst = consume(SEMICOLON_DELIM, "Expected a semicolon ';' after for init expression.");
                node->add(TerminalNode::make(scFirst));

                node->add(checkAdd(expression(), previous(), "Expected an expression after."));
                auto scSecond = consume(SEMICOLON_DELIM, "Expected a semicolon ';' after for condition expression.");
                node->add(TerminalNode::make(scSecond));

                node->add(forCounter());

                auto rightParen =
                    consume(RIGHT_PAREN_DELIM, "Expected a closing parenthesis ')' after 'for' statement expression.");
                node->add(TerminalNode::make(rightParen));

                auto leftCurly =
                    consume(LEFT_CURLY_DELIM, "Expected an opening curly brace '{' before 'for' body statement.");
                node->add(TerminalNode::make(leftCurly));

                node->add(body());

                auto rightCurly =
                    consume(RIGHT_CURLY_DELIM, "Expected a closing curly brace '}' after 'for' body statement.");
                node->add(TerminalNode::make(rightCurly));
                return true;
            }
            return false;
        });
    }

    CST forInit() {
        return tryParse("FOR_INIT", [&](auto node) {
            if (auto type = dt()) {
                node->add(type);
                node->add(
                    checkAdd(forDec(), previous(), "Expected an identifier in 'for' init declaration statement.")
                );

                while (auto comma = match(COMMA_OP)) {
                    node->add(TerminalNode::make(comma));
                    node->add(checkAdd(
                        forDec(), previous(),
                        "Expected another declaration in 'for' init declaration statement after comma ','"
                    ));
                }
            }
            else {
                node->add(EpsilonNode::make());
            }

            return true;
        });
    }

    CST forDec() {
        return tryParse("FOR_DEC", [&](auto node) {
            if (auto ident = id()) {
                node->add(ident);
                node->add(forDecTail());
                return true;
            }

            return false;
        });
    }

    CST forDecTail() {
        return tryParse("FOR_DEC_TAIL", [&](auto node) {
            if (auto equal = match(EQUAL_ASS_OP)) {
                node->add(TerminalNode::make(equal));
                node->add(checkAdd(expression(), previous(), "Expected an expression after."));
            }
            else {
                node->add(EpsilonNode::make());
            }
            return true;
        });
    }

    CST forCounter() {
        return tryParse("FOR_COUNTER", [&](auto node) {
            if (auto stmt = assStmnt()) {
                node->add(stmt);
            }
            else {
                node->add(EpsilonNode::make());
            }
            return true;
        });
    }

    // [[ FUNCTION PRODUCTION RULE ]]

    // Function Declarations
    CST funcStmnt() {
        return tryParse("FUNC_STMNT", [&](auto node) {
            if (auto ident = id()) {
                node->add(ident);
                auto leftParen = consume(
                    LEFT_PAREN_DELIM,
                    "Expected an opening parenthesis '(' after a function declaration as a start fo parameter list."
                );

                node->add(TerminalNode::make(leftParen));
                node->add(paramList());

                auto rightParen = consume(
                    LEFT_PAREN_DELIM, "Expected a closing parenthesis '(' after  function declaration parameter list."
                );

                node->add(TerminalNode::make(rightParen));
                node->add(funcStmtSuffix());
                return true;
            }
            return false;
        });
    }

    CST funcStmtSuffix() {
        return tryParse("FUNC_STMT_SUFFIX", [&](auto node) {
            if (auto sc = match(SEMICOLON_DELIM)) {
                node->add(TerminalNode::make(sc));
                return true;
            }
            else if (auto leftCurly = match(LEFT_CURLY_DELIM)) {
                node->add(TerminalNode::make(leftCurly));
                node->add(body());
                auto rightCurly = consume(RIGHT_CURLY_DELIM, "Expected a closing curly brace '}' after function body.");
                node->add(TerminalNode::make(rightCurly));
                return true;
            }
            throw error(peek(), "Expected a semicolon ';' or body after function declaration.");
        });
    }

    CST paramList() {
        return tryParse("PARAM_LIST", [&](auto node) {
            if (auto type = dt()) {
                node->add(type);
                node->add(checkAdd(id(), previous(), "Expected an identifier for a function parameter declaration."));

                while (auto comma = match(COMMA_OP)) {
                    node->add(checkAdd(
                        dt(), previous(), "Expected a data type after comma ',' for function parameter declaration."
                    ));
                    node->add(
                        checkAdd(id(), previous(), "Expected an identifier for a function parameter declaration.")
                    );
                }
            }
            else {
                node->add(EpsilonNode::make());
            }
            return true;
        });
    }

    // Function Calls
    CST funcCall() {
        return tryParse("FUNC_CALL", [&](auto node) {
            const auto bt = m_current;
            if (auto ident = id()) {
                if (auto leftParen = match(LEFT_PAREN_DELIM)) {
                    node->add(ident);
                    node->add(TerminalNode::make(leftParen));

                    auto rightParen = consume(
                        RIGHT_PAREN_DELIM, "Expected a closing parenthesis ')' after a function call argument list."
                    );
                    node->add(TerminalNode::make(rightParen));
                    node->add(funcCallSuffix());
                    return true;
                }
            }
            m_current = bt;
            return false;
        });
    }

    CST funcCallSuffix() {
        return tryParse("FUNC_CALL_SUFFIX", [&](auto node) {
            if (auto sc = match(SEMICOLON_DELIM)) {
                node->add(TerminalNode::make(sc));
            }
            else {
                node->add(EpsilonNode::make());
            }

            return true;
        });
    }

    // List of possible argument combinations
    CST args() {
        return tryParse("ARGS", [&](auto node) {
            if (auto argLs = argList()) {
                node->add(argLs);

                while (auto comma = match(COMMA_OP)) {
                    node->add(TerminalNode::make(comma));
                    node->add(checkAdd(
                        argList(), previous(), "Expected another argument after comma ',' inside function call."
                    ));
                }
            }
            else {
                node->add(EpsilonNode::make());
            }
            return true;
        });
    }

    CST argList() {
        return tryParse("ARG_LIST", [&](auto node) {
            if (auto argS = argSingle()) {
                node->add(argS);

                while (auto plus = match(ADD_OP)) {
                    node->add(TerminalNode::make(plus));
                    node->add(checkAdd(
                        argSingle(), previous(),
                        "Expected another argument after addition operator '+' inside function call."
                    ));
                }

                return true;
            }
            return false;
        });
    }

    CST argSingle() {
        return tryParse("ARG_LIST", [&](auto node) {
            if (auto allChar = match(CHAR_LITERAL)) {
                node->add(TerminalNode::make(allChar));
                return true;
            }
            else if (auto call = funcCall()) {
                node->add(call);
                return true;
            }
            else if (auto expr = expression()) {
                node->add(expr);
                return true;
            }
            return false;
        });
    }

    // [[ Struct Production Rule ]]

    // Basic Struct Structure
    CST structType() {
        return tryParse("STRUCT_TYPE", [&](auto node) {
            if (auto ident = id(USER_STRUCT_RESW)) {
                node->add(ident);
                return true;
            }
            return false;
        });
    }

    CST structStmnt() {
        return tryParse("STRUCT_STMNT", [&](auto node) {
            if (auto kw = match(STRUCT_TYPE_RESW)) {
                node->add(TerminalNode::make(kw));
                node->add(
                    checkAdd(structType(), previous(), "Expected a struct type identifier after 'struct' keyword.")
                );

                auto leftCurly = consume(
                    LEFT_CURLY_DELIM,
                    "Expected a left curly brace '{' after struct type identifier as a start of struct body."
                );
                node->add(TerminalNode::make(leftCurly));

                while (auto body = structBody()) {
                    node->add(body);

                    auto sc = consume(SEMICOLON_DELIM, "Expected a semicolon after struct member declaration.");
                    node->add(TerminalNode::make(sc));
                }

                auto rightCurly = consume(
                    RIGHT_CURLY_DELIM,
                    "Expected a right curly brace '}' after struct body."
                );
                node->add(TerminalNode::make(rightCurly));
                return true;
            }
            return false;
        });
    }

    CST structBody() {
        return tryParse("STRUCT_BODY", [&](auto node) {
            if (auto stmt = decStmnt()) {
                node->add(stmt);
                return true;
            }
            else if (auto dec = structDec()) {
                node->add(dec);
                return true;
            }

            return false;
        });
    }

    // Declaring instances of structs
    CST structDec() {
        return tryParse("STRUCT_DEC", [&](auto node) {
            if (auto type = structType()) {
                node->add(type);
                node->add(checkAdd(
                    structId(), previous(),
                    "Expected an identifier after type in a struct declartion statement."
                ));

                while (auto comma = match(COMMA_OP)) {
                    node->add(TerminalNode::make(comma));
                    node->add(checkAdd(
                        structId(), previous(),
                        "Expected another identifier after comma in a struct declaration statement."
                    ));
                }
                return true;
            }
            return false;
        });
    }

    CST structId() {
        return tryParse("STRUCT_DEC", [&](auto node) {
            if (auto ident = id()) {
                node->add(ident);
                node->add(idSuffix());
                return true;
            }
            return false;
        });
    }


    // [[ SONA Machine Production Rule ]]
    

    /* ================= Helpers ================= */

    template <typename Func>
    CST tryParse(const std::string &nodeName, Func fn) {
        auto node = NonTerminalNode::make(nodeName);

        try {
            if (!fn(node)) {
                return nullptr;
            }
            return node;
        }
        catch (ParseError &error) {
            if (error.node) {
                node->add(error.node);
            }
            if (error.token) {
                node->add(ErrorNode::make(error.token, synchronize()));
                error.token = nullptr;
            }

            error.node = node;
            throw;
        }
    }

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
        return ParseError(token);
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
            throw error(token, "Expected an identifier paired with the operator.", "INCREMNT_OP");
        }
        if (token->lexeme() == "--") {
            throw error(token, "Expected an identifier paired with the operator.", "DECREMNT_OP");
        }
        if (token->lexeme() == "-") {
            throw error(token, "Expected to be used as a unary(negation) or binary(subtract) operator.", "MINUS_OP");
        }
        if (token->lexeme() == "+") {
            throw error(token, "Expected to be used as a unary(positive) or binary(addition) operator.", "PLUS_OP");
        }
        if (token->lexeme() == "*") {
            throw error(token, "Expected to be used as a binary(multiplication) operator.", "MULTIPLY_OP");
        }
        if (token->lexeme() == "/") {
            throw error(token, "Expected to be used as a binary(division) operator.", "DIVIDE_OP");
        }
        if (token->lexeme() == "%") {
            throw error(token, "Expected to be used as a binary(modulo) operator.", "MODULO_OP");
        }
        if (token->lexeme() == "+=") {
            throw error(
                token, "Invalid use of addition assignment operator (Example: x += <expression>)", "ADD_ASS_OP"
            );
        }
        if (token->lexeme() == "-=") {
            throw error(
                token, "Invalid use of subtract assignment operator (Example: x -= <expression>)", "SUBTRCT_ASS_OP"
            );
        }
        if (token->lexeme() == "*=") {
            throw error(
                token, "Invalid use of multiply assignment operator (Example: x *= <expression>)", "MULTPLY_ASS_OP"
            );
        }
        if (token->lexeme() == "/=") {
            throw error(
                token, "Invalid use of divide assignment operator (Example: x /= <expression>)", "DIVIDE_ASS_OP"
            );
        }
        if (token->lexeme() == "%=") {
            throw error(
                token, "Invalid use of modulo assignment operator (Example: x %= <expression>)", "MODULO_ASS_OP"
            );
        }
        if (token->lexeme()[0] == '\'') {
            throw error(token, "Non terminated ' in a character literal (Example: 'x' or '1')", "CHAR_LITERAL");
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
