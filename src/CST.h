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
    explicit NonTerminalNode(const std::string &name)
    : m_name(name) {}

    ordered_json toJson(const std::string &path = "") const override {
        std::string currentPath = path.empty() ? m_name : path + ">" + m_name;

        ordered_json result = ordered_json::array();

        for (const auto &child: m_children) {
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

        return result;
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
    inline static Ref<NonTerminalNode> s_track;
};

class TerminalNode : public CSTNode {
public:
    TerminalNode(const Ref<Token> &token)
    : m_lexeme(token->lexeme()), m_tokenType(token->typeString()) {}

    ordered_json toJson(const std::string &path = "") const override {
        return ordered_json{
            {          "type",  "terminal" },
            {        "lexeme",    m_lexeme },
            {    "token_type", m_tokenType },
            { "non_terminals",        path }
        };
    }

    static Ref<TerminalNode> make(const Ref<Token> &token) {
        return MakeRef<TerminalNode>(token);
    }

private:
    std::string m_lexeme;
    std::string m_tokenType;
};

class EpsilonNode : public CSTNode {
public:
    EpsilonNode() {}

    ordered_json toJson(const std::string &path = "") const override {
        return ordered_json{
            {          "type", "epsilon" },
            { "non_terminals",      path }
        };
    }

    static Ref<EpsilonNode> make() {
        return MakeRef<EpsilonNode>();
    }
};

class ErrorNode : public CSTNode {
public:
    ErrorNode(const Ref<Token> &token, int32_t synchronize)
    : m_line(token->line()), m_synchronize(synchronize) {}

    ordered_json toJson(const std::string &path = "") const {
        return ordered_json{
            {          "type",                "error" },
            {          "line",                 m_line },
            {         "index", s_errorIndex[m_line]++ },
            {   "synchronize",          m_synchronize },
            { "non_terminals",                   path }
        };
    }

    static Ref<ErrorNode> make(const Ref<Token> &token, size_t synchronize) {
        return MakeRef<ErrorNode>(token, synchronize);
    }

private:
    inline static std::unordered_map<size_t, size_t> s_errorIndex;
    const size_t m_line;
    const int64_t m_synchronize;
};
