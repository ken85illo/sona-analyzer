#pragma once
#include "Expr.h"

class AstPrinter : public Visitor {
public:
    std::string print(Ref<Expr> expr) {
        return expr->accept(*this);
    }

    std::string visitBinaryExpr(Ref<BinaryExpr> expr) override {
        return parenthesize(expr->op->lexeme(), expr->left, expr->right);
    }

    std::string visitGroupingExpr(Ref<GroupingExpr> expr) override {
        return parenthesize("()", expr->expression);
    }

    std::string visitLiteralExpr(Ref<LiteralExpr> expr) override {
        return *expr->value;
    }

    std::string visitUnaryExpr(Ref<UnaryExpr> expr) override {
        return parenthesize(expr->op->lexeme(), expr->right);
    }

private:
    template <typename... Exprs>
    std::string parenthesize(const std::string &name, Exprs &...exprs) {
        std::stringstream ss;

        ss << "(" << name;
        ((ss << " " << exprs->accept(*this)), ...);
        ss << ")";

        return ss.str();
    }
};
