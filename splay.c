/**
	@file
	@brief Implementation for a splay tree, a kind of binary search tree.
	@author Andrew Predoehl

	Many functions below say they return EXIT_SUCCESS or EXIT_FAILURE.
	That is shorthand for the following conditions:
	- The return value, an int, equals either EXIT_SUCCESS or EXIT_FAILURE.
	- The return value never equals anything else.
	- Iff (if and only if) the return value is EXIT_SUCCESS,
		the function completed its task successfully.

	Many binary search trees disallow duplicate keys, but I permit them.

	The basic idea of a splay tree is of a lazily self-balancing binary search
	tree.  Basic dictionary operations (search, insert, erase, etc.) take
	amortized logarithmic time, i.e., proportional to log(n) when averaged
	across all operations, starting from empty.  The laziness means that the
	tree might at times be quite tall, and that is why
	the terms "amortized" and "average" appear above.
	Each basic op causes the tree to reshape itself by moving a sought-after
	node to the root, via a series of rotations.  Such a move is called a
	"splay."  Please see the paper by Sleator and Tarjan, or a textbook
	covering splay trees (such as Goodrich and Tamassia) for details. 
	Intuitively, if the tree is quite tall and the search path down is long,
	then the effect of splaying tends to make the tree shorter.
	Whereas any operations that make the tree taller tend to be quick.

	In implementation, a splay tree is structurally very simple, though
	behaviorally surprising.  The nodes lack any color information
	(such as a red-black tree has) or other balance information (such as
	an AVL tree has).  In this implementation, the nodes also lack
	a parent pointer:  each node has links just to its children, if any.
	Nodes may have zero, one, or two children, as usual for BSTs.

	All splay trees perform an operation called splaying, which means changing
	the tree topology to move a particular node to the root.  There are two
	different kinds splaying:  bottom-up and top-down.  A given implementation
	can do either kind, and that's enough -- there is never any functional
	need to do both kinds.  This implementation performs top-down splaying.
  	That means that as we descend the tree, we move nodes along the search
	path (and, indirectly, their subtrees that are disjoint from the search
	path) into two "remainder trees," which are metaphorically two buckets
	that hold the nodes we traverse that are not of interest.
	The two buckets hold the nodes with smaller and larger keys than
	the key we seek (at least in the case of unique keys, which are not
	required).  Smaller keys go in one bucket, larger in the other bucket.
	When search is complete, we splice the bucket contents back into the
	tree.  The details are in the paper by Sleator and Tarjan but not in
	every textbook presentation, because bottom-up splaying is more
	commonly presented.

	The implementation of the above is via the struct splay_Topdown object
	and its associated methods.

	This code does not offer an array-input constructor because it is not
	necessary -- if you have a sorted array of records, just insert them one
	by one.  The splay tree will only use linear time, unlike a naive BST.

	Another design choice:  this code could work in a highly constrained
	environment where printf and its stdio.h friends are unavailable.
	However, the code is nicer when stdio.h is available -- you can get DOT
	output and diagnostic info in case of a bug.  If you can live without those
	perks and want the .o file to be free of a stdio.h dependency, then compile
	the code with macro SPLAY_HAS_DOT_OUTPUT defined to be zero.

	I did not offer the same options for stdlib.h because malloc() and free()
	are pretty handy; however, they are only called once each in the program
	text, so if you really want to use this in an IOT matchbox or shovel or
	something, you could also peel off stdlib.h pretty easily if you do your
	own memory allocation and define EXIT_SUCCESS and EXIT_FAILURE symbols. */

/*	$Id: splay.c 172 2018-07-21 01:42:09Z predoehl $
	Tab size: 4
*/

#ifndef SPLAY_HAS_DOT_OUTPUT
/**	@brief Macro to control string-output functions and stdio dependency.

	Are you desperate to keep this code as slim and dependency-free as
	possible?  Do you never expect to want any diagnostic output, debug
	output, or DOT output (the language of Graphviz)?  Do you hate stdio?
	If you answer "yes" to all these questions, you can compile this
	code with macro symbol set to zero, via flag -DSPLAY_HAS_DOT_OUTPUT=0
	and this code will not require linkage to any printf-related function
	of stdio.h.

	Whereas if you are less parsimonious or strident, then you probably
	DO want the possibility of debug output, in which case, don't do anything
	special.  The function splay_debug_print_tree() will write to standard
	output as described in the documentation.  See also splay_dot_output() and
	splay_health_check().  None of these functions work to their full potential
	absent access to stdio.h. */
#define SPLAY_HAS_DOT_OUTPUT 1
#endif

#if SPLAY_HAS_DOT_OUTPUT
#ifndef _BSD_SOURCE
#define _BSD_SOURCE /**< macro recognized by GCC to allow use of snprintf */
#endif
#include <stdio.h>
#include <limits.h>
#endif

#include <stdlib.h> /* for malloc, free, EXIT_SUCCESS, stuff like that. */

#include "splay.h"

/** The SPLAY_DEBUG macro controls the internal invariant checking.  It can be
	set to 0, 1, or 2.  These are meant to be in a "Goldilocks" cofiguration
	of too little, just right, too much.

	Level 0: No assertions at all.  This is probably TOO risky with TOO little
				payoff.
	Level 1: Inexpensive assertions enabled.  This provides lots of elementary
				invariant checking and incurs only slight (probably
				insignificant) performance penalty.  Meant to be JUST RIGHT.
	Level 2: Expensive invariant checking enabled.  This significantly slows
				some functions.  Unless you are chasing a known bug, this is
				probably TOO cautious. */
#ifndef SPLAY_DEBUG
#define SPLAY_DEBUG 1
#endif

/** If SPLAY_DEBUG > 0 and SPLAY_VERBOSE > 0 then the code prints trace messages
	to standard output. */
#ifndef SPLAY_VERBOSE
#define SPLAY_VERBOSE 0
#endif


/*	The following macros will print debug info, conditioned on SPLAY_DEBUG true
	and SPLAY_VERBOSE true.  In order to reduce code clutter, the conditional
	compilation is wrapped up in these macros instead of sprinkling #ifs all
	over the code. */

