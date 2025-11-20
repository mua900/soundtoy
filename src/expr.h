#pragma once

#include "token.h"
#include "template.h"

enum Op_Unary {
    Unop_Negate, Unop_Not
};

// @todo bitwise operators, ternary
enum Op_Binary {
    Binop_Unknown = 0,
    Binop_Add,
    Binop_Sub,
    Binop_Mul,
    Binop_Div,
    Binop_Mod,
    Binop_Eq,
    Binop_Neq,
    Binop_Gt,
    Binop_Ge,
    Binop_Lt,
    Binop_Le,
};

enum class Value_Type {
    INTEGER,
    REAL,
    BOOL,
};

struct Value {
    Value_Type type;

    union {
        bool boolean;
        s64 integer;
        double real;
    };


    Value(bool b) : boolean(b) {
        type = Value_Type::BOOL;
    }
    Value(long long integer) : integer(integer) {
        type = Value_Type::INTEGER;
    }
    Value(double real) : real(real) {
        type = Value_Type::REAL;
    }

    bool is_numeric() {
        return type == Value_Type::INTEGER || type == Value_Type::REAL;
    }
};

enum class Expr_Type {
    Literal,
    Variable,
    Unary,
    Binary,
    Grouping,
    Call,
};

struct Expr {
    Expr_Type type;
};

void print_expr(Expr* expr, int indent);

struct Expr_Literal : Expr {
    Value value;

    Expr_Literal(bool b) : value(b) {
        type = Expr_Type::Literal;
    }
    Expr_Literal(long long integer) : value(integer) {
        type = Expr_Type::Literal;
    }
    Expr_Literal(double real) : value(real) {
        type = Expr_Type::Literal;
    }
};

struct Expr_Unary : Expr {
    Op_Unary op;
    Expr* operand = NULL;

    Expr_Unary(Op_Unary p_op, Expr* p_operand) : op(p_op), operand(p_operand) {
        type = Expr_Type::Unary;
    }
};

struct Expr_Binary : Expr {
    Expr* left = NULL;
    Expr* right = NULL;
    Op_Binary op;

    Expr_Binary(Expr* l, Expr* r, Op_Binary p_op) : left(l), right(r), op(p_op) {
        type = Expr_Type::Binary;
    }
};

Op_Binary get_binop(Token_Type type);
const char* get_binop_string(Op_Binary op);
bool binop_is_arithmetic(Op_Binary op);
bool binop_is_comparison(Op_Binary op);

struct Expr_Grouping : Expr {
    Expr* expr = NULL;

    Expr_Grouping(Expr* p_expr) : expr(p_expr) {
        type = Expr_Type::Grouping;
    }
};

// we can keep this simple since this isn't a complete programming language where expressions can evaluate to functions
struct Expr_Call : Expr {
    String function_name;
    Array<Expr*> arguments;
    int fn_id = 0;  // function id assigned by parser

    Expr_Call(String f_name, Array<Expr*> args, int func_id) : function_name(f_name), arguments(args), fn_id(func_id) {
        type = Expr_Type::Call;
    }
};

struct Expr_Variable : Expr {
    String name;
    int var_id = 0;
    Value_Type variable_type;  // @todo

    Expr_Variable(String var_name, int id) : name(var_name), var_id(id), variable_type(Value_Type::INTEGER) {
        type = Expr_Type::Variable;
    }
};