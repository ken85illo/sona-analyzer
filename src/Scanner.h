#pragma once

#include "Token.h"
#include "TokenType.h"
#include "TokenUtils.h"

class Scanner {

    using TokenRef = std::shared_ptr<Token>;
    using TokenVec = std::vector<TokenRef>;
    using TokenSet = std::unordered_set<std::string>;

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
    void string();
    void number();
    void identifier();
    bool unknownArithmetic();

    void addToken(TokenType type) {
        std::string text = TokenUtils::substring(m_start, m_current, m_source);
        m_tokens.push_back(std::make_shared<DefToken>(type, text, m_line));
    }

    void addUserDefinedToken() {
        using namespace std::literals::string_literals;

        std::string text = TokenUtils::substring(m_start, m_current, m_source);
        m_userDefinedTokens.insert(text);

        m_tokens.push_back(std::make_shared<UserToken>(text, m_line));
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
};
