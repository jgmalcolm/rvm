#include <assert.h>
#include <stdio.h>
#include "rvm.h"

int main(void)
{
    rvm_t rvm = rvm_init("rvm_segments");

    rvm_destroy(rvm, "testseg");
    rvm_destroy(rvm, "testseg2");

    char *segs_a[2];
    segs_a[0] = (char *)rvm_map(rvm, "testseg", 100);
    segs_a[1] = (char *)rvm_map(rvm, "testseg2", 50);
    char *segs_b[2];
    segs_b[0] = (char *)rvm_map(rvm, "testseg3", 100);
    segs_b[1] = segs_a[1];

    trans_t trans_a = rvm_begin_trans(rvm, 2, (void **)segs_a);
    trans_t trans_b = rvm_begin_trans(rvm, 2, (void **)segs_b);

    assert(trans_a != (trans_t)-1);
    assert(trans_b == (trans_t)-1);

    return 0;
}