#if SPLAY_DEBUG && SPLAY_VERBOSE
/** Print debug info, in string form, with no extra arguments. */
#define SPLAY_VERBOSE_PUTS(s)			puts(s)
/** Print debug info, in string form, with a format string and one argument. */
#define SPLAY_VERBOSE_PRINTF1(f,a)		printf((f),(a))
/** Print debug info, in string form: a format string and two arguments. */
#define SPLAY_VERBOSE_PRINTF2(f,a,b)	printf((f),(a),(b))
/** Print debug info, in string form: a format string and three arguments. */
#define SPLAY_VERBOSE_PRINTF3(f,a,b,c)	printf((f),(a),(b),(c))
#else
#define SPLAY_VERBOSE_PUTS(s)			do {} while(0)	/**< nop stand-in */
#define SPLAY_VERBOSE_PRINTF1(f,a)		do {} while(0)	/**< nop stand-in */
#define SPLAY_VERBOSE_PRINTF2(f,a,b)	do {} while(0)	/**< nop stand-in */
#define SPLAY_VERBOSE_PRINTF3(f,a,b,c)	do {} while(0)	/**< nop stand-in */
#endif


#if SPLAY_DEBUG
#include <assert.h>
/** This macro lets us turn assertions on or off locally to this code, without
	having to touch the NDEBUG macro, which is kind of a big deal. */
#define SPLAY_ASSERT(p) assert(p)
#else
/** No-operation macro for when we are totally sure we don't need to check any
	invariants at all. */
#define SPLAY_ASSERT(p) do {} while(0) /* NOP */
#endif


/** Blank result structure used for searches when rank is unavailable. */
#define SPLAY_BLANK_RESULT	{0, 0, NULL}	/* found? key, sat */


/** Release the memory for the current node.  In a macro for easy access. */
#define FREENODE(n) free(n)


/* Key comparison is put into the next two macros so we can find them easily.
   We completely avoid direct testing for equality (or inequality).

   One can infer equality as the condition ! LESSKEY(p,k) && ! KEYLESS(k,p).
   However, I always leave that as an else condition, like so:
   if (LESSKEY(p,k)) { ... } else if (KEYLESS(k,p)) { ... } else { equal } */

/** In order to cope with equal keys consistently, let's prefer to
	put the existing tree pointer as the first argument, and the query value
	as the second.  This means a true return value is associated with
	branch p -> right. */
#define LESSKEY(p,k) ((p) -> keiy < (k))


/**	If LESSKEY fails, then we usually need to test for the left subtree like
    so.  We only use this after using LESSKEY. */
#define KEYLESS(k,p) ((k) < (p) -> keiy)


/** @brief Basic BST node of the tree.  */
struct splay_Node {

	/** Records are stored and searched based on key values in a total order.
		The key values are not required to be unique (though users may
		impose that restriction upon themselves). */
	splay_Key keiy;

	/** Satellite data */
	splay_Satellite sat;

	/** Pointer to subtree of records with keys not exceeding 'key'. */
	struct splay_Node *left;

	/** Pointer to subtree of records with keys at least as large as 'key'. */
	struct splay_Node *right;
};






/**
 * Left rotation, where t points to the TOP of the rotated link.
 *
 * This returns an updated pointer to the root of the tree, according
 * to the x=change(x) idiom.  Note that t is no longer the root at exit. */
static struct splay_Node* left_rot(struct splay_Node* t)
{
	struct splay_Node* u;
	SPLAY_ASSERT(t && t -> right);
	u = t -> right;
	t -> right = u -> left;
	u -> left = t;

	return u;
}



/**
 * Right rotation, where t points to the top of the rotated link.
 *
 * This returns an updated pointer to the root of the tree, according
 * to the x=change(x) idiom.  Note that t is no longer the root at exit. */
static struct splay_Node* right_rot(struct splay_Node* t)
{
	struct splay_Node* s;
	SPLAY_ASSERT(t && t -> left);
	s = t -> left;
	t -> left = s -> right;
	s -> right = t;

	return s;
}


/* Allocate and initialize a new BST node.  Return NULL if allocation fails. */
static struct splay_Node* node_ctor(splay_Key k, splay_Satellite s)
{
	struct splay_Node *n
		= (struct splay_Node*) malloc(sizeof(struct splay_Node));

	if (n) {
		n -> keiy = k;
		n -> sat = s;
		n -> left = n -> right = NULL;
	}
	return n;
}




/** Symbolic constants to use with the splay_Topdown::history array. */
enum td_history_keys { RIGHT_FIRST, LEFT_FIRST, RIGHT_2ND, LEFT_2ND,
						TD_HIST_KEYS_END };


/**
 * @brief This object is used to hold state when performing top-down splaying.
 *
 * This object supports top-down splaying, as described in the paper by
 * Sleator and Tarjan (e.g., Fig. 12).   As search proceeds downwards,
 * we store the nodes we find in what I call "history storage."  If we fill
 * that, or if search ends, the contents of history storage are moved to
 * what I call "remainder trees."  The final step of top-down splaying is
 * grafting the remainder trees to the new root, and grafting the root's
 * former subtrees to the "tips" (see below) of the remainder trees.
 *
 * Search proceeds downwards in one or two steps (e.g., comparisons) at a time,
 * and the root pointer is advanced at each step.  Former root nodes are
 * stored in two-level history storage, along with a record of which way the
 * links go from parent to child, left or right.  If the history fills up
 * before the search key is found, or before the search path is exhausted,
 * the history contents are moved to the remainder trees via function
 * topdown_set_aside.  It only takes two steps (e.g., two comparisons) to fill
 * the history.
 *
 * There are two remainder trees.  Tree nodes leftwards of (e.g., with
 * smaller keys than) the target key are stored in a BST that called the
 * left remainder tree.  Tree nodes rightwards of (e.g., with larger keys
 * than) the target key are stored in a BST called the right remainder tree.
 * Sleator and Tarjan call them "L" and "R" respectively, in their paper.
 * They start as empty trees, and nodes are appended at the "tips," the site
 * of a relatively deep NULL pointer.  Nodes added to the tip must be in
 * nondecreasing order of keys, for the left remainder tree, or nonincreasing
 * order, for the right one.
 *
 * The remainder trees are pulled out of this object when they are complete,
 * by direct manipulation of the fields, rather than via encapsulated methods,
 * because doing so avoids some conditional tests that would sometimes be
 * unneccesary.
 *
 * The following functions and macros are like the methods for this object:
 *
 *	initialize_topdown	- construct an empty object
 *	is_td_history_blank	- test whether the history storage is empty
 *	update_right_tip	- attach a new node to the right remainder tree tip
 *	update_left_tip		- attach a new node to the left remainder tree tip
 *	topdown_set_aside	- move nodes from history storage to remainder trees
 *	STEP_RIGHT_FIRST	- store pointer (root) in history lvl 1 and step right
 *	STEP_LEFT_FIRST		- store pointer (root) in history lvl 1 and step left
 *	STEP_RIGHT_2ND		- store pointer (root) in history lvl 2 and step right
 *	STEP_LEFT_2ND		- store pointer (root) in history lvl 2 and step left
 *	undo_first_step		- return history lvl 1 (e.g. back to root) and erase.
 *	undo_2nd_step		- return history lvl 2 (e.g. back to root) and erase.
 *
 * Also, the symbols in enum td_history_keys are relevant to interpreting
 * the members of the history[] array.
 */
