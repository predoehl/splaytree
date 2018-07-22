/* $Id: driver2.c 165 2018-07-17 14:49:37Z predoehl $ */

#include <stdio.h>
#include <stdlib.h>

#include "splay.h"

int fail(const char* msg)
{
	fprintf(stderr, "Error: %s\n", msg);
	return EXIT_FAILURE;
}


int main(int argc, char** argv)
{
	struct splay_Tree t;
	int j;
	int key_schedule[] = {1, 2, 4, 8, 12, 24, 40, 56, /* sentinel */ -1}, *p;
	char fn[] = "stringy2a.dot";
	
	if (splay_tree_empty_ctor(&t) != EXIT_SUCCESS)
		return fail("cannot construct");

	for (j = 0; j < 1000; ++j)
		if (splay_insert(&t, j+1, NULL) != EXIT_SUCCESS)
			return fail("problem while inserting");

	/* Next two lines are useless but they hush a pedantic compiler warning. */
	argv[argc]=argv[0];
	j = argc;

	if (splay_dot_output(&t, fn) != EXIT_SUCCESS)
		return fail("bad dot output 0");
	fn[8] += 1;

	for (p = key_schedule; *p > 0; fn[8] += 1) {
		struct splay_Result r = splay_find(&t, *p++);
		if (! r.found) {
			fprintf(stderr, "sought key %d,\n", --*p);
			return fail("key not found");
		}
		if (splay_dot_output(&t, fn) != EXIT_SUCCESS) {
			fprintf(stderr, "sought key %d,\n", --*p);
			return fail("bad dot output");
		}
	}

	splay_tree_dtor(&t);
	return EXIT_SUCCESS;
}
