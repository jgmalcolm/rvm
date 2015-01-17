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


/* call about_to_modify multiple times on the same memory area, overlapping and non-overlapping memory area*/
void proc1() 
{
     rvm_t rvm;
     trans_t trans;
     char* segs[0];
     
     char* string1= calloc(sizeof(char)*200,1);
     memset(string1, 1, 199);
     
     
     
     rvm = rvm_init("rvm_segments");
     rvm_destroy(rvm, "testseg");
     segs[0] = (char *) rvm_map(rvm, "testseg", 10000);
     trans = rvm_begin_trans(rvm, 1, (void **) segs);
     rvm_about_to_modify(trans, segs[0], 0, 100);
     rvm_about_to_modify(trans, segs[0], 0, 100);
     rvm_about_to_modify(trans, segs[0], 50, 150);
     rvm_about_to_modify(trans, segs[0], 100, 200);
     
     sprintf(segs[0], string1);     
     rvm_commit_trans(trans);
          
     abort();
}


/* proc2 opens the segments and reads from them */
void proc2() 
{
     char* segs[1];
     rvm_t rvm;
     
     char* string1= calloc(sizeof(char)*200,1);
     memset(string1, 1, 199);
     
     rvm = rvm_init("rvm_segments");
     segs[0] = (char *) rvm_map(rvm, "testseg", 10000);
     if(strcmp(segs[0], string1)) {
	  printf("ERROR: transaction 1 not persistent\n");
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