struct splay_Topdown {

	/*
	 * The remainder trees each grow from a working tip, denoted "tip."
	 * So, tip always points to a NULL pointer where future descendants
	 * (if any) are to be attached.  The "root" field is the root of the
	 * remainder tree, just as the name suggests.
	 *
	 * This anonymous structure represents one remainder tree.
	 *
	 * rem[0] is the left remainder tree, rem[1] is the right remainder tree.
	 *
	 * The tip of the left remainder tree either points to the root, or
	 * points to the right-child field of the maximal node.  The right
	 * remainder tree is symmetric.
	 */
	struct {
		struct splay_Node *root, **tip;
	} rem[2];

	/*
	 * During travel downwards (e.g., for search or insertion) the root steps
	 * down by one or two links, prior to calling topdown_set_aside.
	 * We need to remember the path it travels, so the four variables below
	 * label memory that stores zero, one or two proper ancestors of *root --
	 * i.e., its original values after one or two steps down.
	 *
	 * If root does not step down at all, all pointers remain set to NULL.
	 * (This happens for example when search for a key already at the root.)
	 *
	 * If root only steps down one step, then exactly one of
	 * {history[0], history[1]} equals root's former value, and the other
	 * equals NULL.  Specifically, either history[0] -> right == root
	 * or history[1] -> left == root.  But history[2] and history[3] are NULL.
	 * This happens when the search key is found after only one comparison.
	 *
	 * If root steps down two steps, then exactly one of
	 * {history[0], history[1]} equals root's original value before the
	 * first step, as described above.  Also, exactly one of
	 * {history[2], history[3]} equals root's value after the first step and
	 * before the second step.  The values in history[2] and history[3] are
	 * analogous to history[0] and [1], just one level deeper:  either
	 * history[2] -> right == root or history[3] -> left == root, but not both.
	 *
	 * The benefit of all this is that you can reach the relevant ancestors
	 * of *root, and determine the link directions by testing for NULL.
	 *
	 * SUMMARY:
	 * The history array remembers two degrees of ancestry.
	   Entries 0, 1 are for the high level (grandparent level).
	   Entries 2, 3 are for the low level (parent level).
	   Entries 0, 2 are for ancestors linking RIGHT to the reference point.
	   Entries 1, 3 are for ancestors linking LEFT to the reference point.

		EXAMPLE:
		C
		 \
		  E
		   \
		    (reference)

		history[0] points to C
		history[2] points to E
		history[1] == history[3] == NULL

		Using enum td_history_keys,
		history[RIGHT_FIRST] points to C
		history[RIGHT_2ND] points to E
		history[LEFT_FIRST] == history[LEFT_2ND] == NULL
	 */
	struct splay_Node* history[TD_HIST_KEYS_END];
};


#define STEP_RIGHT_FIRST(t, r)	(r) = ((t).history[RIGHT_FIRST] = (r)) -> right
#define STEP_LEFT_FIRST(t, r)	(r) = ((t).history[LEFT_FIRST] = (r)) -> left
#define STEP_RIGHT_2ND(t, r)	(r) = ((t).history[RIGHT_2ND] = (r)) -> right
#define STEP_LEFT_2ND(t, r)		(r) = ((t).history[LEFT_2ND] = (r)) -> left


/* This is only called by the general splay_Topdown initialization, and at the
 * end of topdown_set_aside.  I believe there is no need to call it elsewhere.
 */
static void initialize_td_history(struct splay_Topdown* td)
{
#if 0
	/* conventional idiom */
	int i;
	for (i = 0; i < TD_HIST_KEYS_END; ++i)
		td -> history[i] = NULL;
#else
	/* a mania for efficiency goads me to offer this one instead. */
	register int i = TD_HIST_KEYS_END;
	while (i)
		td -> history[--i] = NULL;
#endif
}


/* This is like a ctor for struct splay_Topdown -- call it on a fresh,
 * uninitialized object.
 */
static void initialize_topdown(struct splay_Topdown* td)
{
	/* Clear the remainder trees */
	int i;
	for (i = 0; i < 2; ++i) {
		td -> rem[i].root = NULL;
		td -> rem[i].tip = & td -> rem[i].root;
	}

	/* Clear the history */
	initialize_td_history(td);
}


static int is_td_history_blank(const struct splay_Topdown* td)
{
	/* check the *_FIRST history -- if that is blank, the 2nd row must be. */
	return		NULL == td -> history[RIGHT_FIRST]
			&&	NULL == td -> history[LEFT_FIRST];
}


/* This adds a node *n to the right remainder tree.
   It is ONLY called by topdown_set_aside() -- nobody else shoud call it. */
static void update_right_tip(struct splay_Topdown* td, struct splay_Node* n)
{
	SPLAY_ASSERT(n && td && td -> rem[1].tip);
	SPLAY_ASSERT(NULL == * td -> rem[1].tip);

	* td -> rem[1].tip = n;
	n -> left = NULL;
	td -> rem[1].tip = & n -> left;
}


/* This adds a node *n to the left remainder tree.
   It is ONLY called by topdown_set_aside() -- nobody else shoud call it. */
