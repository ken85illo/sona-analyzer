#include "Scanner.h"
#include "SpecialWords.h"

void Scanner::scanToken() {
    char c = advance();

    switch (c) {
    case '(':
        addToken(LEFT_PAREN_DELIM);
        break;
    case ')':
        addToken(RIGHT_PAREN_DELIM);
        break;
    case '{':
        addToken(LEFT_CURLY_DELIM);
        break;
    case '}':
        addToken(RIGHT_CURLY_DELIM);
        break;
    case '[':
        addToken(LEFT_SQUARE_DELIM);
        break;
    case ']':
        addToken(RIGHT_SQUARE_DELIM);
        break;
    case ',':
        addToken(COMMA_OP);
        break;
    case '.':
        addToken(DOT_OP);
        break;
    case ';':
        addToken(SEMICOLON_DELIM);
        break;
    case '*':
        addToken(match('=') ? MULTPLY_ASS_OP : MULTIPLY_OP);
        break;
    case '%':
        addToken(match('=') ? MODULO_ASS_OP : MODULO_OP);
        break;
    case '-':
        if (peek() == '-') {
            advance();
            if (unknownArithmetic()) {
                break;
            }
            addToken(TokenUtils::isIdentifier(lastToken()) ? POST_DECRMNT_OP : PRE_DECRMNT_OP);
        }
        else if (peek() == '=') {
            advance();
            addToken(SUBTRCT_ASS_OP);
        }
        else {
            if (unknownArithmetic()) {
                break;
            }
            addToken(TokenUtils::isValue(lastToken()) ? SUBTRACT_OP : NEGATIVE_OP);
            break;
        }
        break;
    case '+':
        if (peek() == '+') {
            advance();
            if (unknownArithmetic()) {
                break;
            }
            addToken(TokenUtils::isIdentifier(lastToken()) ? POST_INCRMNT_OP : PRE_DECRMNT_OP);
        }
        else if (peek() == '=') {
            advance();
            addToken(ADD_ASS_OP);
        }
        else {
            if (unknownArithmetic()) {
                break;
            }
            addToken(TokenUtils::isValue(lastToken()) ? ADD_OP : POSITIVE_OP);
        }
        break;
    case '&':
        if (match('&')) {
            addToken(AND_LOG_OP);
        }
        break;
    case '|':
        if (match('|')) {
            addToken(OR_LOG_OP);
        }
        break;
    case '!':
        addToken(match('=') ? NOT_EQUAL_REL_OP : NOT_LOG_OP);
        break;
    case '=':
        addToken(match('=') ? EQUAL_REL_OP : EQUAL_ASS_OP);
        break;
    case '<':
        addToken(match('=') ? LESS_EQUAL_REL_OP : LESS_REL_OP);
        break;
    case '>':
        addToken(match('=') ? GREATER_EQUAL_REL_OP : GREATER_REL_OP);
        break;
    case '/':
        if (peek() == '/') {
            // Add line comment token
            for (char p = peek(); p != '\n' && p != '\0'; p = peek()) {
                advance();
            }
            addToken(LINE_COMNT);
        }
        else if (peek() == '*') {
            // Add multiline comment token

            // skip the first *
            advance();
            for (char p = peek(), pn = peekNext(); p != '*' && pn != '/'; p = peek(), pn = peekNext()) {
                advance();
            }

            // skip the remaining *  and /
            advance();
            advance();

            addToken(MULTILINE_COMNT);
        }
        else {
            addToken(match('=') ? DIVIDE_ASS_OP : DIVIDE_OP);
        }

    case ' ':
    case '\t':
        break;
    case '\r':
    case '\n':
        ++m_line;
        break;
    case '"':
        string();
        break;
    default:
        if (TokenUtils::isDigit(c)) {
            number();
        }
        else if (TokenUtils::isAlpha(c) || c == '@') {
            identifier();
        }
        else {
            addToken(UNKNOWN);
        }

        break;
    }
}

void Scanner::string() {
    for (char p = peek(); p != '"' && p != '\0'; p = peek()) {
        advance();
    }

    // Skip the last double quote
    advance();
    addToken(STR_LITERAL);
}

void Scanner::number() {
    TokenType type = INT_LITERAL;

    while (TokenUtils::isDigit(peek())) {
        advance();
    }

    if (peek() == '.' && TokenUtils::isDigit(peekNext())) {
        advance();
        while (TokenUtils::isDigit(peek())) {
            advance();
        }
        type = FLT_LITERAL;
    }

    addToken(type);
}

void Scanner::identifier() {
    while (TokenUtils::isAlphaNumeric(peek())) {
        advance();
    }

    // When we declare a machine it must be added to user defined token set
    if (lastToken() == MACHINE_TYPE_RESW || lastToken() == STRUCT_TYPE_RESW) {
        addUserDefinedToken();
        return;
    }

    std::string text = TokenUtils::substring(m_start, m_current, m_source);

    // Check if a type is user defined
    if (m_userDefinedTokens.find(text) != m_userDefinedTokens.end()) {
        addUserDefinedToken();
        return;
    }

    TokenType type = (specialWords.find(text) != specialWords.end()) ? specialWords.at(text) : IDENTIFIER;
    addToken(type);
}

bool Scanner::unknownArithmetic() {
    if (TokenUtils::isArithmetic(peek())) {
        while (TokenUtils::isArithmetic(peek())) {
            advance();
        }
        addToken(UNKNOWN);
        return true;
    }
    return false;
}
