/* basic.c - test that basic persistency works */

#include "rvm.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>

#define TEST_STRING "hello, world"
#define TEST_STRING2 "goodbye, world"
#define OFFSET2 1000


/* call on offset out of range (<0, > size_to_create)*/


void proc1() 
{
     rvm_t rvm;
     trans_t trans;
     char* segs[0];
     
     rvm = rvm_init("rvm_segments");
     rvm_destroy(rvm, "testseg");
     segs[0] = (char *) rvm_map(rvm, "testseg", 10000);
     trans = rvm_begin_trans(rvm, 1, (void **) segs);
     rvm_about_to_modify(trans, segs[0], -100, 100);   /* wrong offset */
     sprintf(segs[0], TEST_STRING);     
     rvm_commit_trans(trans);
     abort();
}


/* proc2 opens the segments and reads from them */
void proc2() 
{
     char* segs[1];
     rvm_t rvm;
     
     rvm = rvm_init("rvm_segments");
     segs[0] = (char *) rvm_map(rvm, "testseg", 10000);
     if(strcmp(segs[0], TEST_STRING)==0) {
	  printf("ERROR: transaction 1 should not be written correctly due to offset problem \n");
	  exit(2);
     }
     
     printf("OK -- should have printed error because of invalid transaction range\n");
     exit(0);
}


int main(int argc, char **argv)
{
     int pid;

     pid = fork();
     if(pid < 0) {
	  perror("fork");
	  exit(2);
     }
     if(pid == 0) {
	  proc1();
	  exit(0);
     }

     waitpid(pid, NULL, 0);
     proc2();

     return 0;
}
