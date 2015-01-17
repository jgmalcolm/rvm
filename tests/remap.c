/* test that replays log on segments unmapped then remapped */
#include <assert.h>
#include <stdio.h>
#include "rvm.h"

int main(void)
{
    rvm_t rvm = rvm_init("rvm_segments");

    rvm_destroy(rvm, "testseg");

    char *segs[1];
    segs[0] = (char *)rvm_map(rvm, "testseg", 20);

    trans_t trans = rvm_begin_trans(rvm, 1, (void **)segs);
    rvm_about_to_modify(trans, segs[0], 0, 10);
    segs[0][3] = 1;
    rvm_commit_trans(trans);

    /* remap */
    rvm_unmap(rvm, segs[0]);
    segs[0] = (char *)rvm_map(rvm, "testseg", 100);

    trans = rvm_begin_trans(rvm, 1, (void **)segs);
    assert(segs[0][3] == 1);

    rvm_about_to_modify(trans, segs[0], 0, 10);
    segs[0][3] = 2;
    rvm_abort_trans(trans);

    trans = rvm_begin_trans(rvm, 1, (void **)segs);
    assert(segs[0][3] == 1);

/*     rvm_truncate_log(rvm); */

    return 0;
}
