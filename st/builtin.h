#pragma once

#include "expr.h"

#define CONSTANT_PI  3.14159265359
#define CONSTANT_E   2.71828182846
#define CONSTANT_TAU CONSTANT_PI * 2.0

typedef void (*GenericFunctionPointer)(void);
typedef void (*StFunction)(Value* parameters, Value* return_values);

struct FunctionSignature {
    String name = {};
    Array<Variable_Type> return_types = {};
    Array<Variable_Type> parameter_types = {};

    FunctionSignature()
    {}
    FunctionSignature(String n, Array<Variable_Type> ret_types, Array<Variable_Type> param_types)
        :
        name(n), return_types(ret_types), parameter_types(param_types)
    {}

    bool operator==(const FunctionSignature& other) const {
        return  name == other.name &&
                return_types == other.return_types &&
                parameter_types == other.parameter_types;
    }
};

struct Function {
    StFunction implementation = nullptr;
    FunctionSignature signature = {};

    Function() {}
    Function(FunctionSignature sign) : signature(sign) {}
    Function(StFunction impl, FunctionSignature sign) : implementation(impl), signature(sign) {}
};

// results should be an array of length of func.signature.return_types.size
// if the function doesn't return anything it can be NULL
void call_function(Function func, Value* parameters, Value* results);

// --- Builtin functions

// @todo rewrite how to add new functions
enum Builtin_Func_Type {
    BUILTIN_FUNC_EXP,
    BUILTIN_FUNC_ABS,
    BUILTIN_FUNC_SIGN,
    BUILTIN_FUNC_CEIL,
    BUILTIN_FUNC_FLOOR,
    BUILTIN_FUNC_SIN,
    BUILTIN_FUNC_COS,
    BUILTIN_FUNC_ARCSIN,
    BUILTIN_FUNC_ARCCOS,
    BUILTIN_FUNC_SMOOTHSTEP,

    BUILTIN_FUNC_CLAMP,
    BUILTIN_FUNC_POW,

    BUILTIN_FUNC_COUNT,

    BUILTIN_FUNC_UNKNOWN,
};

enum Builtin_Variable : unsigned int {
    // constant per evaluation
    BUILTIN_VARIABLE_TIME = 0,
    BUILTIN_VARIABLE_SAMPLE_RATE,

    BUILTIN_VARIABLE_COUNT
};

enum Builtin_Constants {
    // constant
    BUILTIN_CONSTANT_PI,
	BUILTIN_CONSTANT_TAU,
    BUILTIN_CONSTANT_E,

    BUILTIN_CONSTANT_COUNT,
};

using Builtin_Function_List = Function[BUILTIN_FUNC_COUNT];

// the argument must be an array of pointers of size BUILTIN_FUNC_COUNT
void get_default_builtin_functions(Function* func_list);

bool is_builtin_function(const Expr_Call* call);
bool is_builtin_variable(const Expr_Variable* var);