static void update_left_tip(struct splay_Topdown* td, struct splay_Node* n)
{
	SPLAY_ASSERT(n && td && td -> rem[0].tip);
	SPLAY_ASSERT(NULL == * td -> rem[0].tip);

	* td -> rem[0].tip = n;
	n -> right = NULL;
	td -> rem[0].tip = & n -> right;
}


/* Move topdown state from history into the remainder trees. 

	Precondition:  the history must contain something, either FIRST and 2ND
	level history; or just FIRST without 2ND.  But, it should not be blank.

	This is NOT idempotent. */
static void topdown_set_aside(struct splay_Topdown *td)
{
	SPLAY_ASSERT(td);

	/* pl == parent of new root on the left side, its rightward link to root.
	 * pr == parent of new root on the right side, leftward link to root.
	 */
	register struct splay_Node	*pl = td -> history[RIGHT_2ND],
								*pr = td -> history[LEFT_2ND];

	if (NULL == pl && NULL == pr) /* just a zig? */
		if (td -> history[RIGHT_FIRST])
			/* zig right, in \ */
			update_left_tip(td, td -> history[RIGHT_FIRST]);
		else
			/* zig left, in / */
			update_right_tip(td, td -> history[LEFT_FIRST]);

	else if (NULL == pr)
		/* parent's rightward link points to newroot: zigzig \\ or zigzag < */
		if (td -> history[RIGHT_FIRST])
			/* zigzig \\ */
			update_left_tip(td, left_rot(td -> history[RIGHT_FIRST]));

		else {
			/* zigzag < */
			update_right_tip(td, td -> history[LEFT_FIRST]);
			update_left_tip(td, td -> history[RIGHT_2ND]);
		}

	else
		/* parent's left link points to newroot: zigzig // or zigzag > */
		if (td -> history[LEFT_FIRST])
			/* zigzig // */
			update_right_tip(td, right_rot(td -> history[LEFT_FIRST]));

		else {
			/* zigzag > */
			update_left_tip(td, td -> history[RIGHT_FIRST]);
			update_right_tip(td, td -> history[LEFT_2ND]);
		}

	initialize_td_history(td);
}


/** This removes the non-NULL pointer in level-1 history, and returns it. */
static struct splay_Node* undo_first_step(struct splay_Topdown* td)
{
	struct splay_Node* root;

	SPLAY_ASSERT(	NULL == td -> history[RIGHT_FIRST]
				||	NULL == td -> history[LEFT_FIRST]);
	if (td -> history[RIGHT_FIRST]) {
		root = td -> history[RIGHT_FIRST];
		td -> history[RIGHT_FIRST] = NULL;
	}
	else {
		root = td -> history[LEFT_FIRST];
		td -> history[LEFT_FIRST] = NULL;
	}
	SPLAY_ASSERT(root);
	return root;
}


/** This removes the non-NULL pointer from level-2 history, and returns it. */
static struct splay_Node* undo_2nd_step(struct splay_Topdown* td)
{
	/* Roll back the second step!  Keep the first step though. */
	struct splay_Node* root;
	SPLAY_ASSERT(	NULL == td -> history[RIGHT_2ND]
				||	NULL == td -> history[LEFT_2ND]);
	if (td -> history[RIGHT_2ND]) {
		root = td -> history[RIGHT_2ND];
		td -> history[RIGHT_2ND] = NULL;
	}
	else {
		root = td -> history[LEFT_2ND];
		td -> history[LEFT_2ND] = NULL;
	}
	SPLAY_ASSERT(root);
	return root;
}


#if 0
/* This does NOT update the tree's size field!  It cannot! */
struct splay_Node* naive_insert(
	struct splay_Node* radix,
	splay_Key k,
	splay_Satellite sat
)
{
	if (NULL == radix)
		radix = node_ctor(k, sat);
	else if (LESSKEY(radix, k))
		radix -> right = naive_insert(radix -> right, k, sat);
	else
		radix -> left = naive_insert(radix -> left, k, sat);

	return radix;
}
#endif


/** @brief Initialize the fields of a raw tree object.

	Take an uninitialized splay_Tree structure and construct an empty tree
	from it.  We assume t points to uninitialized memory.

	@returns EXIT_SUCCESS unless t is NULL.

	@note This is a constructor function.
	Like all constructors, this function assumes that the contents
	of pointer 't' are uninitialized.  All other functions in this
	interface (i.e., all except for the constructors) implicitly assume
	and require that the struct splay_Tree pointer points
	to an initialized object.

	@warning Don't use this to erase an existing tree;
	call splay_tree_clear() instead. */
int splay_tree_empty_ctor(struct splay_Tree *t)
{
	if (NULL == t)
		return EXIT_FAILURE;

	t -> root = NULL;
	t -> size = 0;

	return EXIT_SUCCESS;
}




/* Print the subtree rooted at this node *n, to stdout. */
#if SPLAY_HAS_DOT_OUTPUT
static void db_print_tree(
	const struct splay_Node* n,
	const struct splay_Node* emmpty,
	int depth
)
{
	if (n) {
		int j = depth;
		while (j --> 0)
			putchar(' ');

		printf("Node at %p has key %d, "
				"left %p, right %p\n",
				(void*) n, n -> keiy,
				(void*) n -> left, (void*)n -> right);
		db_print_tree(n -> left, emmpty, 1+depth);
		db_print_tree(n -> right, emmpty, 1+depth);
	}
}
#endif



/**	@brief Print a generic debug-text description of the tree to stdout.

	This code can be disbled by defining macro @ref SPLAY_HAS_DOT_OUTPUT
	as 0. */
void splay_debug_print_tree(const struct splay_Tree* t)
{
#if SPLAY_HAS_DOT_OUTPUT
	if (t) {
		printf("Tree size: %d\n", t -> size);
		db_print_tree(t -> root, NULL, 0);
	}
#endif
}


static const char* shape = "[shape=box;color=black;fontcolor=black;"
							"style=filled;fillcolor=white]";



/* Postorder traverse and destroy the tree. */
static
void splay_dtor_helper(struct splay_Node* n)
{
	if (n) {
		splay_dtor_helper(n -> left);
		splay_dtor_helper(n -> right);
		FREENODE(n);
	}
}


