#ifndef __LL_H
#define __LL_H

struct ll_t {
    void *data;
    struct ll_t *next;
};

typedef void (*ll_free_fn) (void *a);
typedef int (*ll_cmp_fn) (void *a, void *b);
typedef char *(*ll_tostring_fn) (void *a);    /* malloc'ed string repres. */
typedef int (*ll_test_fn)(void *a);
typedef int (*ll_testpair_fn)(void *a, void *b);
typedef void *(*ll_map_fn)(void *a);
typedef struct ll_t *(*ll_update_fn)(void *cur, void *next);

/*-- Adding --*/
struct ll_t *ll_add(struct ll_t *, void *);
struct ll_t *ll_add_to_back(struct ll_t *, void *);
struct ll_t *ll_add_to_front(struct ll_t *, void *);
struct ll_t *ll_add_sorted(struct ll_t *, ll_cmp_fn, void *);
struct ll_t *ll_concat(struct ll_t *, struct ll_t *);

/*-- Removing --*/
void *ll_remove_first(struct ll_t **);
void *ll_remove_last(struct ll_t **);
void *ll_remove(struct ll_t **, ll_test_fn);
void ll_move(struct ll_t **from, struct ll_t **to);
void ll_straighten(struct ll_t *lst); /* un-circularize list */

/*-- Finding --*/
void *ll_find(struct ll_t *, ll_cmp_fn, void *);
void *ll_find_sorted(struct ll_t *, ll_cmp_fn, void *);

/*-- Freeing --*/
void ll_free(struct ll_t *, ll_free_fn); /* free_fn == NULL iff only free
                                          * linked list data structure, not
                                          * actual data */
void ll_free_c(struct ll_t *, ll_free_fn); /* see ll_free */

/*-- Count --*/
int ll_count(struct ll_t *);
int ll_count_c(struct ll_t *); /* circular lists */
int ll_empty(struct ll_t *);

/*-- Traversing --*/
void ll_print(struct ll_t *, ll_tostring_fn);
char *ll_tostring(struct ll_t *, ll_tostring_fn);

/* 1 iff all pass test_fn==1 */
int ll_all(struct ll_t *, ll_test_fn);

/* 1 iff all pairwise combinations pass */
int ll_all_pair(struct ll_t *, struct ll_t *, ll_testpair_fn);

/* call fn on each element */
void ll_map(struct ll_t *, ll_map_fn);
void ll_map_c(struct ll_t *, ll_map_fn); /* circular lists */

/* fold functionality */
typedef void *(*fold_fn)(void *a, void *init);
void *ll_foldl(fold_fn fn, void *init, struct ll_t *lst);
void *ll_foldr(fold_fn fn, void *init, struct ll_t *lst);

/* form new list of all elements that pass test_fn()==1 */
struct ll_t *ll_filter(struct ll_t *, ll_test_fn);
struct ll_t *ll_filter_set(struct ll_t **, ll_test_fn); /* destructive */
struct ll_t *ll_update_c(struct ll_t *, ll_update_fn);

/*-- Peek --*/
void *ll_peek(struct ll_t *);
void *ll_peek_nth(struct ll_t *, unsigned int); /* zero-indexed */
struct ll_t *ll_next(struct ll_t *);

#endif


/* Note, at times I took as my inspiration the SML Basis library List
 * functionality, e.g. ll_all().  But, it is really hard without the use of
 * nested scope inherent in SML; I actually am furious that such lexical
 * scoping is not available in C. */
