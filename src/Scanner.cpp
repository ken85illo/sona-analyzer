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
        handleMinus();
        break;
    case '+':
        handlePlus();
        break;
    case '&':
        handleDouble('&', AND_LOG_OP);
        break;
    case '|':
        handleDouble('|', OR_LOG_OP);
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
        handleSlash();
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
            word();
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

void Scanner::word() {
    while (TokenUtils::isAlphaNumeric(peek())) {
        advance();
    };

    std::string text = TokenUtils::substring(m_start, m_current, m_source);

    // Check for special Words, User defined types, and identifiers
    if (specialWords.find(text) != specialWords.end()) {
        addToken(specialWords.at(text));
    }
    else if (TokenUtils::isUserType(lastToken()) || m_userDefinedTokens.find(text) != m_userDefinedTokens.end()) {
        addUserDefinedToken();
    }
    else if (TokenUtils::isPrimitiveType(lastToken()) || m_idsDefined.find(text) != m_idsDefined.end()) {
        addIdentifier();
    }
    else {
        addToken(UNKNOWN);
    }
}
