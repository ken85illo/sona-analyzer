#include "Scanner.h"

const std::unordered_map<std::string, TokenType> Scanner::s_specialWords = {
    {    "break",    BREAK },
    { "continue", CONTINUE },
    {      "for",      FOR },
    {       "do",       DO },
    {    "while",    WHILE },
    {      "int",      INT },
    {   "double",   DOUBLE },
    {    "float",    FLOAT },
    {     "char",     CHAR },
    {   "string",   STRING },
    {     "void",     VOID },
    {     "bool",     BOOL },
    {   "struct",   STRUCT },
    {     "enum",     ENUM },
    {    "const",    CONST },
    {   "static",   STATIC },
    { "unsigned", UNSIGNED },
    {   "return",   RETURN },
    {       "if",       IF },
    {     "else",     ELSE },
    {     "elif",     ELIF },
    {     "func",     FUNC }
};

void Scanner::scanToken() {
    char c = advance();

    switch (c) {
    case '(':
        addToken(LEFT_PAREN);
        break;
    case ')':
        addToken(RIGHT_PAREN);
        break;
    case '{':
        addToken(LEFT_BRACE);
        break;
    case '}':
        addToken(RIGHT_BRACE);
        break;
    case ',':
        addToken(COMMA);
        break;
    case '.':
        addToken(DOT);
        break;
    case ';':
        addToken(SEMICOLON);
        break;
    case '*':
        addToken(MULTIPLY);
        break;
    case '-':
        addToken(match('-') ? DECREMENT : MINUS);
        break;
    case '+':
        addToken(match('+') ? INCREMENT : PLUS);
        break;
    case '&':
        if (match('&')) {
            addToken(AND);
        }
        break;
    case '|':
        if (match('|')) {
            addToken(OR);
        }
        break;
    case '!':
        addToken(match('=') ? NOT_EQUAL : NOT);
        break;
    case '=':
        addToken(match('=') ? EQUAL_EQUAL : EQUAL);
        break;
    case '<':
        addToken(match('=') ? LESS_EQUAL : LESS);
        break;
    case '>':
        addToken(match('=') ? GREATER_EQUAL : GREATER);
        break;
    case '/': {
        if (!match('/')) {
            addToken(DIVIDE);
            break;
        }

        // Skip inline comments
        for (char p = peek(); p != '\n' && p != '\0'; p = peek()) {
            advance();
        }
        break;
    }
    case ' ':
    case '\r':
    case '\t':
    case '\n':
        break;
    case '"':
        string();
        break;
    default:
        if (isDigit(c)) {
            number();
        }
        else if (isAlpha(c)) {
            identifier();
        }
        else {
            addToken(UNKNOWN);
        }

        break;
    }
}
