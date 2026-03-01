#include "bytecode.h"

#include <cmath>

Value_Location_Info compile_expr(Expr* expr, Bytecode_Program& program);
static Bytecode_Opcode get_arithmetic_binop_opcode_integer(Op_Binary binary);
static Bytecode_Opcode get_arithmetic_binop_opcode_float(Op_Binary binary);
static bool opcode_is_binary(Bytecode_Opcode opcode);
static bool opcode_is_unary(Bytecode_Opcode opcode);


bool bytecode_compile_expression(Bytecode_Program& program, Expr* root) {
    program.reset();

    Value_Location_Info location = compile_expr(root, program);
    if (location.location_type == Value_Location_Type::INTEGER_REGISTER) {
        u16 freg = program.allocate_float_register();
        program.emit_bytecode_instruction(INSTR_MOV_I_TO_F, freg, location.integer_register);

        location.location_type = Value_Location_Type::FLOATING_POINT_REGISTER;
        location.floating_point_register = freg;
    }
    program.emit_bytecode_instruction(INSTR_RET, location.floating_point_register, 0);

    program.print_program();

	return true;
}

Value_Location_Info compile_expr(Expr* expr, Bytecode_Program& program) {
	switch (expr->type) {
        case Expr_Type::Literal: {
            auto literal = static_cast<Expr_Literal*>(expr);

            Constant_Id const_id = program.constant_block.add_constant(literal->value);
            Value_Location_Info loaded_location = program.emit_load_constant(const_id);
            u16 freg = program.get_value_to_float_register(loaded_location);

            Value_Location_Info location;
            location.location_type = Value_Location_Type::FLOATING_POINT_REGISTER;
            location.floating_point_register = freg;

            return location;
        }
        case Expr_Type::Variable: {
            auto variable = static_cast<Expr_Variable*>(expr);

            if (is_builtin_variable(variable)) {
                u16 freg = program.allocate_float_register();
                program.emit_bytecode_instruction(INSTR_LOAD_BUILTIN, freg, variable->var_id);

                Value_Location_Info builtin_location_info;
                builtin_location_info.location_type = Value_Location_Type::FLOATING_POINT_REGISTER;
                builtin_location_info.floating_point_register = freg;

                return builtin_location_info;
            }
            else {
                auto freg = program.allocate_float_register();

                Value_Location_Info location;
                location.location_type = Value_Location_Type::FLOATING_POINT_REGISTER;
                location.floating_point_register = freg;

                program.emit_bytecode_instruction(INSTR_LOAD_VAR, freg, variable->var_id);

                return location;
            }
        }
        case Expr_Type::Unary: {
            auto unary = static_cast<Expr_Unary*>(expr);

            Value_Location_Info operand_location = compile_expr(unary->operand, program);

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

            // if the results of the left and right branches are in different register files
            // then we need to decide on one of them to do the operations on and move the value
            // in the other file to that one.

            // we pick floating point registers since the other way around loses data and
            // we would be mostly dealing with floating point.

            if (right_location.location_type != left_location.location_type) {
                if (left_location.location_type == Value_Location_Type::INTEGER_REGISTER) {
                    auto freg = program.allocate_float_register();
                    program.emit_bytecode_instruction(INSTR_MOV_I_TO_F, freg, left_location.integer_register);

                    left_location.location_type = Value_Location_Type::FLOATING_POINT_REGISTER;
                    left_location.floating_point_register = freg;
                }
                if (right_location.location_type == Value_Location_Type::INTEGER_REGISTER) {
                    auto freg = program.allocate_float_register();
                    program.emit_bytecode_instruction(INSTR_MOV_I_TO_F, freg, right_location.integer_register);

                    right_location.location_type = Value_Location_Type::FLOATING_POINT_REGISTER;
                    right_location.floating_point_register = freg;
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
                    opcode = get_arithmetic_binop_opcode_integer(binary->op);
                    program.emit_bytecode_instruction(opcode, left_location.integer_register, right_location.integer_register);
                }
                else {
                    opcode = get_arithmetic_binop_opcode_float(binary->op);
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
                    default:
                        panic("Bytecode Compilation: Unexpected binary comparison");
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
                Function builtin = program.constant_block.builtin_function[call->fn_id];

                // should be checked before
                ASSERT (call->arguments.size == builtin.signature.parameter_types.size);

                int arg_count = builtin.signature.parameter_types.size;

                if (arg_count == 1) {
                    Expr* argument = call->arguments.get(0);
                    Value_Location_Info arg_location = compile_expr(argument, program);

                    u16 freg = program.get_value_to_float_register(arg_location);

                    program.emit_bytecode_instruction(INSTR_CALL_BUILTIN, call->fn_id, freg);

                    location.location_type = Value_Location_Type::FLOATING_POINT_REGISTER;
                    location.floating_point_register = freg;
                }
                else {
                    // @todo support multiple return values

                    program.processor.stack.ensure_size(arg_count);

                    u16 ret_register = program.allocate_float_register();

                    for (int i = 0; i < arg_count; i += 1) {
                        Expr* argument = call->arguments.get(i);

                        Value_Location_Info loc = compile_expr(argument, program);
                        u16 freg = program.get_value_to_float_register(loc);
                        program.emit_bytecode_instruction(INSTR_PUSH, freg, 0);
                    }

                    program.emit_bytecode_instruction(INSTR_CALL, call->fn_id, ret_register);

                    location.location_type = Value_Location_Type::FLOATING_POINT_REGISTER;
                    location.floating_point_register = ret_register;
                }
            }
            else {
                panic("User defined functions not implemented");
            }

            return location;
        }
    	case Expr_Type::Ternary: {
			auto ternary = static_cast<Expr_Ternary*>(expr);

			u32 then_length;
			u32 else_length;

			{
				// @hack
				Bytecode_Program dumy;

                bytecode_compile_expression(dumy, ternary->condition);

				then_length = dumy.code.size();

			    dumy.reset();

				else_length = dumy.code.size();
				dumy.reset();
			}

			Value_Location_Info cond_result = compile_expr(ternary->condition, program);

			if (cond_result.location_type == Value_Location_Type::INTEGER_REGISTER) {
				program.emit_bytecode_instruction(INSTR_TEST, cond_result.integer_register, 0);
			}
			else if (cond_result.location_type == Value_Location_Type::FLOATING_POINT_REGISTER) {
				program.emit_bytecode_instruction(INSTR_TEST_F, cond_result.floating_point_register, 0);
			}

			// both paths will write their results to a common register
			// emulating returning
			u32 result_register = program.allocate_float_register();

			// there is no jump if not instruction so else block goes first and then block goes second
			// + 1 is for the jump instruction at the beginning
			// additional + 2 for the necessary jump and copy at the end of the branch that will go on top of the other one
			u32 then_branch = program.code.index() + 1 + else_length + 2;
			u32 else_branch = program.code.index() + 1;
			u32 expr_end = program.code.index() + 1 + else_length + 2 + then_length;

			program.emit_bytecode_instruction(INSTR_JMP_COND, then_branch, 0);

			Value_Location_Info else_loc = compile_expr(ternary->else_, program);
			program.emit_bytecode_instruction(INSTR_JMP, expr_end, 0);
			program.copy_value_to_float_register(else_loc, result_register);  // single instruction

			Value_Location_Info then_loc = compile_expr(ternary->then_, program);
			program.copy_value_to_float_register(then_loc, result_register);

			Value_Location_Info result_loc;
			result_loc.location_type = Value_Location_Type::FLOATING_POINT_REGISTER;
			result_loc.floating_point_register = result_register;

			return result_loc;
		}
        case Expr_Type::Tuple: {
            // @todo
            panic("How do we compile tuples?");
        }
        default: {
            panic("Unknown expression type");
        }
	}
}

Constant_Id Constant_Block::add_constant(Value value)
{
    Constant_Id const_id;

	switch (value.type)
	{
        case Var_Type_Boolean:  // fallthrough
		case Var_Type_Integer:
			{
				const_id.constant_index = integer.add_unique(value.integer);
                const_id.constant_type = CONSTANT_TYPE_INTEGER;
				break;
			}
		case Var_Type_Real:
			{
				const_id.constant_index = real.add_unique(value.real);
                const_id.constant_type = CONSTANT_TYPE_REAL;
				break;
			}
		default:
		{
			panic("Unknown value type");
		}
	}

	return const_id;
}

u16 Bytecode_Program::allocate_integer_register() {
	return processor.regs.add(0);
}

u16 Bytecode_Program::allocate_float_register() {
	return processor.fregs.add(0.0);
}

// guaranteed to output a single instruction
void Bytecode_Program::copy_value_to_float_register(Value_Location_Info val_loc, u16 dest_reg) {
    if (val_loc.location_type == Value_Location_Type::FLOATING_POINT_REGISTER) {
		emit_bytecode_instruction(INSTR_MOVF, dest_reg, val_loc.floating_point_register);
    }
    else if (val_loc.location_type == Value_Location_Type::INTEGER_REGISTER) {
        emit_bytecode_instruction(INSTR_MOV_I_TO_F, dest_reg, val_loc.integer_register);
    }
}

u16 Bytecode_Program::get_value_to_float_register(Value_Location_Info val_info)
{
    if (val_info.location_type == Value_Location_Type::FLOATING_POINT_REGISTER) {
        return val_info.floating_point_register;
    }
    else {
        u16 freg = allocate_float_register();
        if (val_info.location_type == Value_Location_Type::INTEGER_REGISTER) {
            emit_bytecode_instruction(INSTR_MOV_I_TO_F, freg, val_info.integer_register);
        }

        return freg;
    }
}

u16 Bytecode_Program::get_value_to_integer_register(Value_Location_Info val_info)
{
    if (val_info.location_type == Value_Location_Type::INTEGER_REGISTER) {
        return val_info.integer_register;
    }
    else {
        u16 reg = allocate_integer_register();
        if (val_info.location_type == Value_Location_Type::FLOATING_POINT_REGISTER) {
            emit_bytecode_instruction(INSTR_MOV_I_TO_F, reg, val_info.floating_point_register);
        }

        return reg;
    }
}

void Bytecode_Program::emit_bytecode_instruction(Bytecode_Opcode opcode, u16 arg0, u16 arg1) {
	code.code.add(Bytecode_Instr(opcode, arg0, arg1));
}

Value_Location_Info Bytecode_Program::emit_load_constant(Constant_Id const_id)
{
    Value_Location_Info location;

    switch (const_id.constant_type) {
        case CONSTANT_TYPE_INTEGER: {
                u32 reg = allocate_integer_register();
                emit_bytecode_instruction(INSTR_LOAD, reg, const_id.constant_index);
                location.location_type = Value_Location_Type::INTEGER_REGISTER;
                location.integer_register = reg;
                break;
            }
        case CONSTANT_TYPE_REAL: {
                u32 freg = allocate_float_register();
                emit_bytecode_instruction(INSTR_LOADF, freg, const_id.constant_index);
                location.location_type = Value_Location_Type::FLOATING_POINT_REGISTER;
                location.floating_point_register = freg;
                break;
            }
        case CONSTANT_TYPE_BUILTIN: {
            u32 freg = allocate_float_register();
            emit_bytecode_instruction(INSTR_LOAD_BUILTIN, freg, const_id.constant_index);
            location.location_type = Value_Location_Type::FLOATING_POINT_REGISTER;
            location.floating_point_register = freg;
            break;
        }
    }

    return location;
}

void Bytecode_Program::step_time(double step)
{
    constant_block.builtin_variable[BUILTIN_VARIABLE_TIME] += step;
}

void Bytecode_Program::set_input_stream(InputStream istream)
{
    input_stream = istream;
}

void Bytecode_Program::set_sample_rate(double sample_rate) {
    constant_block.builtin_variable[BUILTIN_VARIABLE_SAMPLE_RATE] = sample_rate;
}

double Bytecode_Program::get_sample_rate() const {
    return constant_block.builtin_variable[BUILTIN_VARIABLE_SAMPLE_RATE];
}

void Bytecode_Program::set_sample_time(double sample_time) {
    constant_block.builtin_variable[BUILTIN_VARIABLE_TIME] = sample_time;
}

double Bytecode_Program::get_sample_time() const {
    return constant_block.builtin_variable[BUILTIN_VARIABLE_TIME];
}

void Bytecode_Program::reset() {
    processor.regs.reset();
    processor.fregs.reset();
    processor.result_flags = 0;
    code.code.reset();
    constant_block.real.reset();
    constant_block.integer.reset();
    constant_block.builtin_variable[BUILTIN_VARIABLE_TIME] = 0.0;
    get_default_builtin_functions(constant_block.builtin_function);
    sample_index = 0;
}

void Bytecode_Program::set_builtin_variable(double value, u32 builtin_variable) {
    constant_block.builtin_variable[builtin_variable] = value;
}

// Be very careful matching the signatures before using this.
void Bytecode_Program::set_builtin_function(StFunction implementation, u32 builtin_function) {
    constant_block.builtin_function[builtin_function].implementation = implementation;
}

void Bytecode_Program::print_program() const {
    String_Builder builder(1024);

    builder.append(make_string("=== Bytecode Program ===\n"));

    {
        builder.append(make_string("Program stats: \n"));

        builder.append(make_string("    "));
        builder.append(make_string("Floating point register count: "));
        builder.append_integer(processor.fregs.size());
        builder.append_char('\n');

        builder.append(make_string("    "));
        builder.append(make_string("Integer register count: "));
        builder.append_integer(processor.regs.size());
        builder.append_char('\n');
    }

    builder.append_char('\n');

    {
        builder.append(make_string("Code: \n"));
        for (int i = 0; i < code.code.size(); i++) {
            auto instr = code.code.get(i);
            String opcode = make_string(opcode_string(instr.opcode));
            builder.append(make_string("    "));
            builder.append(opcode);
            builder.append_char(' ');

            switch (instr.opcode) {
                case INSTR_JMP:     // fallthrough
                case INSTR_JMP_COND: {
                    u16 address = ((u16)instr.op0 << 8) | ((u16)instr.op1);
                    char address_string[64];
                    snprintf(address_string, sizeof(address_string), "0x%08x", address);
                    builder.append(make_string(address_string));
                    break;
                }
                case INSTR_TEST_RESULT: {
                    u16 test = instr.op0;
                    char address_string[64];
                    snprintf(address_string, sizeof(address_string), "0x%08x", test);
                    builder.append(make_string(address_string));
                    break;
                }
                case INSTR_LOAD:    // fallthrough
                case INSTR_LOADF: {
					auto register_index = instr.op0;
					auto const_index = instr.op1;
                    builder.append_integer(register_index);
                    builder.append_char(' ');

                    builder.append(make_string("const ["));
                    builder.append_integer(const_index);
                    builder.append(make_string("]   ("));
					if (instr.opcode == INSTR_LOAD) {
						builder.append_integer(constant_block.integer.get(const_index));
					}
					else if (instr.opcode == INSTR_LOADF) {
						builder.append_float(constant_block.real.get(const_index));
					}

					builder.append_char(')');
                    break;
                }
                case INSTR_LOAD_BUILTIN: {
                    builder.append_integer(instr.op0);
                    builder.append_char(' ');

                    const char* builtin_name = "unknown_builtin";
                    if (instr.op1 == BUILTIN_VARIABLE_TIME) {
                        builtin_name = "time";
                    }
                    else if (instr.op1 == BUILTIN_VARIABLE_SAMPLE_RATE) {
                        builtin_name = "sample_rate";
                    }

                    builder.append(make_string(builtin_name));
                    break;
                }
                default: {
                    if (opcode_is_binary(instr.opcode)) {  // @todo knowning register types would be useful
                        builder.append_integer(instr.op0);
                        builder.append_char(' ');
                        builder.append_integer(instr.op1);
                    }
                    else if (opcode_is_unary(instr.opcode)) {
                        builder.append_integer(instr.op0);
                    }
                }
            }

            builder.append_char('\n');
        }
    }

    builder.append_char('\n');

    {
        builder.append(make_string("Constant Block: \n"));
        builder.append(make_string("    Reals: \n"));
        for (auto real : constant_block.real) {
            builder.append(make_string("    "));
            char real_string[64];
            snprintf(real_string, sizeof(real_string), "%.3f", real);
            builder.append(make_string(real_string));
            builder.append_char('\n');
        }

        builder.append(make_string("    Integers: \n"));
        for (auto integer : constant_block.integer) {
            builder.append(make_string("    "));
            builder.append_integer(integer);
            builder.append_char('\n');
        }

        builder.append(make_string("    Builtin Variables: \n"));
        for (double builtin : constant_block.builtin_variable) {
            builder.append(make_string("    "));
            char string[64];
            snprintf(string, sizeof(string), "%.3f", builtin);
            builder.append(make_string(string));
            builder.append_char('\n');
        }
    }

    printf("%s\n", builder.c_string());

    builder.free_buffer();
}

// -- Bytecode runner

double bytecode_run(Bytecode_Program& program)
{
    Bytecode_Processor& processor = program.processor;
    Constant_Block& constant_block = program.constant_block;
    Bytecode_Code& code = program.code;
	DArray<double>& variables = program.variables;

#define BYTECODE_PROGRAM_MAXIMUM_ITERATION_COUNT 2000
    int iteration_count = 0;
    int instruction_pointer = 0;

    while (instruction_pointer < code.code.size()) {
        if (iteration_count >= BYTECODE_PROGRAM_MAXIMUM_ITERATION_COUNT) {
            panic("Too many iterations in bytecode program");
        }

        Bytecode_Instr instr = code.code.get(instruction_pointer);

        instruction_pointer += 1;
        iteration_count += 1;

        switch (instr.opcode) {
            case INSTR_LOAD: {
                u16 reg = instr.op0;
                u16 const_index = instr.op1;

                processor.regs.get_ref(reg) = constant_block.integer.get(const_index);
                break;
            }
            case INSTR_LOADF: {
                u16 freg = instr.op0;
                u16 const_index = instr.op1;

                processor.fregs.get_ref(freg) = constant_block.real.get(const_index);
                break;
            }
            case INSTR_LOAD_BUILTIN: {
                u16 freg = instr.op0;
                u16 builtin_index = instr.op1;

                processor.fregs.get_ref(freg) = constant_block.builtin_variable[builtin_index];

                break;
            }
		case INSTR_LOAD_VAR: {
			u16 freg = instr.op0;
			u16 var_id = instr.op1;

			processor.fregs.get_ref(freg) = variables.get(var_id);
			break;
		}
		case INSTR_LOAD_I_TO_F: {
			u16 freg = instr.op0;
			u16 const_int = instr.op1;

			processor.fregs.get_ref(freg) = constant_block.integer.get(const_int);
			break;
		}
		case INSTR_LOAD_F_TO_I: {
			u16 reg = instr.op0;
			u16 const_float = instr.op1;

			processor.regs.get_ref(reg) = constant_block.real.get(const_float);
			break;
		}
            case INSTR_MOV: {
                u16 reg_0 = instr.op0;
                u16 reg_1 = instr.op1;

                processor.regs.get_ref(reg_0) = processor.regs.get(reg_1);
                break;
            }
            case INSTR_MOVF: {
                u16 freg_0 = instr.op0;
                u16 freg_1 = instr.op1;

                processor.fregs.get_ref(freg_0) = processor.fregs.get(freg_1);
                break;
            }

            case INSTR_MOV_I_TO_F: {
                u16 freg = instr.op0;
                u16 ireg = instr.op1;

                processor.fregs.get_ref(freg) = (double)processor.regs.get(ireg);
                break;
            }
            case INSTR_MOV_F_TO_I: {
                u16 ireg = instr.op0;
                u16 freg = instr.op1;

                processor.regs.get_ref(ireg) = (s64)processor.fregs.get(freg);
                break;
            }

            case INSTR_ADD: {
                s64 op0 = processor.regs.get(instr.op0);
                s64 op1 = processor.regs.get(instr.op1);

                processor.regs.get_ref(instr.op0) = op0 + op1;
                break;
            }
            case INSTR_SUB: {
                s64 op0 = processor.regs.get(instr.op0);
                s64 op1 = processor.regs.get(instr.op1);

                processor.regs.get_ref(instr.op0) = op0 - op1;
                break;
            }
            case INSTR_MUL: {
                s64 op0 = processor.regs.get(instr.op0);
                s64 op1 = processor.regs.get(instr.op1);

                processor.regs.get_ref(instr.op0) = op0 * op1;
                break;
            }
            case INSTR_DIV: {
                s64 op0 = processor.regs.get(instr.op0);
                s64 op1 = processor.regs.get(instr.op1);

                processor.regs.get_ref(instr.op0) = op0 / op1;
                break;
            }
            case INSTR_MOD: {
                s64 op0 = processor.regs.get(instr.op0);
                s64 op1 = processor.regs.get(instr.op1);

                processor.regs.get_ref(instr.op0) = op0 % op1;

                break;
            }

            case INSTR_ADDF: {
                double op0 = processor.fregs.get(instr.op0);
                double op1 = processor.fregs.get(instr.op1);

                processor.fregs.get_ref(instr.op0) = op0 + op1;

                break;
            }
            case INSTR_SUBF: {
                double op0 = processor.fregs.get(instr.op0);
                double op1 = processor.fregs.get(instr.op1);

                processor.fregs.get_ref(instr.op0) = op0 - op1;

                break;
            }
            case INSTR_MULF: {
                double op0 = processor.fregs.get(instr.op0);
                double op1 = processor.fregs.get(instr.op1);

                processor.fregs.get_ref(instr.op0) = op0 * op1;
                break;
            }
            case INSTR_DIVF: {
                double op0 = processor.fregs.get(instr.op0);
                double op1 = processor.fregs.get(instr.op1);

                processor.fregs.get_ref(instr.op0) = op0 / op1;

                break;
            }
            case INSTR_MODF: {
                double op0 = processor.fregs.get(instr.op0);
                double op1 = processor.fregs.get(instr.op1);

                processor.fregs.get_ref(instr.op0) = fmodf(op0, op1);

                break;
            }

            case INSTR_NEGATE: {
                s64 value = processor.regs.get(instr.op0);
                processor.regs.get_ref(instr.op0) = - value;
                break;
            }
            case INSTR_NOT: {
                s64 value = processor.regs.get(instr.op0);
                processor.regs.get_ref(instr.op0) = (value == 0);
                break;
            }

            case INSTR_NEGATE_F: {
                double value = processor.fregs.get(instr.op0);
                processor.fregs.get_ref(instr.op0) = - value;
                break;
            }

            case INSTR_CMP: {
                s64 left = processor.regs.get(instr.op0);
                s64 right = processor.regs.get(instr.op1);
                if (left == right) {
                    processor.result_flags |= COMPARISON_RESULT_EQUALS;
                    processor.result_flags &= ~COMPARISON_RESULT_NOT_EQUALS;
                }
                else {
                    processor.result_flags |= COMPARISON_RESULT_NOT_EQUALS;
                    processor.result_flags &= ~COMPARISON_RESULT_EQUALS;
                }

                if (left > right) {
                    processor.result_flags |= COMPARISON_RESULT_GREATER_THAN;
                    processor.result_flags &= ~COMPARISON_RESULT_LESS_THAN;
                }
                else {
                    processor.result_flags |= COMPARISON_RESULT_LESS_THAN;
                    processor.result_flags &= ~COMPARISON_RESULT_GREATER_THAN;
                }

                break;
            }
            case INSTR_CMPF: {
                double left = processor.fregs.get(instr.op0);
                double right = processor.fregs.get(instr.op1);
                if (left == right) {
                    processor.result_flags |= COMPARISON_RESULT_EQUALS;
                    processor.result_flags &= ~COMPARISON_RESULT_NOT_EQUALS;
                }
                else {
                    processor.result_flags |= COMPARISON_RESULT_NOT_EQUALS;
                    processor.result_flags &= ~COMPARISON_RESULT_EQUALS;
                }

                if (left > right) {
                    processor.result_flags |= COMPARISON_RESULT_GREATER_THAN;
                    processor.result_flags &= ~COMPARISON_RESULT_LESS_THAN;
                }
                else {
                    processor.result_flags |= COMPARISON_RESULT_LESS_THAN;
                    processor.result_flags &= ~COMPARISON_RESULT_GREATER_THAN;
                }

                break;
            }
		case INSTR_TEST: {
			u16 reg = instr.op0;
			u16 result = processor.regs.get(reg);
			if (result) {
				processor.result_flags |= CONDITION_RESULT;
			}
			else {
				processor.result_flags &= ~CONDITION_RESULT;
			}

			break;
		}
		case INSTR_TEST_F: {
			u16 freg = instr.op0;
			u16 result = processor.fregs.get(freg);
			if (result != 0.0) {
				processor.result_flags |= CONDITION_RESULT;
			}
			else {
				processor.result_flags &= ~CONDITION_RESULT;
			}

			break;
		}

            case INSTR_TEST_RESULT: {
                u32 test_value = (u32)instr.op0;

                u32 result = test_value & processor.result_flags;
                if (result) {
                    processor.result_flags |= CONDITION_RESULT;
                }
                else {
                    processor.result_flags &= ~CONDITION_RESULT;
                }

                break;
            }

            case INSTR_JMP: {
                u16 address = ((u16)instr.op0 << 8) | ((u16)instr.op1);
                instruction_pointer = address;

                break;
            }
            case INSTR_JMP_COND: {
                if (processor.result_flags & CONDITION_RESULT) {
                    u16 address = ((u16)instr.op0 << 8) | ((u16)instr.op1);
                    instruction_pointer = address;
                }

                break;
            }

            case INSTR_CALL_BUILTIN: {
                u16 fn_id = instr.op0;
                u16 freg = instr.op1;

				Value param = Value(processor.fregs.get(freg));
                Value result;
                call_function(program.constant_block.builtin_function[fn_id], &param, &result);

                processor.fregs.get_ref(freg) = result.real;
                break;
            }

            case INSTR_CALL: {
                u16 fn_id = instr.op0;
                u16 result_fregister = instr.op1;

                Value result;
                call_function(constant_block.builtin_function[fn_id], processor.stack.data(), &result);

                processor.stack.discard_data();  // the parameter values are used in the call.

                processor.fregs.get_ref(result_fregister) = result.real;

                break;
            }

            case INSTR_PUSH: {
                u16 freg = instr.op0;

                Value val = Value(processor.fregs.get(freg));
                processor.stack.add(val);

                break;
            }

            case INSTR_RET: {
                return processor.fregs.get(instr.op0);
            }

            default: {
                panic("Invalid instruction opcode");
            }
        }
    }

    fprintf(stderr, "Bytecode Runner: No return instruction at the end of the bytecode program!");
    return 0.0;
}

bool opcode_is_unary(Bytecode_Opcode opcode) {
    switch (opcode) {
	    case INSTR_NEGATE: case INSTR_NOT: case INSTR_NEGATE_F:
        case INSTR_TEST_RESULT:
        case INSTR_RET:
            return true;
        default:
            return false;
    }
}

bool opcode_is_binary(Bytecode_Opcode opcode) {
    return !opcode_is_unary(opcode);  // @fix
}


const char* opcode_string(Bytecode_Opcode opcode) {
    // @update
    switch (opcode) {
        case INSTR_LOAD: return "INSTR_LOAD";
        case INSTR_LOADF: return "INSTR_LOADF";
        case INSTR_LOAD_BUILTIN: return "INSTR_LOAD_BUILTIN";
	    case INSTR_LOAD_VAR: return "INSTR_LOAD_VAR";
    	case INSTR_LOAD_I_TO_F: return "INSTR_LOAD_I_TO_F";
        case INSTR_LOAD_F_TO_I: return "INSTR_LOAD_F_TO_I";
        case INSTR_MOV: return "INSTR_MOV";
        case INSTR_MOVF: return "INSTR_MOVF";
        case INSTR_MOV_I_TO_F: return "INSTR_MOV_I_TO_F";
        case INSTR_MOV_F_TO_I: return "INSTR_MOV_F_TO_I";

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

        case INSTR_TEST: return "INSTR_TEST";
        case INSTR_TEST_F: return "INSTR_TEST_F";

        case INSTR_TEST_RESULT: return "INSTR_TEST_RESULT";

        case INSTR_JMP: return "INSTR_JMP";
        case INSTR_JMP_COND: return "INSTR_JMP_COND";

        case INSTR_CALL_BUILTIN: return "INSTR_CALL_BUILTIN";
        case INSTR_CALL: return "INSTR_CALL";

        case INSTR_PUSH: return "INSTR_PUSH";

        case INSTR_RET: return "INSTR_RET";

        case INSTR_COUNT: return "INSTR_COUNT";
        case INSTR_SENTINEL: return "INSTR_SENTINEL";
        default: panic("Unknown instruction");
    }
}

static Bytecode_Opcode get_arithmetic_binop_opcode_integer(Op_Binary binary) {
    switch (binary) {
        case Binop_Add: return INSTR_ADD;
        case Binop_Sub: return INSTR_SUB;
        case Binop_Mul: return INSTR_MUL;
        case Binop_Div: return INSTR_DIV;
        case Binop_Mod: return INSTR_MOD;
        default: panic("Invalid arithmetic binary operation");
    }
}

static Bytecode_Opcode get_arithmetic_binop_opcode_float(Op_Binary binary) {
    switch (binary) {
        case Binop_Add: return INSTR_ADDF;
        case Binop_Sub: return INSTR_SUBF;
        case Binop_Mul: return INSTR_MULF;
        case Binop_Div: return INSTR_DIVF;
        case Binop_Mod: return INSTR_MODF;
        default: panic("Invalid arithmetic binary operation");
    }
}
