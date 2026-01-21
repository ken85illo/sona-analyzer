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

        Ref<Token> token;
        CST node;
    };

    Parser(const TokenVec &tokens)
    : m_tokens(tokens) {}

    CST parse() {
        return sonaBase();
    }

private:
    const TokenVec &m_tokens;
    Ref<ErrorNode> lastError;
    int64_t m_current = 0;

    /* ================= Grammar ================= */

    // Main Block
    CST sonaBase() {
        return tryParse("_SONA_BASE", [&](auto node) {
            Ref<Token> current = nullptr;
            while (!isAtEnd() && !checkMain()) {
                if (auto list = offMainList()) {

                    // Modify last synchronize
                    if (current && lastError) {
                        lastError->setSychronize(current);
                        current = nullptr;
                    }

                    node->add(list);
                    continue;
                }
                advance();
                current = peek();
            }

            auto intType = consume(INT_TYPE_RESW, "Expect an 'int' type keyword for main function.");
            node->add(TerminalNode::make(intType));

            auto main = consume(IDENTIFIER, "Expect 'main' identifier for main function.");
            node->add(TerminalNode::make(main));

            auto leftParen = consume(LEFT_PAREN_DELIM, "Expected opening parenthesis '(' for main function arguments.");
            node->add(TerminalNode::make(leftParen));

            auto rightParen =
                consume(RIGHT_PAREN_DELIM, "Expected closing parenthesis ')' for main function arguments.");
            node->add(TerminalNode::make(rightParen));

            auto leftCurly = consume(LEFT_CURLY_DELIM, "Expected opening curly brace '{' before main function body.");
            node->add(TerminalNode::make(leftCurly));

            node->add(body());

            auto rightCurly = consume(RIGHT_CURLY_DELIM, "Expected closing curly brace '}' after main function body.");
            node->add(TerminalNode::make(rightCurly));

            if (!isAtEnd()) {
                throw error(peek(), "Expected end of file after main function.");
            }

            return true;
        }, false, false, true);
    }

    CST offMainList() {
        return tryParse("_OFF_MAIN_LIST", [&](auto node) {
            // Skip all line comment and multiline comments
            while (check(LINE_COMNT) || check(MULTILINE_COMNT)) {
                advance();
            }

            if (auto stmt = decSign()) {
                node->add(stmt);
                return true;
            }
            else if (auto stmt = machStmnt()) {
                node->add(stmt);
                return true;
            }
            else if (auto stmt = machDec()) {
                node->add(stmt);
                return true;
            }
            else if (auto stmt = structStmnt()) {
                node->add(stmt);
                return true;
            }
            else if (auto stmt = structDec()) {
                node->add(stmt);
                return true;
            }
            return false;
        }, false);
    }

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
                node->add(EpsilonNode::make(peek()));
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
            if (auto stmt = funcStmnt()) {
                node->add(stmt);
                return true;
            }
            else if (auto stmt = decStmnt()) {
                node->add(stmt);
                auto sc = consume(SEMICOLON_DELIM, "Expected a semicolon.");
                node->add(TerminalNode::make(sc));
                return true;
            }
            throw error(previous(), "Expected a variable or function declaration statement");
        });
    }

    // Body
    CST body() {
        return tryParse("_BODY", [&](auto node) {
            while (auto stmt = statements()) {
                node->add(stmt);

                if (isAtEnd() || check(RIGHT_CURLY_DELIM)) {
                    break;
                }
            }

            if (node->empty()) {
                node->add(EpsilonNode::make(peek()));
            }
            return true;
        });
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
            else if (auto stmt = machStmnt()) {
                node->add(stmt);
                return true;
            }
            else if (auto stmt = machDec()) {
                node->add(stmt);
                return true;
            }
            else if (auto stmt = structStmnt()) {
                node->add(stmt);
                return true;
            }
            else if (auto stmt = structDec()) {
                node->add(stmt);
                return true;
            }
            else if (auto stmt = funcCall()) { // Must be called before assignment stmnt (both <id> first)
                node->add(stmt);
                auto sc = consume(SEMICOLON_DELIM, "Expected a semicolon after function call statement.");
                node->add(TerminalNode::make(sc));
                return true;
            }
            else if (auto stmt = assStmnt()) {
                node->add(stmt);
                auto sc = consume(SEMICOLON_DELIM, "Expected a semicolon after assignment statement.");
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
            else if (auto stmt = returnStmnt()) {
                node->add(stmt);
                return true;
            }
            return false;
        }, false);
    }

    // General Return Statements
    CST returnStmnt() {
        return tryParse("RETURN_STMNT", [&](auto node) {
            if (auto ret = match(RETURN_RESW)) {
                node->add(TerminalNode::make(ret));
                node->add(checkAdd(expression(), peek(), "Expected an expression after 'return' keyword."));

                auto sc = consume(SEMICOLON_DELIM, "Expected a semicolon after return statement.");
                node->add(TerminalNode::make(sc));
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
            if (auto nt = logicalLevel()) {
                node->add(nt);
                return true;
            }
            return false;
        });
    }

    // Expression Precedence: Parentheses > Arithmetic > Relational > Logical
    CST logicalLevel() {
        return tryParse("LOGICAL_LEVEL", [&](auto node) {
            if (auto nt = relLevel()) {
                node->add(nt);

                while (auto nt = logOp()) {
                    node->add(nt);
                    node->add(relLevel());
                }
                return true;
            }
            return false;
        });
    }

    CST relLevel() {
        return tryParse("REL_LEVEL", [&](auto node) {
            if (auto nt = arithLevel()) {
                node->add(nt);

                while (auto nt = relOp()) {
                    node->add(nt);
                    node->add(arithLevel());
                }
                return true;
            }
            return false;
        });
    }

    CST arithLevel() {
        return tryParse("ARITH_LEVEL", [&](auto node) {
            if (auto nt = term()) {
                node->add(nt);

                while (auto nt = addOp()) {
                    node->add(nt);
                    node->add(term());
                }
                return true;
            }
            return false;
        });
    }

    CST term() {
        return tryParse("TERM", [&](auto node) {
            if (auto nt = factor()) {
                node->add(nt);

                while (auto nt = multOp()) {
                    node->add(nt);
                    node->add(factor());
                }
                return true;
            }
            return false;
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
            if (auto func = funcCall()) {
                node->add(func);
                return true;
            }
            else if (auto nt = operandLiteral()) {
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
                node->add(memberSuffix());
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
                node->add(EpsilonNode::make(peek()));
            }
            return true;
        });
    }

    // Accessing struct/machine variables
    CST memberSuffix() {
        return tryParse("MEMBER_SUFFIX", [&](auto node) {
            if (auto dot = match(DOT_OP)) {
                node->add(TerminalNode::make(dot));
                node->add(
                    checkAdd(id(), previous(), "Expected an identifier after dot operator '.' for member accessor.")
                );
                node->add(idSuffix());
            }
            else {
                node->add(EpsilonNode::make(peek()));
            }
            return true;
        });
    }

    // Data types
    CST dt() {
        return tryParse("DT", [&](auto node) {
            if (auto dataType = match(
                    INT_TYPE_RESW, CHAR_TYPE_RESW, STRING_TYPE_RESW, FLOAT_TYPE_RESW, DOUBLE_TYPE_RESW, VOID_TYPE_RESW,
                    BOOL_TYPE_RESW
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
                node->add(EpsilonNode::make(peek()));
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
                node->add(EpsilonNode::make(peek()));
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
                node->add(EpsilonNode::make(peek()));
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
                node->add(EpsilonNode::make(peek()));
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
                node->add(EpsilonNode::make(peek()));
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
                node->add(EpsilonNode::make(peek()));
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
                node->add(EpsilonNode::make(peek()));
            }
            return true;
        });
    }

    // [[ FUNCTION PRODUCTION RULE ]]
    // Function Declarations
    CST funcStmnt() {
        return tryParse("FUNC_STMNT", [&](auto node) {
            const auto bt = m_current;
            if (auto ident = id()) {
                if (auto leftParen = match(LEFT_PAREN_DELIM)) {
                    node->add(ident);
                    node->add(TerminalNode::make(leftParen));
                    node->add(paramList());

                    auto rightParen = consume(
                        RIGHT_PAREN_DELIM,
                        "Expected a closing parenthesis ')' after  function declaration parameter list."
                    );

                    node->add(TerminalNode::make(rightParen));
                    node->add(funcStmtSuffix());
                    return true;
                }
            }
            m_current = bt;
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
                node->add(EpsilonNode::make(peek()));
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

                    node->add(args());
                    auto rightParen = consume(
                        RIGHT_PAREN_DELIM, "Expected a closing parenthesis ')' after a function call argument list."
                    );
                    node->add(TerminalNode::make(rightParen));
                    return true;
                }
            }
            m_current = bt;
            return false;
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
                node->add(EpsilonNode::make(peek()));
            }
            return true;
        });
    }

    CST argList() {
        return tryParse("ARG_LIST", [&](auto node) {
            if (auto allChar = match(STR_LITERAL)) {
                node->add(TerminalNode::make(allChar));

                while (auto plus = match(ADD_OP)) {
                    node->add(TerminalNode::make(plus));
                    node->add(checkAdd(
                        argSingle(), previous(),
                        "Expected another argument after addition operator '+' inside function call."
                    ));
                }

                return true;
            }
            else if (auto expr = expression()) {
                node->add(expr);
                return true;
            }
            return false;
        });
    }

    CST argSingle() {
        return tryParse("ARG_SINGLE", [&](auto node) {
            if (auto allChar = match(STR_LITERAL)) {
                node->add(TerminalNode::make(allChar));
                return true;
            }
            else if (auto list = operand()) {
                node->add(list);
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

                auto rightCurly = consume(RIGHT_CURLY_DELIM, "Expected a right curly brace '}' after struct body.");
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
                    structId(), previous(), "Expected an identifier after type in a struct declartion statement."
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

    // Basic Machine Structure
    CST machType() {
        return tryParse("MACH_TYPE", [&](auto node) {
            if (auto ident = id(USER_MACHINE_RESW)) {
                node->add(ident);
                return true;
            }
            return false;
        });
    }

    CST machStmnt() {
        return tryParse("MACH_STMNT", [&](auto node) {
            if (auto mach = match(MACHINE_TYPE_RESW)) {
                node->add(TerminalNode::make(mach));
                node->add(
                    checkAdd(machType(), previous(), "Expected a machine type identifier after 'Machine' keyword.")
                );

                auto eq = consume(EQUAL_ASS_OP, "Expected an assignment operator '=' after machine type declaration.");
                node->add(TerminalNode::make(eq));

                auto leftCurly = consume(
                    LEFT_CURLY_DELIM,
                    "Expected a left curly brace '{' after machine type identifier as a start of machine body."
                );
                node->add(TerminalNode::make(leftCurly));

                node->add(machBody());

                auto rightCurly = consume(RIGHT_CURLY_DELIM, "Expected a right curly brace '}' after machine body.");
                node->add(TerminalNode::make(rightCurly));
                return true;
            }
            return false;
        });
    }

    // Machine Body
    CST machBody() {
        return tryParse("MACH_BODY", [&](auto node) {
            while (auto dec = decStmnt()) {
                node->add(dec);

                auto sc = consume(SEMICOLON_DELIM, "Expected a semicolon ';' after declaration statement.");
                node->add(TerminalNode::make(sc));
            }

            if (auto con = context()) {
                node->add(con);
                node->add(stateDec());
                node->add(startDec());
                node->add(machBodyList());
                return true;
            }

            throw error(
                previous(), "Expected machine body statement after open curly brace '{' (Base order: @context, "
                            "@states, @start, @transitions, @state)"
            );
        });
    }

    // Different Machine Cases: (1) Without @final &@finalState. (2) With @final & without @finalState. (3) With @final
    // & @finalState.
    CST machBodyList() {
        return tryParse("MACH_BODY_LIST", [&](auto node) {
            if (auto body = baseMach()) {
                node->add(body);
                return true;
            }
            else if (auto body = finalMach()) {
                node->add(body);
                return true;
            }
            throw error(
                peek(), "Expected at least a @transitions declaration and a @state body declaration after @start for a "
                        "base machine (@transitions body must come first)."
            );
        });
    }

    CST baseMach() {
        return tryParse("BASE_MACH", [&](auto node) {
            if (auto trans = transDec()) {
                node->add(trans);
                node->add(
                    checkAdd(stateBodyDec(), previous(), "Expected a @state body declaration after @transitions.")
                );
                while (auto dec = stateBodyDec()) {
                    node->add(dec);
                }
                return true;
            }
            return false;
        });
    }

    CST finalMach() {
        return tryParse("FINAL_MACH", [&](auto node) {
            if (auto final = finalDec()) {
                node->add(final);
                node->add(checkAdd(baseMach(), previous(), "Expected a @transitions body declaration after @final."));
                node->add(finalSuffix());
                return true;
            }
            return false;
        });
    }

    CST finalSuffix() {
        return tryParse("FINAL_SUFFIX", [&](auto node) {
            if (auto dec = finalBodyDec()) {
                node->add(dec);
            }
            else {
                node->add(EpsilonNode::make(peek()));
            }
            return true;
        });
    }

    // Parts of the Machine
    CST context() {
        return tryParse("CONTEXT", [&](auto node) {
            if (auto con = match(MAC_CONTEXT_RESW)) {
                node->add(TerminalNode::make(con));

                auto equal = consume(EQUAL_ASS_OP, "Expected an assignment operator '=' after @context declaration.");
                node->add(TerminalNode::make(equal));

                auto leftCurly = consume(
                    LEFT_CURLY_DELIM, "Expected a left curly brace '{' after assignment operator of @context as a "
                                      "start of declaration statements."
                );
                node->add(TerminalNode::make(leftCurly));

                node->add(contextVar());

                auto rightCurly = consume(
                    RIGHT_CURLY_DELIM, "Expected a right curly brace '}' after @context declaration statements."
                );
                node->add(TerminalNode::make(rightCurly));
                return true;
            }
            return false;
        });
    }

    CST contextVar() {
        return tryParse("CONTEXT_VAR", [&](auto node) {
            node->add(mod());

            if (auto type = dt()) {
                node->add(type);
                node->add(
                    checkAdd(decStmnt(), previous(), "Expected an identifier for variable declaration in @context.")
                );
                return true;
            }

            throw error(previous(), "Expected a variable declaration inside @context statement.");
        });
    }

    CST stateDec() {
        return tryParse("STATE_DEC", [&](auto node) {
            if (auto st = match(MAC_STATES_RESW)) {
                node->add(TerminalNode::make(st));

                auto equal = consume(EQUAL_ASS_OP, "Expected an assignment operator '=' after @states declaration.");
                node->add(TerminalNode::make(equal));

                auto leftCurly = consume(
                    LEFT_CURLY_DELIM, "Expected a left curly brace '{' after assignment operator of @states as a "
                                      "start of state identifier list."
                );
                node->add(TerminalNode::make(leftCurly));

                auto firstId = consume(STR_LITERAL, "Expected a state identifier inside @states declaration.");
                node->add(TerminalNode::make(firstId));

                while (auto comma = match(COMMA_OP)) {
                    node->add(TerminalNode::make(comma));

                    auto nextId = consume(STR_LITERAL, "Expected another state identifier after comma operator.");
                    node->add(TerminalNode::make(nextId));
                }

                auto rightCurly =
                    consume(RIGHT_CURLY_DELIM, "Expected a right curly brace '}' after @states identifier list.");
                node->add(TerminalNode::make(rightCurly));

                auto sc = consume(SEMICOLON_DELIM, "Expected a semicolon ';' after @states declaration statement.");
                node->add(TerminalNode::make(sc));
                return true;
            }
            throw error(previous(), "Expected @states after @context declaration statement.");
        }, false);
    }

    CST startDec() {
        return tryParse("START_DEC", [&](auto node) {
            if (auto st = match(MAC_START_RESW)) {
                node->add(TerminalNode::make(st));

                auto equal = consume(EQUAL_ASS_OP, "Expected an assignment operator '=' after @start declaration.");
                node->add(TerminalNode::make(equal));

                auto ident = consume(STR_LITERAL, "Expected state identifier for @start declaration.");
                node->add(TerminalNode::make(ident));

                auto sc = consume(SEMICOLON_DELIM, "Expected a semicolon ';' after @start declaration statement.");
                node->add(TerminalNode::make(sc));
                return true;
            }
            throw error(previous(), "Expected @start after @states declaration statement.");
        }, false);
    }

    CST finalDec() {
        return tryParse("FINAL_DEC", [&](auto node) {
            if (auto final = match(MAC_FINAL_RESW)) {
                node->add(TerminalNode::make(final));

                auto equal = consume(EQUAL_ASS_OP, "Expected an assignment operator '=' after @final declaration.");
                node->add(TerminalNode::make(equal));

                auto leftCurly = consume(
                    LEFT_CURLY_DELIM, "Expected a left curly brace '{' after assignment operator of @final as a "
                                      "start of state identifier list."
                );
                node->add(TerminalNode::make(leftCurly));

                auto firstId = consume(STR_LITERAL, "Expected a state identifier inside @final declaration.");
                node->add(TerminalNode::make(firstId));

                while (auto comma = match(COMMA_OP)) {
                    node->add(TerminalNode::make(comma));

                    auto nextId = consume(STR_LITERAL, "Expected another state identifier after comma operator.");
                    node->add(TerminalNode::make(nextId));
                }

                auto rightCurly =
                    consume(RIGHT_CURLY_DELIM, "Expected a right curly brace '}' after @final identifier list.");
                node->add(TerminalNode::make(rightCurly));

                auto sc = consume(SEMICOLON_DELIM, "Expected a semicolon ';' after @final declaration statement.");
                node->add(TerminalNode::make(sc));
                return true;
            }
            return false;
        });
    }

    CST transDec() {
        return tryParse("TRANS_DEC", [&](auto node) {
            if (auto trans = match(MAC_TRANSITIONS_RESW)) {
                node->add(TerminalNode::make(trans));

                auto equal =
                    consume(EQUAL_ASS_OP, "Expected an assignment operator '=' after @transitions declaration.");
                node->add(TerminalNode::make(equal));

                auto leftCurly = consume(
                    LEFT_CURLY_DELIM, "Expected a left curly brace '{' after assignment operator of @transitions as a "
                                      "start of transition list."
                );
                node->add(TerminalNode::make(leftCurly));

                if (auto trans = transition()) {
                    node->add(trans);
                }
                else {
                    node->add(EpsilonNode::make(peek()));
                }

                while (auto trans = transition()) {
                    node->add(trans);
                }

                auto rightCurly =
                    consume(RIGHT_CURLY_DELIM, "Expected a right curly brace '}' after @transitions list.");
                node->add(TerminalNode::make(rightCurly));

                return true;
            }
            return false;
        });
    }

    CST transition() {
        return tryParse("TRANSITION", [&](auto node) {
            if (auto leftParen = match(LEFT_PAREN_DELIM)) {
                node->add(TerminalNode::make(leftParen));

                auto ident = consume(STR_LITERAL, "Expected an initial state identifier for a transition statement.");
                node->add(TerminalNode::make(ident));

                auto comma =
                    consume(COMMA_OP, "Expected a comma after initial state identifier in transition statement.");
                node->add(TerminalNode::make(comma));

                node->add(
                    checkAdd(expression(), previous(), "Expected an expression after comma in transition statement.")
                );

                auto rightParen =
                    consume(RIGHT_PAREN_DELIM, "Expected a closing parenthesis ')' after transition expression.");
                node->add(TerminalNode::make(rightParen));

                auto equal = consume(EQUAL_ASS_OP, "Expected an assignment operator '=' for a transition statement.");
                node->add(TerminalNode::make(equal));

                auto output = consume(
                    STR_LITERAL,
                    "Expected an output state identifier after assignment operator '=' in a transition statement."
                );
                node->add(TerminalNode::make(output));

                auto sc = consume(SEMICOLON_DELIM, "Expected a semicolon ';' after transition statement.");
                node->add(TerminalNode::make(sc));
                return true;
            }
            return false;
        });
    }

    CST stateBodyDec() {
        return tryParse("STATE_BODY_DEC", [&](auto node) {
            if (auto dec = match(MAC_STATE_RESW)) {
                node->add(TerminalNode::make(dec));

                node->add(checkAdd(
                    id(), previous(),
                    "Expected an identifier after '@state' keyword in state body declaration statement."
                ));

                auto equal = consume(EQUAL_ASS_OP, "Expected an assignment operator '=' after @state declaration.");
                node->add(TerminalNode::make(equal));

                auto leftCurly = consume(
                    LEFT_CURLY_DELIM, "Expected a left curly brace '{' after assignment operator of @state as a "
                                      "start of state function body."
                );
                node->add(TerminalNode::make(leftCurly));

                node->add(body());

                auto rightCurly =
                    consume(RIGHT_CURLY_DELIM, "Expected a right curly brace '}' after @state function body.");
                node->add(TerminalNode::make(rightCurly));

                return true;
            }
            return false;
        });
    }

    CST finalBodyDec() {
        return tryParse("FINAL_BODY_DEC", [&](auto node) {
            if (auto final = match(MAC_FINAL_STATE_RESW)) {
                node->add(TerminalNode::make(final));

                auto equal =
                    consume(EQUAL_ASS_OP, "Expected an assignment operator '=' after @finalState declaration.");
                node->add(TerminalNode::make(equal));

                auto leftCurly = consume(
                    LEFT_CURLY_DELIM, "Expected a left curly brace '{' after assignment operator of @finalState as a "
                                      "start of final state function body."
                );
                node->add(TerminalNode::make(leftCurly));

                node->add(body());

                auto rightCurly =
                    consume(RIGHT_CURLY_DELIM, "Expected a right curly brace '}' after @finalState function body.");
                node->add(TerminalNode::make(rightCurly));

                return true;
            }
            return false;
        });
    }

    // Declaring instances of Machines
    CST machDec() {
        return tryParse("MACH_DEC", [&](auto node) {
            if (auto mach = machType()) {
                node->add(mach);

                node->add(checkAdd(
                    id(), previous(),
                    "Expected an identifier after machine type in a machine instance declaration statement."
                ));

                while (auto comma = match(COMMA_OP)) {
                    node->add(TerminalNode::make(comma));
                    node->add(checkAdd(
                        id(), previous(),
                        "Expected another identifier after comma operator ',' in a machine instance declaration "
                        "statement."
                    ));
                }

                auto sc =
                    consume(SEMICOLON_DELIM, "Expected a semicolon ';' after machine instance declaration statement.");
                node->add(TerminalNode::make(sc));
                return true;
            }
            return false;
        });
    }

    /* ================= Helpers ================= */

    bool checkMain() {
        return m_current + 1 < m_tokens.size() && m_tokens[m_current]->lexeme() == "int" &&
               m_tokens[m_current + 1]->lexeme() == "main";
    }

    template <TokenType delim = SEMICOLON_DELIM, typename Func>
    CST
    tryParse(const std::string &nodeName, Func fn, bool rethrow = true, bool checkStmnt = false, bool endFile = false) {
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
                lastError = ErrorNode::make(error.token, synchronize(delim, checkStmnt, endFile));
                node->add(lastError);
                error.token = nullptr;
            }

            error.node = node;
            if (rethrow) {
                throw;
            }
            return error.node;
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
        if (token->lexeme()[0] == '\"') {
            throw error(token, "Non terminated \" in a string literal (Example: \"Hello\" or \"123\")", "CHAR_LITERAL");
        }
        if (token->lexeme()[0] == '\'') {
            throw error(token, "Non terminated ' in a character literal (Example: 'x' or '1')", "CHAR_LITERAL");
        }

        throw error(token, "Undefined identifier or keyword");
    }

    Ref<Token> synchronize(TokenType delim, bool checkStmnt, bool endFile) {
        if (endFile) {
            m_current = m_tokens.size();
            return nullptr;
        }

        advance();
        while (!isAtEnd()) {
            if (previous()->type() == delim) {
                return peek();
            }

            if (checkStmnt) {
                advance();
                continue;
            }

            switch (peek()->type()) {
            case MACHINE_TYPE_RESW:
            case STRUCT_TYPE_RESW:
            case MAC_CONTEXT_RESW:
            case MAC_FINAL_RESW:
            case MAC_FINAL_STATE_RESW:
            case MAC_START_RESW:
            case MAC_STATE_RESW:
            case MAC_STATES_RESW:
            case MAC_TRANSITIONS_RESW:
            case UNSIGNED_RESW:
            case CONST_RESW:
            case STATIC_RESW:
            case INT_TYPE_RESW:
            case FLOAT_TYPE_RESW:
            case DOUBLE_TYPE_RESW:
            case STRING_TYPE_RESW:
            case BOOL_TYPE_RESW:
            case CHAR_TYPE_RESW:
            case VOID_TYPE_RESW:
            case FOR_KEYW:
            case IF_RESW:
            case WHILE_KEYW:
            case RETURN_RESW:
                return peek();
                break;
            default:
                break;
            }

            advance();
        }

        return nullptr;
    }
};
