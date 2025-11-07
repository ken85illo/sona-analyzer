#pragma once

#include "Token.h"
#include "TokenType.h"
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <unordered_map>
#include <vector>

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

    void outputToFile(const std::string &path) {
        std::ofstream output(path);

        if (!output.is_open()) {
            std::cout << "Unable to create file!\n";
            return;
        }

        std::cout << "\n";
        std::cout << std::left << std::setw(20) << "[LEXEME]" << "[TOKEN]" << "\n";
        output << "[LEXEME]," << "[TOKEN]\n";
        for (auto &token: m_tokens) {
            std::string tokenStr = token->toString();
            size_t npos = tokenStr.find(",");

            std::string first = tokenStr.substr(0, npos);
            std::string second = tokenStr.substr(npos + 1);

            std::cout << std::left << std::setw(20) << first << second << "\n";
            output << tokenStr << "\n";
        }
        std::cout << "\n";
    }

private:
    static const std::unordered_map<std::string, TokenType> s_specialWords;

    const std::string m_source;
    TokenVec m_tokens;

    uint32_t start = 0;
    uint32_t current = 0;

    void scanToken();

    void addToken(TokenType type) {
        std::string text = substring(start, current);
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
        while (isDigit(peek())) {
            advance();
        }

        if (peek() == '.' && isDigit(peekNext())) {
            advance();
            while (isDigit(peek())) {
                advance();
            }
        }

        addToken(INT_LITERAL);
    }

    void identifier() {
        while (isAlphaNumeric(peek())) {
            advance();
        }

        std::string text = substring(start, current);
        TokenType type = (s_specialWords.find(text) != s_specialWords.end()) ? s_specialWords.at(text) : IDENTIFIER;
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

    bool isDigit(char c) {
        return c >= '0' && c <= '9';
    }

    bool isAlpha(char c) {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
    }

    bool isAlphaNumeric(char c) {
        return isAlpha(c) || isDigit(c);
    }

    bool isValue(TokenType type) {
        return type == IDENTIFIER || type == INT_LITERAL || type == FLT_LITERAL;
    }

    bool isIdentifier(TokenType type) {
        return type == IDENTIFIER;
    }

    std::string substring(uint32_t start, uint32_t end) {
        return m_source.substr(start, current - start);
    }
};
