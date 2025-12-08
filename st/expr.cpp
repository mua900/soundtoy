#include "expr.h"

Op_Binary get_binop(Token_Type type)
{
    switch (type)
    {
        case TOKEN_TYPE_PLUS:               return Binop_Add;
        case TOKEN_TYPE_MINUS:              return Binop_Sub;
        case TOKEN_TYPE_STAR:               return Binop_Mul;
        case TOKEN_TYPE_SLASH:              return Binop_Div;
        case TOKEN_TYPE_PERCENT:            return Binop_Mod;
        case TOKEN_TYPE_EQUALS_EQUALS:      return Binop_Eq;
        case TOKEN_TYPE_EXCLAMATION_EQUALS: return Binop_Neq;
        case TOKEN_TYPE_GREATER:            return Binop_Gt;
        case TOKEN_TYPE_GREATER_EQUALS:     return Binop_Ge;
        case TOKEN_TYPE_LESS:               return Binop_Lt;
        case TOKEN_TYPE_LESS_EQUALS:        return Binop_Le;
        default:
            return Binop_Unknown;
    }
}

bool binop_is_arithmetic(Op_Binary op) {
    return op >= Binop_Add && op <= Binop_Mod;
}

bool binop_is_comparison(Op_Binary op) {
    return op >= Binop_Eq && op <= Binop_Le;
}

const char* get_binop_string(Op_Binary op)
{
    switch (op)
    {
        case Binop_Unknown: return "Binop_Unknown";
        case Binop_Add: return "Binop_Add";
        case Binop_Sub: return "Binop_Sub";
        case Binop_Mul: return "Binop_Mul";
        case Binop_Div: return "Binop_Div";
        case Binop_Mod: return "Binop_Mod";
        case Binop_Eq: return "Binop_Eq";
        case Binop_Neq: return "Binop_Neq";
        case Binop_Gt: return "Binop_Gt";
        case Binop_Ge: return "Binop_Ge";
        case Binop_Lt: return "Binop_Lt";
        case Binop_Le: return "Binop_Le";
    }

    return NULL;
}