/** @brief Reset a tree to empty state, size zero.

	@param t	Pointer to tree to be erased.

	@pre This is not a constructor: tree *t must have been initialized earlier.
	If *t has not yet been initialized, call splay_tree_empty_ctor() instead.

	@returns EXIT_SUCCESS or EXIT_FAILURE (if t equals NULL). */
int splay_tree_clear(struct splay_Tree* t)
{
	if (t)
		splay_dtor_helper(t -> root);
	return splay_tree_empty_ctor(t);
}


/** @brief Destructor: release all memory used by the tree.

	@param t	Pointer either equal to NULL, or pointing to the
				tree to be destroyed.

	@pre Like virtually every function in this interface, if t is
	not equal to NULL, then tree *t must have been initialized
	prior to calling this function.
	If *t has not yet been initialized, call splay_tree_empty_ctor() instead.

	As is conventional, this destructor is idempotent.  It is safe for
	you to call it twice, and it is safe to call on a NULL argument. */
void splay_tree_dtor(struct splay_Tree* t)
{
	/*	We deliberately disregard the return value.  Destructors
		never return anything anyways.  In this case the return value
		from splay_tree_clear() isn't even helpful; if t is equal to NULL,
		which is a perfectly appropriate input to a destructor,
		the return code from splay_tree_clear() is EXIT_FAILURE, i.e.,
		undeservedly harsh and gloomy. */
	splay_tree_clear(t);
}




#if SPLAY_HAS_DOT_OUTPUT
/* Print an invisible DOT node to DOT output, analogous to a LaTeX \phantom.
   We want invisible nodes as siblings of only children to make the arrows from
   the parent more likely to point in a direction suggesting a binary search
   tree as traditionally presented.

   See also:  Austrian film Ich seh, Ich seh (2014) (a.k.a. Goodnight Mommy).
 */
static int print_phantom(FILE* f, int parent_key)
{
	int phantom_key = rand(); /* collisions? inconceivable! */
	SPLAY_ASSERT(f);
	if (0 > fprintf(f,
					"  %d [style=invis];\n"
					"  %d -> %d [style=invis];\n",
					phantom_key,
					parent_key, phantom_key))
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}


/* Render the entire tree in DOT format, to a given stream, in linear time.

   This is recursive.  It prints node *t each time it is called.
   If *t also has a parent (*tpar) then print that edge too. */
