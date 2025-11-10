#pragma once

#include "SpecialWords.h"
#include "StringUtils.h"
#include "Token.h"
#include "TokenType.h"
#include <set>

class Scanner {

    using TokenRef = std::shared_ptr<Token>;
    using TokenVec = std::vector<TokenRef>;
    using TokenSet = std::set<std::string>;

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
    TokenSet m_userDefinedTokens;

    uint32_t start = 0;
    uint32_t current = 0;

    void scanToken();

    void addToken(TokenType type) {
        std::string text = StringUtils::substring(start, current, m_source);
        m_tokens.emplace_back(new Token(type, text));
    }

    void addUserDefinedToken() {
        using namespace std::literals::string_literals;

        std::string text = StringUtils::substring(start, current, m_source);
        std::string type = text;

        std::transform(type.begin(), type.end(), type.begin(), [](auto c) {
            return std::toupper(c);
        });

        type += "_TYPE"s;

        m_userDefinedTokens.insert(type);
        m_tokens.emplace_back(new Token(type, text));
    }

    bool isAtEnd() const {
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

        if (!m_tokens.empty() && lastToken() == MACHINE_TYPE) {
            addUserDefinedToken();
            return;
        }

        TokenType type = (specialWords.find(text) != specialWords.end()) ? specialWords.at(text) : IDENTIFIER;
        addToken(type);
    }

    char peek() const {
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

    TokenType lastToken() const {
        if (m_tokens.back()->type().has_value()) {
            return *m_tokens.back()->type();
        }
        return UNKNOWN;
    }
};
