/* attempt to destroy a segment that is currently mapped in */

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

     char *segs[1];
     segs[0] = (char *) rvm_map(rvm, "testseg", 10);

     rvm_destroy(rvm, "testseg");

     return 0;
}
