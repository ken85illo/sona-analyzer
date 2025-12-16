#pragma once

#include "TokenType.h"
#include "TokenUtils.h"

class Token {
public:
    Token(const std::string &lexeme, uint32_t line)
    : m_lexeme(lexeme), m_line(line) {}

    virtual std::string token() = 0;

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

    TokenType type() const {
        return m_type;
    }

    void setType(TokenType type) {
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

private:
    std::string token() override {
        return TokenUtils::getTypeName(m_lexeme);
    }
};
