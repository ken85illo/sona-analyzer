#pragma once
#include "Expr.h"
#include "Ref.h"
#include "Token.h"

class ExpressionStmt;
class PrintStmt;
class StmtVisitor;

class Stmt {
public:
    virtual std::string accept(StmtVisitor &visitor) = 0;
    virtual ~Stmt() = default;
};

class StmtVisitor {
public:
    virtual std::string visitExpressionStmt(const ExpressionStmt &stmt) = 0;
    virtual std::string visitPrintStmt(const PrintStmt &stmt) = 0;
    virtual ~StmtVisitor() = default;
};

class ExpressionStmt : public Stmt {
public:
    ExpressionStmt(const Ref<Expr> expression)
    : expression(expression) {}

    std::string accept(StmtVisitor &visitor) override {
        return visitor.visitExpressionStmt(*this);
    }

    const Ref<Expr> expression;

    static Ref<ExpressionStmt> make(const Ref<Expr> expression) {
        return MakeRef<ExpressionStmt>(expression);
    }
};

class PrintStmt : public Stmt {
public:
    PrintStmt(const Ref<Expr> expression)
    : expression(expression) {}

    std::string accept(StmtVisitor &visitor) override {
        return visitor.visitPrintStmt(*this);
    }

    const Ref<Expr> expression;

    static Ref<PrintStmt> make(const Ref<Expr> expression) {
        return MakeRef<PrintStmt>(expression);
    }
};
