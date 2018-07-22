/**
 * @file
 * @author Andrew Predoehl
 * @brief Demo of searching a very small splay tree
 *
 * This is almost the same as driver1.c.  However,
 * if the argument is not present, it tries keys 1-31 and generates DOT files
 * for each.  Each key is tried in the tree splayed by the previous searches,
 * unlike driver1.
 *
 * DOT output can be used by graphviz(7) and specifically its dot(1) program,
 * to visualize the tree.
 */

/* $Id: driver3.c 165 2018-07-17 14:49:37Z predoehl $ */

#include <stdio.h>
#include <stdlib.h>

#include "splay.h"

static
int fail(const char* msg)
{
	fprintf(stderr, "Error: %s\n", msg);
	return EXIT_FAILURE;
}

static
void set_up(struct splay_Tree* t)
{
	int j;
	for (j = 30; j > 0; j -= 2)
		splay_insert(t, j, NULL);
}

int main(int argc, char** argv)
{
	struct splay_Tree t;

	if (splay_tree_empty_ctor(&t) != EXIT_SUCCESS)
		return fail("cannot construct");

	set_up(&t);

	if (splay_dot_output(&t, "grover.dot") != EXIT_SUCCESS)
		return fail("bad dot output 1");

	if (argc > 1) {
		const int k = atoi(argv[1]);
		struct splay_Result r = splay_find(&t, k);
		if (splay_dot_output(&t, "henry.dot") != EXIT_SUCCESS)
			return fail("bad dot output 2");
		if (r.found) {
			puts("found!");
			if (r.key != k)
				return fail("inappropriate key found 1");
		}
		else
			puts("NOT FOUND"); /* that is not a software error, though */
	}
	else {
		/* auto-generate results for a number of henries */
		int j;
		for (j = 1; j < 32; ++j) {
			struct splay_Result r = splay_find(&t, j);
			char fn[] = "henry999.dot";
			sprintf(fn, "henry%3d.dot", j+100);
			if (splay_dot_output(&t, fn) != EXIT_SUCCESS)
				return fail("bad dot output 3");
			if (r.found) {
				if (r.key != j)
					return fail("inappropriate key found 2");
				if (j & 1)
					return fail("nonexistent key found");
			}
			else if (~j & 1) {
				printf("searched for key %d.\n", j);
				return fail("failed to find searchkey (but it should have).");
			}
			/*
			splay_tree_clear(&t);
			set_up(&t);
			*/
		}
	}

	splay_tree_dtor(&t);
	return EXIT_SUCCESS;
}
