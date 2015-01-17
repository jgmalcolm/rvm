/* destroy_nonexistant.c - test that deleting a nonexistant segment does not
 * result in error but simply returns gracefully */

#include "rvm.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv)
{
     rvm_t r;
     r = rvm_init("rvm_segments");
     rvm_destroy(r, "nonexistant");
     return 0;
}

