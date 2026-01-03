#pragma once

#include "Ref.h"
#include "Token.h"

static bool hasError = false;

static void report(size_t line, const std::string &where, const std::string &message) {
    std::cerr << "[line " << line << "] Error" << where << ": " << message;
}

inline void error(Ref<Token> token, const std::string &message) {
    std::stringstream ss;
    ss << " at '" << token->lexeme() << "'";

    report(token->line(), ss.str(), message);
    hasError = true;
}

inline bool hadError() {
    return hasError;
}
