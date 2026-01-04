#pragma once
#include "CST.h"
#include "Expr.h"

struct ParseResult {
    Ref<Expr> ast;
    CST cst;
};
