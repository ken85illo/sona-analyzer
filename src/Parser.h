#pragma once

#include "CST.h"
#include "Error.h"
#include "Token.h"

class Parser {
public:
    class ParseError : public std::runtime_error {
    public:
        ParseError(const Ref<Token> &token)
        : std::runtime_error(""), token(token), nonTermNode(nullptr), errorNode(nullptr) {}

        Ref<Token> token;
        Ref<NonTerminalNode> nonTermNode;
        Ref<ErrorNode> errorNode;
    };

    Parser(const TokenVec &tokens)
    : m_tokens(tokens) {}

    CST parse() {
        return sonaBase();
    }

private:
    const TokenVec &m_tokens;
    int64_t m_lastSynchronize = -1;
    int64_t m_current = 0;
    int64_t m_scopeDepth = 0;

    /* ================= Grammar ================= */

    // Main Block
    CST sonaBase() {
        try {
            return tryParse("_SONA_BASE", [&](auto node) {
                Ref<Token> current = nullptr;
                while (!isAtEnd() && !checkMain()) {
                    node->add(offMainList());
                }

                auto intType = consume(INT_TYPE_RESW, "int", "for main function");
                node->add(TerminalNode::make(intType));

                auto main = consume(IDENTIFIER, "main", "as main function identifier");
                node->add(TerminalNode::make(main));

                auto leftParen = consume(LEFT_PAREN_DELIM, "(", "before main function arguments");
                node->add(TerminalNode::make(leftParen));

                node->add(paramList());

                auto rightParen = consume(RIGHT_PAREN_DELIM, ")", "after main function arguments");
                node->add(TerminalNode::make(rightParen));

                auto leftCurly = consume(LEFT_CURLY_DELIM, "{", "before main function body");
                node->add(TerminalNode::make(leftCurly));

                node->add(body());

                auto rightCurly = consume(RIGHT_CURLY_DELIM, "}", "after main function body");
                node->add(TerminalNode::make(rightCurly));

                if (!isAtEnd()) {
                    throw error(peek(), "File must end after the main function");
                }

                return true;
            });
        }
        catch (ParseError &error) {
            m_current = m_tokens.size() - 1;
            error.errorNode->setSychronize(peek());
            return error.nonTermNode;
        }
    }

