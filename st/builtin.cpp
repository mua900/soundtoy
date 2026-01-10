#include "builtin.h"
#include <cmath>

// custom implemented builtins
double get_sign(double x);
double smoothstep(double x);
double clamp_range_normal(double x);
double clamp_range_audio(double x);  // @todo get rid

bool call_function(Function func, Value* results) {
    for (int i = 0; i < func.signature.return_types.size; i++) {
        results[i] = Value(0.0);
    }

    return false;
}

void get_default_builtin_functions(Function* list)
{
    // @todo remove static
    static Variable_Type single_value[1] = { Var_Type_Real };

    list[BUILTIN_FUNC_ABS] = Function(
        (GenericFunctionPointer)fabs,
        FunctionSignature(String("abs"), make_array(single_value), make_array(single_value))
    );
    list[BUILTIN_FUNC_SIGN] = Function(
        (GenericFunctionPointer)get_sign,
        FunctionSignature(String("sign"), make_array(single_value), make_array(single_value))
    );
    list[BUILTIN_FUNC_CEIL] = Function(
        (GenericFunctionPointer)ceil,
        FunctionSignature(String("ceil"), make_array(single_value), make_array(single_value))
    );
    list[BUILTIN_FUNC_FLOOR] = Function(
        (GenericFunctionPointer)floor,
        FunctionSignature(String("floor"), make_array(single_value), make_array(single_value))
    );
    list[BUILTIN_FUNC_SMOOTHSTEP] = Function(
        (GenericFunctionPointer)smoothstep,
        FunctionSignature(String("smoothstep"), make_array(single_value), make_array(single_value))
    );
    list[BUILTIN_FUNC_CLAMP_RANGE_NORMAL] = Function(
        (GenericFunctionPointer)clamp_range_normal,
        FunctionSignature(String("clamp_normal"), make_array(single_value), make_array(single_value))
    );
    list[BUILTIN_FUNC_CLAMP_RANGE_AUDIO] = Function(
        (GenericFunctionPointer)clamp_range_audio,
        FunctionSignature(String("clamp_range_audio"), make_array(single_value), make_array(single_value))
    );
    list[BUILTIN_FUNC_SIN] = Function(
        (GenericFunctionPointer)sin,
        FunctionSignature(String("sin"), make_array(single_value), make_array(single_value))
    );
    list[BUILTIN_FUNC_COS] = Function(
        (GenericFunctionPointer)cos,
        FunctionSignature(String("cos"), make_array(single_value), make_array(single_value))
    );
    list[BUILTIN_FUNC_ARCSIN] = Function(
        (GenericFunctionPointer)asin,
        FunctionSignature(String("asin"), make_array(single_value), make_array(single_value))
    );
    list[BUILTIN_FUNC_ARCCOS] = Function(
        (GenericFunctionPointer)acos,
        FunctionSignature(String("acos"), make_array(single_value), make_array(single_value))
    );
    list[BUILTIN_FUNC_EXP] = Function(
        (GenericFunctionPointer)exp,
        FunctionSignature(String("exp"), make_array(single_value), make_array(single_value))
    );
}

bool is_builtin_function(const Expr_Call* call)
{
    return call->fn_id <= BUILTIN_FUNC_COUNT;
}

bool is_builtin_variable(const Expr_Variable* var)
{
    return var->var_id <= BUILTIN_VARIABLE_COUNT;
}

double get_sign(double x)
{
    // @todo more robust version
    return (x > 0) - (x < 0);
}

double smoothstep(double x) {
    x = CLAMP(x, 0.0, 1.0);
    return x * x * (3.0 - 2.0 * x);
}

double clamp_range_normal(double x) {
    return CLAMP(x, 0.0, 1.0);
}

double clamp_range_audio(double x) {
    return CLAMP(x, -1.0, 1.0);
}
