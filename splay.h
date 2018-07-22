/**
	@file
	@brief Interface for splay tree.
	@author Andrew Predoehl
*/
/*	$Id: splay.h 171 2018-07-19 22:14:04Z predoehl $
	Tab size: 4 */

#ifndef PREDOEHL_SPLAY_H_2018_INCLUDED_
#define PREDOEHL_SPLAY_H_2018_INCLUDED_ 1


/** Key type used for the tree.  This type must be in a total order. */
typedef int splay_Key;

/** Satellite data type for each record. */
typedef void* splay_Satellite;

/** @brief Output of a search */
struct splay_Result
{
	int found;				/**< boolean value:  was the search successful? */
	splay_Key key;			/**< copy of the key of the record found */
	splay_Satellite sat;	/**< copy of the satellite data of the record */
};

struct splay_Node; /* deliberately left unspecified */

/** @brief Tree object, useful as a dictionary, set, multimap, or multiset */
struct splay_Tree
{
	/**	Opaque pointer to the internals of the tree.
		The user should not alter this field. */
	struct splay_Node *root;

	/**	Number of records in the tree.
		The user is welcome to read this field, but should not alter it.
		Behavior is unspecified if the user changes this field. */
	unsigned size;
};


/* PROTOTYPES */

/** @defgroup ExistOps Existential Operations

	@brief Creation, destruction, move, copy and clear */
/** @{ */
int splay_tree_empty_ctor(struct splay_Tree*);
void splay_tree_dtor(struct splay_Tree* t);
int splay_tree_copy(const struct splay_Tree* ti, struct splay_Tree* to);
int splay_tree_move(struct splay_Tree* ti, struct splay_Tree* to); /*ti -> to*/
int splay_tree_clear(struct splay_Tree*);
/** @} */


/** @defgroup DictOps Dictionary Operations

	@brief Find, update, insert, erase

	Note that keys need not be unique values. */
/** @{ */
int splay_insert(struct splay_Tree* t, splay_Key k, splay_Satellite sat);
int splay_update(struct splay_Tree* t, splay_Key k, splay_Satellite sat);
int splay_erase(struct splay_Tree* t, splay_Key k, splay_Satellite* psat);

struct splay_Result splay_find(struct splay_Tree *t, splay_Key k);
struct splay_Result splay_max(struct splay_Tree *t);
struct splay_Result splay_min(struct splay_Tree *t);
/** @} */



/** @defgroup SupportOps Support Operations

	@brief visualization and health check

	These are linear time, and they do not cause splaying.

	There is no function to query the number of records, but
	you the user are welcome to peek at the size field of splay_Tree.

	They perform string and operations that stdio.h, which you
	probably have linked in, unless you made the effort to eliminate
	that library dependency by setting macro @ref SPLAY_HAS_DOT_OUTPUT to zero
	at compile time. */
/** @{ */
void splay_debug_print_tree(const struct splay_Tree* t);
int splay_dot_output(const struct splay_Tree* t, const char* filename);
int splay_health_check(const struct splay_Tree *t, char buf[], unsigned bufsz);
/** @} */

#endif
