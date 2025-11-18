#pragma once

#include "TokenType.h"
#include "TokenUtils.h"

class Token {
public:
    Token(const std::string &lexeme, uint32_t line)
    : m_lexeme(lexeme), m_line(line) {}

    std::string toString() {
        std::stringstream stream;

        stream << "#" << getTokenStr().length() << " " << getTokenStr() << ",#" << m_lexeme.length() << " " << m_lexeme;
        return stream.str();
    }

    uint32_t line() const {
        return m_line;
    }

protected:
    const std::string m_lexeme;
    const uint32_t m_line;

private:
    virtual std::string getTokenStr() = 0;
};

class DefToken : public Token {
public:
    DefToken(TokenType type, const std::string &lexeme, uint32_t line)
    : Token(lexeme, line), m_type(type) {}

    TokenType type() const {
        return m_type;
    }

private:
    std::string getTokenStr() override {
        std::stringstream stream;
        stream << magic_enum::enum_name(m_type);
        return stream.str();
    }

    const TokenType m_type;
};

class UserToken : public Token {
public:
    UserToken(const std::string &lexeme, uint32_t line)
    : Token(lexeme, line) {}

private:
    std::string getTokenStr() override {
        return TokenUtils::getTypeName(m_lexeme);
    }
};
