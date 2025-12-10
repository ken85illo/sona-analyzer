#pragma once

#include "TokenType.h"

namespace TokenUtils {

inline bool isDigit(char c) {
    return c >= '0' && c <= '9';
}

inline bool isAlpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

inline bool isAlphaNumeric(char c) {
    return isAlpha(c) || isDigit(c);
}

inline bool isValue(TokenType type) {
    return type == IDENTIFIER || type == INT_LITERAL || type == FLT_LITERAL || type == POST_INCRMNT_OP ||
           type == POST_DECRMNT_OP;
}

inline bool isUserType(TokenType type) {
    return type == MACHINE_TYPE_RESW || type == STRUCT_TYPE_RESW;
}

inline bool isPrimitiveType(TokenType type) {
    return type == BOOL_TYPE_RESW || type == CHAR_TYPE_RESW || type == DOUBLE_TYPE_RESW || type == FLOAT_TYPE_RESW ||
           type == INT_TYPE_RESW || type == VOID_TYPE_RESW || type == STRING_TYPE_RESW;
}

inline bool isIdentifier(TokenType type) {
    return type == IDENTIFIER;
}

inline bool isArithmetic(char c) {
    return c == '+' || c == '-' || c == '*' || c == '/';
}

inline std::string substring(uint32_t start, uint32_t end, const std::string &text) {
    return text.substr(start, end - start);
}

inline std::string getTypeName(std::string type) {
    using namespace std::literals::string_literals;

    std::transform(type.begin(), type.end(), type.begin(), [](auto c) {
        return std::toupper(c);
    });

    type += "_TYPE_RESW"s;

    return type;
}

} // namespace TokenUtils
