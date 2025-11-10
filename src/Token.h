#pragma once

#include "TokenType.h"

class Token {
public:
    Token(TokenType type, const std::string &lexeme, uint32_t line)
    : m_type(type), m_userDefinedType(std::nullopt), m_lexeme(lexeme), m_line(line) {}

    Token(const std::string &type, const std::string &lexeme, uint32_t line)
    : m_type(std::nullopt), m_userDefinedType(type), m_lexeme(lexeme), m_line(line) {}

    std::string toString() const {
        std::stringstream stream;

        std::stringstream enumStr;
        enumStr << ((m_type) ? magic_enum::enum_name(*m_type) : *m_userDefinedType);

        stream << "#" << enumStr.str().length() << " " << enumStr.str() << ",#" << m_lexeme.length() << " " << m_lexeme;
        return stream.str();
    }

    const std::optional<TokenType> &type() const {
        return m_type;
    }

    const std::optional<std::string> &userDefinedType() const {
        return m_userDefinedType;
    }

    uint32_t line() const {
        return m_line;
    }

private:
    const std::optional<TokenType> m_type;
    const std::optional<std::string> m_userDefinedType;
    const std::string m_lexeme;
    const uint32_t m_line;
};
