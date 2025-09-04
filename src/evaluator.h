#pragma once

#include "common.h"

// manual polymorphism

enum Expr_Type {
    Expr_Unary,
    Expr_Binary,
    Expr_Grouping,
    Expr_Call,
};

struct Expr {
    Expr_Type type;
};

enum Op_Unary {
    Unop_Negate, Unop_Not
};

struct Expr_Unary : Expr {
    Op_Unary op;
    Expr* operand = NULL;
};

enum Op_Binary {
    Binop_Add,
    Binop_Sub,
    Binop_Mul,
    Binop_Div,
    Binop_Mod,
};

struct Expr_Binary : Expr {
    Expr* left = NULL;
    Expr* right = NULL;
    Op_Binary op;
};

struct Expr_Grouping : Expr {
    Expr* expr;
};

Array<Expr*> parse_expression(String expression);
