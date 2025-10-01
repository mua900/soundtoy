#pragma once

#include "common.h"
#include "template.h"
#include "expr.h"

enum GP_Register {
	R_0,
	R_1,
	R_2,
	R_3,
	R_4,
	R_5,
	REGISTER_COUNT,
};

enum FP_Register {
	F_0,
	F_1,
	F_2,
	F_3,
	F_4,
	F_5,
	FP_REGISTER_COUNT,
};

#define PROCESSOR_FLAG_ZERO 	BIT(0)
#define PROCESSOR_FLAG_NEGATIVE BIT(1)
#define PROCESSOR_FLAG_CARRY	BIT(2)
#define PROCESSOR_FLAG_OVERFLOW BIT(3)

struct Processor {
	u32 regs[REGISTER_COUNT] = {};
	float fregs[FP_REGISTER_COUNT] = {};
	u16 program_counter = 0;
	u8 flags = 0;
};


/*
	3AC bytecode
	opcode(32 bit) operand1(16 bit), operand2(16 bit)

	64 bit fixed length instructions.
	Composed of a 32 bit opcode and a pair of 16 bit arguments.
	Some instructions may only use the first argument and discard the second slot.
	Some other ones might not need any arguments at all.
	Instruction lengths must stay fixed either way.
	Instructions can take arguments as registers or identifiers for known constants or functions.
	Currently we don't need heap memory for this programming model.
	This only has a stack and push and pop instructions to interact with it.
	The stack can also be used internally for other common tasks like saving return addresses, saving context and so on.

	Instructions that evaluate a value store the result in the first register they take as argument.
*/

enum Bytecode_Opcode : u32
{
	INSTR_MOV,		// mov reg, const_id
	// INSTR_LOAD,	// load reg, mem_location
	// INSTR_WRT,   // wrt reg, mem_location
	INSTR_PUSH,		// push reg
	INSTR_POP,		// pop reg

	INSTR_ADD,		// add reg1, reg2
	INSTR_SUB,		// sub reg1, reg2
	INSTR_MUL,		// mul reg1, reg2
	INSTR_DIV,		// div reg1, reg2
	INSTR_MOD,		// mod reg1, reg2

	INSTR_CALL,		// call func_id
	INSTR_NEGATE,	// negate op1
	INSTR_NOT,		// not op1

	INSTR_JMP,		// jmp op1
	INSTR_JZ,		// jz op1
	INSTR_JNZ,		// jnz op1
	INSTR_RET,		// ret

	INSTR_COUNT,
};

struct Bytecode_Instr {
	Bytecode_Opcode opcode = {};
	u16 operand_0 = 0;
	u16 operand_1 = 0;
};

struct Bytecode_Program {
	DArray<Bytecode_Instr> code = {};
	DArray<double> constant_block = {};

	Bytecode_Program(DArray<Bytecode_Instr> instrs, DArray<double> cblock) : code(instrs), constant_block(cblock) {}
};

Bytecode_Program compile_expr(Expr* expr);

// @todo return value
void run(Processor proc, Bytecode_Program program);
