
#ifndef _STACK_TRACE_H_
#define _STACK_TRACE_H_

#ifdef _cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include<pthread.h>
#include <execinfo.h> // for backtrace
//#include <cxxabi.h> // for __cxa_demangle
#include <sys/types.h>
#include <unistd.h>
#include <clLogApi.h>

#define MAX_FUNC_NAME 128
#define MAX_DEBUG_THREAD 10
#define MAXFRAME 8
#define MAX_FRAME_SIZE 256
#define DEBUG_THREAD_SIZE (MAXFRAME * MAX_FRAME_SIZE)
#define PATH_MAX 128

struct ThreadDebugBuffer
{
    int num_threads;
    int current;
    char buffer[MAX_DEBUG_THREAD * DEBUG_THREAD_SIZE];
};

void InitTrace(void);
void addToBuffer(char *item);
void getCurrentThreadTrace(char *item);
void getThreadTrace(int num,char *item);
void get_backtrace (char* trace_buf,char* progname);
void getStackTrace(char* progname);
void writeAllThreadTraceToFile(char* fileName);


#endif


