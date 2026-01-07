#pragma once

enum TokenType {
    // Single-character tokens
    LEFT_PAREN_DELIM,
    RIGHT_PAREN_DELIM,
    LEFT_CURLY_DELIM,
    RIGHT_CURLY_DELIM,
    LEFT_SQUARE_DELIM,
    RIGHT_SQUARE_DELIM,
    SEMICOLON_DELIM,
    COMMA_OP,
    DOT_OP,

    // Unary Operator
    NEGATIVE_OP,
    POSITIVE_OP,
    PRE_DECRMNT_OP,
    PRE_INCRMNT_OP,
    POST_INCRMNT_OP,
    POST_DECRMNT_OP,

    // Binary Operator
    SUBTRACT_OP,
    ADD_OP,
    DIVIDE_OP,
    MULTIPLY_OP,
    MODULO_OP,

    // Logical Operators
    AND_LOG_OP,
    OR_LOG_OP,
    NOT_LOG_OP,

    // Relational Operators
    NOT_EQUAL_REL_OP,
    EQUAL_REL_OP,
    GREATER_REL_OP,
    GREATER_EQUAL_REL_OP,
    LESS_REL_OP,
    LESS_EQUAL_REL_OP,

    // Assignment Operators
    EQUAL_ASS_OP,
    ADD_ASS_OP,
    SUBTRCT_ASS_OP,
    MULTPLY_ASS_OP,
    DIVIDE_ASS_OP,
    MODULO_ASS_OP,

    // Identifier
    IDENTIFIER,

    // Literals
    STR_LITERAL,
    INT_LITERAL,
    FLT_LITERAL,
    TRUE_LITERAL,
    FALSE_LITERAL,
    CHAR_LITERAL,

    // Keywords
    BREAK_KEYW,
    CONTINUE_KEYW,
    DO_KEYW,
    FOR_KEYW,
    WHILE_KEYW,

    // Reserved Words
    BOOL_TYPE_RESW,
    CHAR_TYPE_RESW,
    CONST_RESW,
    DOUBLE_TYPE_RESW,
    ELIF_RESW,
    ELSE_RESW,
    ENUM_RESW,
    FLOAT_TYPE_RESW,
    IF_RESW,
    INT_TYPE_RESW,
    MACHINE_TYPE_RESW,
    RETURN_RESW,
    STATIC_RESW,
    STRING_TYPE_RESW,
    STRUCT_TYPE_RESW,
    UNSIGNED_RESW,
    VOID_TYPE_RESW,
    MAC_CONTEXT_RESW,
    MAC_FINAL_RESW,
    MAC_FINAL_STATE_RESW,
    MAC_START_RESW,
    MAC_STATE_RESW,
    MAC_STATES_RESW,
    MAC_TRANSITIONS_RESW,

    // Comments
    LINE_COMNT,
    MULTILINE_COMNT,

    // Undefined token
    UNKNOWN,

    // User Defined token
    USER_DEFINED_RESW,
};
