/* test_multi2.c - 
- map 100 segments
- start 100 transactions (one per segment)
- trans1: modify all segments to aa
- map/unmap with DIFFERENT RANGE
- trans1: modify all segments to ab
- commit every other segment
- abort
- check the log
*/



#include "rvm.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/wait.h>

#define OFFSET0  10
#define OFFSET1  100

#define STRING0 "hello, world"
#define STRING2 "goodbye, world"
#define MAX_SEGMENT_NAME_LEN 100

int numSegments = 100;

void proc1() 
{
    rvm_t rvm;
    char* segs[2];
    trans_t trans;
    char** segnames;
    int i;

    rvm = rvm_init("rvm_segments");
     
    segnames = (char **)malloc(numSegments * sizeof(char *));
     
    for(i = 0; i < numSegments; i++) {
        segnames[i] = (char *)malloc(MAX_SEGMENT_NAME_LEN);   
        sprintf(segnames[i],"testseg%03d",i);
        rvm_destroy(rvm, segnames[i]);
    }

    
    for(i = 0; i < numSegments; i++) {
        segs[0] = (char *) rvm_map(rvm, segnames[i], 3);
        trans = rvm_begin_trans(rvm, 1, (void **) segs);
        rvm_about_to_modify(trans, segs[0], 0, 3); 
        sprintf(segs[0],"aa");
        rvm_commit_trans(trans);
        rvm_unmap(rvm,segs[0]);
    }

    
    for(i = 0; i < numSegments; i++) {
        segs[0] = (char *) rvm_map(rvm, segnames[i], 4);
        trans = rvm_begin_trans(rvm, 1, (void **) segs);
        rvm_about_to_modify(trans, segs[0], 2, 2); 
        segs[0][2] = 'b';
        segs[0][3] = '\0';
        assert(strcmp("aab", segs[0]) == 0);
        if(i%2 == 0){
            rvm_commit_trans(trans);
        }
    }
    

    for(i = 0; i < numSegments; i++) {
        free(segnames[i]);
    }
    free(segnames);

    abort();
}


void proc2() 
{
    rvm_t rvm;
    char *segs[2];
    char** segnames;
    int i;

    segnames = (char **)malloc(numSegments * sizeof(char *));
     
    for(i = 0; i < numSegments; i++) {
        segnames[i] = (char *)malloc(MAX_SEGMENT_NAME_LEN);   
        sprintf(segnames[i],"testseg%03d",i);
    }
    rvm = rvm_init("rvm_segments");
     
    for(i = 0; i < numSegments; i++) {
      
        segs[0] = (char *) rvm_map(rvm, segnames[i], 4);
        if(i%2 == 0){
            assert(strcmp("aab", segs[0]) == 0);
        } else {
            assert(strcmp(segs[0], "aa") == 0);
        }
        rvm_unmap(rvm,segs[0]);
    }
     

    printf("OK\n");

    for(i = 0; i < numSegments; i++) {
        free(segnames[i]);
    }
    free(segnames);
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

