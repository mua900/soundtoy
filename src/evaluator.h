#pragma once

#include "common.h"
#include "template.h"

#include "token.h"
#include "expr.h"

DArray<Token> tokenize(String expression);

// @todo turn this into a something that can register and remember errors
struct Error {
    String message = {};
    int offset = 0;
};

struct Parser {
    Parser() {}

    int cursor = 0;
    Error parser_error = {};

    Array<Expr*> parse(String expression);

private:
    Array<Token> tokens;

    Expr* parse_expression();

    // according to precedence in order from high to low
    Expr* parse_equality_expr();
    Expr* parse_comparison_expr();
    Expr* parse_arithmetic_expr();
    Expr* parse_factor_expr();
    Expr* parse_unary_expr();
    Expr* parse_call_expr();
    Expr* parse_primary_expr();

    bool consume(Token_Type type);

    void report_error();  // @todo
};

enum Builtin_Variables {
    BUILTIN_TIME = 0,
    BUILTIN_SAMPLE_RATE,

    BUILTIN_COUNT
};

struct Eval {
    double value;
    bool success;
};

struct Evaluator
{
    DArray<Expr*> m_expressions = {};
    double m_builtins[BUILTIN_COUNT] = {}; // @todo not ideal to store everything as floating point
    Error m_error = {};

    void set(double sample_rate, double time);
    void update(double time);

    void add(String expr_string);
    Eval evaluate(Expr* expr);
};
