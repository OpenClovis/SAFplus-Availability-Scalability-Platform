#include "stackTrace.h"
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include "libunwind.h"

pthread_mutex_t threadDebugLock;
static struct ThreadDebugBuffer thread_debug_buffer;
static int isInit = 0;
static char libPath[3][MAX_FUNC_NAME];
long memBegin[3];


void writeToFile(char* fileName,char* data)
{
	FILE *f = fopen(fileName, "a+");
	if (f == NULL)
	{
        return;
	}
	fprintf(f, "Thread Trace : \n%s\n", data);
	fclose(f);
}

long getBeginMem(pid_t pid, char* libname, char* libPath)
{
    char fname[PATH_MAX];
    FILE *f;

    sprintf(fname, "/proc/%ld/maps", (long)pid);
    f = fopen(fname, "r");

    if(!f)
    return -1;

    while(!feof(f))
    {

	    char buf[PATH_MAX+100], perm[5], dev[6];
	    unsigned long begin, end, inode, foo;
	    if(fgets(buf, sizeof(buf), f) == 0)
	    break;
	    sscanf(buf, "%lx-%lx %4s %lx %5s %ld %s", &begin, &end, perm, &foo, dev, &inode, libPath);
	    if(strstr(libPath, libname) != NULL && strstr(perm, "r-xp") != NULL)
	    {
	        clLog(CL_LOG_SEV_TRACE,"DEB","STA","lib begin mem : %lx lib path : %s\n", begin,libPath);
            fclose(f);
            return begin;
	    }
    }
    fclose(f);
    return -1;
}
void InitThreadDebugBuffer(char* progname)
{
    clLog(CL_LOG_SEV_INFO,"DEB","STA"," InitThreadDebugBuffer \n");
    if(isInit==0)
    {
        memset(thread_debug_buffer.buffer, 0, MAX_DEBUG_THREAD*DEBUG_THREAD_SIZE);
        thread_debug_buffer.current=0;
        thread_debug_buffer.num_threads=0;
        char* libname[3];   
        libname[2]=progname;
        libname[1]="libmw";
        libname[0]="libpthread";
        int i=0;
        pid_t pid = getpid();
        for(i=0;i<3;i++)
        {
            libPath[i][0] = '\0';
            char* p =libPath[i];
            memBegin[i]=getBeginMem(pid,libname[i],p);
        }
        isInit++;
    }
}

void ThreadDebugBufferWrite(char *item)
{
    // Write in a single step
    pthread_mutex_lock(&threadDebugLock);
    thread_debug_buffer.current = thread_debug_buffer.current +1;
    if (thread_debug_buffer.current > MAX_DEBUG_THREAD)
    {
        thread_debug_buffer.current=1;
    }
    int index = (thread_debug_buffer.current -1)* DEBUG_THREAD_SIZE;
    char * temp = thread_debug_buffer.buffer;
    memcpy(temp + index, item,DEBUG_THREAD_SIZE);
    thread_debug_buffer.num_threads=thread_debug_buffer.num_threads+1;
    if(thread_debug_buffer.num_threads>MAX_DEBUG_THREAD)
    {
    	thread_debug_buffer.num_threads=MAX_DEBUG_THREAD;
    }
    pthread_mutex_unlock(&threadDebugLock);
}

void ThreadDebugBufferRead(char *item)
{
	pthread_mutex_lock(&threadDebugLock);
	clLog(CL_LOG_SEV_TRACE,"DEB","STA"," Get current item from debug buffer : %d\n",thread_debug_buffer.current);
    if(thread_debug_buffer.current<=0)
    {
        pthread_mutex_unlock(&threadDebugLock);
        return;
    }
    char* temp= thread_debug_buffer.buffer;
    memcpy(item, temp + (thread_debug_buffer.current -1)*DEBUG_THREAD_SIZE, DEBUG_THREAD_SIZE);
    pthread_mutex_unlock(&threadDebugLock);
}

void ThreadDebugBufferGet(int num,char *item)
{
    pthread_mutex_lock(&threadDebugLock);
    if(num<=0 || num > MAX_DEBUG_THREAD)
    {
        pthread_mutex_unlock(&threadDebugLock);
        return;
    }
    char* temp= thread_debug_buffer.buffer;
    memcpy(item, temp + (num -1)*DEBUG_THREAD_SIZE, DEBUG_THREAD_SIZE);
    pthread_mutex_unlock(&threadDebugLock);
}

