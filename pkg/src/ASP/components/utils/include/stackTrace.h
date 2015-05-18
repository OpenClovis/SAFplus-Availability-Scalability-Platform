
#ifndef _STACK_TRACE_H_
#define _STACK_TRACE_H_

#ifdef _cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include<pthread.h>
#include <execinfo.h> // for backtrace
#include <cxxabi.h> // for __cxa_demangle
#include <sys/types.h>
#include <unistd.h>

#define MAX_FUNC_NAME 100
#define MAX_DEBUG_THREAD 10
#define MAXFRAME 8
#define MAX_FUNC_NAME 100
#define MAX_FRAME_SIZE 256
#define DEBUG_THREAD_SIZE (MAXFRAME*MAX_FRAME_SIZE)

struct ThreadDebugBuffer
{
    int num_threads;
    int current;
    char buffer[MAX_DEBUG_THREAD*DEBUG_THREAD_SIZE];
};

static struct ThreadDebugBuffer *thread_debug_buffer;
pthread_mutex_t threadDebugLock;
int isInit = 0;

void InitTrace(void);
void addToBuffer(char *item);
void getCurrentFromBuffer(char *item);
void getFromBuffer(int num,char *item);
void getFromBuffer(int num,char *item);
void stack_backtrace(char* trace_buf);
void getStackTrace();

#endif


