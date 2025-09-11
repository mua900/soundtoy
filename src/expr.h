#pragma once

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

struct Value {
    enum Value_Type {
        VALUE_INTEGER,
        VALUE_REAL,
        VALUE_STRING,
    } type;

    union {
        long long integer;
        double real;
        String string;
    };

    Value(long long integer) : integer(integer) {
        type = VALUE_INTEGER;
    }
    Value(double real) : real(real) {
        type = VALUE_REAL;
    }
    Value(String string) : string(string) {
        type = VALUE_STRING;
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

Expr* collapse_expr(Expr* root);

void print_expr(Expr* expr, int indent);

struct Expr_Literal : Expr {
    Value value;

    Expr_Literal(double real) : value(real) {}
    Expr_Literal(long long integer) : value(integer) {}
    Expr_Literal(String string) : value(string) {}
};

struct Expr_Variable : Expr {
    String name;

    Expr_Variable(String var_name) : name(var_name) {}
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

    Expr_Call(String f_name, Array<Expr*> args) : function_name(f_name), arguments(args) {
        type = Expr_Type::Call;
    }
};
