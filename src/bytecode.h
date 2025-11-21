#pragma once

#include "common.h"
#include "template.h"
#include "expr.h"

/*
	opcode(32 bit) operand1(16 bit), operand2(16 bit)

	64 bit fixed length instructions.
	Composed of a 32 bit opcode and a pair of 16 bit arguments.
	Some instructions may only use the first argument and discard the second slot.
	Some other ones might not need any arguments at all.
	Instructions can take arguments as registers or identifiers for known constants or functions.
	There are unlimited registers (as long as it can be addressed by the representable range of the id) and a constant block containing all the constants used in the expression for bytecode program to use.
*/

// For Value_Id the last 2 bits are the type of the value. Maps directly to value types.
// The remaining part is the index of the value in its type.

struct Value_Id {
	Value_Type value_type;
	u16 value_index;
};

using Func_Id = u16;

enum Bytecode_Opcode : u32
{
	INSTR_LOAD,			// load reg, const_int
	INSTR_LOADF,		// load freg, const_float
	INSTR_MOV,			// mov  reg, reg
	INSTR_MOVF,			// mov  freg, freg
	INSTR_MOV_FG,		// mov  freg, reg		   -- bitwise copy of the value
	INSTR_MOV_GF,		// mov  reg, freg		   -- bitwise copy of the value

	INSTR_CONV_TO_INT,	// conv freg			   -- convert a floating point value to an integer
	INSTR_CONV_TO_F,	// conv reg				   -- convert an integer to a floating point value

	INSTR_MOV_CONV_INT, // mov_and_conv freg, reg  -- MOV_FG freg, reg CONV_TO_INT freg
	INSTR_MOV_CONV_F,	// mov_and_conv reg, freg  -- MOV_GF reg, freg CONV_TO_F reg

	INSTR_ADD,			// add reg1, reg2
	INSTR_SUB,			// sub reg1, reg2
	INSTR_MUL,			// mul reg1, reg2
	INSTR_DIV,			// div reg1, reg2
	INSTR_MOD,			// mod reg1, reg2

	INSTR_ADDF,			// add freg1, freg2
	INSTR_SUBF,			// sub freg1, freg2
	INSTR_MULF,			// mul freg1, freg2
	INSTR_DIVF,			// div freg1, freg2
	INSTR_MODF,			// mod freg1, freg2

	INSTR_NEGATE,		// negate reg
	INSTR_NOT,			// not 	  reg

	INSTR_NEGATE_F,		// negate freg

	// the result of the comparison is stored in the flags register.
	INSTR_CMP,			// cmp reg0, reg1
	INSTR_CMPF,			// cmpf freg0, freg1

	// test sets the condition flag of the processor
	INSTR_TEST_RESULT,	// test immediate16

	// jump
	INSTR_JMP,			// jmp  address
	// jump if the condition bit is true (1)
	INSTR_JMP_COND,		// cjmp address

	// calls the builtin function identified by func_id and place the result in freg
	// all builtin functions take a single floating point number as argument and return a single floating point number
	// the calling convention is that the register carrying the arguments gets passed in the instruction
	// and that register is overwriten with the result of the call to return the value
	INSTR_CALL_BUILTIN,	// call_builtin func_id freg

	INSTR_RET,			// ret

	INSTR_COUNT,

	INSTR_SENTINEL,
};

enum Value_Location_Type {
	INTEGER_REGISTER,
	FLOATING_POINT_REGISTER,
	CONSTANT_BLOCK,
};

struct Value_Location_Info {
	u32 integer_register = 0;
	u32 floating_point_register = 0;
	Value_Id value_id;

	Value_Location_Type location_type;
};

Bytecode_Opcode bytecode_get_floating_point_version(Bytecode_Opcode opcode);

struct Bytecode_Instr {
	Bytecode_Opcode opcode = {};
	u16 op0 = 0;
	u16 op1 = 0;

	Bytecode_Instr() {}
	Bytecode_Instr(Bytecode_Opcode p_opcode)
		: opcode(p_opcode)
	{}

	Bytecode_Instr(Bytecode_Opcode p_opcode, u16 operand)
		: opcode(p_opcode), op0(operand)
	{}

	Bytecode_Instr(Bytecode_Opcode p_opcode, u16 operand_0, u16 operand_1)
		: opcode(p_opcode), op0(operand_0), op1(operand_1)
	{}
};

// @todo we don't need to create this from expression tree when we can build it at parse time and replace values in the literals on the tree with simple ids.
struct Constant_Block {
	DArray<float> real = {};
	DArray<s32> integer = {};

	Value_Id add_constant(Value value);
	Value get_value(Value_Id val_id);
};

struct Bytecode_Code {
	DArray<Bytecode_Instr> code;
};

#define CONDITION_RESULT				BIT(0)

#define COMPARISON_RESULT_EQUALS		BIT(4)
#define COMPARISON_RESULT_NOT_EQUALS	BIT(5)
#define COMPARISON_RESULT_GREATER_THAN	BIT(6)
#define COMPARISON_RESULT_LESS_THAN		BIT(7)

struct Bytecode_Processor {
	DArray<u32> regs;
	DArray<float> fregs;
	u32 program_counter = 0;
	u32 result_flags = 0;

	Bytecode_Processor()
		: regs(8), fregs(8)
	{}
};

struct Bytecode_Program {
	Bytecode_Processor processor = {};  // execution context
	Bytecode_Code code = {};			// code
	Constant_Block constant_block = {};

	Bytecode_Program() : processor(), code(), constant_block() {}

	u32 allocate_gp_register();
	u32 allocate_fp_register();

	void emit_bytecode_instruction(Bytecode_Opcode opcode, u16 arg0, u16 arg1);

	Value_Location_Info emit_load_constant(Value_Id value_id);

	void print_program();
};

bool bytecode_compile_expression(Bytecode_Program& program, Expr* expr);
float bytecode_run(Bytecode_Processor proc, Bytecode_Program program);

// - register usage
// - common subexpression elimination
void bytecode_optimize(Bytecode_Program& program);