void writeThreadDebugBufferToFile(char* fileName)
{
	for(int i=1;i<=thread_debug_buffer.num_threads;i++)
	{
	    char stackInfo[DEBUG_THREAD_SIZE] ;
	    char *pstackInfo= stackInfo;
	    ThreadDebugBufferGet(i,pstackInfo);
	    writeToFile(fileName,pstackInfo);
	}
}




void getFileAndLine (unw_word_t addr, char *file, size_t flen, int *line,pid_t pid)
{
    static char buf[MAX_FUNC_NAME];
    int i=0;
    for(i=0;i<3;i++)
    {
        if(memBegin[i] !=-1 && i!=2)
        {
             sprintf (buf, "/usr/bin/addr2line -e %s -f -i %lx",libPath[i],(long)addr - memBegin[i]);
        }
        else
        {
             sprintf (buf, "/usr/bin/addr2line -C -e %s -f -i %lx",libPath[i],addr);
        }
        FILE* f = popen (buf, "r");
        if (f == NULL)
        {
            perror(buf);
            return;
        }
        // get function name
        fgets(buf, MAX_FUNC_NAME, f);
        // get file and line
        fgets(buf, MAX_FUNC_NAME, f);
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
            pclose(f);
            return;
        }
        else
        {
            strcpy (file,"unkown");
            *line = 0;
        }
        pclose(f);
    }
}




void get_backtraceDetail(char* trace_buf)
{
    char name[MAX_FUNC_NAME];
    pid_t pid = getpid();
    unw_cursor_t cursor; unw_context_t uc;
    unw_word_t ip, sp, offp;
    unw_getcontext(&uc);
    unw_init_local(&cursor, &uc);
    int lenght=0;
    while (unw_step(&cursor) > 0)
    {
        char file[MAX_FUNC_NAME];
        int line = 0;
        name[0] = '\0';
        pid_t tid = syscall(__NR_gettid);
        unw_get_proc_name(&cursor, name, MAX_FUNC_NAME, &offp);
        unw_get_reg(&cursor, UNW_REG_IP, &ip);
        unw_get_reg(&cursor, UNW_REG_SP, &sp);
        getFileAndLine((long)ip, file, MAX_FUNC_NAME, &line,pid);
        if(ip > 0)
        {
            lenght+=snprintf(trace_buf + lenght ,DEBUG_THREAD_SIZE - lenght,"TID %d, Address: %lx, name %s in file %s:%d\n",tid,ip,name,file,line);
            clLog(CL_LOG_SEV_TRACE,"DEB","STA","lenght : %d\n", lenght);
            clLog(CL_LOG_SEV_TRACE,"DEB","STA","stack frame : TID %d, Address: %lx, name %s in file %s:%d\n",tid,ip,name,file,line);
        }
    }
}

void get_backtrace(char* trace_buf)
{
    char name[MAX_FUNC_NAME];
    pid_t pid = getpid();
    unw_cursor_t cursor; unw_context_t uc;
    unw_word_t ip, sp, offp;
    unw_getcontext(&uc);
    unw_init_local(&cursor, &uc);
    int lenght=0;
    while (unw_step(&cursor) > 0)
    {
        char file[MAX_FUNC_NAME];
        int line = 0;
        name[0] = '\0';
        unw_get_proc_name(&cursor, name, MAX_FUNC_NAME, &offp);
        unw_get_reg(&cursor, UNW_REG_IP, &ip);
        unw_get_reg(&cursor, UNW_REG_SP, &sp);
        pid_t tid = syscall(__NR_gettid);
        if(ip > 0)
        {
            lenght+=snprintf(trace_buf + lenght ,DEBUG_THREAD_SIZE - lenght,"TID %d, Address: %lx, name %s \n",tid,ip,name);
            clLog(CL_LOG_SEV_TRACE,"DEB","STA","lenght : %d\n", lenght);
            clLog(CL_LOG_SEV_TRACE,"DEB","STA","stack frame : PID %d, Address: %lx, name %s",pid,ip,name);
        }
    }
}

void getCurrentThreadStack()
{
    char stackInfo[DEBUG_THREAD_SIZE] ;
    memset(stackInfo,0,DEBUG_THREAD_SIZE);
    char *pstackInfo= stackInfo;
    get_backtrace(pstackInfo);
    ThreadDebugBufferWrite(stackInfo);
}
void getCurrentThreadStackDetail()
{
    char stackInfo[DEBUG_THREAD_SIZE] ;
    memset(stackInfo,0,DEBUG_THREAD_SIZE);
    char *pstackInfo= stackInfo;
    get_backtraceDetail(pstackInfo);
    ThreadDebugBufferWrite(stackInfo);
}