static
int dot_out_help(
	const struct splay_Node* t,
	const struct splay_Node* tpar,
	FILE* f
)
{
	SPLAY_ASSERT(f);

	if (NULL == tpar) {
		/* Print *t, which must be the root, but print no in-edge. */
		if (0 > fprintf(f, "  %d %s;\n", t -> keiy, shape))
			return EXIT_FAILURE;
	}
	else if (t) {
		/* Print a non-root node plus its in-edge. */

		if (NULL == tpar -> left) /* print left phantom sibling? */
			if (print_phantom(f, tpar -> keiy) != EXIT_SUCCESS)
				return EXIT_FAILURE;

		/* Print *t, plus its in-edge. */
		if (0 > fprintf(f, "  %d %s;\n"
					"  %d -> %d\n",
					t -> keiy, shape,
					tpar -> keiy, t -> keiy))
			return EXIT_FAILURE;

		if (NULL == tpar -> right) /* print right phantom sibling? */
			if (print_phantom(f, tpar -> keiy) != EXIT_SUCCESS)
				return EXIT_FAILURE;
	}

	if (t) {
		/* recurse */
		if (dot_out_help(t -> left, t, f) != EXIT_SUCCESS)
			return EXIT_FAILURE;
		if (dot_out_help(t -> right, t, f) != EXIT_SUCCESS)
			return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
#endif


/** @brief Write a representation of the tree in DOT format.

	@returns EXIT_SUCCESS or EXIT_FAILURE.

	DOT is the name of a graph-description language for graph visualization
	software called GraphViz, developed by researchers at Bell Labs.

	This function attempt to write a DOT file representing the tree,
	to the given filename.  This returns EXIT_FAILURE if the file I/O fails.

	This code can be disbled by defining macro @ref SPLAY_HAS_DOT_OUTPUT as 0.

	The implementation here is something of a demonstration of the
	nightmare surrounding consistent checking of return codes.  This is why
	error handling is coded using exceptions in modern languages. */
int splay_dot_output(const struct splay_Tree* t, const char* filename)
{
	int rc = EXIT_FAILURE;
#if SPLAY_HAS_DOT_OUTPUT
	if (t) {
		FILE* f = fopen(filename, "w");
		if (f) {
			if (0 <= fputs("digraph {\n  bgcolor=lightblue;\n", f)) {
				if (EXIT_SUCCESS ==
						(rc = dot_out_help(t -> root, NULL, f)))
					if (EOF == fputs("}\n", f))
						rc = EXIT_FAILURE;
			}
			if (EOF == fclose(f))
				rc = EXIT_FAILURE;
		}
	}
#endif
	return rc;
}


#if 0
/* This code is used to visualize the contents of the splay_Topdown object. */
static
const struct splay_Topdown* db_print_splay_td(const struct splay_Topdown* td)
{
#if SPLAY_DEBUG && SPLAY_VERBOSE
	int i;

	puts("Start db_print_splay_td");

	puts("Left remainder tree:");
	db_print_tree(td -> rem[0].root, NULL, 0);
	printf("Left tip is: %p\n", td -> rem[0].tip);

	puts("Right remainder tree:");
	db_print_tree(td -> rem[1].root, NULL, 0);
	printf("Right tip is: %p\n", td -> rem[1].tip);

	puts("History:");
	for (i = 0; i < TD_HIST_KEYS_END; ++i) {
		printf("history[%d] = %p", i, td -> history[i]);
		if (td -> history[i])
			printf(" (key = %d)", td -> history[i] -> keiy);
		puts("");
	}

	puts("End db_print_splay_td");
#endif
	return td;
}
#endif


/*	Search tree for key k, return logical true iff found, plus splaying.

	@param		rrr		Pointer to the root of the entire tree
	@param		k		Key that is sought
	@param[out]	found	Boolean output: 1 iff the key was found, else 0.
						As a pointer input, 'found' must not equal NULL.

	@returns updated root to the tree, using the x=change(x) idiom.

	@pre Pointer 'found' does not equal NULL.

	@post If the key k is present, the tree is reshaped so that the root node
	contains k, and a pointer to the root is returned.  If the key k is not
	present, the tree is reshaped so that the last BST node queried is splayed
	to the root (except when the tree is empty).

	This is the most complicated example of splaying because search might
	succeed and might fail, but either way, we want to splay something to
	the root.  So if we search till failure, we have to back up. */
static
struct splay_Node* search_and_splay(
	struct splay_Node* root,
	splay_Key k,
	int *found
)
{
	/* Storage for state during top-down splaying. */
	struct splay_Topdown td;

#if SPLAY_DEBUG && SPLAY_VERBOSE
	SPLAY_VERBOSE_PUTS(__func__);
	SPLAY_VERBOSE_PUTS("Here is the input tree:");
	db_print_tree(root, NULL, 0);
	SPLAY_VERBOSE_PUTS("----End of tree----");
#endif

	/* Special case makes the loop below simpler -- no condition to test */
	SPLAY_ASSERT(found);
	if (NULL == root) {
		*found = 0;
		return NULL;
	}

	/* This is an infinite loop with four break statements to exit.  Each time
	   it runs, it is willing to step down the tree from *root to its child
	   or grandchild (but no deeper) and store history in object td.  The four
	   exit conditions, in order --
		1.	*root contains key k.
		2.  *root does not match k and has an empty subtree for continuing
	   		the search.
		3.  *root does not match k but has a child containing k.
		4.  *root does not match k, but it has a nonempty subtree s for
			continuing the search.  However, s.root (a child of *root)
			does not contain k and has an empty subtree for continuing the
			search.

		{1,3} are for search success.
		{2,4} are for search failure.
		{1,2} the loop body does a single comparison before breaking.
				The root does not advance downwards at all.
		{3,4} the loop body does two comparisons before breaking, and
				the root advances downwards just one step.  Thus the contents
				of history in td causes a final zig outside the loop. */
	for (initialize_topdown(&td); 1; topdown_set_aside(&td)) {

		SPLAY_ASSERT(root);

		/* First step down? */
		if (LESSKEY(root, k))
			STEP_RIGHT_FIRST(td, root);
		else if (KEYLESS(k, root))
			STEP_LEFT_FIRST(td, root);
		else {
			/* Found at root: no steps down. */
			*found = 1;
			break;
		}

		if (NULL == root) {
			root = undo_first_step(&td);
			*found = 0; /* Not found:  no steps down. */
			break;
		}

		/* Second step down? */
		if (LESSKEY(root, k))
			STEP_RIGHT_2ND(td, root);
		else if (KEYLESS(k, root))
			STEP_LEFT_2ND(td, root);
		else {
			/* FOUND AT ROOT'S CHILD.  Final zig done outside this loop. */
			*found = 1;
			break;
		}

		if (NULL == root) {
			root = undo_2nd_step(&td);
			*found = 0; /* Not found.  Final zig done outside this loop. */
			break;
		}
	}

	SPLAY_ASSERT(root);
	SPLAY_ASSERT(1 == *found || LESSKEY(root, k) || KEYLESS(k, root));
	SPLAY_ASSERT(0 == *found || k == root -> keiy);

	/* Possible final zig to perform (for great justice). */
	if (! is_td_history_blank(&td))
		topdown_set_aside(&td);

	/*
	 * Now the remainder trees contain all nodes encountered on the path to
	 * *root, which is now at the root of the tree, just as its name implies.
	 * The subtrees of *root should be attached to the working tips
	 * of the corresponding remainder trees, and the remainder trees
	 * become the new subtrees of *root.  (That is simply the standard recipe
	 * for top-down splaying.)
	 */
	* td.rem[0].tip = root -> left;
	root -> left = td.rem[0].root;

	* td.rem[1].tip = root -> right;
	root -> right = td.rem[1].root;

	SPLAY_VERBOSE_PUTS("Exiting search-and-splay");
	return root;
}


struct splay_Result splay_find(struct splay_Tree *t, splay_Key k)
{
	struct splay_Result r = SPLAY_BLANK_RESULT;

	if (t) {
	   	t -> root = search_and_splay(t -> root, k, & r.found);
		if (r.found) {
			r.key = k;
			r.sat = t -> root -> sat;
		}
	}
	return r;
}


/** @brief Search for the maximum element in the tree (which we splay).
 *
 * Implementation: the splaying code is simpler because all nodes we encounter
 * are either the new root, or they go in the right remainder tree.  There are
 * no comparisons and nothing in the left remainder tree.  So the code is
 * simpler.
 */
struct splay_Result splay_min(struct splay_Tree *t)
{
	struct splay_Result r = SPLAY_BLANK_RESULT;

	if (NULL == t || NULL == t -> root)
		r.found = 0;
	else {
		register struct splay_Node* root = t -> root;
		struct splay_Topdown td;
		SPLAY_ASSERT(root);

		/* Walk down the left links from t -> root to the last node;
		 * store all nodes in the right remainder tree.  The loop body
		 * does this two links at a time (unless there is just one).
		 */
		for (initialize_topdown(&td); root -> left; topdown_set_aside(&td)) {
			STEP_LEFT_FIRST(td, root);
			if (root -> left)
				STEP_LEFT_2ND(td, root);
		}

		/* Now *root is the deepest node in that chain of left links.
		 * By the BST property, that means *root is the minimum node.  (For if
		 * there were a node with a smaller key, it would be in the left
		 * subtree of *root.  Which is absurd: it is empty.)
		 */
		SPLAY_ASSERT(root && NULL == root -> left);

		/* Almost all the other nodes are now in the right remainder tree,
		 * except for *root's right subtree, which we now graft to the tip.
		 */
		* td.rem[1].tip = root -> right;

		/* Now graft the right remainder tree as root's right subtree. */
		root -> right = td.rem[1].root;

		/* Don't bother with the left remainder tree because it is empty. */
		SPLAY_ASSERT(NULL == td.rem[0].root);

		/* Copy the root local variable back into the tree.  There wasn't
		 * any real need to copy it to a local variable except that it makes
		 * the code a bit more concise.
		 */
		t -> root = root;

		/* Fill in the result object. */
		r.found = 1;
		r.key = root -> keiy;
		r.sat = root -> sat;
	}

	return r;
}


int splay_erase(struct splay_Tree *t, splay_Key k, splay_Satellite *psat)
{
	struct splay_Node* radix;
	struct splay_Result r = splay_find(t, k);

	if (! r.found)
		return EXIT_FAILURE;

	SPLAY_ASSERT(r.key == k);
	SPLAY_ASSERT(t && t -> root && t -> root -> keiy == k);
	SPLAY_ASSERT(r.sat == t -> root -> sat);

	/* Save the satellite data if the user asked us to (via write param) */
	if (psat)
		*psat = r.sat;

	/* Temporarily store the root (which is the target to delete). */
	radix = t -> root;

	/* Splay the root's successor s (if it exists) -- we do so by temporarily
	 * reducing the tree to just the right subtree of *radix, and splay its
	 * min elt, which must be s.
	 *
	 * Then the left subtree of *radix becomes the left subtree of s.
	 */
	if ((t -> root = t -> root -> right) != NULL) {
		struct splay_Result succ = splay_min(t);
		SPLAY_ASSERT(succ.found && t -> root && NULL == t -> root -> left);
		t -> root -> left = radix -> left;
	}
	else /* Then *radix has no successor -- so just splice it out. */
		t -> root = radix -> left;

	/* Unnecessary!
	radix -> left = radix -> right = NULL;
	 */

	FREENODE(radix);

	t -> size -= 1;
	return EXIT_SUCCESS;
}


/** @brief Search for the maximum element in the tree (which we splay).
 *
 * If you want to extract the maximum element, just perform an erase on the
 * key you find.
 *
 * @code
 * struct splay_Result r = splay_max(&t);
 * if (r.found)
 *     splay_erase(&t, r.key);
 * @endcode
 */
struct splay_Result splay_max(struct splay_Tree *t)
{
	struct splay_Result r = SPLAY_BLANK_RESULT;

	if (NULL == t || NULL == t -> root)
		r.found = 0;
	else {
		struct splay_Node* root = t -> root;
		struct splay_Topdown td;
		SPLAY_ASSERT(root);

		/* See the comments for splay_min for a complete exegesis.
		 * We walk down the chain of rightward links and store everything
		 * in the left remainder tree.
		 */
		for (initialize_topdown(&td); root -> right; topdown_set_aside(&td)) {
			STEP_RIGHT_FIRST(td, root);
			if (root -> right)
				STEP_RIGHT_2ND(td, root);
		}

		SPLAY_ASSERT(root && NULL == root -> right);

		* td.rem[0].tip = root -> left;
		root -> left = td.rem[0].root;
		t -> root = root;

		/* Don't bother with the right remainder tree because it is empty. */
		SPLAY_ASSERT(NULL == td.rem[1].root);

		r.found = 1;
		r.key = root -> keiy;
		r.sat = root -> sat;
	}

	return r;
}


/* Test whether a node is a leaf in a BST sense. */
static int is_bst_leaf(const struct splay_Node* n)
{
	return n && NULL == n -> left && NULL == n -> right;
}


/* This is also a simple kind of splaying -- we just rip the tree in half
 * according to the new node's key, until root becomes null.  Then glue
 * the remainder trees onto n. */
static
struct splay_Node* insert_and_splay(
	struct splay_Node* root,
	struct splay_Node* n
)
{
	/* Storage for state during top-down splaying. */
	struct splay_Topdown td;

	/* Special case for simplicity. */
	if (NULL == root)
		return n;

	/* Partition ALL old nodes into the left and right remainder trees. */
	for (initialize_topdown(&td); root; topdown_set_aside(&td)) {

		SPLAY_ASSERT(root);

		if (LESSKEY(root, n -> keiy))
			STEP_RIGHT_FIRST(td, root);
		else
			STEP_LEFT_FIRST(td, root);

		if (root) {
			if (LESSKEY(root, n -> keiy))
				STEP_RIGHT_2ND(td, root);
			else
				STEP_LEFT_2ND(td, root);
		}
	}

	/* Graft the remainder trees onto the new node n -- hail the new root! */
	SPLAY_ASSERT(is_bst_leaf(n));
	n -> left = td.rem[0].root;
	n -> right = td.rem[1].root;
	return n;
}


int splay_insert(struct splay_Tree* t, splay_Key k, splay_Satellite sat)
{
	struct splay_Node* n = node_ctor(k, sat);
	if (NULL == n)
		return EXIT_FAILURE;

	t -> root = insert_and_splay(t -> root, n);
	SPLAY_ASSERT(n == t -> root);
	t -> size += 1;
	return EXIT_SUCCESS;
}


/** @brief Update the satellite field of an existing node with key k.
	@returns EXIT_SUCCESS or EXIT_FAILURE (if k is not found).

	@warning This function is almost never useful in a multimap application.
	If the tree holds duplicate keys, just one matching record is affected.
	There is no way to control which of the records with key k is altered.

	@see @ref MAPSEM

	Time complexity: O(log n) for a tree of size n. */
int splay_update(struct splay_Tree* t, splay_Key k, splay_Satellite sat)
{
	int rc = EXIT_FAILURE, found = 0;
	t -> root = search_and_splay(t -> root, k, & found);
	if (found) {
		SPLAY_ASSERT(k == t -> root -> keiy);
		rc = EXIT_SUCCESS;
		t -> root -> sat = sat;
	}
	return rc;
}


/* Support for splay_health_check:  is the BST property satisfied in a subtree
   rooted at *t?  Return a boolean value.  If not, print an error message. */
static
int breaks_bst_property(
	const struct splay_Node* t,
	splay_Key minkey,
	splay_Key maxkey,
	char *buf,
	unsigned bufsize
)
{
	if (NULL == t)
		return 0;
	if (LESSKEY(t, minkey) || KEYLESS(maxkey, t)) {
#if SPLAY_HAS_DOT_OUTPUT
		if (buf)
			snprintf(buf, bufsize, "Node with key %d violates the "
							"BST property; should be in range [%d, %d].",
							t -> keiy, minkey, maxkey);
#endif
		return 1;
	}
	return
		breaks_bst_property(t-> left, minkey, t -> keiy, buf, bufsize)
		|| breaks_bst_property(t-> right, t -> keiy, maxkey, buf, bufsize);
}


/* Count tree size, return the answer -- linear time!
   Not for normal use -- used just for diagnostic testing and reflection.

   This will loop infinitely if the nodes have directed cycles, and will give
   the wrong answer if there any are unreachable nodes, parallel arcs, or
   the pointers don't have a valid tree topology. */
static
unsigned splay_nodecount(const struct splay_Node* t)
{
	return t ? 1 +splay_nodecount(t -> left) +splay_nodecount(t -> right) : 0;
}


/* Check tree size -- is it ok?  Return a boolean.  If not ok, generate an
   error message.  Check both the size field and the NULL or not-NULL status
   of root. */
static
int splay_size_failure(const struct splay_Tree* t, char* buf, unsigned bufsize)
{
	SPLAY_ASSERT(t);
	if (t -> root && 0 == t -> size) {
#if SPLAY_HAS_DOT_OUTPUT
		if (buf)
			snprintf(buf, bufsize,
				"Size counter is zero but tree has non-nil root.");
#endif
		return 1;
	}

	if (NULL == t -> root && t -> size != 0) {
#if SPLAY_HAS_DOT_OUTPUT
		if (buf)
			snprintf(buf, bufsize,
				"Size counter is %u but tree has nil root.", t -> size);
#endif
		return 1;
	}

	if (t -> size != splay_nodecount(t -> root)) {
#if SPLAY_HAS_DOT_OUTPUT
		if (buf)
			snprintf(buf, bufsize,
				"Size counter is %u but tree has %u reachable nodes.",
				t -> size, splay_nodecount(t -> root));
#endif
		return 1;
	}

	return 0;
}


/**	@brief Test the tree for internal problems, and report findings.

	@returns EXIT_FAILURE if the tree is discovered to be unhealthy,
	i.e., we observe that it breaks its invariants.  Otherwise,
	silently (see note) return EXIT_SUCCESS.

	As E. W. Dijkstra famously observed, a test like this can detect certain
	errors, but no test can detect flawlessness.

	If the code is compiled normally (without setting the macro
	@ref SPLAY_HAS_DOT_OUTPUT to zero), then this function can also generate
	a human-readable diagnostic message string to array 'buf[]',
	describing the error.  Array 'buf[]' is a character array of size
	'bufsz'; the interface is similar to snprintf().
	If you do not want the diagnostic message, you can pass in a NULL
	argument for the 'buf' parameter.

	This takes linear time.  Splay trees are so simple that there is not
	much to check, but we still have to count all the nodes.

	@note
	If 'buf' is not equal to NULL and 'bufsz' is positive, and if the
	tree passes all its tests, and if the tree is non-empty,
	then the output message is an empty string.
	If 'buf' is not equal to NULL and 'bufsz' is positive, and if the
	macro SPLAY_HAS_DOT_OUTPUT was zero at compile time, and the tree is
	non-empty, then the output message is also an empty string
	(regardless of the return value). */
int splay_health_check(const struct splay_Tree *t, char buf[], unsigned bufsz)
{
	/* Empty tree is easy to validate! */
	if (NULL == t || (0 == t -> size && NULL == t -> root))
		return EXIT_SUCCESS;

	/* Initialize string output to empty string */
	if (buf && bufsz > 0)
		buf[0] = '\0';

	/* Check size */
	if (splay_size_failure(t, buf, bufsz))
		return EXIT_FAILURE;

	/* Check keys */
	if (breaks_bst_property(t -> root, INT_MIN, INT_MAX, buf, bufsz))
		return EXIT_FAILURE;

	return EXIT_SUCCESS; /* We lack evidence of any problem.  :-|  */
}


/*	This does a postorder deep copy. */
static int copy_helper(const struct splay_Node* ni, struct splay_Node** no)
{
	SPLAY_ASSERT(no);

	if (NULL == ni)
		*no = NULL;
	else {
		struct splay_Node *l, *r;
		if (copy_helper(ni -> left, &l) == EXIT_FAILURE)
			return EXIT_FAILURE;
		if (copy_helper(ni -> right, &r) == EXIT_FAILURE) {
			/* release memory in l */
			splay_dtor_helper(l);
			return EXIT_FAILURE;
		}
		if (NULL == (*no = node_ctor(ni -> keiy, ni -> sat))) {
			/* release memory already allocated */
			splay_dtor_helper(l);
			splay_dtor_helper(r);
			return EXIT_FAILURE;
		}
		SPLAY_ASSERT(*no);
		(*no) -> left = l;
		(*no) -> right = r;
	}
	return EXIT_SUCCESS;
}


/** @brief Copy tree *ti to empty tree *to.

	@pre Tree *to must be empty.  (Call splay_tree_clear() if not.)

	Tree *ti is unaffected by the operation.

	@warning
	This is not a constructor:  output tree *to must be initialized and in an
	empty state.  Call splay_tree_empty_ctor() if it is uninitialized.

	@see splay_tree_move()

	@returns EXIT_SUCCESS or EXIT_FAILURE (e.g., failed memory allocation). */
int splay_tree_copy(const struct splay_Tree* ti, struct splay_Tree* to)
{
	if (!ti || !to || to -> root != NULL)
		return EXIT_FAILURE;

	SPLAY_ASSERT(0 == to -> size);
	to -> size = ti -> size;

	return copy_helper(ti -> root, & to -> root);
}


/** @brief Move contents of tree *ti to empty tree *to.

	Tree *ti loses its contents, which are transferred to tree *to.
	The semantics are similar to a C++ move assignment.
	This requires only constant time.

	@pre Tree *to must be empty.  (Call splay_tree_clear() if not.)

	@warning This is not a constructor:  output tree *to must be
	initialized and in an empty state, perhaps by splay_tree_empty_ctor().

	@post Tree *ti is in an empty state when this finishes successfully.

	@returns EXIT_SUCCESS or EXIT_FAILURE. */
int splay_tree_move(struct splay_Tree* ti, struct splay_Tree* to)
{
	if (!ti || !to || to -> root != NULL)
		return EXIT_FAILURE;

	SPLAY_ASSERT(0 == to -> size);
	to -> size = ti -> size;
	ti -> size = 0;

	to -> root = ti -> root;
	ti -> root = NULL;

	return EXIT_SUCCESS;
}

