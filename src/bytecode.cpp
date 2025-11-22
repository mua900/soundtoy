#include "bytecode.h"

Value_Location_Info compile_expr(Expr* expr, Bytecode_Program& program);

bool bytecode_compile_expression(Bytecode_Program& program, Expr* root) {

	program.reset();
	compile_expr(root, program);
    bytecode_optimize(program);

	return true;
}

// @todo handle errors
Value_Location_Info compile_expr(Expr* expr, Bytecode_Program& program) {
	switch (expr->type) {
        case Expr_Type::Literal: {
            auto literal = static_cast<Expr_Literal*>(expr);

            Value_Location_Info location;
            location.location_type = Value_Location_Type::CONSTANT_BLOCK;
            location.value_id = program.constant_block.add_constant(literal->value);

            program.emit_load_constant(location.value_id);

            return location;
        }
        case Expr_Type::Variable: {
            auto variable = static_cast<Expr_Variable*>(expr);

            Value_Location_Info location;
            switch (variable->variable_type) {
                case Value_Type::INTEGER:
                case Value_Type::BOOL:
                    {
                        location.location_type = Value_Location_Type::INTEGER_REGISTER;
                        location.integer_register = program.allocate_gp_register();
                        break;
                    }
                case Value_Type::REAL:
                    {
                        location.location_type = Value_Location_Type::FLOATING_POINT_REGISTER;
                        location.floating_point_register = program.allocate_fp_register();
                        break;
                    }
            }

            return location;
        }
        case Expr_Type::Unary: {
            auto unary = static_cast<Expr_Unary*>(expr);

            Value_Location_Info operand_location = compile_expr(unary->operand, program);

            // if the operand is inside the constant block.
            // load it and put it to a register first
            if (operand_location.location_type == Value_Location_Type::CONSTANT_BLOCK) {
                operand_location = program.emit_load_constant(operand_location.value_id);
            }

            if (unary->op == Unop_Negate) {
                if (operand_location.location_type == Value_Location_Type::INTEGER_REGISTER) {
                    program.emit_bytecode_instruction(INSTR_NEGATE, operand_location.integer_register, 0);
                }
                else if (operand_location.location_type == Value_Location_Type::FLOATING_POINT_REGISTER) {
                    program.emit_bytecode_instruction(INSTR_NEGATE_F, operand_location.floating_point_register, 0);
                }
            }
            else if (unary->op == Unop_Not) {
                program.emit_bytecode_instruction(INSTR_NOT, operand_location.integer_register, 0);
            }

            return operand_location;
        }
        case Expr_Type::Binary: {
            auto binary = static_cast<Expr_Binary*>(expr);

            Value_Location_Info left_location = compile_expr(binary->left, program);
            Value_Location_Info right_location = compile_expr(binary->right, program);

            if (left_location.location_type == Value_Location_Type::CONSTANT_BLOCK) {
                left_location = program.emit_load_constant(left_location.value_id);
            }
            else if (right_location.location_type == Value_Location_Type::CONSTANT_BLOCK) {
                right_location = program.emit_load_constant(right_location.value_id);
            }

            // if the results of the left and right branches are in different register files
            // then we need to decide on one of them to do the operations on and move the value
            // in the other file to that one.

            // we pick floating point registers since the other way around loses data and
            // we would be mostly dealing with floating point.

            if (right_location.location_type != left_location.location_type) {
                if (left_location.location_type == Value_Location_Type::INTEGER_REGISTER) {
                    left_location.location_type = Value_Location_Type::FLOATING_POINT_REGISTER;
                    left_location.floating_point_register = program.allocate_fp_register();
                }
                if (right_location.location_type == Value_Location_Type::INTEGER_REGISTER) {
                    right_location.location_type = Value_Location_Type::FLOATING_POINT_REGISTER;
                    right_location.floating_point_register = program.allocate_fp_register();
                }
            }

            bool is_using_integer_registers = left_location.location_type == Value_Location_Type::INTEGER_REGISTER;

            Bytecode_Opcode opcode;

            if (binary->op == Binop_Unknown) {
                panic("Unknown binary operation");
                break;
            }

            if (binop_is_arithmetic(binary->op))
            {
                if (is_using_integer_registers) {
                    program.emit_bytecode_instruction(opcode, left_location.integer_register, right_location.integer_register);
                }
                else {
                    program.emit_bytecode_instruction(opcode, left_location.floating_point_register, right_location.floating_point_register);
                }
            }
            else if (binop_is_comparison(binary->op))
            {
                opcode = (is_using_integer_registers) ? INSTR_CMP : INSTR_CMPF;

                if (is_using_integer_registers) {
                    program.emit_bytecode_instruction(opcode, left_location.integer_register, right_location.integer_register);
                }
                else {
                    program.emit_bytecode_instruction(opcode, left_location.floating_point_register, right_location.floating_point_register);
                }

                u16 test = 0;
                switch (binary->op) {
                    case Binop_Eq:  test = COMPARISON_RESULT_EQUALS; break;
                    case Binop_Neq: test = COMPARISON_RESULT_NOT_EQUALS; break;
                    case Binop_Gt:  test = COMPARISON_RESULT_GREATER_THAN; break;
                    case Binop_Ge:  test = COMPARISON_RESULT_GREATER_THAN | COMPARISON_RESULT_EQUALS; break;
                    case Binop_Lt:  test = COMPARISON_RESULT_LESS_THAN; break;
                    case Binop_Le:  test = COMPARISON_RESULT_LESS_THAN | COMPARISON_RESULT_EQUALS; break;
                }

                program.emit_bytecode_instruction(INSTR_TEST_RESULT, test, 0);
            }

            return left_location;
        }
        case Expr_Type::Grouping: {
            auto grouping = static_cast<Expr_Grouping*>(expr);
            return compile_expr(grouping->expr, program);
        }
        case Expr_Type::Call: {
            auto call = static_cast<Expr_Call*>(expr);

            Value_Location_Info location;

            if (is_builtin_function(call)) {
                u32 freg = program.allocate_fp_register();
                program.emit_bytecode_instruction(INSTR_CALL_BUILTIN, call->fn_id, freg);

                location.location_type = Value_Location_Type::FLOATING_POINT_REGISTER;
                location.floating_point_register = freg;
            }
            else {
                panic("User defined functions not implemented");
            }

            return location;
        }
	}
}

