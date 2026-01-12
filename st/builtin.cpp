#include "builtin.h"
#include <cmath>

// custom implemented builtins
double get_sign(double x);
double smoothstep(double x);
double clamp_range_normal(double x);
double clamp_range_audio(double x);  // @todo get rid

// wrappers
void st_fabs(Value* parameters, Value* results);
void st_get_sign(Value* parameters, Value* results);
void st_ceil(Value* parameters, Value* results);
void st_floor(Value* parameters, Value* results);
void st_smoothstep(Value* parameters, Value* results);
void st_clamp(Value* parameters, Value* results);
void st_sin(Value* parameters, Value* results);
void st_cos(Value* parameters, Value* results);
void st_asin(Value* parameters, Value* results);
void st_acos(Value* parameters, Value* results);
void st_exp(Value* parameters, Value* results);

void call_function(Function func, Value* parameters, Value* results) {
    func.implementation(parameters, results);
}

void get_default_builtin_functions(Function* list)
{
    static Variable_Type single_value[1] = { Var_Type_Real };
    static Variable_Type three_values[3] = { Var_Type_Real, Var_Type_Real, Var_Type_Real };
    
    list[BUILTIN_FUNC_ABS] = Function(
        st_fabs,
        FunctionSignature(String("abs"), make_array(single_value), make_array(single_value))
    );
    list[BUILTIN_FUNC_SIGN] = Function(
        st_get_sign,
        FunctionSignature(String("sign"), make_array(single_value), make_array(single_value))
    );
    list[BUILTIN_FUNC_CEIL] = Function(
        st_ceil,
        FunctionSignature(String("ceil"), make_array(single_value), make_array(single_value))
    );
    list[BUILTIN_FUNC_FLOOR] = Function(
        st_floor,
        FunctionSignature(String("floor"), make_array(single_value), make_array(single_value))
    );
    list[BUILTIN_FUNC_SIN] = Function(
        st_sin,
        FunctionSignature(String("sin"), make_array(single_value), make_array(single_value))
    );
    list[BUILTIN_FUNC_COS] = Function(
        st_cos,
        FunctionSignature(String("cos"), make_array(single_value), make_array(single_value))
    );
    list[BUILTIN_FUNC_ARCSIN] = Function(
        st_asin,
        FunctionSignature(String("asin"), make_array(single_value), make_array(single_value))
    );
    list[BUILTIN_FUNC_ARCCOS] = Function(
        st_acos,
        FunctionSignature(String("acos"), make_array(single_value), make_array(single_value))
    );
    list[BUILTIN_FUNC_EXP] = Function(
        st_exp,
        FunctionSignature(String("exp"), make_array(single_value), make_array(single_value))
    );
    list[BUILTIN_FUNC_SMOOTHSTEP] = Function(
        st_smoothstep,
        FunctionSignature(String("smoothstep"), make_array(single_value), make_array(three_values))
    );
    list[BUILTIN_FUNC_CLAMP] = Function(
        st_clamp,
        FunctionSignature(String("clamp"), make_array(single_value), make_array(three_values))
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

double smoothstep(double x, double lower, double upper) {
    x = CLAMP(x, lower, upper);
    return x * x * (3.0 - 2.0 * x);
}

double clamp(double x, double lower, double upper) {
    return CLAMP(x, lower, upper);
}

// wrappers

void st_fabs(Value* parameters, Value* results) { results[0] = Value(fabs(parameters[0].real)); }
void st_get_sign(Value* parameters, Value* results) { results[0] = Value(fabs(parameters[0].real)); }
void st_ceil(Value* parameters, Value* results) { results[0] = Value(ceil(parameters[0].real)); }
void st_floor(Value* parameters, Value* results) { results[0] = Value(floor(parameters[0].real)); }
void st_sin(Value* parameters, Value* results) { results[0] = Value(sin(parameters[0].real)); }
void st_cos(Value* parameters, Value* results) { results[0] = Value(cos(parameters[0].real)); }
void st_asin(Value* parameters, Value* results) { results[0] = Value(asin(parameters[0].real)); }
void st_acos(Value* parameters, Value* results) { results[0] = Value(acos(parameters[0].real)); }
void st_exp(Value* parameters, Value* results) { results[0] = Value(exp(parameters[0].real)); }
void st_smoothstep(Value* parameters, Value* results) { results[0] = Value(smoothstep(parameters[0].real, parameters[1].real, parameters[2].real)); }
void st_clamp(Value* parameters, Value* results) { results[0] = Value(clamp(parameters[0].real, parameters[1].real, parameters[2].real)); }
