#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <string.h>
char *strdup(const char *s);
#include <stdlib.h>
#include <stdio.h>
int snprintf(char *str, size_t size, const char *format, ...);
#include <unistd.h>
int fsync(int fd);
#include <errno.h>
#include <err.h>

#include "ll.h"
#include "rvm.h"




#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define FULL_PERM (S_IRUSR | S_IWUSR |          \
                   S_IRGRP | S_IWGRP |          \
                   S_IROTH | S_IROTH)




/*-- rvm information --*/
struct rvminfo_t {
    int log_fd; /* file descriptor number for log file */
    char *dir; /* base directory of log and disk (backing store) */
    struct ll_t *seg_lst; /* list of mapped segments */
};

/*-- transaction --*/
struct transinfo_t {
    trans_t id; /* unique id */
    rvm_t rvm;
    struct ll_t *seg_lst;
};

/*-- segment --*/
struct seg_t {
    char *segname;
    void *segbase; /* where mapped into memory (for rvm_unmap lookup) */
    void *segbase_copy;
    int seglen;
    struct ll_t *mod_lst; /* list of modifications specified in
                           * rvm_about_to_modify() calls */
};

/*-- mod --*/
struct mod_t {
    int offset, size;
};







/*-- global data structures --*/
static struct rvminfo_t *rvms = NULL; /* uniquely indexed by rvm_t */
static unsigned int rvms_cnt = 0;

static struct ll_t *trans_lst = NULL; /* list of open transactions */
static unsigned int trans_cnt = 0;









rvm_t rvm_init(const char *dir)
{
    char fn[strlen(dir) + strlen("/rvm.log") + 1];
    assert(snprintf(fn, sizeof(fn), "%s/rvm.log", dir) < sizeof(fn));
    assert(access(dir, R_OK|W_OK) == 0); /* assert access to directory */

    /* allocate new rvminfo_t */
    rvms_cnt++;
    assert(rvms = realloc(rvms, rvms_cnt*sizeof(rvms[0])));
    rvm_t r = rvms_cnt - 1; /* zero-indexed */
    assert(rvms[r].dir = strdup(dir));

    /* if a non-empty log exists, then perform crash recovery */
    struct stat log_st;
    if (stat(fn, &log_st) == 0 && log_st.st_size > 0) {
        assert((rvms[r].log_fd = open(fn, O_RDONLY)) != -1);
        rvm_truncate_log(r);
        assert(close(rvms[r].log_fd) != -1);
    }

    /* open fresh log file */
    assert((rvms[r].log_fd = open(fn, O_RDWR|O_CREAT|O_TRUNC, FULL_PERM)) != -1);
    rvms[r].seg_lst = NULL; /* empty list of segments */

    return r;
}







static int cmp_segbase_fn(void *va, void *vb)
{
    struct seg_t *a = va, *b = vb;
    return a->segbase - b->segbase;
}

static int cmp_segname_fn(void *va, void *vb)
{
    struct seg_t *a = va, *b = vb;
    return strcmp(a->segname, b->segname);
}

/* replay log (from current position) for segment into memory.  If segname ==
 * NULL, then replay every segment encountered. */
typedef void (*save_fn)(char *segname, struct mod_t *mod, int fd);
static void replay_log_on_segment(rvm_t r, save_fn save)
{
    /*-- (1) close and reopen log so at beginning --*/
    /*-- (2) replay each matching segment with its modifications --*/

    /*-- (1) --*/
    char fn[strlen(rvms[r].dir) + strlen("/rvm.log") + 1];
    assert(snprintf(fn, sizeof(fn), "%s/rvm.log", rvms[r].dir) < sizeof(fn));
    assert(close(rvms[r].log_fd) == 0);
    assert((rvms[r].log_fd = open(fn, O_RDWR|O_CREAT, FULL_PERM)) != -1);

    int log_fd = rvms[r].log_fd;
    assert(lseek(log_fd, 0, SEEK_SET) == 0);


    /*-- (2) --*/
    void replay_segment(void)
    {
        struct stat st;
        assert(fstat(log_fd, &st) == 0);
        /* (attempt) check if this is the desired segment */
        int segname_len = 0;
        if (read(log_fd, &segname_len, sizeof(segname_len)) <= 0)
            return; /* DONE: end of log */

        char segname_buf[segname_len+1];
        assert(read(log_fd, segname_buf, segname_len) != -1);
        segname_buf[segname_len] = '\0';

        void replay_mod(void)
        {
            struct mod_t mod;
            assert(read(log_fd, &mod.size, sizeof(mod.size)) != -1);

            /* play modification */
            assert(read(log_fd, &mod.offset, sizeof(mod.offset)) != -1);
            save(segname_buf, &mod, log_fd); /* use callback to delegate
                                              * saving */
        }

        /* get number of following segments */
        int mod_cnt;
        assert(read(log_fd, &mod_cnt, sizeof(mod_cnt)) != -1);
        while (mod_cnt > 0) {
            replay_mod();
            mod_cnt--;
        }

        replay_segment(); /* recurse/curry */
    }

    replay_segment(); /* start recursion */
}








