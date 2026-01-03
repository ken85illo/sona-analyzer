#pragma once

#include "TokenType.h"
#include "TokenUtils.h"

class Token {
public:
    Token(const std::string &lexeme, uint32_t line)
    : m_lexeme(lexeme), m_line(line) {}

    virtual std::string token() = 0;
    virtual TokenType type() = 0;
    virtual void setType(TokenType type) = 0;

    uint32_t line() const {
        return m_line;
    }

    const std::string &lexeme() const {
        return m_lexeme;
    }

protected:
    const std::string m_lexeme;
    const uint32_t m_line;
};

class DefToken : public Token {
public:
    DefToken(TokenType type, const std::string &lexeme, uint32_t line)
    : Token(lexeme, line), m_type(type) {}

    TokenType type() override {
        return m_type;
    }

    void setType(TokenType type) override {
        m_type = type;
    }

private:
    std::string token() override {
        std::stringstream stream;
        stream << magic_enum::enum_name(m_type);
        return stream.str();
    }

    TokenType m_type;
};

class UserToken : public Token {
public:
    UserToken(const std::string &lexeme, uint32_t line)
    : Token(lexeme, line) {}

    TokenType type() override {
        return USER_DEFINED_RESW;
    }

    void setType(TokenType type) override {
        return;
    }

private:
    std::string token() override {
        return TokenUtils::getTypeName(m_lexeme);
    }
};

using TokenRef = std::shared_ptr<Token>;
using TokenVec = std::vector<TokenRef>;
using TokenSet = std::unordered_set<std::string>;
