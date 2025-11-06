#pragma once

enum TokenType {
    // Single-character tokens
    LEFT_PAREN,
    RIGHT_PAREN,
    LEFT_BRACE,
    RIGHT_BRACE,
    COMMA,
    DOT,
    SEMICOLON,
    DIVIDE,
    MULTIPLY,

    // One or two character tokens
    MINUS,
    PLUS,
    DECREMENT,
    INCREMENT,
    AND,
    OR,
    NOT,
    NOT_EQUAL,
    EQUAL,
    EQUAL_EQUAL,
    GREATER,
    GREATER_EQUAL,
    LESS,
    LESS_EQUAL,

    // Literals
    IDENTIFIER,
    CHAR_STR,
    NUMBER,

    // Keywords
    BREAK,
    CONTINUE,
    FOR,
    DO,
    WHILE,

    // Reserved Words
    INT,
    DOUBLE,
    FLOAT,
    CHAR,
    STRING,
    VOID,
    BOOL,
    STRUCT,
    ENUM,
    CONST,
    STATIC,
    UNSIGNED,
    RETURN,
    IF,
    ELSE,
    ELIF,
    FUNC,

    // Error checking stuff
    UNKNOWN,
};