void *rvm_map(rvm_t r, const char *segname, int size_to_create)
{
    /*-- (1) form path from dir and filename --*/
    /*-- (2) read in segment from disk if such a copy exists --*/
    /*-- (3) replay log on segment --*/


    /*-- (1) --*/
    char fn[strlen(rvms[r].dir) + 1 + strlen(segname) + 4 + 1];
    assert(snprintf(fn, sizeof(fn),
                    "%s/%s.seg",
                    rvms[r].dir, segname) < sizeof(fn));

    /* check if already present */
    struct seg_t search_seg;
    search_seg.segname = (char *)segname;
    if (ll_find_sorted(rvms[r].seg_lst, cmp_segname_fn, &search_seg))
        errx(1, "segment '%s' already mapped in", segname);

    /* allocate a segment in memory */
    char *segbase;
    assert(segbase = malloc(size_to_create));
    memset(segbase, 0x00, size_to_create); /* zero out */

    /*-- (2) --*/
    struct stat st;
    if (access(fn, R_OK|W_OK) == 0 && stat(fn, &st) == 0) {
        /* (1) check that requested size is at least actual size, (2) attempt
         * open, (3) read into allocated memory */
        if (size_to_create < st.st_size)
            errx(3, "segment request size (%d) less than current size (%ld)",
                 size_to_create, st.st_size);
        int fd;
        if ((fd = open(fn, O_RDONLY)) == -1)
            errx(2, "failed to open segment '%s'", fn);
        assert(read(fd, segbase, size_to_create) != -1); /* BUG: may not read
                                                            as much as we
                                                            requested */
        assert(close(fd) != -1);
    }

    /* create segment bookkeeping datastructure */
    struct seg_t *seg;
    assert(seg = malloc(sizeof(*seg)));
    rvms[r].seg_lst = ll_add(rvms[r].seg_lst, seg);
    seg->segname = strdup(segname);
    seg->segbase = segbase;
    seg->segbase_copy = NULL; /* populated when beginning transactions */
    seg->seglen = size_to_create;
    seg->mod_lst = NULL;

    /*-- (3) --*/
    void save_fn(char *log_segname, struct mod_t *mod, int fd)
    {
        if (strcmp(log_segname, segname) == 0) {
            /* save */
            assert(read(fd, seg->segbase + mod->offset, mod->size) != -1);
        } else {
            /* skip */
            assert(lseek(fd, mod->size, SEEK_CUR) != -1);
        }

    }
    replay_log_on_segment(r, save_fn);

    return segbase;
}







void rvm_unmap(rvm_t r, void *segbase)
{
    /*-- (1) ensure no open transaction is using this segment --*/
    /*-- (2) ensure this segment exists --*/
    /*-- (3) remove from segment list --*/
    /*-- (4) deallocate segment --*/

    /*-- (1) --*/
    int test_withoutseg_fn(void *vtrans) /* 1 iff no such seg */
    {
        struct transinfo_t *trans = vtrans;
        /* 1 iff segment not the one looking to unmap */
        int test_segnotequal_fn(void *vseg)
        {
            struct seg_t *seg = vseg;
            return seg->segbase != segbase;
        }
        return ll_all(trans->seg_lst, test_segnotequal_fn);
    }
    if (ll_all(trans_lst, test_withoutseg_fn) == 0)
        errx(1, "attempt to unmap segment in open transaction");

    /*-- (2), (3) --*/
    struct seg_t search_seg;
    search_seg.segbase = segbase;
    struct seg_t *seg;
    seg = ll_remove(&rvms[r].seg_lst, cmp_segbase_fn, &search_seg);
    if (seg == NULL)
        errx(1, "segment does not exist");

    /*-- (4) --*/
    free(seg->segname);
    free(seg->segbase);
    free(seg->segbase_copy);
    free(seg);
}







