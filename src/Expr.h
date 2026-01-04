#pragma once
#include "Ref.h"
#include "Token.h"

class BinaryExpr;
class GroupingExpr;
class LiteralExpr;
class UnaryExpr;
class Visitor;

class Expr {
public:
    virtual std::string accept(Visitor &visitor) = 0;
    virtual ~Expr() = default;
};

class Visitor {
public:
    virtual std::string visitBinaryExpr(const BinaryExpr &expr) = 0;
    virtual std::string visitGroupingExpr(const GroupingExpr &expr) = 0;
    virtual std::string visitLiteralExpr(const LiteralExpr &expr) = 0;
    virtual std::string visitUnaryExpr(const UnaryExpr &expr) = 0;
    virtual ~Visitor() = default;
};

class BinaryExpr : public Expr {
public:
    BinaryExpr(const Ref<Expr> left, const Ref<Token> op, const Ref<Expr> right)
    : left(left), op(op), right(right) {}

    std::string accept(Visitor &visitor) override {
        return visitor.visitBinaryExpr(*this);
    }

    const Ref<Expr> left;
    const Ref<Token> op;
    const Ref<Expr> right;
};

class GroupingExpr : public Expr {
public:
    GroupingExpr(const Ref<Expr> expression)
    : expression(expression) {}

    std::string accept(Visitor &visitor) override {
        return visitor.visitGroupingExpr(*this);
    }

    const Ref<Expr> expression;
};

class LiteralExpr : public Expr {
public:
    LiteralExpr(const Ref<Token> value)
    : value(value) {}

    std::string accept(Visitor &visitor) override {
        return visitor.visitLiteralExpr(*this);
    }

    const Ref<Token> value;
};

class UnaryExpr : public Expr {
public:
    UnaryExpr(const Ref<Token> op, const Ref<Expr> right)
    : op(op), right(right) {}

    std::string accept(Visitor &visitor) override {
        return visitor.visitUnaryExpr(*this);
    }

    const Ref<Token> op;
    const Ref<Expr> right;
};
