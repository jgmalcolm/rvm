#include <assert.h>
#include <stdio.h>
#include "rvm.h"

int main(void)
{
    rvm_t rvm = rvm_init("rvm_segments");

    rvm_destroy(rvm, "testseg");

    char *segs[1];
    segs[0] = (char *)rvm_map(rvm, "testseg1", 100);

    trans_t trans = rvm_begin_trans(rvm, 1, (void **)segs);
    rvm_about_to_modify(trans, segs[0], 0, 5);
    rvm_about_to_modify(trans, segs[0], 4, 6);
    rvm_about_to_modify(trans, segs[0], 10, 5);
    rvm_about_to_modify(trans, segs[0], 16, 4);
    rvm_about_to_modify(trans, segs[0], 15, 1);

    /* should result in one modification stored since all of the above
     * coalesce */

    return 0;
}
