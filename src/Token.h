#pragma once

#include <sstream>
#include <string.h>

class Token {
public:
    Token(const std::string &type, const std::string &lexeme)
    : m_type(type), m_lexeme(lexeme) {}

    std::string toString() const {
        std::stringstream stream;
        stream << m_type << ", " << m_lexeme;
        return stream.str();
    }

private:
    const std::string m_type;
    const std::string m_lexeme;
};
