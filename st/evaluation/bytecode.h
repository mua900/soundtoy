#pragma once

#include "common.h"
#include "template.h"
#include "expr.h"
#include "builtin.h"

/*
Instruction layout:
	opcode(32 bit) operand1(16 bit), operand2(16 bit)
	64 bit fixed length instructions.

	Composed of a 32 bit opcode and a pair of 16 bit arguments.
	All instructions take 2 arguments but they aren't required to use either of them.
	The arguments can be registers or identifiers for known constants or functions.

Registers:
	The registers are seperated into independent integer and floating point files.
	There are no hard limits on register file size (it is limited by the addressable range of the 16-bit id).

Constant Block:
	The constant values used in the expression doesn't fit into the instruction.
	There is a seperate constant block created when compiling the expression that contains all constants used in the expression.
	They are used via instructions to load values from the constant block into registers.
*/

enum Bytecode_Opcode : u32
{
	// destination source syntax
	// arithmetic operations are destructive and overwrite the first argument register
	INSTR_LOAD,			// load reg, const_int
	INSTR_LOADF,		// load freg, const_float
	INSTR_LOAD_BUILTIN,	// load_builtin freg builtin_id
	INSTR_LOAD_VAR,		// load_var freg var_id
	INSTR_LOAD_I_TO_F,  // load freg, const_int
	INSTR_LOAD_F_TO_I,  // load reg, const_float

	INSTR_MOV,			// mov  reg, reg
	INSTR_MOVF,			// mov  freg, freg
	INSTR_MOV_I_TO_F,	// mov  freg, reg		   -- bitwise copy of the value
	INSTR_MOV_F_TO_I,	// mov  reg, freg		   -- bitwise copy of the value

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

	// the result of the comparison is stored in the relevant bits of the flags register.
	INSTR_CMP,			// cmp reg0, reg1
	INSTR_CMPF,			// cmpf freg0, freg1

	// test if the value stored in the argument register is 0 and set the condition bit to 0 if it is and 1 if it is not.
	INSTR_TEST,         // test reg
	INSTR_TEST_F,       // test freg

	// commit the result of the previous generic compare operation for a specific case to the condition bit
	INSTR_TEST_RESULT,	// test immediate8

	// jump
	INSTR_JMP,			// jmp  address16 (little endian)
	// jump if the condition bit is true (1)
	INSTR_JMP_COND,		// cjmp address16 (little endian)

	// calls the builtin function identified by func_id and place the result in freg
	// all builtin functions take a single floating point number as argument and return a single floating point number
	// the calling convention is that the register carrying the arguments gets passed in the instruction
	// and that register is overwriten with the result of the call to return the value
	INSTR_CALL_BUILTIN,	// call_builtin func_id freg

	// @todo support multiple return values

	// calls the function identified by func_id and place the result in the freg
	// the arguments are placed on the stack per call and used by the called function
	INSTR_CALL,			// call func_id freg

	// push a value to the value stack
	INSTR_PUSH,			// push freg

	INSTR_RET,			// ret freg  -- return the floating point value in the freg the result of the expression

	INSTR_COUNT,

	INSTR_SENTINEL,
};

const char* opcode_string(Bytecode_Opcode opcode);

enum Constant_Type {
	CONSTANT_TYPE_INTEGER,
	CONSTANT_TYPE_REAL,
	CONSTANT_TYPE_BUILTIN,
};

struct Constant_Id {
	Constant_Type constant_type;
	u16 constant_index;
};

using Func_Id = u16;

enum Value_Location_Type {
	INTEGER_REGISTER,
	FLOATING_POINT_REGISTER,
	CONSTANT_BLOCK,
};

struct Value_Location_Info {
	u32 integer_register = 0;
	u32 floating_point_register = 0;
	Constant_Id const_id = {};

	Value_Location_Type location_type = {};
};

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

struct Constant_Block {
	DArray<double> real = {};
	DArray<s64> integer = {};
	double builtin_variable[BUILTIN_VARIABLE_COUNT] = {};
	Builtin_Function_List builtin_function = {};
	
	Constant_Id add_constant(Value value);
};

struct Bytecode_Code {
	DArray<Bytecode_Instr> code;

	u32 index() const {
		return code.size() - 1;
	}

	u32 size() const {
		return code.size();
	}
};

#define CONDITION_RESULT				BIT(0)

#define COMPARISON_RESULT_EQUALS		BIT(4)
#define COMPARISON_RESULT_NOT_EQUALS	BIT(5)
#define COMPARISON_RESULT_GREATER_THAN	BIT(6)
#define COMPARISON_RESULT_LESS_THAN		BIT(7)

struct Bytecode_Processor {
	DArray<s64> regs = {};
	DArray<double> fregs = {};

	DArray<Value> stack = {};

	u32 result_flags = 0;

	Bytecode_Processor()
		: regs(8), fregs(8)
	{}
};

struct InputStream {
	Array<float> samples = {};
	int stride = 0;
	int sample_index = 0; // @todo a builtin variable to access this

	InputStream() {}
	InputStream(Array<float> p_samples, int p_stride)
		:
		samples(p_samples), stride(p_stride)
	{}
};

struct Bytecode_Program {
	Bytecode_Processor processor = {};
	Bytecode_Code code = {};
	Constant_Block constant_block = {};
	InputStream input_stream = {};
	int sample_index = 0;

	DArray<Variable> symbols;
	DArray<double> variables = {};

	Bytecode_Program() : processor(), code(), constant_block() {
		get_default_builtin_functions(constant_block.builtin_function);
	}

	void reset();

	u16 allocate_gp_register();
	u16 allocate_fp_register();

	void copy_value_to_fp_register(Value_Location_Info val_loc, u16 dest_reg);
	u16 get_value_to_fp_register(Value_Location_Info val_info);
	u16 get_value_to_gp_register(Value_Location_Info val_info);

	void emit_bytecode_instruction(Bytecode_Opcode opcode, u16 arg0, u16 arg1);

	Value_Location_Info emit_load_constant(Constant_Id value_id);

    int add_symbol(Variable symbol) {
        symbols.add(symbol);
        return variables.add(0.0);
    }

	void set_input_stream(InputStream istream);

	void set_builtin_variable(double value, u32 builtin_id);
	void set_builtin_function(StFunction  implementation, u32 builtin_function);
	void step_time(double time);
	void set_sample_time(double sample_time);
	void set_sample_rate(double sample_rate);
	double get_sample_rate() const;
	double get_sample_time() const;
	
	void print_program() const;
};

bool bytecode_compile_expression(Bytecode_Program& program, Expr* expr);
double bytecode_run(Bytecode_Program& program);
