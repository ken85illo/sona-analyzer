#pragma once

#include "TokenType.h"

class Token {
public:
    Token(TokenType type, const std::string &lexeme)
    : m_type(type), m_lexeme(lexeme) {}

    std::string toString() const {
        std::stringstream stream;

        auto enumStr = magic_enum::enum_name(m_type);

        stream << "#" << enumStr.length() << " " << enumStr << ",#" << m_lexeme.length() << " " << m_lexeme;
        return stream.str();
    }

    TokenType type() const {
        return m_type;
    }

private:
    const TokenType m_type;
    const std::string m_lexeme;
};
