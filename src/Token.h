#pragma once

#include "TokenType.h"
#include "TokenUtils.h"

class Token {
public:
    Token(const std::string &lexeme, size_t line)
    : m_lexeme(lexeme), m_line(line) {}

    virtual TokenType type() = 0;
    virtual void setType(TokenType type) = 0;
    virtual std::string typeString() = 0;

    size_t line() const {
        return m_line;
    }

    const std::string &lexeme() const {
        return m_lexeme;
    }

protected:
    const std::string m_lexeme;
    const size_t m_line;
};

class DefToken : public Token {
public:
    DefToken(TokenType type, const std::string &lexeme, size_t line)
    : Token(lexeme, line), m_type(type) {}

    TokenType type() override {
        return m_type;
    }

    void setType(TokenType type) override {
        m_type = type;
    }

    std::string typeString() override {
        std::stringstream stream;
        stream << magic_enum::enum_name(m_type);
        return stream.str();
    }

private:
    TokenType m_type;
};

class UserToken : public Token {
public:
    UserToken(TokenType type, const std::string &lexeme, size_t line)
    : Token(lexeme, line), m_type(type) {}

    TokenType type() override {
        return m_type;
    }

    void setType(TokenType type) override {
        return;
    }

    std::string typeString() override {
        return TokenUtils::getTypeName(m_lexeme);
    }

private:
    const TokenType m_type;
};

using TokenRef = std::shared_ptr<Token>;
using TokenVec = std::vector<TokenRef>;
using TokenMap = std::unordered_map<std::string, TokenType>;
