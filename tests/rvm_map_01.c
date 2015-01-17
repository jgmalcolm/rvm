/* unmap a segment with outstanding uncommited transaction */

#include "rvm.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <err.h>

int main(int argc, char **argv)
{
     rvm_t rvm = rvm_init("rvm_segments");
     rvm_destroy(rvm, "testseg");

     char* segs[1];
     segs[0] = (char *) rvm_map(rvm, "testseg", 10);
     rvm_begin_trans(rvm, 1, (void **) segs);
     rvm_unmap(rvm, segs[0]);

     printf("OK -- printed error about unmapping segment involved in tx\n");
     return 0;
}
