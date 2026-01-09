#include "Scanner.h"
#include "TokenWords.h"

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
        handleAsterisk();
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
    case '\'':
        character();
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
    while (peek() != '"' && peek() != '\0') {
        advance();
    }

    // Skip the last double quote
    advance();
    addToken(STR_LITERAL);
}

void Scanner::character() {
    if (TokenUtils::isAlphaNumeric(peek())) {
        // Consume char
        advance();
        if (match('\'')) {
            addToken(CHAR_LITERAL);
            return;
        }
    }

    addToken(UNKNOWN);
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

    const auto prevToken = lastToken();

    const bool isSpecialWord = specialWords.contains(text);

    const bool isUserDefinedType = TokenUtils::isUserType(prevToken) || m_userDefinedTokens.contains(text);

    const bool isIdentifier = TokenUtils::isPrimitiveType(prevToken) || userDefIdentifier() ||
                              m_idsDefined.contains(text) || validIdentifier();

    if (isSpecialWord) {
        addToken(specialWords.at(text));
    }
    else if (isUserDefinedType) {
        addUserDefinedToken(prevToken);
    }
    else if (isIdentifier) {
        addIdentifier();
    }
    else {
        addToken(UNKNOWN);
    }
}
