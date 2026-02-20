#include "../st/evaluation/evaluator.h"

void test_expression(String expression) {
	Parser parser = {};
	Expr* expr = parser.parse(expression);
	print_expression(expr);
}

int main() {
	test_expression(make_string("sin(2*PI*440*t)"));
	test_expression(make_string("2*4*1*4"));
	test_expression(make_string("sin(2*4*1*4)"));

	return 0;
}
