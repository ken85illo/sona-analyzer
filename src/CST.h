#pragma once

#include "Ref.h"
#include "Token.h"

using ordered_json = nlohmann::ordered_json;

class CSTNode;
using CST = std::shared_ptr<CSTNode>;

class CSTNode {
public:
    virtual ~CSTNode() = default;
    virtual ordered_json toJson(const std::string &path = "") const = 0;
};

class NonTerminalNode : public CSTNode {
public:
    NonTerminalNode(const std::string &name)
    : m_name(name) {}

    ordered_json toJson(const std::string &path = "") const override {
        std::string currentPath = path.empty() ? m_name : path + "|" + m_name;

        ordered_json result = ordered_json::array();

        for (const auto &child: m_children) {
            if (child) {
                auto childJson = child->toJson(currentPath);

                if (childJson.is_array()) {
                    for (auto &elem: childJson) {
                        result.push_back(elem);
                    }
                }
                else {
                    result.push_back(childJson);
                }
            }
        }

        return result;
    }

    void add(const CST &node) {
        m_children.push_back(node);
    }

    bool empty() {
        return m_children.empty();
    }

    static Ref<NonTerminalNode> make(const std::string &name) {
        return MakeRef<NonTerminalNode>(name);
    }

private:
    std::string m_name;
    std::vector<CST> m_children;
    inline static Ref<NonTerminalNode> s_track;
};

class TerminalNode : public CSTNode {
public:
    TerminalNode(const Ref<Token> &token)
    : m_lexeme(token->lexeme()), m_line(token->line()), m_tokenType(token->typeString()) {}

    ordered_json toJson(const std::string &path = "") const override {
        return ordered_json{
            {          "type",  "terminal" },
            {        "lexeme",    m_lexeme },
            {          "line",      m_line },
            {    "token_type", m_tokenType },
            { "non_terminals",        path }
        };
    }

    static Ref<TerminalNode> make(const Ref<Token> &token) {
        return MakeRef<TerminalNode>(token);
    }

private:
    const std::string m_lexeme;
    const std::string m_tokenType;
    const size_t m_line;
};

class EpsilonNode : public CSTNode {
public:
    EpsilonNode(size_t line)
    : m_line(line) {}

    ordered_json toJson(const std::string &path = "") const override {
        return ordered_json{
            {          "type", "epsilon" },
            {          "line",    m_line },
            { "non_terminals",      path }
        };
    }

    static Ref<EpsilonNode> make(size_t line) {
        return MakeRef<EpsilonNode>(line);
    }

private:
    const size_t m_line;
};

class ErrorNode : public CSTNode {
public:
    ErrorNode(size_t line, const Ref<Token> &synchronize)
    : m_line(line), m_synchronize(synchronize) {}

    ordered_json toJson(const std::string &path = "") const {
        if (m_synchronize) {
            return ordered_json{
                {          "type","error"                                  },
                {          "line",                 m_line },
                {         "index", s_errorIndex[m_line]++ },
                {   "synchronize",
                 {
                 { "lexeme", m_synchronize->lexeme() },
                 { "token_type", m_synchronize->typeString() },
                 { "line", m_synchronize->line() },
                 }                                       },
                { "non_terminals",                   path }
            };
        }

        return ordered_json{
            {          "type",                "error" },
            {          "line",                 m_line },
            {         "index", s_errorIndex[m_line]++ },
            {   "synchronize",                nullptr },
            { "non_terminals",                   path }
        };
    }

    void setSychronize(const Ref<Token> &synchronize) {
        m_synchronize = synchronize;
    }

    static Ref<ErrorNode> make(size_t line, const Ref<Token> &synchronize) {
        return MakeRef<ErrorNode>(line, synchronize);
    }

private:
    inline static std::unordered_map<size_t, size_t> s_errorIndex;
    const size_t m_line;
    Ref<Token> m_synchronize;
};