void rvm_destroy(rvm_t r, const char *segname)
{
    /*-- (1) ensure segment unmapped --*/
    /*-- (2) unlink file --*/
    /* Note, forget if has entries in log because replay and truncate handle
     * the case where segment no longer exists--they just continue on to next
     * log entry. */

    /*-- (1) --*/
    struct seg_t search_seg;
    search_seg.segname = (char *)segname;
    if (ll_find(rvms[r].seg_lst, cmp_segname_fn, &search_seg))
        errx(1, "cannot destroy mapped segment");

    /*-- (2) --*/
    char fn[strlen(rvms[r].dir) + 1 + strlen(segname) + 4 + 1];
    assert(snprintf(fn, sizeof(fn),
                    "%s/%s.seg",
                    rvms[r].dir, segname) < sizeof(fn));
    unlink(fn);
}






static struct ll_t *create_seglst(rvm_t r, int numsegs, void **segbases)
{
    struct ll_t *lst = NULL;

    int i;
    for (i = 0; i < numsegs; i++) {
        struct seg_t search_seg;
        search_seg.segbase = segbases[i];
        struct seg_t *seg;
        seg = ll_find(rvms[r].seg_lst, cmp_segbase_fn, &search_seg);
        if (seg == NULL)
            errx(1, "transaction references noexistent segment");

        assert(seg->mod_lst == NULL); /* should not have any modifications */
        seg->mod_lst = NULL;

        /* ensure no other transaction is using, i.e. backup copy present */
        if (seg->segbase_copy != NULL) {
            ll_free(lst, NULL); /* free the linked list of what we have, not
                                   the data */
            return NULL; /* error */
        }
        /* create segment backup copy for aborted transactions */
        assert(seg->segbase_copy == NULL); /* nothing should be there */
        assert(seg->segbase_copy = malloc(seg->seglen));
        memcpy(seg->segbase_copy, seg->segbase, seg->seglen);

        lst = ll_add(lst, seg);
    }
    
    return lst;
}

trans_t rvm_begin_trans(rvm_t r, int numsegs, void **segbases)
{
    /*-- (1) allocate and populate a transaction (used in next step) --*/
    /*-- (2) create transaction segment list ensuring segment existence --*/
    /*-- (3) ensure no other transaction contains any of these segments --*/
    /*-- (4) add to transaction list --*/

    /*-- (1) --*/
    struct transinfo_t *trans;
    assert(trans = malloc(sizeof(*trans)));
    trans->id = trans_cnt;
    trans->rvm = r;
    trans->seg_lst = NULL; /* changed in (2) */

    /*-- (2) --*/
    trans->seg_lst = create_seglst(r, numsegs, segbases);

    /*-- (3) --*/
    if (trans->seg_lst == NULL && numsegs != 0) {
        warnx("attempt to use segment in multiple transactions");
        free(trans);
        return (trans_t)-1;
    }

    /*-- (4) --*/
    trans_lst = ll_add(trans_lst, trans);
    trans_cnt++;

    return trans->id;
}



static int trans_cmp_fn(void *va, void *vb)
{
    struct transinfo_t *a = va, *b = vb;
    return a->id - b->id;
}




