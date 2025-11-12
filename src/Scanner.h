#pragma once

#include "SpecialWords.h"
#include "Token.h"
#include "TokenType.h"
#include "TokenUtils.h"

class Scanner {

    using TokenRef = std::shared_ptr<Token>;
    using TokenVec = std::vector<TokenRef>;
    using TokenSet = std::set<std::string>;

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

    uint32_t m_start = 0;
    uint32_t m_current = 0;
    uint32_t m_line = 1;

    void scanToken();

    void addToken(TokenType type) {
        std::string text = TokenUtils::substring(m_start, m_current, m_source);
        m_tokens.emplace_back(new Token(type, text, m_line));
    }

    void addUserDefinedToken() {
        using namespace std::literals::string_literals;

        std::string text = TokenUtils::substring(m_start, m_current, m_source);
        std::string type = TokenUtils::getTypeName(text);

        m_userDefinedTokens.insert(type);
        m_tokens.emplace_back(new Token(type, text, m_line));
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

    void identifier() {
        while (TokenUtils::isAlphaNumeric(peek())) {
            advance();
        }

        // When we declare a machine it must be added to user defined token set
        if (!m_tokens.empty() && lastToken() == MACHINE_TYPE) {
            addUserDefinedToken();
            return;
        }

        std::string text = TokenUtils::substring(m_start, m_current, m_source);

        // Check if a type is user defined
        if (m_userDefinedTokens.find(TokenUtils::getTypeName(text)) != m_userDefinedTokens.end()) {
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
        if (m_tokens.back()->type().has_value()) {
            return *m_tokens.back()->type();
        }
        return UNKNOWN;
    }
};
