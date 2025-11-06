#include "Scanner.h"

const std::unordered_map<std::string, std::string> Scanner::s_specialWords = {
    {    "break",    STRINGIFY(BREAK) },
    { "continue", STRINGIFY(CONTINUE) },
    {      "for",      STRINGIFY(FOR) },
    {       "do",       STRINGIFY(DO) },
    {    "while",    STRINGIFY(WHILE) },
    {      "int",      STRINGIFY(INT) },
    {   "double",   STRINGIFY(DOUBLE) },
    {    "float",    STRINGIFY(FLOAT) },
    {     "char",     STRINGIFY(CHAR) },
    {   "string",   STRINGIFY(STRING) },
    {     "void",     STRINGIFY(VOID) },
    {     "bool",     STRINGIFY(BOOL) },
    {   "struct",   STRINGIFY(STRUCT) },
    {     "enum",     STRINGIFY(ENUM) },
    {    "const",    STRINGIFY(CONST) },
    {   "static",   STRINGIFY(STATIC) },
    { "unsigned", STRINGIFY(UNSIGNED) },
    {   "return",   STRINGIFY(RETURN) },
    {       "if",       STRINGIFY(IF) },
    {     "else",     STRINGIFY(ELSE) },
    {     "elif",     STRINGIFY(ELIF) },
    {     "func",     STRINGIFY(FUNC) }
};

void Scanner::scanToken() {
    char c = advance();

    switch (c) {
    case '(':
        ADD_TOKEN(LEFT_PAREN);
        break;
    case ')':
        ADD_TOKEN(RIGHT_PAREN);
        break;
    case '{':
        ADD_TOKEN(LEFT_BRACE);
        break;
    case '}':
        ADD_TOKEN(RIGHT_BRACE);
        break;
    case ',':
        ADD_TOKEN(COMMA);
        break;
    case '.':
        ADD_TOKEN(DOT);
        break;
    case ';':
        ADD_TOKEN(SEMICOLON);
        break;
    case '*':
        ADD_TOKEN(MULTIPLY);
        break;
    case '-':
        ADD_TOKEN_TERN(match('-'), DECREMENT, MINUS);
        break;
    case '+':
        ADD_TOKEN_TERN(match('+'), INCREMENT, PLUS);
        break;
    case '&':
        if (match('&')) {
            ADD_TOKEN(AND);
        }
        break;
    case '|':
        if (match('|')) {
            ADD_TOKEN(OR);
        }
        break;
    case '!':
        ADD_TOKEN_TERN(match('='), NOT_EQUAL, NOT);
        break;
    case '=':
        ADD_TOKEN_TERN(match('='), EQUAL_EQUAL, EQUAL);
        break;
    case '<':
        ADD_TOKEN_TERN(match('='), LESS_EQUAL, LESS);
        break;
    case '>':
        ADD_TOKEN_TERN(match('='), GREATER_EQUAL, GREATER);
        break;
    case '/': {
        if (!match('/')) {
            ADD_TOKEN(DIVIDE);
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
            ADD_TOKEN(UNKNOWN);
        }

        break;
    }
}