void rvm_about_to_modify(trans_t tid, void *segbase, int offset, int size)
{
    /*-- (1) ensure transaction exists and contains this segment --*/
    /*-- (2) ensure modification is within segment --*/
    /*-- (3) repeatedly attempt to coalesce modifications --*/
    /*-- (4) tack this possibly coalesced modification onto the transaction --*/

    /*-- (1) --*/
    /* find transaction */
    struct transinfo_t search_trans;
    search_trans.id = tid;
    struct transinfo_t *trans = ll_find(trans_lst, trans_cmp_fn,
                                        &search_trans);
    if (trans == NULL)
        errx(1, "transaction not found");

    /* find segment in this transaction */
    struct seg_t search_seg;
    search_seg.segbase = segbase;
    struct seg_t *seg = ll_find(trans->seg_lst, cmp_segbase_fn, &search_seg);
    if (seg == NULL) {
        errx(1, "transaction does not contain specified segment");
    }

    /*-- (2) --*/
    if (offset < 0 || seg->seglen <= offset || seg->seglen < offset + size)
        errx(1, "modification out of segment bounds");

    /*-- (3) --*/
    struct mod_t *mod;
    assert(mod = malloc(sizeof(*mod)));
    mod->offset = offset;
    mod->size = size;
    int test_overlap_fn(void *va, void *vb)  /* 0 iff overlapping */
    {
        struct mod_t *a = va, *b = vb;
        if (b->offset < a->offset)
            return test_overlap_fn(b, a); /* fold so force a < b */
        /* if b starts anywhere within a, then they overlap */
        return !(b->offset <= a->offset + a->size);
    }
    struct mod_t *coalesce(struct mod_t *a, struct mod_t *b)
    {
        struct mod_t *new;
        assert(new = malloc(sizeof(*new)));

        /* who starts first? */
        if (a->offset < b->offset)
            new->offset = a->offset;
        else
            new->offset = b->offset;

        /* who ends last? */
        if (a->offset + a->size < b->offset + b->size)
            new->size = b->offset + b->size - new->offset;
        else
            new->size = a->offset + a->size - new->offset;

        return new;
    }
    /* continue coalescing overlapping modifications */
    struct mod_t *overlapping;
    while ((overlapping = ll_remove(&seg->mod_lst, test_overlap_fn, mod))
           != NULL) { /* while find overlapping mod */
        struct mod_t *coalesced = coalesce(overlapping, mod);
        free(mod);
        free(overlapping);
        mod = coalesced; /* repeat using this as the reference point */
    }

    /*-- (4) --*/
    seg->mod_lst = ll_add(seg->mod_lst, mod);
}





    

void rvm_commit_trans(trans_t tid)
{
    /*-- (1) stream segments to log --*/
    /*-- (2) flush log to disk --*/
    /*-- (3) delete and deallocate transaction --*/
    /* Note, could speed up if remove (3) when attempting to find in (1). */

    /*-- (1) --*/
    struct transinfo_t search_trans;
    search_trans.id = tid;
    struct transinfo_t *trans;
    trans = ll_find(trans_lst, trans_cmp_fn, &search_trans);
    if (trans == NULL)
        errx(1, "transaction not found");
    
    int log_fd = rvms[trans->rvm].log_fd;
    void commitseg_fn(void *v_seg)
    {
        /*-- (1) write name --*/
        /*-- (2) stream modifications to log --*/

        /*-- (1) --*/
        struct seg_t *seg = v_seg;
        int segname_len = strlen(seg->segname);
        assert(write(log_fd, &segname_len, sizeof(segname_len)) != -1);
        assert(write(log_fd, seg->segname, segname_len) != -1);

        /*-- (2) --*/
        int mod_cnt = ll_count(seg->mod_lst);
        assert(write(log_fd, &mod_cnt, sizeof(mod_cnt)) != -1);
        void stream_mod_fn(void *vmod)
        {
            struct mod_t *mod = vmod;

            assert(write(log_fd, &mod->size, sizeof(mod->size)) != -1);
            assert(write(log_fd, &mod->offset, sizeof(mod->offset)) != -1);
            assert(write(log_fd, seg->segbase + mod->offset,
                         mod->size) != -1);
        }
        ll_map(seg->mod_lst, stream_mod_fn);
    }
    ll_map(trans->seg_lst, commitseg_fn);

    /*-- (2) --*/
    assert(fsync(log_fd) == 0);

    /*-- (3) --*/
    trans = ll_remove(&trans_lst, trans_cmp_fn, &search_trans);
    void seglst_free_fn(void *vseg)
    {
        void free_mod_fn(void *vmod)
        {
            struct mod_t *mod = vmod;
            free(mod);
        }
        struct seg_t *seg = vseg;
        ll_free(seg->mod_lst, free_mod_fn); /* clear modifications on segments */
        seg->mod_lst = NULL;

        free(seg->segbase_copy);
        seg->segbase_copy = NULL;
    }
    ll_free(trans->seg_lst, seglst_free_fn); /* clears segment modifications */
    free(trans);
}






