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


/* call with a segment NOT in rvm_begin_trans */
void proc1() 
{
     rvm_t rvm;
     trans_t trans;
     char* segs[0];
     char* segs2[0];
     
     rvm = rvm_init("rvm_segments");
     rvm_destroy(rvm, "testseg");
     segs[0] = (char *) rvm_map(rvm, "testseg", 10000);
     trans = rvm_begin_trans(rvm, 1, (void **) segs);
     rvm_about_to_modify(trans, segs[0], 0, 100);
     sprintf(segs[0], TEST_STRING);     
     rvm_commit_trans(trans);
     
     segs2[0] = (char *) rvm_map(rvm, "testseg2", 10000);
     trans = rvm_begin_trans(rvm, 1, (void **) segs2);
     rvm_about_to_modify(trans, segs[0], 0, 100); /* about to modify segs, but
                                                     transaction is for
                                                     segs2 */
     sprintf(segs[0], TEST_STRING2);     
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
     if(strcmp(segs[0], TEST_STRING)) {
	  printf("ERROR: unvalid transaction 2 overwrites transaction 1\n");
	  exit(2);
     }
     
     printf("OK -- test should have printed error because call about_to_modify on wrong segs\n");
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
