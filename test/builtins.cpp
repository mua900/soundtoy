#include <cstring>
#include <cstdio>
#include "api.h"

bool test_expression(St_Sampler* sampler, const char* expression);

int main() {
	St_Sampler* sampler = st_sampler_create(60);

	if (!test_expression(sampler, "sin(t)")) { return 1; }
	if (!test_expression(sampler, "abs(t)")) { return 1; }
	if (!test_expression(sampler, "cos(t)")) { return 1; }
	if (!test_expression(sampler, "abs(sin(2.0*pi*t) + cos(t*t))")) {
		const char* error = st_get_last_error();
		fprintf(stderr, "%s\n", error);
		return 1;
	}

	return 0;
}

bool test_expression(St_Sampler* sampler, const char* expression)
{
	bool success = st_sampler_set_expression(sampler, expression, strlen(expression));
	if (!success) {
		printf("Failed setting expression %s\n", expression);
	}

	return success;
}