Value_Id Constant_Block::add_constant(Value value)
{
	int index = 0;

	switch (value.type)
	{
        case Value_Type::BOOL:  // fallthrough
		case Value_Type::INTEGER:
			{
				index = integer.add_unique(value.integer);
				break;
			}
		case Value_Type::REAL:
			{
				index = real.add_unique(value.real);
				break;
			}
		default:
		{
			panic("Unknown value type");
		}
	}

	Value_Id value_id;
    value_id.value_type = value.type;
    value_id.value_index = index;

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

        default:
            return INSTR_SENTINEL;
	}

	panic("Unhandled instruction for bytecode_get_floating_point_version");
}

// @todo proper register allocation

u32 Bytecode_Program::allocate_gp_register() {
	return processor.regs.add(0);
}

u32 Bytecode_Program::allocate_fp_register() {
	return processor.fregs.add(0.0);
}

void Bytecode_Program::emit_bytecode_instruction(Bytecode_Opcode opcode, u16 arg0, u16 arg1) {
	code.code.add(Bytecode_Instr(opcode, arg0, arg1));
}

Value_Location_Info Bytecode_Program::emit_load_constant(Value_Id value_id)
{
    Value_Location_Info location;

    switch (value_id.value_type) {
        case Value_Type::BOOL:
        case Value_Type::INTEGER:
            {
                u32 reg = allocate_gp_register();
                emit_bytecode_instruction(INSTR_LOAD, reg, value_id.value_index);
                location.location_type = Value_Location_Type::INTEGER_REGISTER;
                location.floating_point_register = reg;
                break;
            }
        case Value_Type::REAL:
            {
                u32 freg = allocate_fp_register();
                emit_bytecode_instruction(INSTR_LOADF, freg, value_id.value_index);
                location.location_type = Value_Location_Type::FLOATING_POINT_REGISTER;
                location.floating_point_register = freg;
                break;
            }
    }

    return location;
}

void Bytecode_Program::step_time(float step)
{
    constant_block.builtin_variable[BUILTIN_VARIABLE_TIME] += step;
}