void rvm_abort_trans(trans_t tid)
{
    /*-- (1) remove transaction --*/
    /*-- (2) revert and clear modifications --*/

    /*-- (1) --*/
    struct transinfo_t search_trans;
    search_trans.id = tid;
    struct transinfo_t *trans;
    trans = ll_remove(&trans_lst, trans_cmp_fn, &search_trans);
    if (trans == NULL)
        errx(1, "transaction not found");

    /* cleans up the aborted transaction segment list, i.e. clears modifications,
     * reverts to backup copy, deallocates modified version */
    void seglst_aborted_fn(void *vseg)
    {
        void free_mod_fn(void *vmod)
        {
            struct mod_t *mod = vmod;
            free(mod);
        }
        struct seg_t *seg = vseg;
        ll_free(seg->mod_lst, free_mod_fn); /* clear modifications on segments */
        seg->mod_lst = NULL;

        /* revert to saved version of segment */
        memcpy(seg->segbase, seg->segbase_copy, seg->seglen);
        free(seg->segbase_copy);
        seg->segbase_copy = NULL;
    }

    /*-- (2) --*/
    ll_free(trans->seg_lst, seglst_aborted_fn);
    free(trans);
}







/* Note, only writes out modifications in the log, i.e. up through last
 * committed transaction.  Leaves log empty and open. */
void rvm_truncate_log(rvm_t r)
{
    /* save function: opens segment, modifies, flushes to disk */
    void save_fn(char *segname, struct mod_t *mod, int fd)
    {
        char seg_fn[strlen(rvms[r].dir) + 1 + strlen(segname) + 4 + 1];
        assert(snprintf(seg_fn, sizeof(seg_fn), "%s/%s.seg",
                        rvms[r].dir, segname) < sizeof(seg_fn));
        int seg_fd;
        assert((seg_fd = open(seg_fn, O_WRONLY|O_CREAT, FULL_PERM)) != -1);
        assert(lseek(seg_fd, mod->offset, SEEK_SET)  != (off_t)-1);
        char buf[mod->size];
        assert(read(fd, buf, mod->size) != -1); /* read in from log */
        assert(write(seg_fd, buf, mod->size) != -1); /* write out to seg */
        assert(fsync(seg_fd) == 0); /* flush segment to disk */
        assert(close(seg_fd) == 0);
    }
    replay_log_on_segment(r, save_fn);

    /* close and reopen log to truncate it */
    assert(close(rvms[r].log_fd) == 0);
    char fn[strlen(rvms[r].dir) + strlen("/rvm.log") + 1];
    assert(snprintf(fn, sizeof(fn), "%s/rvm.log", rvms[r].dir) < sizeof(fn));
    assert((rvms[r].log_fd = open(fn, O_WRONLY|O_TRUNC, FULL_PERM)) != -1);
}





/* TODO: Possible to optimize the amount of memory traffic in transaction
 * commits and aborts.  Currently, at the start of every transaction, a backup
 * copy is created for both the purpose of finding differences and reverting
 * in case of an abort.  This could potentially be a lot of memory to backup
 * and check through for differences.  This could be optimized down if the
 * backup were performed during rvm_about_to_modify() so that only those
 * regions are backed up and checked for differences, not the entire segment.
 * Remember that modifications are coalesced in rvm_about_to_modify(). */


/*-- LOG FORMAT --
  Transaction:
  <seg 0>
  <seg 1>
  <seg 2>
  ...
  <seg n>

  Segment:
  int segname_len;   --length of following segment name (character count)
  char segname[segname_len]; --null terminator not needed because length-prefixed
  int mod_cnt;
  <mod 0>
  <mod 1>
  <mod 2>
  ...
  <mod mod_cnt-1>

  Modification:
  int size;
  int offset;
  <data>

--*/

