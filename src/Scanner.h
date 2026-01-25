#pragma once

#include "Token.h"
#include "TokenType.h"
#include "TokenUtils.h"
#include "TokenWords.h"

class Scanner {
    using IdSet = std::unordered_set<std::string>;

public:
    Scanner(const std::string &source)
    : m_source(source) {}

    const TokenVec &scanTokens() {
        while (!isAtEnd()) {
            m_start = m_current;
            scanToken();
        }
        m_tokens.push_back(std::make_shared<DefToken>(END_OF_FILE, R"(\0)", m_line - 1));

        return m_tokens;
    }

    ordered_json output() {
        size_t currentLine = 0;
        ordered_json out;

        for (auto &elem: m_tokens) {
            if (currentLine != elem->line()) {
                currentLine = elem->line();
            }

            out[std::to_string(currentLine)].push_back(
                {
                    {  "token", elem->typeString() },
                    { "lexeme",     elem->lexeme() }
            }
            );
        }
        return out;
    }

private:
    const std::string m_source;

    TokenVec m_tokens;
    TokenMap m_userDefinedTokens;
    IdSet m_idsDefined{ "readChar", "readLine", "print", "parseInt", "parseString", "exit" };

    size_t m_start = 0;
    size_t m_current = 0;
    size_t m_line = 1;

    void scanToken() {
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

    void string() {
        while (peek() != '"' && peek() != '\0' && peek() != '\n') {
            advance();
        }

        if (peek() != '"') {
            addToken(UNKNOWN);
            return;
        }

        // Skip the last double quote
        advance();
        addToken(STR_LITERAL);
    }

    void character() {
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

    void number() {
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

    void word() {
        while (TokenUtils::isAlphaNumeric(peek())) {
            advance();
        };

        std::string text = TokenUtils::substring(m_start, m_current, m_source);

        auto prevToken = lastToken();
        bool isSpecialWord = specialWords.contains(text);
        bool isUserDefinedType = TokenUtils::isUserType(prevToken) || m_userDefinedTokens.contains(text);
        bool isIdentifier = TokenUtils::isPrimitiveType(prevToken) || userDefIdentifier() ||
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

    void addToken(TokenType type, std::optional<size_t> line = std::nullopt) {
        std::string text = TokenUtils::substring(m_start, m_current, m_source);
        m_tokens.push_back(std::make_shared<DefToken>(type, text, line.value_or(m_line)));
    }

    void addUserDefinedToken(TokenType previous) {
        std::string text = TokenUtils::substring(m_start, m_current, m_source);

        if (!m_userDefinedTokens.contains(text)) {
            TokenType type = (previous == STRUCT_TYPE_RESW) ? USER_STRUCT_RESW : USER_MACHINE_RESW;
            m_userDefinedTokens[text] = type;
        }

        m_tokens.push_back(std::make_shared<UserToken>(m_userDefinedTokens[text], text, m_line));
    }

    void addIdentifier() {
        std::string text = TokenUtils::substring(m_start, m_current, m_source);
        m_idsDefined.insert(text);

        m_tokens.push_back(std::make_shared<DefToken>(IDENTIFIER, text, m_line));
    }

    bool isAtEnd() const {
        return m_current >= m_source.length();
    }

    bool match(char expected) {
        if (isAtEnd() || m_source[m_current] != expected) {
            return false;
        }

        ++m_current;
        return true;
    }

    char peek() const {
        if (isAtEnd()) {
            return '\0';
        }
        return m_source[m_current];
    }

    char peekNext() {
        if (m_current + 1 >= m_source.length()) {
            return '\0';
        }
        return m_source[m_current + 1];
    }

    char advance() {
        return m_source[m_current++];
    }

    TokenType lastToken(size_t back = 0) const {
        if (m_tokens.size() <= back) {
            return UNKNOWN;
        }

        auto token = m_tokens[m_tokens.size() - 1 - back];
        return token->type();
    }

    void handleDouble(char expected, TokenType type) {
        if (match(expected)) {
            addToken(type);
        }
        else {
            addToken(UNKNOWN);
        }
    }

    void
    handleSignedOp(char symbol, TokenType binary, TokenType unary, TokenType post, TokenType pre, TokenType assign) {
        if (match('=')) {
            addToken(assign);
            return;
        }

        TokenType prev = lastToken();

        if (match(symbol)) {
            addToken(TokenUtils::isIdentifier(prev) ? post : pre);
            return;
        }

        bool isBinary = TokenUtils::isValue(prev) || prev == STR_LITERAL;
        addToken(isBinary ? binary : unary);
    }

    void handlePlus() {
        handleSignedOp('+', ADD_OP, POSITIVE_OP, POST_INCRMNT_OP, PRE_INCRMNT_OP, ADD_ASS_OP);
    }

    void handleMinus() {
        handleSignedOp('-', SUBTRACT_OP, NEGATIVE_OP, POST_DECRMNT_OP, PRE_DECRMNT_OP, SUBTRCT_ASS_OP);
    }

    void handleWhitespace(char c) {
        if (c == '\n') {
            ++m_line;
        }
    }

    void handleSlash() {
        if (peek() == '/') {
            // Add line comment token
            singleLineComment();
        }
        else if (peek() == '*') {
            // Add multiline comment token
            multiLineComment();
        }
        else {
            addToken(match('=') ? DIVIDE_ASS_OP : DIVIDE_OP);
        }
    }

    void handleAsterisk() {
        addToken(match('=') ? MULTPLY_ASS_OP : MULTIPLY_OP);
    }

    void singleLineComment() {
        while (peek() != '\n' && peek() != '\0') {
            advance();
        }
        addToken(LINE_COMNT);
    }

    void multiLineComment() {
        size_t currentLine = m_line;
        while (!(peek() == '*' && peekNext() == '/') && !isAtEnd()) {
            if (peek() == '\n') {
                ++m_line;
            }
            advance();
        }
        // consume the last "*/"
        if (!isAtEnd()) {
            advance();
            advance();
        }
        addToken(MULTILINE_COMNT, currentLine);
    }

    bool userDefIdentifier() {
        if (m_tokens.empty()) {
            return false;
        }
        return m_userDefinedTokens.contains(m_tokens.back()->lexeme()) || lastToken() == MAC_STATE_RESW;
    }

    bool validIdentifier() {
        const auto t0 = lastToken();
        const auto t1 = lastToken(1);
        const auto t2 = lastToken(2);
        const auto t3 = lastToken(3);

        return (t1 == IDENTIFIER || (t3 == IDENTIFIER && t2 == EQUAL_ASS_OP && TokenUtils::isValue(t1))) &&
               t0 == COMMA_OP;
    }
};
