#pragma once
#include "CST.h"
#include "Stmt.h"

struct ParseResult {
    Ref<Expr> ast;
    CST cst;

    static Ref<ParseResult> make(Ref<Expr> ast, CST cst) {
        return MakeRef<ParseResult>(ast, cst);
    }
};
