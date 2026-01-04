#pragma once

#include "Ref.h"
#include "Token.h"

using ordered_json = nlohmann::ordered_json;

class CSTNode;
using CST = std::shared_ptr<CSTNode>;

class CSTNode {
public:
    virtual ~CSTNode() = default;
    virtual ordered_json toJson() const = 0;
};

class NonTerminalNode : public CSTNode {
public:
    std::string name;
    std::vector<CST> children;

    explicit NonTerminalNode(const std::string &name)
    : name(name) {}

    ordered_json toJson() const override {
        ordered_json j;
        j["type"] = "non_terminal";
        j["name"] = name;
        j["children"] = ordered_json::array();

        for (const auto &child: children) {
            if (child != nullptr) {
                j["children"].push_back(child->toJson());
            }
        }

        return j;
    }
};

class TerminalNode : public CSTNode {
public:
    std::string lexeme;
    std::string tokenType;

    TerminalNode(Ref<Token> token)
    : lexeme(token->lexeme()), tokenType(token->typeString()) {}

    ordered_json toJson() const override {
        return ordered_json{
            {       "type", "terminal" },
            {     "lexeme",     lexeme },
            { "token_type",  tokenType }
        };
    }
};
