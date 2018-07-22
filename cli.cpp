/*
 * Command line interface for splay tree
 */
/* $Id: cli.cpp 164 2018-07-17 14:17:59Z predoehl $ */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE /* for asprintf */
#endif

#include <string>
#include <sstream>
#include <vector>
#include <iostream>
#include <cstdlib>
#include <cassert>
#include <cstdio>

extern "C" {
#include "splay.h"
}

namespace {

using std::cin;

int fail(const std::string& msg)
{
	std::cerr << "Error: " << msg << '\n';
	return EXIT_FAILURE;
}

const char* helptext =
	"Key:  N represents a decimal integer\n"
	"      S represents a nonempty string not containing whitespace\n\n"
	"in N S \tInsert record (N,S) into tree (as multiset).\n"
	"up N S \tUpdate record with key N, now associating it with S.\n"
	"er N   \tErase one record with key N from tree (if any).\n"
	"fi N   \tFind key N once, print its associated string.\n"
	"fa N   \tFind key N in tree, print all associated strings.\n"
	"min    \tFind and print the minimum key in the tree.\n"
	"max    \tFind and print the minimum key in the tree.\n"
	"pre N  \tFind the precessor key in the tree to N.\n"
	"suc N  \tFind the successor key in the tree to N.\n"
	"prn    \tPrint tree contents, in freeform human-readable format.\n"
	"dot    \tWrite tree contents to file in DOT format -- see graphviz(1).\n"
	"x      \tExit\n"
	"help   \tShow this list of commands\n"
	;


// Convert a std::string into a C-style string allocated as with malloc().
inline char* zz(const std::string& s)
{
	char* strang = NULL;
	assert(
		asprintf(&strang, "%s", s.c_str())
		>=0); // make sure memory allocation worked
	return strang;
}


void print_result(const struct splay_Result& res)
{
	if (res.found)
		std::cout << "present\nkey = " << res.key
					<< ", sat = " << (char*)res.sat << '\n';
	else
		std::cout << "absent\n";
}


int execute_cmd(splay_Tree* tree, const std::string& cmd)
{
	static int filenumber = 1000;

	if ("in" == cmd) {
		int n;
		std::string s;
		char* z;
		if (cin >> n >> s) {
			if (EXIT_FAILURE == splay_insert(tree, n, z=zz(s))) {
				free(z);
				return fail("Insertion failed");
			}
			return EXIT_SUCCESS;
		}
		return fail("cannot scan integer and string arguments "
				"for command " + cmd);
	}
	else if ("up" == cmd) {
		int n;
		std::string s;
		char* z;
		if (cin >> n >> s) {
			if (EXIT_FAILURE == splay_update(tree, n, z=zz(s))) {
				free(z);
				std::cout << "Warning: update failed\n";
			}
			return EXIT_SUCCESS;
		}
		return fail("cannot scan integer and string arguments "
				"for command " + cmd);
	}
	else if ("er" == cmd) {
		int n;
		void* s;
		if (cin >> n)
			if (EXIT_SUCCESS == splay_erase(tree, n, &s))
				free(s);
			else
				std::cout << "Warning: erase failed\n";
		else
			return fail("cannot scan integer argument for command " + cmd);
	}
	else if ("fi" == cmd) {
		int n;
		if (cin >> n)
			print_result(splay_find(tree, n));
		else
			return fail("cannot scan integer argument for command " + cmd);
	}
	/*
	else if ("fa" == cmd) {
		int n;
		if (cin >> n) {
			unsigned ct = rb_count_range(tree, n, n);
			std::cout << "Retrieving " << ct << " records\n";
			std::vector<void*> ss(ct, NULL);
			if (EXIT_SUCCESS == rb_read_range(tree, n, n, NULL, &ss.front()))
				for ( ; ! ss.empty(); ss.pop_back())
					std::cout << (char*) ss.back() << '\n';
			else
				return fail("unable to read range");
		}
		else
			return fail("cannot scan integer argument for command " + cmd);
	}
	*/
	else if ("min" == cmd)
		print_result(splay_min(tree));
	else if ("max" == cmd)
		print_result(splay_max(tree));
	/*
	else if ("pre" == cmd) {
		int n;
		if (cin >> n)
			print_result(rb_find_pred(tree, n));
		else
			return fail("cannot scan integer argument for command " + cmd);
	}
	else if ("suc" == cmd) {
		int n;
		if (cin >> n)
			print_result(rb_find_succ(tree, n));
		else
			return fail("cannot scan integer argument for command " + cmd);
	}
	*/
	else if ("dot" == cmd) {
		std::ostringstream fn;
		fn << "tree" << ++filenumber << ".dot";
		std::cout << "Writing to file " << fn.str() << '\n';
		return splay_dot_output(tree, fn.str().c_str());
	}
	else if ("prn" == cmd)
		splay_debug_print_tree(tree);
	else if ("help" == cmd)
		std::cout << helptext;
	else if ("x" == cmd)
		/* NOP */;
	else
		std::cout << "Warning: unrecognized command "
			"(enter 'help' for a list)\n";
	return EXIT_SUCCESS;
}


int cleanup(int rc, struct splay_Tree* tree)
{
	for (struct splay_Result r; (r = splay_max(tree)).found; free(r.sat))
		if (splay_erase(tree, r.key, NULL) != EXIT_SUCCESS)
			return fail("Error cleaning up tree");

	splay_tree_dtor(tree);
	return EXIT_SUCCESS;
}

}


int main(int argc, const char* const* argv)
{
	int rc = EXIT_SUCCESS;
	struct splay_Tree tree;
	std::vector<char> err_msg(4096);

	if (EXIT_FAILURE == splay_tree_empty_ctor(&tree))
		return fail("cannot construct tree");

	std::cout << "Enter 'help' for a list of commands.\n";
	
	for (std::string cmd; std::cin >> cmd && cmd != "x"; ) {
		if (EXIT_FAILURE == execute_cmd(&tree, cmd)) {
			rc = fail("Command failed");
			break;
		}
		if (EXIT_FAILURE == splay_health_check(&tree,
								& err_msg.front(), err_msg.size())) {
			rc = fail("Health check failed");
			std::cerr << & err_msg.front() << '\n';
			break;
		}
	}

	return cleanup(rc, &tree);
}

