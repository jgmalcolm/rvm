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

     rvm_unmap(rvm, (void *)0x56345);

     return 0;
}
