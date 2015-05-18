#include "stackTrace.h"
#include <stdlib.h>
#include <string.h>
#include "libunwind.h"



void InitTrace(void)
{
    if(isInit==0)
    {
        memset(thread_debug_buffer, 0, DEBUG_THREAD_SIZE);
        thread_debug_buffer->current=0;
        thread_debug_buffer->num_threads=0;
        isInit=1;
    }
}

void addToBuffer(char *item)
{
    // Write in a single step
    pthread_mutex_lock(&threadDebugLock);
    thread_debug_buffer->current = thread_debug_buffer->current +1;
    if (thread_debug_buffer->current > DEBUG_THREAD_SIZE)
    {
        thread_debug_buffer->current=1;
    }
    memcpy(thread_debug_buffer + (thread_debug_buffer->current -1)* DEBUG_THREAD_SIZE, item, DEBUG_THREAD_SIZE);
    thread_debug_buffer->num_threads=thread_debug_buffer->num_threads+1;
    if(thread_debug_buffer->num_threads>MAX_DEBUG_THREAD)
    {
    	thread_debug_buffer->num_threads=MAX_DEBUG_THREAD;
    }
    pthread_mutex_unlock(&threadDebugLock);

}

void getCurrentFromBuffer(char *item)
{
	pthread_mutex_lock(&threadDebugLock);
    if(thread_debug_buffer->current<=0)
    {
        return;
    }
    memcpy(item,thread_debug_buffer + (thread_debug_buffer->current -1)*DEBUG_THREAD_SIZE, DEBUG_THREAD_SIZE);
    pthread_mutex_unlock(&threadDebugLock);
}

void getFromBuffer(int num,char *item)
{
    pthread_mutex_lock(&threadDebugLock);
    if(num<=0 || num > MAX_DEBUG_THREAD)
    {
        return;
    }
    memcpy(item,thread_debug_buffer + (num -1)*DEBUG_THREAD_SIZE, DEBUG_THREAD_SIZE);
    pthread_mutex_unlock(&threadDebugLock);
}

//void stack_backtrace(char* trace_buf)
//{
//    pid_t pid = getpid();
//    unsigned int *callstack[MAXFRAME];
//    const int nMaxFrames = sizeof(callstack) / sizeof(callstack[0]);
//    int nFrames = backtrace((void**)callstack, nMaxFrames);
//    if (nFrames == 0)
//    {
//        return;
//    }
//    char **symbols = backtrace_symbols((void**)callstack, nFrames);
//    size_t funcnamesize = 150;
//    char* funcname = (char*)malloc(funcnamesize);
//    int lenght=0;
//    for (int i = 2; i < nFrames; i++)
//    {
//        printf("%s\n", symbols[i]);
//        char *begin_name = 0, *begin_offset = 0, *end_offset = 0;
//        for (char *p = symbols[i]; *p; ++p)
//        {
//            if (*p == '(')
//            begin_name = p;
//            else if (*p == '+')
//            begin_offset = p;
//            else if (*p == ')' && begin_offset)
//              {
//                end_offset = p;
//                break;
//              }
//        }
//        if (begin_name && begin_offset && end_offset && begin_name < begin_offset)
//        {
//            *begin_name++ = '\0';
//            *begin_offset++ = '\0';
//            *end_offset = '\0';
//            int status;
//            char* ret = __cxa_demangle(begin_name,funcname, &funcnamesize, &status);
//            if (status == 0)
//            {
//                funcname = ret; // use possibly realloc()-ed string
//                lenght += snprintf(trace_buf + lenght,MAX_FRAME_SIZE,"(PID:%d) %s : %s+%s\n",pid, symbols[i], funcname, begin_offset);
//            }
//            else
//            {
//                //demangling failed. Output function name as a C function with
//                // no arguments.
//                lenght+=snprintf(trace_buf + lenght,MAX_FRAME_SIZE,"(PID:%d) %s : %s()+%s\n",pid, symbols[i], begin_name, begin_offset);
//            }
//        }
//        else
//        {
//            // couldn't parse the line? print the whole line.
//            lenght+=snprintf(trace_buf + lenght,MAX_FRAME_SIZE, "(PID:%d) %s: ??\n", pid, symbols[i]);
//        }
//    }
//    free(funcname);
//    free(symbols);
//}


void getFileAndLine (unw_word_t addr, char *file, size_t flen, int *line)
{
    static char buf[256];
    sprintf (buf, "/usr/bin/addr2line -C -e ./a.out -f -i %lx", addr);
    FILE* f = popen (buf, "r");
    if (f == NULL)
    {
        perror(buf);
        return;
    }
    // get function name
    fgets(buf, 256, f);
    // get file and line
    fgets(buf, 256, f);
    if (buf[0] != '?')
    {
        char *p = buf;
        // file name is until ':'
        while (*p != ':')
        {
            p++;
        }
        *p++ = 0;
        // after file name follows line number
        strcpy (file , buf);
        sscanf (p,"%d", line);
    }
    else
    {
        strcpy (file,"unkown");
        *line = 0;
    }
    pclose(f);
}

void get_backtrace (char* trace_buf)
{
    char name[150];
    pid_t pid = getpid();
    unw_cursor_t cursor; unw_context_t uc;
    unw_word_t ip, sp, offp;
    unw_getcontext(&uc);
    unw_init_local(&cursor, &uc);
    int lenght=0;
    while (unw_step(&cursor) > 0)
    {
        char file[256];
        int line = 0;
        name[0] = '\0';
        unw_get_proc_name(&cursor, name, 256, &offp);
        unw_get_reg(&cursor, UNW_REG_IP, &ip);
        unw_get_reg(&cursor, UNW_REG_SP, &sp);
        getFileAndLine((long)ip, file, 256, &line);
        lenght+=snprintf("PID %d, name %s in file %s line %d\n",pid, name, file, line);
        printf("trang khung");
	}
}

void getStackTrace()
{
    char stackInfo[256] ;
    char *pstackInfo= stackInfo;
    get_backtrace(pstackInfo);
    addToBuffer(pstackInfo);
}