void Bytecode_Program::reset() {
    processor.regs.reset();
    processor.fregs.reset();
    processor.program_counter = 0;
    processor.result_flags = 0;
    code.code.reset();
    constant_block.real.reset();
    constant_block.integer.reset();
    for (float& var : constant_block.builtin_variable) {
        var = 0;
    }
    get_default_builtin_functions(constant_block.builtin_function);
}

void Bytecode_Program::set_builtin_variable(double value, u32 builtin_variable) {
    constant_block.builtin_variable[builtin_variable] = value;
}

void Bytecode_Program::set_builtin_function(Builtin_Function implementation, u32 builtin_function) {
    constant_block.builtin_function[builtin_function] = implementation;
}

void Bytecode_Program::print_program() {
    printf("Processor\n");
    printf("Integers registers:\n");
    for (int i = 0; i < processor.regs.size(); i++) {
        printf("%d: %d\n", i, processor.regs.get(i));
    }
    printf("Floating point registers:\n");
    for (int i = 0; i < processor.regs.size(); i++) {
        printf("%d: %f\n", i, processor.fregs.get(i));
    }

    printf("Constant Block\n");
    printf("Integer constants:\n");
    for (int i = 0; i < constant_block.integer.size(); i++) {
        printf("%d\n", constant_block.integer.get(i));
    }
    printf("Float constants:\n");
    for (int i = 0; i < constant_block.real.size(); i++) {
        printf("%f\n", constant_block.real.get(i));
    }

    printf("\n");
    printf("Code:\n");
    for (auto instr : code.code) {
        printf("%s %x %x\n", opcode_string(instr.opcode), instr.op0, instr.op1);
    }
}

// -- Bytecode runner

float bytecode_run(Bytecode_Program& block)
{
	// @todo
	NOT_IMPLEMENTED("Bytecode run")
}

void bytecode_optimize(Bytecode_Program& program) {
    // not implemented
}


const char* opcode_string(Bytecode_Opcode opcode) {
    switch (opcode) {
        case INSTR_LOAD: return "INSTR_LOAD";
        case INSTR_LOADF: return "INSTR_LOADF";
        case INSTR_MOV: return "INSTR_MOV";
        case INSTR_MOVF: return "INSTR_MOVF";
        case INSTR_MOV_FG: return "INSTR_MOV_FG";
        case INSTR_MOV_GF: return "INSTR_MOV_GF";

        case INSTR_CONV_TO_INT: return "INSTR_CONV_TO_INT";
        case INSTR_CONV_TO_F: return "INSTR_CONV_TO_F";

        case INSTR_MOV_CONV_INT: return "INSTR_MOV_CONV_INT";
        case INSTR_MOV_CONV_F: return "INSTR_MOV_CONV_F";

        case INSTR_ADD: return "INSTR_ADD";
        case INSTR_SUB: return "INSTR_SUB";
        case INSTR_MUL: return "INSTR_MUL";
        case INSTR_DIV: return "INSTR_DIV";
        case INSTR_MOD: return "INSTR_MOD";

        case INSTR_ADDF: return "INSTR_ADDF";
        case INSTR_SUBF: return "INSTR_SUBF";
        case INSTR_MULF: return "INSTR_MULF";
        case INSTR_DIVF: return "INSTR_DIVF";
        case INSTR_MODF: return "INSTR_MODF";

        case INSTR_NEGATE: return "INSTR_NEGATE";
        case INSTR_NOT: return "INSTR_NOT";

        case INSTR_NEGATE_F: return "INSTR_NEGATE_F";

        case INSTR_CMP: return "INSTR_CMP";
        case INSTR_CMPF: return "INSTR_CMPF";

        case INSTR_TEST_RESULT: return "INSTR_TEST_RESULT";

        case INSTR_JMP: return "INSTR_JMP";
        case INSTR_JMP_COND: "INSTR_JMP_COND";

        case INSTR_CALL_BUILTIN: return "INSTR_CALL_BUILTIN";

        case INSTR_RET: return "INSTR_RET";

        case INSTR_COUNT: return "INSTR_COUNT";
        case INSTR_SENTINEL: return "INSTR_SENTINEL";
    }
}
