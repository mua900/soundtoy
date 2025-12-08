#pragma once

#include "common.h"

enum Token_Type {
    TOKEN_TYPE_UNKNOWN = 0,
    TOKEN_TYPE_PLUS,
    TOKEN_TYPE_MINUS,
    TOKEN_TYPE_STAR,
    TOKEN_TYPE_SLASH,
    TOKEN_TYPE_PERCENT,
    TOKEN_TYPE_COMMA,
    TOKEN_TYPE_COLON,
    TOKEN_TYPE_SEMICOLON,
    TOKEN_TYPE_AMPERSAND,
    TOKEN_TYPE_PAREN_OPEN,
    TOKEN_TYPE_PAREN_CLOSE,
    TOKEN_TYPE_BRACE_OPEN,
    TOKEN_TYPE_BRACE_CLOSE,

    TOKEN_TYPE_EXCLAMATION,
    TOKEN_TYPE_EQUALS,
    TOKEN_TYPE_GREATER,
    TOKEN_TYPE_LESS,

    TOKEN_TYPE_EXCLAMATION_EQUALS,
    TOKEN_TYPE_EQUALS_EQUALS,
    TOKEN_TYPE_GREATER_EQUALS,
    TOKEN_TYPE_LESS_EQUALS,

    TOKEN_TYPE_IDENT,
    TOKEN_TYPE_LITERAL_INT,
    TOKEN_TYPE_LITERAL_FLOAT,
    TOKEN_TYPE_LITERAL_STRING,

    TOKEN_TYPE_END,
};

struct Token {
    String token_string = {};
    Token_Type type = TOKEN_TYPE_UNKNOWN;
    int offset = 0;

    Token() {}
    Token(String s, Token_Type t_type, int p_offset) : token_string(s), type(t_type), offset(p_offset) {}
};

void print_token(Token& token, String_Builder* sb, bool ignore_some=false);
const char* get_token_type_string(Token_Type type);