#pragma once

#include "SpecialWords.h"
#include "StringUtils.h"
#include "Token.h"
#include "TokenType.h"

class Scanner {
    using TokenRef = std::shared_ptr<Token>;
    using TokenVec = std::vector<TokenRef>;

public:
    Scanner(const std::string &source)
    : m_source(source) {}

    const TokenVec &scanTokens() {
        while (!isAtEnd()) {
            start = current;
            scanToken();
        }

        return m_tokens;
    }

    void output() {
        for (auto &token: m_tokens) {
            std::cout << token->toString() << ",";
        }
    }

private:
    const std::string m_source;
    TokenVec m_tokens;

    uint32_t start = 0;
    uint32_t current = 0;

    void scanToken();

    void addToken(TokenType type) {
        std::string text = StringUtils::substring(start, current, m_source);
        m_tokens.emplace_back(new Token(type, text));
    }

    bool isAtEnd() {
        return current >= m_source.length();
    }

    bool match(char expected) {
        if (isAtEnd() || m_source[current] != expected) {
            return false;
        }

        ++current;
        return true;
    }

    void string() {
        for (char p = peek(); p != '"' && p != '\0'; p = peek()) {
            advance();
        }

        // Skip the last double quote
        advance();
        addToken(STR_LITERAL);
    }

    void number() {
        TokenType type = INT_LITERAL;

        while (StringUtils::isDigit(peek())) {
            advance();
        }

        if (peek() == '.' && StringUtils::isDigit(peekNext())) {
            advance();
            while (StringUtils::isDigit(peek())) {
                advance();
            }
            type = FLT_LITERAL;
        }

        addToken(type);
    }

    void identifier() {
        while (StringUtils::isAlphaNumeric(peek())) {
            advance();
        }

        std::string text = StringUtils::substring(start, current, m_source);
        TokenType type = (specialWords.find(text) != specialWords.end()) ? specialWords.at(text) : IDENTIFIER;
        addToken(type);
    }

    char peek() {
        if (isAtEnd()) {
            return '\0';
        }
        return m_source[current];
    }

    char peekNext() {
        if (current + 1 >= m_source.length()) {
            return '\0';
        }
        return m_source[current + 1];
    }

    char advance() {
        return m_source[current++];
    }
};
