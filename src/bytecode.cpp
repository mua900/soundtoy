#include "bytecode.h"

Bytecode_Program compile_expr(Expr* root)
{
	DArray<Bytecode_Instr> instrs(8);
	DArray<double> constant_block(8);

	auto stack = DArray<Expr*>(8);
	stack.add(root);

	while (stack.m_size > 0)
	{
		Expr* expr = stack.pop();

		switch (expr->type)
		{
			case Expr_Type::Literal:
			{
				auto literal = static_cast<Expr_Literal*>(expr);
				
				break;
			}
			case Expr_Type::Binary:
			case Expr_Type::Unary:
			case Expr_Type::Grouping:
			case Expr_Type::Variable:
			{
				auto variable = static_cast<Expr_Variable*>(expr);

				break;
			}
			case Expr_Type::Call:
			{
				auto call = static_cast<Expr_Call*>(expr);
				break;
			}
			default:
			{
				panic("Unhandled expression case in compiler");
			}
		}
	}

	stack.free();

	return Bytecode_Program(instrs, constant_block);
}

void run(Processor proc, Bytecode_Program block)
{
	proc.program_counter = 0;
	while (proc.program_counter < block.size)
	{
		switch (block.code[proc.program_counter])
		{
			
		}
	}
}
