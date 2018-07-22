/**
 * @file
 * @author Andrew Predoehl
 * @brief Demo of searching a very small splay tree
 *
 * This program optionally takes one command line argument, a decimal integer.
 * The program uses splay.o to build a perfectly complete BST with even integer
 * keys 2 to 30, and produces DOT output called "grover.dot" in the current
 * directory.  If a command line argument is present, this scans it and
 * searches and splays the tree for that key, and then generates DOT output
 * called "henry.dot" showing the splayed tree (whether the key was present
 * or not).
 *
 * If the argument is not present, it tries keys 1-31 and generates DOT files
 * for each.  Each key is tried starting from the same perfectly-shaped tree:
 * the splaying of one search is not allowed to affect future searches.
 *
 * DOT output can be used by graphviz(7) and specifically its dot(1) program,
 * to visualize the tree.
 */

/* $Id: driver1.c 166 2018-07-17 23:58:23Z predoehl $ */

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
#if 0
	t -> root = naive_insert(t -> root, 16, NULL);

	t -> root = naive_insert(t -> root, 8, NULL);
	t -> root = naive_insert(t -> root, 24, NULL);

	t -> root = naive_insert(t -> root, 4, NULL);
	t -> root = naive_insert(t -> root, 12, NULL);
	t -> root = naive_insert(t -> root, 20, NULL);
	t -> root = naive_insert(t -> root, 28, NULL);

	t -> root = naive_insert(t -> root, 2, NULL);
	t -> root = naive_insert(t -> root, 6, NULL);
	t -> root = naive_insert(t -> root, 10, NULL);
	t -> root = naive_insert(t -> root, 14, NULL);
	t -> root = naive_insert(t -> root, 18, NULL);
	t -> root = naive_insert(t -> root, 22, NULL);
	t -> root = naive_insert(t -> root, 26, NULL);
	t -> root = naive_insert(t -> root, 30, NULL);
#elif 1
	/* SO CLOSE */

	/* leaves */
	splay_insert(t, 2, NULL);
	splay_insert(t, 6, NULL);
	splay_insert(t, 10, NULL);
	splay_insert(t, 14, NULL);
	splay_insert(t, 18, NULL);
	splay_insert(t, 22, NULL);
	splay_insert(t, 26, NULL);
	splay_insert(t, 30, NULL);

	/* parents */
	splay_insert(t, 4, NULL);
	splay_insert(t, 12, NULL);
	splay_insert(t, 20, NULL);
	splay_insert(t, 28, NULL);

	/* grand+parents */
	splay_insert(t, 8, NULL);
	splay_insert(t, 24, NULL);
	splay_insert(t, 16, NULL);
#endif
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
			splay_tree_clear(&t);
			set_up(&t);
		}
	}

	splay_tree_dtor(&t);
	return EXIT_SUCCESS;
}
