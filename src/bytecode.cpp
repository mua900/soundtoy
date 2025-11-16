#include "bytecode.h"

Bytecode_Program bytecode_compile_expression(Expr* expr)
{
	// @todo
	switch (expr->type)
	{
		case Expr_Type::Literal: { break; }
		case Expr_Type::Variable: { break;  }
		case Expr_Type::Unary: { break; }
		case Expr_Type::Binary: { break; }
		case Expr_Type::Grouping: { break; }
		case Expr_Type::Call: { break; }
	}

	return Bytecode_Program();
}

Value_Id make_value_id(int index, Value_Type type)
{
	Value_Id value_id = index;
	value_id >>= 2;
	value_id |= (Value_Id)type;
	return value_id;
}

Value_Id Constant_Block::add_constant(Value value)
{
	int index = 0;

	switch (value.type)
	{
		case Value_Type::INTEGER:
			{
				index = m_integer.add_unique(value.integer);
				break;
			}
		case Value_Type::REAL:
			{
				index = m_real.add_unique(value.real);
				break;
			}
		case Value_Type::BOOL:
			{
				index = m_bool.add_unique(value.boolean);
				break;
			}
		case Value_Type::STRING:
			{
				index = m_string.add_unique(value.string);
				break;
			}
		default:
		{
			panic("Unknown value type");
		}
	}

	Value_Id value_id = make_value_id(index, value.type);

	return value_id;
}


Bytecode_Opcode bytecode_get_floating_point_version(Bytecode_Opcode opcode)
{
	switch (opcode)
	{
		case INSTR_LOAD:
			return INSTR_LOADF;
		case INSTR_MOV:
			return INSTR_MOVF;

		case INSTR_ADD:
			return INSTR_ADDF;
		case INSTR_SUB:
			return INSTR_SUBF;
		case INSTR_MUL:
			return INSTR_MULF;
		case INSTR_DIV:
			return INSTR_DIVF;
		case INSTR_MOD:
			return INSTR_MODF;

		case INSTR_NEGATE:
			return INSTR_NEGATE_F;

		case INSTR_MOV_FG:
		case INSTR_MOV_GF:
		case INSTR_CONV_INT:
		case INSTR_CONV_F:
		case INSTR_CALL:
		case INSTR_NOT:
		case INSTR_JMP:
		case INSTR_TEST_ZERO:
		case INSTR_TEST_NEGATIVE:
		case INSTR_JMP_COND:
		case INSTR_RET:
			return INSTR_SENTINEL;
	}

	panic("Unhandled instruction for bytecode_get_floating_point_version");
}

u32 Bytecode_Program::allocate_gp_register()
{
	// @todo
	return u32();
}

u32 Bytecode_Program::allocate_fp_register()
{
	// @todo
	return u32();
}

void Bytecode_Program::emit_bytecode_instruction(Bytecode_Opcode opcode, u16 arg0, u16 arg1)
{
	code.code.add(Bytecode_Instr(opcode, arg0, arg1));
}

// -- Bytecode runner

float bytecode_run(Bytecode_Processor proc, Bytecode_Program block)
{
	// @todo
	NOT_IMPLEMENTED("Bytecode run")
}
