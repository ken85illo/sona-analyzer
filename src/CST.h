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
    explicit NonTerminalNode(const std::string &name)
    : m_name(name) {}

    ordered_json toJson() const override {
        ordered_json j;
        j["type"] = "non_terminal";
        j["name"] = m_name;
        j["children"] = ordered_json::array();

        for (const auto &child: m_children) {
            if (child != nullptr) {
                j["children"].push_back(child->toJson());
            }
        }

        return j;
    }

    void add(CST node) {
        m_children.push_back(node);
    }

    static Ref<NonTerminalNode> make(const std::string &name) {
        return MakeRef<NonTerminalNode>(name);
    }

private:
    std::string m_name;
    std::vector<CST> m_children;
};

class TerminalNode : public CSTNode {
public:
    TerminalNode(Ref<Token> token)
    : m_lexeme(token->lexeme()), m_tokenType(token->typeString()) {}

    ordered_json toJson() const override {
        return ordered_json{
            {       "type",  "terminal" },
            {     "lexeme",    m_lexeme },
            { "token_type", m_tokenType }
        };
    }

    static Ref<TerminalNode> make(Ref<Token> token) {
        return MakeRef<TerminalNode>(token);
    }

protected:
    std::string m_lexeme;
    std::string m_tokenType;
};

class ErrorNode : public CSTNode {
public:
    ErrorNode(Ref<Token> token, int32_t synchronize)
    : m_line(token->line()), m_synchronize(synchronize) {}

    ordered_json toJson() const {
        return ordered_json{
            {        "type",       "error" },
            {        "line",        m_line },
            {       "index",  errorIndex++ },
            { "synchronize", m_synchronize }
        };
    }

    static Ref<ErrorNode> make(Ref<Token> token, size_t synchronize) {
        return MakeRef<ErrorNode>(token, synchronize);
    }

private:
    inline static size_t errorIndex = 0;
    const size_t m_line;
    const int64_t m_synchronize;
};
