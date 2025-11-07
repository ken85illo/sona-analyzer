#pragma once

#include <TokenType.h>
#include <magic_enum/magic_enum.hpp>
#include <sstream>
#include <string.h>

class Token {
public:
    Token(TokenType type, const std::string &lexeme)
    : m_type(type), m_lexeme(lexeme) {}

    std::string toString() const {
        std::stringstream stream;
        stream << magic_enum::enum_name(m_type) << ", " << m_lexeme;
        return stream.str();
    }

    TokenType type() const {
        return m_type;
    }

private:
    const TokenType m_type;
    const std::string m_lexeme;
};
