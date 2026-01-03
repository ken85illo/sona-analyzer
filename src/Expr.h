#pragma once
#include "Ref.h"
#include "Token.h"

class BinaryExpr;
class GroupingExpr;
class LiteralExpr;
class UnaryExpr;
class Visitor;

class Expr: public std::enable_shared_from_this<Expr> {
public:
    virtual std::string accept(Visitor& visitor) = 0;
    virtual ~Expr() = default;
};

class Visitor {
public:
    virtual std::string visitBinaryExpr(Ref<BinaryExpr> expr) = 0;
    virtual std::string visitGroupingExpr(Ref<GroupingExpr> expr) = 0;
    virtual std::string visitLiteralExpr(Ref<LiteralExpr> expr) = 0;
    virtual std::string visitUnaryExpr(Ref<UnaryExpr> expr) = 0;
    virtual ~Visitor() = default;
};

class BinaryExpr : public Expr{
public:
    BinaryExpr(const Ref<Expr> left, const Ref<Token> op, const Ref<Expr> right)
    : left(left), op(op), right(right) {}

    std::string accept(Visitor& visitor) override {
        return visitor.visitBinaryExpr(StaticCast<BinaryExpr>(shared_from_this()));
    }

    const Ref<Expr> left;
    const Ref<Token> op;
    const Ref<Expr> right;
};

class GroupingExpr : public Expr{
public:
    GroupingExpr(const Ref<Expr> expression)
    : expression(expression) {}

    std::string accept(Visitor& visitor) override {
        return visitor.visitGroupingExpr(StaticCast<GroupingExpr>(shared_from_this()));
    }

    const Ref<Expr> expression;
};

class LiteralExpr : public Expr{
public:
    LiteralExpr(const Ref<std::string> value)
    : value(value) {}

    std::string accept(Visitor& visitor) override {
        return visitor.visitLiteralExpr(StaticCast<LiteralExpr>(shared_from_this()));
    }

    const Ref<std::string> value;
};

class UnaryExpr : public Expr{
public:
    UnaryExpr(const Ref<Token> op, const Ref<Expr> right)
    : op(op), right(right) {}

    std::string accept(Visitor& visitor) override {
        return visitor.visitUnaryExpr(StaticCast<UnaryExpr>(shared_from_this()));
    }

    const Ref<Token> op;
    const Ref<Expr> right;
};

