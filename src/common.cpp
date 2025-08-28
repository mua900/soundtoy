#include "common.h"

#include <cstdio>
#include <cstdlib>

void panic(char const * const msg)
{
	fprintf(stderr, "%s\n", msg);
	exit(1);
}
