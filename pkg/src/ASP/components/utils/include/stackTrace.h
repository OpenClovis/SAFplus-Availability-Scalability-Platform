#ifndef _STACK_TRACE_H_
#define _STACK_TRACE_H_

#ifdef _cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include<pthread.h>
#include <execinfo.h> // for backtrace
#include <sys/types.h>
#include <unistd.h>
#include <clLogApi.h>

#define MAX_FUNC_NAME 128
// max thread in thread debug buffer
#define MAX_DEBUG_THREAD 10
#define MAXFRAME 16
#define MAX_FRAME_SIZE 256
#define DEBUG_THREAD_SIZE (MAXFRAME * MAX_FRAME_SIZE)
#define PATH_MAX 128

//Ring buffer to store thread debug information
struct ThreadDebugBuffer
{
    int num_threads;
    int current;
    char buffer[MAX_DEBUG_THREAD * DEBUG_THREAD_SIZE];
};

// Init  Thread Debug ring buffer
void InitThreadDebugBuffer(void);
// Write to thread debug buffer
void ThreadDebugBufferWrite(char *item);
// read latest thread in thread debug buffer
void ThreadDebugBufferRead(char *item);
// get thread in thread debug buffer
void ThreadDebugBufferGet(int num,char *item);
void get_backtrace (char* trace_buf,char* progname);
// get stack trace of current thread
void getCurrentThreadStack(char* progname);
//Write thread debug buffer to file
void writeThreadDebugBufferToFile(char* fileName);


#endif