    CST offMainList() {
        try {
            return tryParse("_OFF_MAIN_LIST", [&](auto node) {
                skipComments();

                if (isAtEnd()) {
                    return false;
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
                throw error(peek(), "Invalid global statement");
            });
        }
        catch (ParseError &error) {
            return error.nonTermNode;
        }
    }

    // Global + Data Type
    CST mod() {
        return tryParse("MOD", [&](auto node) {
            auto bt = m_current;

            if (auto kw = match(CONST_RESW)) {
                node->add(TerminalNode::make(kw));
                if (auto kwm = match(STATIC_RESW)) {
                    node->add(TerminalNode::make(kwm));
                }
            }
            else if (auto kw = match(STATIC_RESW)) {
                node->add(TerminalNode::make(kw));
            }
            else {
                node->add(EpsilonNode::make(peek()->line()));
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
        try {
            return tryParse("DT_SUFFIX", [&](auto node) {
                if (auto stmt = funcStmnt()) {
                    node->add(stmt);
                    return true;
                }

                auto prev = previous();
                auto stmt = decStmnt();
                if (stmt && prev->type() != VOID_TYPE_RESW) {
                    node->add(stmt);
                    auto sc = consume(SEMICOLON_DELIM, ";", "after declaration statement");
                    node->add(TerminalNode::make(sc));
                    return true;
                }
                throw error(peek(), "Invalid end statement after variable or function declaration");
            });
        }
        catch (ParseError &error) {
            if (match(RIGHT_CURLY_DELIM)) {
                error.errorNode->setSychronize(peek());
            }
            throw;
        }
    }

    // Body
    CST body() {
        return tryParse("_BODY", [&](auto node) {
            skipComments();

            while (!isAtEnd() && !check(RIGHT_CURLY_DELIM)) {
                skipComments();

                if (auto stmt = statements()) {
                    node->add(stmt);
                }
            }

            if (node->empty()) {
                node->add(EpsilonNode::make(peek()->line()));
            }
            return true;
        });
    }

    // List of statements
    CST statements() {
        try {
            return tryParse("_STATEMENTS", [&](auto node) {
                if (isAtEnd()) {
                    return false;
                }

                if (auto stmt = fullDecStmt()) {
                    node->add(stmt);
                    return true;
                }
                if (auto stmt = machStmnt()) {
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
                else if (auto stmt = funcCall()) {
                    node->add(stmt);
                    auto sc = consume(SEMICOLON_DELIM, ";", "after function call statement");
                    node->add(TerminalNode::make(sc));
                    return true;
                }
                else if (auto stmt = assStmnt()) {
                    node->add(stmt);
                    auto sc = consume(SEMICOLON_DELIM, ";", "after assignment statement");
                    node->add(TerminalNode::make(sc));
                    return true;
                }
                else if (auto stmt = expression()) {
                    node->add(stmt);
                    auto sc = consume(SEMICOLON_DELIM, ";", "after expression statement");
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
                throw error(peek(), "Invalid scoped statement");
            });
        }
        catch (ParseError &error) {
            return error.nonTermNode;
        }
    }

    // Full Declaration Statement
    CST fullDecStmt() {
        return tryParse("FULL_DEC_STMT", [&](auto node) {
            node->add(mod());

            if (check(VOID_TYPE_RESW)) {
                return false;
            }

            if (auto type = dt()) {
                node->add(type);
                node->add(checkAdd(decStmnt(), previous(), "assignment statement", "after data type"));
                auto sc = consume(SEMICOLON_DELIM, ";", "after declaration statement");
                node->add(TerminalNode::make(sc));
                return true;
            }
            return false;
        });
    }

    // General Return Statements
    CST returnStmnt() {
        return tryParse("RETURN_STMNT", [&](auto node) {
            if (auto ret = match(RETURN_RESW)) {
                node->add(TerminalNode::make(ret));
                node->add(returnExpr());

                auto sc = consume(SEMICOLON_DELIM, ";", "after return statement");
                node->add(TerminalNode::make(sc));
                return true;
            }
            return false;
        });
    }

    CST returnExpr() {
        return tryParse("RETURN_EXPR", [&](auto node) {
            if (auto expr = expression()) {
                node->add(expr);
            }
            else {
                node->add(EpsilonNode::make(peek()->line()));
            }
            return true;
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
                    node->add(checkAdd(relLevel(), peek(), "expression", "after logical operator"));
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
                    node->add(checkAdd(arithLevel(), previous(), "expression", "after relational operator"));
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
                    node->add(checkAdd(term(), previous(), "expression", "after add or subtract operator"));
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
                    node->add(checkAdd(factor(), previous(), "expression", "after multiply or divide operator"));
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
                auto rightParen = consume(RIGHT_PAREN_DELIM, ")", "after expression");
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

    // String Expression
    CST strExpr() {
        return tryParse("STR_EXPR", [&](auto node) {
            if (auto allChar = match(STR_LITERAL)) {
                node->add(TerminalNode::make(allChar));

                while (auto plus = match(ADD_OP)) {
                    node->add(TerminalNode::make(plus));
                    node->add(checkAdd(strOperand(), previous(), "expression", "after '+' "));
                }

                return true;
            }
            return false;
        });
    }

    CST strOperand() {
        return tryParse("STR_OPERAND", [&](auto node) {
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
                node->add(checkAdd(expression(), previous(), "expression", "after '[' "));
                auto rightSquare = consume(RIGHT_SQUARE_DELIM, "]", "after expression");
                node->add(TerminalNode::make(rightSquare));
            }
            else {
                node->add(EpsilonNode::make(peek()->line()));
            }
            return true;
        });
    }

    // Accessing struct/machine variables
    CST memberSuffix() {
        return tryParse("MEMBER_SUFFIX", [&](auto node) {
            if (auto dot = match(DOT_OP)) {
                node->add(TerminalNode::make(dot));
                node->add(checkAdd(id(), previous(), "identifier", "after '.' "));
                node->add(idSuffix());
            }
            else {
                node->add(EpsilonNode::make(peek()->line()));
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
            auto bt = m_current;
            if (auto op = match(INT_LITERAL)) {
                auto test = TerminalNode::make(op);
                node->add(test);
                return true;
            }
            else if (auto op = match(POSITIVE_OP)) {
                if (auto operand = match(INT_LITERAL)) {
                    node->add(TerminalNode::make(op));
                    node->add(TerminalNode::make(operand));
                    return true;
                }
            }
            else if (auto op = match(NEGATIVE_OP)) {
                if (auto operand = match(INT_LITERAL)) {
                    node->add(TerminalNode::make(op));
                    node->add(TerminalNode::make(operand));
                    return true;
                }
            }
            m_current = bt;
            return false;
        });
    }

    CST realNum() {
        return tryParse("REAL_NUM", [&](auto node) {
            auto bt = m_current;
            if (auto op = match(FLT_LITERAL)) {
                node->add(TerminalNode::make(op));
                return true;
            }
            else if (auto op = match(POSITIVE_OP)) {
                if (auto operand = match(FLT_LITERAL)) {
                    node->add(TerminalNode::make(op));
                    node->add(TerminalNode::make(operand));
                    return true;
                }
            }
            else if (auto op = match(NEGATIVE_OP)) {
                if (auto operand = match(FLT_LITERAL)) {
                    node->add(TerminalNode::make(op));
                    node->add(TerminalNode::make(operand));
                    return true;
                }
            }
            m_current = bt;
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
            auto bt = m_current;

            if (match(IDENTIFIER) && (addOp() || multOp() || relOp() || logOp())) {
                m_current = bt;
                return false;
            }

            m_current = bt;
            if (auto ass = assSingle()) {
                node->add(ass);

                while (auto comma = match(COMMA_OP)) {
                    node->add(TerminalNode::make(comma));
                    node->add(checkAdd(assSingle(), previous(), "assignment statement", "after ',' "));
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
                node->add(checkAdd(expression(), previous(), "expression", "after assignment operator"));
            }
            else if (auto op = unaryAssOp()) {
                node->add(op);
            }
            else {
                node->add(EpsilonNode::make(peek()->line()));
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
                    node->add(checkAdd(id(), previous(), "assignment statement", "after ',' "));
                    node->add(decTail());
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
            else if (auto tail = varTail()) {
                node->add(tail);
            }
            else {
                node->add(EpsilonNode::make(peek()->line()));
            }
            return true;
        });
    }

    CST varTail() {
        return tryParse("VAR_TAIL", [&](auto node) {
            if (auto ass = match(EQUAL_ASS_OP)) {
                node->add(TerminalNode::make(ass));

                if (auto expr = strExpr()) {
                    node->add(expr);
                    return true;
                }
                node->add(checkAdd(expression(), previous(), "expression", "after '=' "));
            }
            else {
                node->add(EpsilonNode::make(peek()->line()));
            }
            return true;
        });
    }

    CST arrSuffix() {
        return tryParse("ARR_SUFFIX", [&](auto node) {
            if (auto leftSquare = match(LEFT_SQUARE_DELIM)) {
                node->add(TerminalNode::make(leftSquare));
                node->add(expression());
                auto rightSquare = consume(RIGHT_SQUARE_DELIM, "]", "after expression");
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

                auto leftCurly = consume(LEFT_CURLY_DELIM, "{", "before array initializer");
                node->add(TerminalNode::make(leftCurly));

                node->add(checkAdd(operand(), previous(), "operand", "after '{' "));

                while (auto comma = match(COMMA_OP)) {
                    node->add(TerminalNode::make(comma));
                    node->add(checkAdd(operand(), previous(), "operand", "after ',' "));
                }

                auto rightCurly = consume(RIGHT_CURLY_DELIM, "}", "after array initializer");
                node->add(TerminalNode::make(rightCurly));
            }
            else {
                node->add(EpsilonNode::make(peek()->line()));
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

                auto leftParen = consume(LEFT_PAREN_DELIM, "(", "after 'if' keyword");
                node->add(TerminalNode::make(leftParen));
                node->add(checkAdd(expression(), previous(), "expression", "after '(' "));

                auto rightParen = consume(RIGHT_PAREN_DELIM, ")", "after expression");
                node->add(TerminalNode::make(rightParen));

                auto leftCurly = consume(LEFT_CURLY_DELIM, "{", "before 'if' body statement");
                node->add(TerminalNode::make(leftCurly));

                node->add(body());

                auto rightCurly = consume(RIGHT_CURLY_DELIM, "}", "after 'if' body statement");
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

                auto leftParen = consume(LEFT_PAREN_DELIM, "(", "after 'elif' keyword");
                node->add(TerminalNode::make(leftParen));
                node->add(checkAdd(expression(), previous(), "expression", "after '(' "));

                auto rightParen = consume(RIGHT_PAREN_DELIM, ")", "after expression");
                node->add(TerminalNode::make(rightParen));

                auto leftCurly = consume(LEFT_CURLY_DELIM, "{", "before 'elif' body statement");
                node->add(TerminalNode::make(leftCurly));

                node->add(body());

                auto rightCurly = consume(RIGHT_CURLY_DELIM, "}", "after 'elif' body statement");
                node->add(TerminalNode::make(rightCurly));

                node->add(elseStmnt());
            }
            else if (auto elseRw = match(ELSE_RESW)) {
                auto leftCurly = consume(LEFT_CURLY_DELIM, "{", "before 'else' body statement");
                node->add(TerminalNode::make(leftCurly));

                node->add(body());

                auto rightCurly = consume(RIGHT_CURLY_DELIM, "}", "after 'else' body statement");
                node->add(TerminalNode::make(rightCurly));

                node->add(elseStmnt());
            }
            else {
                node->add(EpsilonNode::make(peek()->line()));
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

                auto leftParen = consume(LEFT_PAREN_DELIM, "(", "after 'while' keyword");
                node->add(TerminalNode::make(leftParen));
                node->add(checkAdd(expression(), previous(), "expression", "after '(' "));

                auto rightParen = consume(RIGHT_PAREN_DELIM, ")", "after expression");
                node->add(TerminalNode::make(rightParen));

                auto leftCurly = consume(LEFT_CURLY_DELIM, "{", "before 'while' body statement");
                node->add(TerminalNode::make(leftCurly));

                node->add(body());

                auto rightCurly = consume(RIGHT_CURLY_DELIM, "}", "after 'while' body statement");
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

                auto leftCurly = consume(LEFT_CURLY_DELIM, "{", "before 'do while' body statement");
                node->add(TerminalNode::make(leftCurly));

                node->add(body());

                auto rightCurly = consume(RIGHT_CURLY_DELIM, "}", "after 'do while' body statement");

                node->add(TerminalNode::make(rightCurly));

                auto whileKw = consume(WHILE_KEYW, "while", "keyword after '}' ");
                node->add(TerminalNode::make(whileKw));

                auto leftParen = consume(LEFT_PAREN_DELIM, "(", "after 'while' keyword");

                node->add(TerminalNode::make(leftParen));
                node->add(checkAdd(expression(), previous(), "expression", "after '(' "));

                auto rightParen = consume(RIGHT_PAREN_DELIM, ")", "after expression");
                node->add(TerminalNode::make(rightParen));

                auto sc = consume(SEMICOLON_DELIM, ";", "after 'do while' statement");
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

                auto leftParen = consume(LEFT_PAREN_DELIM, "(", "after 'for' keyword");
                node->add(TerminalNode::make(leftParen));

                node->add(forInit());
                auto scFirst = consume(SEMICOLON_DELIM, ";", "after initializer");
                node->add(TerminalNode::make(scFirst));

                node->add(checkAdd(expression(), previous(), "expression", "to be the condition"));
                auto scSecond = consume(SEMICOLON_DELIM, ";", "after condition");
                node->add(TerminalNode::make(scSecond));

                node->add(forCounter());
                auto rightParen = consume(RIGHT_PAREN_DELIM, ")", "after counter");
                node->add(TerminalNode::make(rightParen));

                auto leftCurly = consume(LEFT_CURLY_DELIM, "{", "before 'for' body statement");
                node->add(TerminalNode::make(leftCurly));

                node->add(body());

                auto rightCurly = consume(RIGHT_CURLY_DELIM, "}", "after 'for' body statement");
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
                node->add(checkAdd(forDec(), previous(), "assignment statement", "after data type"));

                while (auto comma = match(COMMA_OP)) {
                    node->add(TerminalNode::make(comma));
                    node->add(checkAdd(forDec(), previous(), "assignment statement", "after ',' "));
                }
            }
            else {
                node->add(EpsilonNode::make(peek()->line()));
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
                node->add(checkAdd(expression(), previous(), "expression", "after '=' "));
            }
            else {
                node->add(EpsilonNode::make(peek()->line()));
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
                node->add(EpsilonNode::make(peek()->line()));
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

                    auto rightParen = consume(RIGHT_PAREN_DELIM, ")", "after function parameter list");

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

                auto rightCurly = consume(RIGHT_CURLY_DELIM, "}", "after function body");
                node->add(TerminalNode::make(rightCurly));
                return true;
            }
            throw error(peek(), "Invalid end statement for variable or function declaration");
        });
    }

    CST paramList() {
        return tryParse("PARAM_LIST", [&](auto node) {
            if (auto type = dt()) {
                node->add(type);
                node->add(checkAdd(id(), previous(), "identifier", "after data type"));

                while (auto comma = match(COMMA_OP)) {
                    node->add(checkAdd(dt(), previous(), "declaration statement", "after ',' "));
                    node->add(checkAdd(id(), previous(), "identifier", "after data type"));
                }
            }
            else {
                node->add(EpsilonNode::make(peek()->line()));
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
                    auto rightParen = consume(RIGHT_PAREN_DELIM, ")", "after function argument list");
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
                    node->add(checkAdd(argList(), previous(), "argument", "after ',' "));
                }
            }
            else {
                node->add(EpsilonNode::make(peek()->line()));
            }
            return true;
        });
    }

    CST argList() {
        return tryParse("ARG_LIST", [&](auto node) {
            if (auto expr = strExpr()) {
                node->add(expr);
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
                node->add(checkAdd(structType(), previous(), "identifier", "after 'struct' keyword"));

                auto leftCurly = consume(LEFT_CURLY_DELIM, "{", "before struct body");
                node->add(TerminalNode::make(leftCurly));

                while (auto body = structBody()) {
                    node->add(body);
                }

                auto rightCurly = consume(RIGHT_CURLY_DELIM, "}", "after struct body");
                node->add(TerminalNode::make(rightCurly));
                return true;
            }
            return false;
        });
    }

    CST structBody() {
        return tryParse("STRUCT_BODY", [&](auto node) {
            if (auto stmt = fullDecStmt()) {
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
                node->add(checkAdd(structId(), previous(), "identifier", "after data type"));

                while (auto comma = match(COMMA_OP)) {
                    node->add(TerminalNode::make(comma));
                    node->add(checkAdd(structId(), previous(), "identifier", "after ',' "));
                }
                auto sc = consume(SEMICOLON_DELIM, ";", "after struct instance declaration");
                node->add(TerminalNode::make(sc));
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
        int64_t topDepth = m_scopeDepth;

        try {
            return tryParse("MACH_STMNT", [&](auto node) {
                if (auto mach = match(MACHINE_TYPE_RESW)) { // Machine identifier  = {}
                    node->add(TerminalNode::make(mach));
                    node->add(checkAdd(machType(), previous(), "identifier", "after 'Machine' keyword"));

                    auto eq = consume(EQUAL_ASS_OP, "=", "after machine declaration");
                    node->add(TerminalNode::make(eq));

                    auto leftCurly = consume(LEFT_CURLY_DELIM, "{", "before machine body");
                    node->add(TerminalNode::make(leftCurly));

                    node->add(machBody());

                    auto rightCurly = consume(RIGHT_CURLY_DELIM, "}", "after machine body");
                    node->add(TerminalNode::make(rightCurly));
                    return true;
                }
                return false;
            });
        }
        catch (ParseError &error) {
            while (!isAtEnd() && m_scopeDepth > topDepth) {
                advance();
            }
            error.errorNode->setSychronize(synchronize());
            throw;
        }
    }

    // Machine Body
    CST machBody() {
        return tryParse("MACH_BODY", [&](auto node) {
            while (auto dec = fullDecStmt()) {
                node->add(dec);
            }

            if (auto con = context()) {
                node->add(con);
                node->add(stateDec());
                node->add(startDec());
                node->add(machBodyList());
                return true;
            }

            throw error(
                previous(), "Machine body should not be empty and should follow the base order from top to bottom: "
                            "[@final], @context, @states, @start, @transitions, @state, [@finalState]"
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
                peek(), "Machine should at least have a @transitions and @state declaration"
                        "(@transitions should ALWAYS be first)"
            );
        });
    }

    CST baseMach() {
        return tryParse("BASE_MACH", [&](auto node) {
            if (auto trans = transDec()) {
                node->add(trans);
                node->add(checkAdd(stateBodyDec(), previous(), "@state declaration", "after @transitions"));
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
                node->add(checkAdd(baseMach(), previous(), "@transitions declaration", "after @final"));
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
                node->add(EpsilonNode::make(peek()->line()));
            }
            return true;
        });
    }

    // Parts of the Machine
    CST context() {
        return tryParse("CONTEXT", [&](auto node) {
            if (auto con = match(MAC_CONTEXT_RESW)) {
                node->add(TerminalNode::make(con));

                auto equal = consume(EQUAL_ASS_OP, "=", "after '@context' keyword");
                node->add(TerminalNode::make(equal));

                auto leftCurly = consume(LEFT_CURLY_DELIM, "{", "before @context body");
                node->add(TerminalNode::make(leftCurly));

                node->add(contextVar());

                auto rightCurly = consume(RIGHT_CURLY_DELIM, "}", "after @context body");
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
                node->add(checkAdd(decStmnt(), previous(), "identifier", "after data type"));
                return true;
            }

            throw error(previous(), "Machine @context body must have a variable declaration");
        });
    }

    CST stateDec() {
        return tryParse("STATE_DEC", [&](auto node) {
            if (auto st = match(MAC_STATES_RESW)) {
                node->add(TerminalNode::make(st));

                auto equal = consume(EQUAL_ASS_OP, "=", "after '@states' keyword");
                node->add(TerminalNode::make(equal));

                auto leftCurly = consume(LEFT_CURLY_DELIM, "{", "before @states identifier list");
                node->add(TerminalNode::make(leftCurly));

                auto firstId = consume(STR_LITERAL, "string literal", "after '{' ");
                node->add(TerminalNode::make(firstId));

                while (auto comma = match(COMMA_OP)) {
                    node->add(TerminalNode::make(comma));

                    auto nextId = consume(STR_LITERAL, "string literal", "after ',' ");
                    node->add(TerminalNode::make(nextId));
                }

                auto rightCurly = consume(RIGHT_CURLY_DELIM, "}", "after @states identifier list");
                node->add(TerminalNode::make(rightCurly));

                auto sc = consume(SEMICOLON_DELIM, ";", "after @states declaration");
                node->add(TerminalNode::make(sc));
                return true;
            }
            throw error(previous(), "Machine must have @states declaration after @context");
        });
    }

    CST startDec() {
        return tryParse("START_DEC", [&](auto node) {
            if (auto st = match(MAC_START_RESW)) {
                node->add(TerminalNode::make(st));

                auto equal = consume(EQUAL_ASS_OP, "=", "after '@start' keyword");
                node->add(TerminalNode::make(equal));

                auto ident = consume(STR_LITERAL, "string literal", "after '=' ");
                node->add(TerminalNode::make(ident));

                auto sc = consume(SEMICOLON_DELIM, ";", "after @start declaration");
                node->add(TerminalNode::make(sc));
                return true;
            }
            throw error(previous(), "Machine must have @start declaration after @states");
        });
    }

    CST finalDec() {
        return tryParse("FINAL_DEC", [&](auto node) {
            if (auto final = match(MAC_FINAL_RESW)) {
                node->add(TerminalNode::make(final));

                auto equal = consume(EQUAL_ASS_OP, "=", "after '@final' keyword");
                node->add(TerminalNode::make(equal));

                auto leftCurly = consume(LEFT_CURLY_DELIM, "{", "before @final identifier list");
                node->add(TerminalNode::make(leftCurly));

                auto firstId = consume(STR_LITERAL, "string literal", "after '{' ");
                node->add(TerminalNode::make(firstId));

                while (auto comma = match(COMMA_OP)) {
                    node->add(TerminalNode::make(comma));

                    auto nextId = consume(STR_LITERAL, "string literal", "after ',' ");
                    node->add(TerminalNode::make(nextId));
                }

                auto rightCurly = consume(RIGHT_CURLY_DELIM, "}", "after @final identifier list");
                node->add(TerminalNode::make(rightCurly));

                auto sc = consume(SEMICOLON_DELIM, ";", "after @final declaration");
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

                auto equal = consume(EQUAL_ASS_OP, "=", "after '@transitions' keyword");
                node->add(TerminalNode::make(equal));

                auto leftCurly = consume(LEFT_CURLY_DELIM, "{", "after @transitions list");
                node->add(TerminalNode::make(leftCurly));

                if (auto trans = transition()) {
                    node->add(trans);
                }
                else {
                    node->add(EpsilonNode::make(peek()->line()));
                }

                while (auto trans = transition()) {
                    node->add(trans);
                }

                auto rightCurly = consume(RIGHT_CURLY_DELIM, "}", "after @transitions list");
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

                auto ident = consume(STR_LITERAL, "string literal", "after '(' ");
                node->add(TerminalNode::make(ident));

                auto comma = consume(COMMA_OP, ",", "after string literal");
                node->add(TerminalNode::make(comma));

                node->add(checkAdd(expression(), previous(), "expression", "after ',' "));

                auto rightParen = consume(RIGHT_PAREN_DELIM, ")", "after expression");
                node->add(TerminalNode::make(rightParen));

                auto equal = consume(EQUAL_ASS_OP, "=", "after ')'");
                node->add(TerminalNode::make(equal));

                auto output = consume(STR_LITERAL, "string literal", "after '=' ");
                node->add(TerminalNode::make(output));

                auto sc = consume(SEMICOLON_DELIM, ";", "after @transitions statement");
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

                node->add(checkAdd(id(), previous(), "identifier", "after '@state' keyword"));

                auto equal = consume(EQUAL_ASS_OP, "=", "after @state declaration");
                node->add(TerminalNode::make(equal));

                auto leftCurly = consume(LEFT_CURLY_DELIM, "{", "before @state function body");
                node->add(TerminalNode::make(leftCurly));

                node->add(body());

                auto rightCurly = consume(RIGHT_CURLY_DELIM, "}", "after @state function body");
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

                auto equal = consume(EQUAL_ASS_OP, "=", "after '@finalState' keyword");
                node->add(TerminalNode::make(equal));

                auto leftCurly = consume(LEFT_CURLY_DELIM, "{", "after @finalState function body");
                node->add(TerminalNode::make(leftCurly));

                node->add(body());

                auto rightCurly = consume(RIGHT_CURLY_DELIM, "}", "after @finalState function body");
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

                node->add(checkAdd(id(), previous(), "identifier", "after data type"));

                while (auto comma = match(COMMA_OP)) {
                    node->add(TerminalNode::make(comma));
                    node->add(checkAdd(id(), previous(), "identifier", "after ',' "));
                }

                auto sc = consume(SEMICOLON_DELIM, ";", "after machine instance declaration statement");
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

    CST tryParse(const std::string &nodeName, std::function<bool(Ref<NonTerminalNode>)> fn) {
        auto nonTermNode = NonTerminalNode::make(nodeName);

        try {
            if (!fn(nonTermNode)) {
                return nullptr;
            }
            return nonTermNode;
        }
        catch (ParseError &error) {
            if (error.nonTermNode) {
                nonTermNode->add(error.nonTermNode);
            }
            if (error.token) {
                auto errorNode = ErrorNode::make(error.token->line(), synchronize());
                error.errorNode = errorNode;
                nonTermNode->add(errorNode);
                error.token = nullptr;
            }

            error.nonTermNode = nonTermNode;
            throw;
        }
    }

    std::string foundToken(const Ref<Token> &tok) {
        return tok->type() != END_OF_FILE ? "'" + tok->lexeme() + "'" : "end of file";
    }

    Ref<Token> consume(TokenType type, const std::string_view &what, const std::string_view &context = "") {
        if (check(type)) {
            return advance();
        }

        std::string msg = "Expected '";
        msg += what;
        msg += "'";

        if (!context.empty()) {
            msg += " ";
            msg += context;
        }

        msg += " but found " + foundToken(peek());

        throw error(peek(), msg);
    }

    CST checkAdd(const CST &cst, Ref<Token> token, const std::string &what, const std::string &context = "") {
        if (!cst) {
            std::string msg = "Must have '" + what + "'";
            if (!context.empty()) {
                msg += " " + context;
            }

            throw error(token, msg);
        }
        return cst;
    }

    ParseError error(Ref<Token> token, std::string message) {
        skipComments();
        token = peek();

        if (message.back() != '.') {
            message += '.';
        }

        ::error(token, message);
        return ParseError(token);
    }

    template <typename... TokenType>
    Ref<Token> match(TokenType... types) {
        validateToken(peek());
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
            auto tok = m_tokens[m_current++];

            if (tok->type() == LEFT_CURLY_DELIM) {
                ++m_scopeDepth;
            }
            else if (tok->type() == RIGHT_CURLY_DELIM) {
                --m_scopeDepth;
            }
        }
        return previous();
    }

    bool isAtEnd() {
        return peek()->type() == END_OF_FILE;
    }

    Ref<Token> peek() {
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

        if (token->lexeme()[0] == '\"') {
            throw error(token, "Non terminated \" in a string literal");
        }
        if (token->lexeme()[0] == '\'') {
            throw error(token, "Non terminated ' in a character literal");
        }

        throw error(token, "Undefined identifier or keyword");
    }

    Ref<Token> synchronize() {
        skipComments();

        // Advance if synchronize is repeated on same token or it is unknown
        if (check(UNKNOWN) || m_lastSynchronize == m_current) {
            advance();
        }

        while (!isAtEnd()) {
            if (previous()->type() == SEMICOLON_DELIM || check(RIGHT_CURLY_DELIM) || isStatementStart()) {
                m_lastSynchronize = m_current;
                return peek();
            }

            advance();
        }

        return peek();
    }

    bool isStatementStart() {
        switch (peek()->type()) {
        case MACHINE_TYPE_RESW:
        case STRUCT_TYPE_RESW:
        case USER_MACHINE_RESW:
        case USER_STRUCT_RESW:
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
        case IDENTIFIER:
        case FOR_KEYW:
        case IF_RESW:
        case WHILE_KEYW:
        case RETURN_RESW:
            return true;
        default:
            return false;
            break;
        }
    }

    void skipComments() {
        // Skip all line comment and multiline comments
        while (check(LINE_COMNT) || check(MULTILINE_COMNT)) {
            advance();
        }
    }
};
