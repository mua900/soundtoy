#pragma once

#include "api.h"
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

struct Value {
    Variable_Type type;

    union {
        bool boolean;
        s64 integer;
        double real;
    };

    Value() : type(Var_Type_Integer), integer(0) {}  // invalid state
    Value(bool b) : type(Var_Type_Boolean), boolean(b) {}
    Value(long long integer) : type(Var_Type_Integer), integer(integer) {}
    Value(double real) : type(Var_Type_Real), real(real) {}

    bool is_numeric() {
        return type == Var_Type_Integer || type == Var_Type_Real;
    }

	bool evaluate_truth_value() {
		switch (type) {
		case Var_Type_Boolean: return boolean;
		case Var_Type_Integer: return integer != 0;
		case Var_Type_Real: return real != 0.0;
		}

		panic("Unknown value type");
	}
};

struct Variable {
    String name = {};
    Variable_Type type = Var_Type_Real;

    Variable() {}
    Variable(String n, Variable_Type) : name(n), type(type) {}

    bool operator==(const Variable& other) const {
        return type == other.type && name == other.name;
    }
};

Find_Result find_symbol(const Array<Variable> symbols, const String name);

enum class Expr_Type {
    Literal,
    Variable,
    Unary,
    Binary,
    Grouping,
    Call,
	Ternary,
	Tuple,
};

struct Expr {
    Expr_Type type;
};

void print_expr(const Expr* expr, int indent);

struct Expr_Literal : Expr {
    Value value;

    Expr_Literal(bool b) : value(b)
	{
        type = Expr_Type::Literal;
    }
    Expr_Literal(long long integer) : value(integer)
	{
        type = Expr_Type::Literal;
    }
    Expr_Literal(double real) : value(real)
	{
        type = Expr_Type::Literal;
    }
    Expr_Literal(Value val) : value(val) {}
};

struct Expr_Unary : Expr {
    Op_Unary op;
    Expr* operand = NULL;

    Expr_Unary(Op_Unary p_op, Expr* p_operand) : op(p_op), operand(p_operand)
	{
        type = Expr_Type::Unary;
    }
};

struct Expr_Binary : Expr {
    Expr* left = NULL;
    Expr* right = NULL;
    Op_Binary op;

    Expr_Binary(Expr* l, Expr* r, Op_Binary p_op) : left(l), right(r), op(p_op)
	{
        type = Expr_Type::Binary;
    }
};

Op_Binary get_binop(Token_Type type);
const char* get_binop_string(Op_Binary op);
bool binop_is_arithmetic(Op_Binary op);
bool binop_is_comparison(Op_Binary op);

struct Expr_Grouping : Expr {
    Expr* expr = NULL;

    Expr_Grouping(Expr* p_expr) : expr(p_expr)
	{
        type = Expr_Type::Grouping;
    }
};

struct Expr_Call : Expr {
    String function_name;  // @note do we need expressions that can return functions? Not currently.
    Array<Expr*> arguments;
    int fn_id = 0;  // function id assigned by parser

    Expr_Call(String f_name, Array<Expr*> args, int func_id) : function_name(f_name), arguments(args), fn_id(func_id)
	{
        type = Expr_Type::Call;
    }
};

struct Expr_Variable : Expr {
    String name;
    unsigned int var_id = 0;
    Variable_Type variable_type;

    Expr_Variable(String var_name, int id, Variable_Type var_type) : name(var_name), var_id(id), variable_type(var_type)
	{
        type = Expr_Type::Variable;
    }
};

struct Expr_Ternary : Expr {
	Expr* condition = nullptr;
	Expr* then_ = nullptr;
	Expr* else_ = nullptr;

	Expr_Ternary(Expr* condition, Expr* then_, Expr* else_) : condition(condition), then_(then_), else_(else_)
	{
		type = Expr_Type::Ternary;
	}
};

struct Expr_Tuple : Expr {
	Array<Expr*> expressions;
	
	Expr_Tuple(Array<Expr*> exprs)
		: expressions(exprs)
	{
		type = Expr_Type::Tuple;
	}
};
