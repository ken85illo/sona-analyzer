#pragma once

enum TokenType {
    // Single-character tokens
    LEFT_PAREN,
    RIGHT_PAREN,
    LEFT_CURLY,
    RIGHT_CURLY,
    LEFT_SQUARE,
    RIGHT_SQUARE,
    COMMA,
    DOT,
    SEMICOLON,

    // Unary Operator
    NEGATIVE,
    POSITIVE,
    PRE_DECRMNT,
    PRE_INCRMNT,
    POST_INCRMNT,
    POST_DECRMNT,

    // Binary Operator
    SUBTRACT,
    ADD,
    DIVIDE,
    MULTIPLY,
    MODULO,

    // Logical Operators
    AND,
    OR,
    NOT,

    // Relational Operators
    NOT_EQUAL,
    EQUAL_REL,
    GREATER,
    GREATER_EQUAL,
    LESS,
    LESS_EQUAL,

    // Assignment Operators
    EQUAL_ASS,
    ADD_ASS,
    SUBTRCT_ASS,
    MULTPLY_ASS,
    DIVIDE_ASS,
    MODULO_ASS,

    // Literals
    IDENTIFIER,
    STR_LITERAL,
    INT_LITERAL,
    FLT_LITERAL,

    // Keywords
    BREAK,
    CONTINUE,
    DO,
    FOR,
    WHILE,

    // Reserved Words
    BOOL_TYPE,
    CHAR_TYPE,
    CONST,
    DOUBLE_TYPE,
    ELIF,
    ELSE,
    ENUM,
    FLOAT_TYPE,
    IF,
    INT_TYPE,
    MACHINE_TYPE,
    RETURN,
    STATIC,
    STRING_TYPE,
    STRUCT_TYPE,
    UNSIGNED,
    VOID_TYPE,
    MAC_CONTEXT,
    MAC_FINAL,
    MAC_FINAL_STATE,
    MAC_START,
    MAC_STATE,
    MAC_STATES,
    MAC_TRANSITIONS,

    // Undefined token
    UNKNOWN,
};
