#pragma once

#include "Ref.h"
#include "Token.h"

static bool hasError = false;
static ordered_json errors;

static void report(size_t line, const std::string &where, const std::string &message) {
    std::stringstream ss;
    ss << "Error" << where << ": " << message;
    errors[std::to_string(line)].push_back(ss.str());
}

inline void error(Ref<Token> token, const std::string &message) {
    std::stringstream ss;
    ss << " at token '" << token->typeString() << "' with lexeme '" << token->lexeme() << "'";

    report(token->line(), ss.str(), message);
    hasError = true;
}

inline ordered_json errorJson() {
    return errors;
}
