/* test basic functionality of truncation (like basic.c but truncate in between both transactions) */

#include "rvm.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>

#define TEST_STRING "hello, world"
#define OFFSET2 50


/* proc1 writes some data, commits it, then exits */
void proc1() 
{
     rvm_t rvm;
     trans_t trans;
     char* segs[0];
     
     rvm = rvm_init("rvm_segments");
     rvm_destroy(rvm, "testseg");
     segs[0] = (char *) rvm_map(rvm, "testseg", 100);

     
     trans = rvm_begin_trans(rvm, 1, (void **) segs);

     rvm_about_to_modify(trans, segs[0], 0, 20);
     sprintf(segs[0], TEST_STRING);
     rvm_commit_trans(trans);

     rvm_truncate_log(rvm);
     
     trans = rvm_begin_trans(rvm, 1, (void **) segs);
     rvm_about_to_modify(trans, segs[0], OFFSET2, 20);
     sprintf(segs[0]+OFFSET2, TEST_STRING);
     rvm_commit_trans(trans);

     abort();
}


/* proc2 opens the segments and reads from them */
void proc2() 
{
     char* segs[1];
     rvm_t rvm;

     rvm = rvm_init("rvm_segments");

     segs[0] = (char *) rvm_map(rvm, "testseg", 100);
     if(strcmp(segs[0], TEST_STRING)) {
	  printf("ERROR: first hello not present, it is instead [%s]\n",segs[0]);
	  exit(2);
     }
     if(strcmp(segs[0]+OFFSET2, TEST_STRING)) {
	  printf("ERROR: second hello not present, it is instead [%s]\n",segs[0]+OFFSET2);
	  exit(2);
     }

     printf("OK\n");
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
