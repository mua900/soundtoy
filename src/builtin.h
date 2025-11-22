#pragma once

#include "expr.h"

// @todo higher level builtins for audio

// @todo support functions with different signatures.
using Builtin_Function = double (*) (double);

// it is trivial to add new builtin functions as long as they have the above signature.
// add an enum entry here
// go to get_function_id and add a new case for the name of the function to match
// and finaly go to get_default_builtin_functions and add an implementaion of your function to the list
enum Builtin_Func_Type {
    BUILTIN_FUNC_EXP,
    BUILTIN_FUNC_ABS,
    BUILTIN_FUNC_SIGN,
    BUILTIN_FUNC_CEIL,
    BUILTIN_FUNC_FLOOR,
    // @todo get rid of different versions of the same function
    BUILTIN_FUNC_CLAMP_RANGE_NORMAL,    // clamp between 0 and 1
    BUILTIN_FUNC_CLAMP_RANGE_AUDIO,     // clamp between -1 and 1
    BUILTIN_FUNC_SMOOTHSTEP,            // smoothstep between 0 and 1
    BUILTIN_FUNC_SIN,
    BUILTIN_FUNC_COS,
    BUILTIN_FUNC_ARCSIN,
    BUILTIN_FUNC_ARCCOS,

    BUILTIN_FUNC_COUNT,

    BUILTIN_FUNC_UNKNOWN,
};

enum Builtin_Variable : int {
    // constant per evaluation
    BUILTIN_VARIABLE_TIME = 0,
    BUILTIN_VARIABLE_SAMPLE_RATE,

    BUILTIN_VARIABLE_COUNT
};

// @todo use these
enum Builtin_Constants {
    // constant
    BUILTIN_CONSTANT_PI,
    BUILTIN_CONSTANT_E,

    BUILTIN_CONSTANT_COUNT,
};

using Builtin_Function_List = Builtin_Function[BUILTIN_FUNC_COUNT];

// the argument must be an array of pointers of size BUILTIN_FUNC_COUNT
void get_default_builtin_functions(Builtin_Function* func_list);

bool is_builtin_function(const Expr_Call* call);
bool is_builtin_variable(const Expr_Variable* var);