#pragma once

#include "Token.h"
#include "TokenType.h"
#include "TokenUtils.h"

class Scanner {

    using TokenRef = std::shared_ptr<Token>;
    using TokenVec = std::vector<TokenRef>;
    using TokenSet = std::unordered_set<std::string>;
    using IdSet = std::unordered_set<std::string>;

public:
    Scanner(const std::string &source)
    : m_source(source) {}

    const TokenVec &scanTokens() {
        while (!isAtEnd()) {
            m_start = m_current;
            scanToken();
        }

        return m_tokens;
    }

    void output() {
        uint32_t currentLine = 0;
        for (auto &token: m_tokens) {
            if (currentLine != token->line()) {
                currentLine = token->line();
                std::cout << "[" << currentLine << "], ";
            }

            std::cout << token->toString() << ",";
        }
    }

private:
    const std::string m_source;
    TokenVec m_tokens;
    TokenSet m_userDefinedTokens;
    IdSet m_idsDefined;

    uint32_t m_start = 0;
    uint32_t m_current = 0;
    uint32_t m_line = 1;

    void scanToken();
    void string();
    void number();
    void word();
    void specWords();

    void addToken(TokenType type, uint32_t line = -1) {
        std::string text = TokenUtils::substring(m_start, m_current, m_source);
        m_tokens.push_back(std::make_shared<DefToken>(type, text, (line == -1) ? m_line : line));
    }

    void addUserDefinedToken() {
        std::string text = TokenUtils::substring(m_start, m_current, m_source);
        m_userDefinedTokens.insert(text);

        m_tokens.push_back(std::make_shared<UserToken>(text, m_line));
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

    TokenType lastToken() const {
        if (m_tokens.empty()) {
            return UNKNOWN;
        }

        if (auto token = std::dynamic_pointer_cast<DefToken>(m_tokens.back())) {
            return token->type();
        }
        return UNKNOWN;
    }

    bool unknownArithmetic() {
        if (TokenUtils::isArithmetic(peek())) {
            while (TokenUtils::isArithmetic(peek())) {
                advance();
            }
            addToken(UNKNOWN);
            return true;
        }
        return false;
    }

    void handleDouble(char expected, TokenType type) {
        if (match(expected)) {
            addToken(type);
        }
        else {
            addToken(UNKNOWN);
        }
    }

    void handlePlus() {
        if (match('=')) {
            addToken(ADD_ASS_OP);
            return;
        }

        TokenType type;

        if (match('+')) {
            type = TokenUtils::isIdentifier(lastToken()) ? POST_INCRMNT_OP : PRE_INCRMNT_OP;
        }
        else {
            type = (TokenUtils::isValue(lastToken()) || lastToken() == STR_LITERAL) ? ADD_OP : POSITIVE_OP;
        }

        if (!unknownArithmetic()) {
            addToken(type);
        }
    }

    void handleMinus() {
        if (match('=')) {
            addToken(SUBTRCT_ASS_OP);
            return;
        }

        TokenType type;

        if (match('-')) {
            type = TokenUtils::isIdentifier(lastToken()) ? POST_DECRMNT_OP : PRE_DECRMNT_OP;
        }
        else {
            type = TokenUtils::isValue(lastToken()) ? SUBTRACT_OP : NEGATIVE_OP;
        }

        if (!unknownArithmetic()) {
            addToken(type);
        }
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

    void singleLineComment() {
        for (char p = peek(); p != '\n' && p != '\0'; p = peek()) {
            advance();
        }
        addToken(LINE_COMNT);
    }

    void multiLineComment() {
        uint32_t currentLine = m_line;
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
};
