/* committing a transaction twice is a (silent) error */

#include "rvm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

int main(int argc, char **argv)
{
     rvm_t rvm;
     char *segs[1];

     rvm = rvm_init("rvm_segments");
     rvm_destroy(rvm, "testseg");
     segs[0] = (char *) rvm_map(rvm, "testseg", 50);

     trans_t trans1 = rvm_begin_trans(rvm, 1, (void **)segs);
     rvm_about_to_modify(trans1, segs[0], 0, 10);
     sprintf(segs[0], "one");
     rvm_commit_trans(trans1);
     
     trans_t trans2 = rvm_begin_trans(rvm, 1, (void **)segs);
     rvm_about_to_modify(trans2, segs[0], 4, 5);
     sprintf(segs[0], "two");
     rvm_commit_trans(trans2);

     rvm_commit_trans(trans1); /* ERROR */

     printf("OK\n");

     return 0;
}
