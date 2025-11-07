#include "Scanner.h"

const std::unordered_map<std::string, TokenType> Scanner::s_specialWords = {
    {    "break",       BREAK },
    { "continue",    CONTINUE },
    {      "for",         FOR },
    {       "do",          DO },
    {    "while",       WHILE },
    {      "int",    INT_TYPE },
    {   "double", DOUBLE_TYPE },
    {    "float",  FLOAT_TYPE },
    {     "char",   CHAR_TYPE },
    {   "string", STRING_TYPE },
    {     "void",        VOID },
    {     "bool",   BOOL_TYPE },
    {   "struct", STRUCT_TYPE },
    {     "enum",        ENUM },
    {    "const",       CONST },
    {   "static",      STATIC },
    { "unsigned",    UNSIGNED },
    {   "return",      RETURN },
    {       "if",          IF },
    {     "else",        ELSE },
    {     "elif",        ELIF },
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
        addToken(match('=') ? MULTPLY_ASS : MULTIPLY);
        break;
    case '%':
        addToken(match('=') ? MODULO_ASS : MODULO);
        break;
    case '-':
        if (peek() == '-') {
            advance();
            addToken(isIdentifier(m_tokens.back()->type()) ? POST_DECRMNT : PRE_DECRMNT);
        }
        else if (peek() == '=') {
            advance();
            addToken(SUBTRCT_ASS);
        }
        else {
            addToken(isValue(m_tokens.back()->type()) ? SUBTRACT : NEGATIVE);
        }
        break;
    case '+':
        if (peek() == '+') {
            advance();
            addToken(isIdentifier(m_tokens.back()->type()) ? POST_INCRMNT : PRE_DECRMNT);
        }
        else if (peek() == '=') {
            advance();
            addToken(ADD_ASS);
        }
        else {
            addToken(isValue(m_tokens.back()->type()) ? ADD : POSITIVE);
        }
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
        addToken(match('=') ? EQUAL_REL : EQUAL_ASS);
        break;
    case '<':
        addToken(match('=') ? LESS_EQUAL : LESS);
        break;
    case '>':
        addToken(match('=') ? GREATER_EQUAL : GREATER);
        break;
    case '/':
        if (peek() != '/') {
            addToken(match('=') ? DIVIDE_ASS : DIVIDE);
            break;
        }

        // Skip inline comments
        for (char p = peek(); p != '\n' && p != '\0'; p = peek()) {
            advance();
        }
        break;
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
