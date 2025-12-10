#pragma once

#include "Token.h"
#include "TokenType.h"
#include "TokenUtils.h"

class Scanner {

    using TokenRef = std::shared_ptr<Token>;
    using Indices = std::stack<uint32_t>;
    using TokenVec = std::vector<TokenRef>;
    using TokenSet = std::unordered_set<std::string>;
    using IdSet = std::unordered_set<std::string>;

public:
    Scanner(const std::string &source)
    : m_source(source) {
        m_idsDefined.insert("readChar");
        m_idsDefined.insert("readLine");
        m_idsDefined.insert("print");
        m_idsDefined.insert("parseInt");
        m_idsDefined.insert("parseString");
        m_idsDefined.insert("exit");
    }

    const TokenVec &scanTokens() {
        while (!isAtEnd()) {
            m_start = m_current;
            scanToken();
        }
        finalCheck();

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
    Indices m_binaryOps, m_sUnaryOps, m_dUnaryOps, m_assOps;
    TokenVec m_tokens;
    TokenSet m_userDefinedTokens;
    IdSet m_idsDefined;

    uint32_t m_start = 0;
    uint32_t m_current = 0;
    uint32_t m_line = 1;

    void scanToken();
    void string();
    void number();
    void specWords();
    void word();

    void finalCheck() {
        // Backtrack checking

        // Check if op is surounded by value "1 + 1"
        checkOps(m_binaryOps, [&](TokenType first, TokenType last) {
            return TokenUtils::isValue(first) && TokenUtils::isValue(last);
        });

        // Checkk if op is followed by value "+1"
        checkOps(m_sUnaryOps, [&](TokenType, TokenType next) {
            return TokenUtils::isValue(next);
        });

        // Check if identifier is preceeded or proceeded by op "++x or x++"
        checkOps(m_dUnaryOps, [&](TokenType first, TokenType last) {
            return TokenUtils::isIdentifier(first) || TokenUtils::isIdentifier(last);
        });

        // Check if if op is surrounded by identifier and value "x = 1"
        checkOps(m_assOps, [&](TokenType first, TokenType last) {
            return TokenUtils::isIdentifier(first) && TokenUtils::isValue(last);
        });
    }

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
            pushOp(m_assOps);
            return;
        }

        TokenType type = UNKNOWN;

        if (match('+')) {
            addToken(TokenUtils::isIdentifier(lastToken()) ? POST_INCRMNT_OP : PRE_INCRMNT_OP);
            pushOp(m_dUnaryOps);
        }
        else {
            addToken((TokenUtils::isValue(lastToken()) || lastToken() == STR_LITERAL) ? ADD_OP : POSITIVE_OP);
            lastToken() == ADD_OP ? pushOp(m_binaryOps) : pushOp(m_sUnaryOps);
        }
    }

    void handleMinus() {
        if (match('=')) {
            addToken(SUBTRCT_ASS_OP);
            pushOp(m_assOps);
            return;
        }

        TokenType type;

        if (match('-')) {
            addToken(TokenUtils::isIdentifier(lastToken()) ? POST_DECRMNT_OP : PRE_DECRMNT_OP);
            pushOp(m_dUnaryOps);
        }
        else {
            addToken(TokenUtils::isValue(lastToken()) ? SUBTRACT_OP : NEGATIVE_OP);
            lastToken() == SUBTRACT_OP ? pushOp(m_binaryOps) : pushOp(m_sUnaryOps);
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
            lastToken() == DIVIDE_ASS_OP ? pushOp(m_assOps) : pushOp(m_binaryOps);
        }
    }

    void handleAsterisk() {
        addToken(match('=') ? MULTPLY_ASS_OP : MULTIPLY_OP);
        lastToken() == MULTPLY_ASS_OP ? pushOp(m_assOps) : pushOp(m_binaryOps);
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

    void pushOp(Indices &stack) {
        stack.push(m_tokens.size() - 1);
    }

    void checkOps(Indices &stack, const std::function<bool(TokenType first, TokenType last)> &condition) {
        auto getToken = [&](int32_t index) {
            return std::dynamic_pointer_cast<DefToken>(m_tokens[index]);
        };

        while (!stack.empty()) {
            int32_t index = stack.top();
            auto current = getToken(index);

            if (index - 1 < 0 || index + 1 >= m_tokens.size()) {
                current->setType(UNKNOWN);
                stack.pop();
                continue;
            }

            auto first = getToken(index - 1);
            auto last = getToken(index + 1);

            if (!first || !last) {
                current->setType(UNKNOWN);
                stack.pop();
                continue;
            }

            if (!condition(first->type(), last->type())) {
                current->setType(UNKNOWN);
            }
            stack.pop();
        }
    }

    bool userDefIdentifier() {
        if (m_tokens.empty()) {
            return false;
        }
        return m_userDefinedTokens.find(m_tokens.back()->lexeme()) != m_userDefinedTokens.end() ||
               lastToken() == MAC_STATE_RESW;
    }
};
