#pragma once

#include "Ref.h"
#include "Token.h"

static bool hasError = false;
static ordered_json errors;

static void report(size_t line, const std::string &where, const std::string &message) {
    std::stringstream ss;
    ss << "[line " << line << "] Error" << where << ": " << message;
    errors[std::to_string(line)] = ss.str();
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

inline ordered_json errorJson() {
    return errors;
}
