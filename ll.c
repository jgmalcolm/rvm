#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "ll.h"

/* Internal prototypes */
static char *ll_strcat(char *, char *);

struct ll_t *ll_add_to_back(struct ll_t *pn, void *d)
{
    if (pn == NULL) {
        struct ll_t *pnew;
        assert(pnew = malloc(sizeof(*pnew)));
        pnew->data = d;
        pnew->next = NULL;
        return pnew;
    } else {
        pn->next = ll_add_to_back(pn->next, d);
        return pn;
    }
}

struct ll_t *ll_add_to_front(struct ll_t *pn, void *d)
{
    struct ll_t *pnew;
    assert(pnew = malloc(sizeof(*pnew)));
    pnew->data = d;
    pnew->next = pn;
    return pnew;
}

struct ll_t *ll_add(struct ll_t *pn, void *d)
{
    return ll_add_to_back(pn, d);
}

struct ll_t *ll_add_sort(struct ll_t *n, ll_cmp_fn cmp_fn, void *d)
{

    if (n == NULL) {
        struct ll_t *pnew;
        assert(pnew = malloc(sizeof(*pnew)));
        pnew->data = d;
        pnew->next = NULL;
        return pnew;
    }

    if (cmp_fn(d, ll_peek(n)) <= 0) {
        /* belongs here or before */
        struct ll_t *pnew;
        assert(pnew = malloc(sizeof(*pnew)));
        pnew->data = d;
        pnew->next = n;
        return pnew;
    } else {
        /* belongs later */
        n->next = ll_add_sort(n->next, cmp_fn, d);
        return n;
    }
}

void *ll_remove_first(struct ll_t **ppn)
{
    struct ll_t *first;
    void *ret;

    if (*ppn == NULL)
        return NULL;

    first = *ppn;           /* Get first node */
    *ppn = (*ppn)->next;    /* Move the head of the list */
    ret = first->data;      /* save pointer to its data */
    free(first);            /* free node */
    return ret;             /* return data */
}

void *ll_remove_last(struct ll_t **ppn)
{
    assert(0 && "not yet implemented"); /* haha, this is legal syntax!!! */
    return (NULL);
}

void *ll_find_sorted(struct ll_t *lst, ll_cmp_fn cmp_fn, void *d)
{
    if (lst == NULL)
        return NULL;

    int cmp = cmp_fn(d, ll_peek(lst));
    if (cmp < 0)
        return NULL; /* should have hit it already */
    else if (cmp == 0)
        return ll_peek(lst);
    else
        return ll_find_sorted(lst->next, cmp_fn, d);
}

void *ll_find(struct ll_t *lst, ll_cmp_fn cmp_fn, void *d)
{
    if (lst == NULL)
        return NULL;
    
    if (cmp_fn(ll_peek(lst), d) == 0)
        return ll_peek(lst);
    else
        return ll_find(lst->next, cmp_fn, d);
}

void ll_free(struct ll_t *pn, ll_free_fn free_fn)
{
    if (pn != NULL) {
        ll_free(pn->next, free_fn);
        if (free_fn) /* need to free data? */
            free_fn(pn->data);
        free(pn);
    }
}

int ll_empty(struct ll_t *n)
{
    return (n == NULL);
}

int ll_count(struct ll_t *pn)
{
    if (pn == NULL)
        return (0);
    return (1 + ll_count(pn->next));
}

void ll_print(struct ll_t *n, ll_tostring_fn tostring)
{
    if (n != NULL) {
        char *str = tostring(n->data);
        fprintf(stdout, "%s", str);
        free(str);
        ll_print(n->next, tostring);
    }
}

char *ll_tostring(struct ll_t *n, ll_tostring_fn tostring)
{
    if (n == NULL)
        return (NULL);

    return (ll_strcat(tostring(n->data),
                ll_tostring(n->next, tostring)));
}

static char *ll_strcat(char *dest, char *src)
{
    char *chk;
    size_t len;

    if (src == NULL)
        return (dest);

    len = strlen(dest) + strlen(src) + 1;
    assert((chk = realloc(dest, len)) != NULL);
    dest = chk;
    strcat(dest, src);
    return (dest);
}

void *ll_peek(struct ll_t *n)
{
    return n->data;
}


struct ll_t *ll_next(struct ll_t *n)
{
    return n->next;
}

void *ll_peek_nth(struct ll_t *p, unsigned int n)
{
    if (n == 0)
        return ll_peek(p);
    else if (p->next != NULL)
        return ll_peek_nth(p->next, n-1);
    else
        return NULL; /* not found */
}


void *ll_remove(struct ll_t **lst, ll_cmp_fn cmp_fn, void *to_find)
{
    /* done? */
    if (*lst == NULL)
        return NULL;

    /* is this it? */
    if (cmp_fn(to_find, ll_peek(*lst)) == 0) {
        /* remove it */
        return ll_remove_first(lst);
    } else {
        /* keep looking */
        struct ll_t **ptr_to_next = &(*lst)->next;
        return ll_remove(ptr_to_next, cmp_fn, to_find);
    }
}



int ll_all(struct ll_t *lst, ll_test_fn test_fn)
{
    if (lst == NULL)
        return 1;

    if (test_fn(ll_peek(lst)) == 0)
        return 0; /* fail */
    else
        return ll_all(ll_next(lst), test_fn);
}


int ll_all_pair(struct ll_t *a, struct ll_t *b, ll_testpair_fn test_fn)
{
    if (a == NULL)
        return 1; /* done */

    int apply_pair(void *a, struct ll_t *lst_b)
    {
        if (lst_b == NULL)
            return 1; /* done */
        if (test_fn(a, ll_peek(lst_b)) == 0)
            return 0; /* fail */
        else
            return apply_pair(a, ll_next(lst_b)); /* continue */
    }
    if (apply_pair(ll_peek(a), b) == 0)
        return 0; /* fail */
    else
        return ll_all_pair(ll_next(a), b, test_fn); /* continue */
}



void ll_map(struct ll_t *lst, ll_map_fn map_fn)
{
    if (lst == NULL)
        return;

    lst->data = map_fn(ll_peek(lst));
    ll_map(ll_next(lst), map_fn);
}



struct ll_t *ll_concat(struct ll_t *first, struct ll_t *last)
{
    if (first == NULL)
        return last; /* put here */

    first->next = ll_concat(first->next, last);
    return first;
}



struct ll_t *ll_filter(struct ll_t *lst, ll_test_fn test)
{
    if (lst == NULL)
        return NULL;

    if (test(ll_peek(lst)) == 1) {
        /* keep this node */
        struct ll_t *new;
        assert(new = malloc(sizeof(*new)));
        new->next = ll_filter(ll_next(lst), test);
        new->data = ll_peek(lst);
        return new;
    } else {
        return ll_filter(ll_next(lst), test);
    }
}
