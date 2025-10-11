#pragma once

#include "common.h"
#include "template.h"
#include "expr.h"

enum Register_Type {
	REGISTER_TYPE_INT,
	REGISTER_TYPE_FLOAT,
};

struct Bytecode_Processor {
	Bucket_List<u32> regs;
	Bucket_List<float> fregs;
	u32 program_counter = 0;
	u32 result_flags = 0;

	Bytecode_Processor()
		: regs(8), fregs(8)
	{}
};

/*
	opcode(32 bit) operand1(16 bit), operand2(16 bit)

	64 bit fixed length instructions.
	Composed of a 32 bit opcode and a pair of 16 bit arguments.
	Some instructions may only use the first argument and discard the second slot.
	Some other ones might not need any arguments at all.
	Instructions can take arguments as registers or identifiers for known constants or functions.
	There are unlimited registers (as long as it can be addressed by the representable range of the id) and a constant block containing all the constants used in the expression for bytecode program to use.
*/

// @todo function calls in bytecode. Calling conventions

enum Bytecode_Opcode : u32
{
	INSTR_LOAD,			// load reg, const_int
	INSTR_LOADF,		// load freg, const_float
	INSTR_MOV,			// mov  reg, reg
	INSTR_MOVF,			// mov  freg, freg
	INSTR_MOV_FG,		// mov  freg, reg		-- bitwise copy of the value
	INSTR_MOV_GF,		// mov  reg, freg		-- bitwise copy of the value

	INSTR_CONV_INT,		// conv freg			-- convert a floating point value to an integer
	INSTR_CONV_F,		// conv reg				-- convert an integer to a floating point value
/*
	INSTR_PUSH,			// push reg
	INSTR_POP,			// pop  reg
	// read/write on to the stack. stack_offset is added to the stack pointer to find where to read/write
	INSTR_STACK_READ,	// read reg stack_offset
	INSTR_STACK_WRITE,	// wrt  reg stack_offset
*/
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

	INSTR_CALL,			// call   func_id
	INSTR_NEGATE,		// negate reg
	INSTR_NOT,			// not 	  reg

	INSTR_NEGATE_F,		// negate freg

	// jump
	INSTR_JMP,			// jmp op1

	// @note do we need carry or overflow information?
	INSTR_TEST_ZERO,	// tz reg
	INSTR_TEST_NEGATIVE,// tn reg

	INSTR_JMP_COND,		// jump if the last result of the last test was true

	INSTR_RET,			// ret

	INSTR_COUNT,

	INSTR_SENTINEL,
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


// there is space for 16 bits in the instruction

// For Value_Id the first 2 bits are the type of the value. Maps directly to value types.
// The remaining part is the index of the value in its type.

using Register_Id = u16;
using Value_Id = u16;
using Func_Id = u16;  // @todo functions on bytecode

Value_Id make_value_id(int index, Value_Type type);

// @todo we don't need to create this from expression tree when we can build it at parse time and replace values in the literals on the tree with simple ids.
struct Constant_Block {
	DArray<double> m_real = {};
	DArray<String> m_string = {};
	DArray<s64> m_integer = {};
	DArray<bool> m_bool = {};

	Value_Id add_constant(Value value);
};

struct Bytecode_Code {
	DArray<Bytecode_Instr> code;
};

enum class Value_Location_Type {
	CONSTANT_BLOCK,
	GP_REGISTER,
	FP_REGISTER,
	// STACK,
};

struct Value_Info {
	u16 location;
	Value_Type value_type;
	Value_Location_Type location_type;

	Value_Info(u16 p_location, Value_Type p_value_type, Value_Location_Type p_location_type)
		: location(p_location), value_type(p_value_type), location_type(p_location_type)
	{}

	Register_Id get_gp() { return (Register_Id)location; }
	Register_Id get_fp() { return (Register_Id)location; }
	Value_Id get_constant() { return (Value_Id)location; }
};

struct Bytecode_Program {
	Bytecode_Processor processor = {};
	Bytecode_Code code = {};
	Constant_Block constant_block = {};

	Bytecode_Program() : processor(), code(), constant_block() {}

	Value_Info compile_expression(Expr* expr);
private:
	u32 allocate_gp_register();
	u32 allocate_fp_register();

	void emit_bytecode_instruction(Bytecode_Opcode opcode, u16 arg0, u16 arg1);
};

// @todo return value
float run(Bytecode_Processor proc, Bytecode_Program program);
