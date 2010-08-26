/*
 * Copyright (C) 2002-2009 by OpenClovis Inc. All  Rights Reserved.
 * 
 * The source code for  this program is not published  or otherwise 
 * divested of  its trade secrets, irrespective  of  what  has been 
 * deposited with the U.S. Copyright office.
 * 
 * This program is  free software; you can redistribute it and / or
 * modify  it under  the  terms  of  the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 * 
 * This program is distributed in the  hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied  warranty  of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 * 
 * You  should  have  received  a  copy of  the  GNU General Public
 * License along  with  this program. If  not,  write  to  the 
 * Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/*
 * Build: 4.2.0
 */
/*
  The memTracker module for memory tracking.
  Does mem logging of backtrace during allocs and frees.
  Cant use container hashing for cyclic deps.
  (container uses malloc and we would reenter incase of
  memtracking.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <sys/time.h>
#include <pthread.h>

#include <clCommon.h>
#include <clCommonErrors.h>
#include <clOsalApi.h>
#include <clDebugApi.h>
#include <clEoApi.h>
#include <clMemTracker.h>
#include "clHash.h"

#define HASH_TABLE_BITS (10)

#define HASH_TABLE_SIZE ( 1 << HASH_TABLE_BITS)

#define HASH_ADD(key,entry) do { ClRcT ret = hashAdd(gClMemTrackerData.hashTable,key,entry); CL_ASSERT(ret == CL_OK); } while(0)

#define HASH_DEL(entry)     do { ClRcT ret = hashDel(entry); CL_ASSERT(ret == CL_OK); } while(0)

#define CL_MEM_TRACKER_LOG_DIRECTORY "mem_logs"

#define CL_MEM_TRACKER_LOG_FILE   gClMemTrackerData.logFile

#define CL_MEM_TRACKER_PNAME     gClMemTrackerData.pName

/*We store traces*/
#define BT_DEPTH (10)

#define CL_MEM_TRACKER_LOCK_INIT() do {                         \
    ClRcT ret = clOsalMutexInit(&gClMemTrackerData.mutex);       \
    CL_ASSERT(ret == CL_OK);                                     \
}while(0)

#define CL_MEM_TRACKER_LOCK() do { \
    ClRcT ret = clOsalMutexLock((ClOsalMutexIdT)&gClMemTrackerData.mutex);\
    CL_ASSERT(ret == CL_OK);\
}while(0)

#define CL_MEM_TRACKER_UNLOCK() do { \
    ClRcT ret = clOsalMutexUnlock((ClOsalMutexIdT)&gClMemTrackerData.mutex);\
    CL_ASSERT(ret == CL_OK); \
}while(0)

#define CL_MEM_TRACKER_INITIALIZED_CHECK(label) do { \
    if(gClMemTrackerData.initialized == CL_FALSE) \
    { \
        CL_OUTPUT("MemTracker system isnt initialized for debugging\n");\
        goto label;\
    }\
}while(0)

#define GET_BT(bt) ( strrchr((bt),'/') ? strrchr((bt),'/')+1:(bt))

/*
 * 100 secs. as the forward time drift.
 */
#define CL_MEM_TRACKER_TIME (gClMemTrackerTimeDrift)

#define CL_MEM_TRACKER_TIME_DEFAULT (100)

#define CL_MEM_TRACKER_DRIFT_THRESHOLD (60*60*24)

#define CL_MEM_TRACKER_REC_SIZE (80+40*(BT_DEPTH-1))
/*
 * Have 4k entries in the fast free offset stack.
 */
#define CL_MEM_TRACKER_OFFSET_CACHE_ENTRIES (1<<12)

#define CL_MEM_TRACKER_OFFSET_INVALID ((ClOffsetT)-1)

typedef struct ClMemTrackerOffsetCache
{
    ClInt32T offsetCacheIndex;
    ClOffsetT offsetCache[CL_MEM_TRACKER_OFFSET_CACHE_ENTRIES];
}ClMemTrackerOffsetCacheT;

typedef struct ClMemTracker
{
#define MAX_BUFFER_SIZE 0x100
    void *pAddress;/*allocation*/
    void *pPrivate;/*private data which could be deciphered if need be*/
    ClUint32T size;/*allocation size*/
    ClCharT id[MAX_BUFFER_SIZE];
    /*backtrace info*/
    void *ppBT[BT_DEPTH];
    ClUint32T btSize;
    ClOffsetT offset;
    struct timeval start;
    /*hash linkage*/
    struct hashStruct hash;
} ClMemTrackerT;

typedef struct ClMemTrackerDataT
{
    ClBoolT initialized;
    struct hashStruct *hashTable[HASH_TABLE_SIZE];
    ClOsalMutexT mutex;
    ClCharT pName[MAX_BUFFER_SIZE];/*process name undergoing tracking*/
    ClCharT logFile[MAX_BUFFER_SIZE];
    void *ppLastBT[BT_DEPTH];
    ClUint32T lastSize;
} ClMemTrackerDataT;

#ifndef __cplusplus
static ClMemTrackerDataT gClMemTrackerData = {
    .initialized = CL_FALSE,
};
#else
static ClMemTrackerDataT gClMemTrackerData = {0};
#endif

static ClMemTrackerOffsetCacheT gClMemTrackerOffsetCache;

static ClUint32T gClMemTrackerTimeDrift = CL_MEM_TRACKER_TIME_DEFAULT;

#if __WORDSIZE == 64

static __inline__ ClUint32T hashGenerator(void *pVal)
{
    ClUint64T hash = (ClUint64T)pVal;
    /*  Sigh, gcc can't optimise this alone like it does for 32 bits. */
    ClUint64T n = hash;
    n <<= 18;
    hash -= n;
    n <<= 33;
    hash -= n;
    n <<= 3;
    hash += n;
    n <<= 3;
    hash -= n;
    n <<= 4;
    hash += n;
    n <<= 2;
    hash += n;
    /* High bits are more random, so use them. */
    return (ClUint32T)(hash >> (64 - HASH_TABLE_BITS));
}

#else

static __inline__ ClUint32T hashGenerator(void *pVal) { 

#define GOLDEN_RATIO_PRIME (0x9e370001UL)

    ClUint32T hash = (ClUint32T)pVal;
    hash *= GOLDEN_RATIO_PRIME;
    return (ClUint32T) ( hash >> (32 - HASH_TABLE_BITS));
}

#endif

static __inline__ ClRcT clMemTrackerOffsetCacheAdd(ClOffsetT offset)
{
    ClRcT rc = CL_ERR_NO_SPACE;
    if(gClMemTrackerOffsetCache.offsetCacheIndex >= CL_MEM_TRACKER_OFFSET_CACHE_ENTRIES)
    {
        CL_DEBUG_PRINT(CL_DEBUG_WARN,("Offset cache index exceeds limit:%d\n",(ClUint32T)CL_MEM_TRACKER_OFFSET_CACHE_ENTRIES));
        goto out;
    }
    gClMemTrackerOffsetCache.offsetCache[gClMemTrackerOffsetCache.offsetCacheIndex++] = offset;
    rc = CL_OK;
    out:
    return rc;
}

/*
 * Pop from the mem tracker offset cache.
 */
static __inline__ ClRcT clMemTrackerOffsetCacheGet(ClOffsetT *pOffset)
{
    ClRcT rc = CL_ERR_OP_NOT_PERMITTED;
    ClOffsetT offset;
    if(gClMemTrackerOffsetCache.offsetCacheIndex == 0 )
    {
        goto out;
    }
    rc = CL_ERR_INVALID_PARAMETER;
    if(pOffset == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Invalid param\n"));
        goto out;
    }
    offset = gClMemTrackerOffsetCache.offsetCache[--gClMemTrackerOffsetCache.offsetCacheIndex];
    *pOffset = offset;
    rc = CL_OK;
    out:
    return rc;
}


static ClUint64T clTimerTimeDiff(struct timeval *pStart,struct timeval *pEnd,ClUint32T drift)
{
    ClUint64T diff;
    pEnd->tv_sec -= pStart->tv_sec;
    pEnd->tv_usec -= pStart->tv_usec;
    if(pEnd->tv_sec <= 0 )
    {
        if(pEnd->tv_sec < 0 )
        {
            return 0ULL;
        }
        if(pEnd->tv_sec == 0 && pEnd->tv_usec < 0 )
        {
            return 0ULL;
        }
    }
    if(pEnd->tv_usec < 0 )
    {
        pEnd->tv_usec += 1000000L;
        --pEnd->tv_sec;
    }

    if(drift && pEnd->tv_sec > (time_t) drift)
    {
        return 0ULL;
    }
    
    diff = pEnd->tv_sec * 1000000L + pEnd->tv_usec;
    return diff;
}


/*Recurse and make directories: should be provided by osal*/
static ClRcT clMakeDir(const ClCharT *pDir,ClInt32T mode)
{
    register ClCharT *p = (ClCharT *)pDir;
    ClCharT *s =  (ClCharT *)pDir;
    ClInt32T lastError = 0;
    ClCharT dirName[MAX_BUFFER_SIZE];
    ClRcT rc = CL_ERR_INVALID_PARAMETER;
    if(pDir == NULL)
    {
        CL_OUTPUT("Invalid param\n");
        goto out;
    }
    while( ( p = strchr(s,'/') ) != NULL)
    {
        ClInt32T len =  p - pDir + 1;
        if(len >= (int) sizeof(dirName))
        {
            len = sizeof(dirName)-1;
            /*no point in continuing after this stage*/
            p = (ClCharT*)pDir + strlen(pDir) - 1;
        }
        strncpy(dirName,pDir,len);
        dirName[len]=0;
        if ( (lastError = mkdir(dirName,mode) ) < 0 )
        {
            s = p+1;
            continue;
        }
        /*creation success.move ahead*/
        while(*p == '/') ++p;
        if(!*p) 
        {
            rc = CL_OK;
            goto out;
        }
        s = p;
    }
    if(*s)
    {
        /*create the whole dir.*/
        lastError = mkdir(pDir,mode);
    }
    if(!lastError || lastError == EEXIST)
    {
        rc= CL_OK;
    }
    else
    {
        rc = CL_ERR_UNSPECIFIED;
    }
    out:
    return rc;

}

static ClRcT clMemTrackerProcessNameGet(ClCharT *pProcessBuf,ClUint32T maxSize)
{
    ClRcT rc = CL_ERR_INVALID_PARAMETER;
    ClCharT name[256] ;
    ClCharT *pName = name;
    
    if(pProcessBuf == NULL || maxSize == 0)
    {
        CL_OUTPUT("Invalid param\n");
        goto out;
    }
    if(clEoProgNameGet(name,sizeof(name)) != CL_OK)
    {
        snprintf(pProcessBuf,maxSize,"%d",(int)getpid());
    }
    else
    {
        ClCharT *tempName ;
        tempName = strrchr(name,'/');
        if(tempName != NULL)
        {
            pName = tempName + 1;
        }
        snprintf(pProcessBuf,maxSize,"%s",pName);
    }
    rc = CL_OK;
    out:
    return rc;
}

static ClRcT clMemTrackerFileNameGet(ClCharT *pFileName,ClUint32T maxSize)
{
    ClCharT *pBase ;
    ClCharT dirBuf[MAX_BUFFER_SIZE] = {0};
    ClCharT processBuf[MAX_BUFFER_SIZE] = {0};
    ClRcT rc ;

    rc = CL_ERR_INVALID_PARAMETER;
    if(pFileName == NULL || maxSize <= 0)
    {
        CL_OUTPUT("Invalid param\n");
        goto out;
    }

    pBase = getenv("ASP_LOGDIR");
    if(pBase)
    {
        
        strncpy(dirBuf,pBase,sizeof(dirBuf));
    } 
    else
    {
        pBase = ".";
    }
    snprintf(dirBuf,sizeof(dirBuf),"%s/%s/",pBase,CL_MEM_TRACKER_LOG_DIRECTORY);

    rc = CL_ERR_UNSPECIFIED;

    if(access(dirBuf,R_OK | W_OK | X_OK) < 0 )
    {
        if(errno == ENOENT)
        {
            if((rc = clMakeDir(dirBuf,0755)) != CL_OK )
            {
                CL_OUTPUT("Error making directory:%s\n",dirBuf);
                goto out;
            }
        }
        else 
        {
            CL_OUTPUT("Error accessing directory:%s\n",dirBuf);
            goto out;
        }
    }

    if((rc = clMemTrackerProcessNameGet(processBuf,sizeof(processBuf))) != CL_OK)
    {
        CL_OUTPUT("Error getting process name\n");
        goto out;
    }
    /*copy the process name*/
    strncpy(CL_MEM_TRACKER_PNAME,processBuf,sizeof(CL_MEM_TRACKER_PNAME)-1);
    snprintf(pFileName,maxSize,"%s%s.log",dirBuf,processBuf);
    out:
    return rc;
}
/*  
  Dump the BT to console or file by parsing the contents of
  addr2line on the fly if its to be dumped on the console.
*/

static void clMemTrackerDumpBTConsole
(ClMemTrackerT *pMemTracker,void **ppBT,ClUint32T btSize,ClBoolT allocFlag)
{
    ClInt32T fds[2];
    ClInt32T err ;

    if(!ppBT || !btSize)
    {
        goto out;
    }
    if( (err = pipe(fds) ) < 0 )
    {
        CL_OUTPUT("Error in pipe\n");
        goto out;
    }
    switch ( (err = fork() ) ) 
    {
    case -1:
        CL_OUTPUT("Fork error:%s\n",strerror(errno));
        goto out;
    case 0:
        {
            /*child*/
            register ClUint32T i;
            FILE *pPtr = NULL;
            ClCharT buf[MAX_BUFFER_SIZE];
            ClCharT binary[MAX_BUFFER_SIZE];
            ClCharT *pEnv = getenv("ASP_BINDIR");
            if(!pEnv) 
            {
                CL_OUTPUT("Skipping dumpstack as ASP_BINDIR is not exported...\n");
                goto out;
            }
            snprintf(binary,sizeof(binary),"%s/%s",pEnv,CL_MEM_TRACKER_PNAME);
            /*Now popen addr2line and fire it up*/
            snprintf(buf,sizeof(buf),"addr2line -e %s",binary);
            close(fds[0]);
            dup2(fds[1],1);
            pPtr = popen(buf,"w");
            if(pPtr == NULL)
            {
                CL_OUTPUT("Error in popen of %s\n",buf);
                goto out_exit;
            }
            /*output to the std. input of addr2line*/
            for(i = 0; i < btSize; ++i)
            {
                fprintf(pPtr,"%p\n",ppBT[i]);
            }
            pclose(pPtr);
            out_exit:
            exit(0);
        }
        break;
    default:
        {
            /*
              On the parent, read it through the pipe
              and filter the ?? outputs of addr2line
            */
            FILE *pPtr = NULL;
            ClCharT buf[MAX_BUFFER_SIZE];
            ClBoolT flag = CL_FALSE;
            close(fds[1]);
            pPtr = fdopen(fds[0],"r");
            if(pPtr == NULL)
            {
                CL_OUTPUT("Error in fdopen for read end of pipe\n");
                goto out;
            }
            while(fgets(buf,sizeof(buf),pPtr))
            {
                ClInt32T bytes = strlen(buf);
                ClCharT *pStr ;
                if(buf[bytes-1] == '\n')
                    buf[bytes-1] = 0;
                if(strstr(buf,"??"))
                {
                    continue;
                }
                if(flag == CL_TRUE)
                {
                    CL_OUTPUT("  ");
                }
                if(flag == CL_FALSE)
                {
                    flag = CL_TRUE;
                }
                if ( (pStr = strrchr(buf,'/')) )
                    ++pStr;
                else
                    pStr = buf;

                CL_OUTPUT("%s - %s Chunk-%p\t%c%d\n",pMemTracker->id,pStr,
                        pMemTracker->pAddress,
                        allocFlag == CL_TRUE ? '+' : '-',pMemTracker->size
                        );
            }
            CL_OUTPUT("\n");
            fclose(pPtr);
        }
        break;
    }
    out:
    return ;
}

static void clMemTrackerDumpBT(ClMemTrackerT *pMemTracker,void **ppBT,ClUint32T btSize,ClBoolT allocFlag,FILE *pFile,ClUint64T timeDiff)
{
    register ClUint32T i;
    ClCharT buf[CL_MEM_TRACKER_REC_SIZE] = { 0 };
    ClUint32T bytes;
    ClBoolT resetOffset = CL_FALSE;
    ClOffsetT offset = 0;
    ClBoolT log = CL_TRUE;
    
    if(!ppBT || !btSize)
    {
        goto out;
    }

    if(!pFile) 
    {
        clMemTrackerDumpBTConsole(pMemTracker,ppBT,btSize,allocFlag);
        goto out;
    }
    
    if(pMemTracker->offset != CL_MEM_TRACKER_OFFSET_INVALID && BT_DEPTH >= 10)
    {
        if(pMemTracker->size == gClMemTrackerData.lastSize)
        {
            for(i = 0; i < BT_DEPTH;++i)
            {
                if(gClMemTrackerData.ppLastBT[i] != ppBT[i])
                {
                    break;
                }
            }
            if(i == BT_DEPTH)
            {
                if(allocFlag == CL_TRUE)
                {
                    goto out;
                }
                log = CL_FALSE;
                goto out_free_check;
            }
        }
        for(i = 0; i < BT_DEPTH;++i)
        {
            gClMemTrackerData.ppLastBT[i] = ppBT[i];
        }
        gClMemTrackerData.lastSize = pMemTracker->size;
    }

    /*
     * Reclaim the space that was allotted to this record.
     */
    out_free_check:
    if(allocFlag == CL_FALSE 
       && 
       CL_MEM_TRACKER_TIME > timeDiff/1000000L
       &&
       pMemTracker->offset != CL_MEM_TRACKER_OFFSET_INVALID
       )
    {
        if(fseek(pFile,pMemTracker->offset,SEEK_SET) != 0 )
        {
            goto out_add;
        }

        /*Zero off the record*/
        bytes = fwrite(buf,1,sizeof(buf),pFile);

        /*
         * If the below fails, just ignore as the hole wouldnt be reclaimed
         * anyway.
         */
        clMemTrackerOffsetCacheAdd(pMemTracker->offset);

        fseek(pFile,0,SEEK_END);

        goto out;
    }

    out_add:

    if(log == CL_FALSE)
    {
        goto out;
    }

    /*
      Output the contents into the file to run addr2line later
    */
    bytes = snprintf(buf,sizeof(buf),
                     "%p\tChunk-%p\t%c%d -%s START",
                     ppBT[0],pMemTracker->pAddress,
                     allocFlag == CL_TRUE ? '+' : '-',pMemTracker->size,
                     pMemTracker->id);

    if(allocFlag == CL_FALSE && timeDiff)
    {
        bytes += snprintf(buf+bytes,sizeof(buf)-bytes,"(%lld.%lld secs.usecs)",timeDiff/1000000LL,timeDiff%1000000LL);
    }

    bytes += snprintf(buf+bytes,sizeof(buf)-bytes,"\n");

    for(i = 1 ; i < btSize && bytes < CL_MEM_TRACKER_REC_SIZE; ++i)
    {
        bytes += snprintf(buf+bytes,sizeof(buf)-bytes,
                          "\t%p\n",ppBT[i]
                          );
    }
    
    /*
     * Write to the end if its a free log thats not recycled
     * owing to the time drift.
     */
    
    if(allocFlag == CL_TRUE 
       &&
       clMemTrackerOffsetCacheGet(&offset) == CL_OK)
    {
        if(fseek(pFile,offset,SEEK_SET) == 0 )
        {
            resetOffset = CL_TRUE;
        }
        else
        {
            goto out_default;
        }
    }
    else
    {
        out_default:
        offset = ftell(pFile);
    }

    pMemTracker->offset = offset;

    bytes = fwrite(buf,1,sizeof(buf),pFile);

    if(resetOffset == CL_TRUE)
    {
      int ret = fseek(pFile,0,SEEK_END);
      CL_ASSERT(ret == 0);
    }

    fflush(pFile);

    out:

    return;
}

static ClMemTrackerT *clMemTrackerFind(void *pAddress)
{
    ClUint32T key;
    register struct hashStruct *pTemp;
 
    key = hashGenerator(pAddress);

    for(pTemp = gClMemTrackerData.hashTable[key] ; pTemp ; pTemp = pTemp->pNext)
    {
        ClMemTrackerT *pMemTracker;
        pMemTracker = hashEntry(pTemp,ClMemTrackerT,hash);
        if(pMemTracker->pAddress == pAddress)
        {
            return pMemTracker;
        }
    }
    return NULL;
}

static void clMemTrackerLog(ClMemTrackerT *pMemTracker,void **ppBT,ClUint32T btSize,ClBoolT alloc)
{
    static FILE *pFile;
    ClUint64T timeDiff = 0;
    if(!ppBT)
    {
        goto out;
    }

    if(!pFile  && !(pFile = fopen(CL_MEM_TRACKER_LOG_FILE,"w")))
    {
        CL_OUTPUT("Error opening file:%s for writing\n",CL_MEM_TRACKER_LOG_FILE);
        goto out;
    }

    if(alloc == CL_FALSE)
    {
        if(pMemTracker->offset != CL_MEM_TRACKER_OFFSET_INVALID)
        {
            struct timeval end;

            gettimeofday(&end,NULL);
            /*
             * Ignore drifts greater than a day.
             */
            timeDiff = clTimerTimeDiff(&pMemTracker->start,&end,CL_MEM_TRACKER_DRIFT_THRESHOLD);
        }
    }
    clMemTrackerDumpBT(pMemTracker,ppBT,btSize,alloc,pFile,timeDiff);
    if(alloc == CL_TRUE)
    {
        gettimeofday(&pMemTracker->start,NULL);
    }
    out:
    return ;
}

static __inline__ void clMemTrackerBTGet(ClMemTrackerT* pMemTracker)
{
    memset(pMemTracker->ppBT,0,sizeof(pMemTracker->ppBT));
    pMemTracker->btSize = backtrace(pMemTracker->ppBT,
                                    sizeof(pMemTracker->ppBT)/
                                    sizeof(pMemTracker->ppBT[0])
                                    );
}

void clMemTrackerAdd
        (
        const char *id,void *pAddress,ClUint32T size,void *pPrivate,
        ClBoolT logFlag
        )
                            
{
    ClMemTrackerT *pMemTracker ;
    ClUint32T key;

    CL_MEM_TRACKER_INITIALIZED_CHECK(out);

    pMemTracker = (ClMemTrackerT*) malloc(sizeof(*pMemTracker));
    if(pMemTracker == NULL)
    {
        CL_OUTPUT("%s:Error allocating memory\n",id);
        goto out;
    }
    memset(pMemTracker,0x0,sizeof(*pMemTracker));
    strncpy(pMemTracker->id,id,sizeof(pMemTracker->id)-1);
    pMemTracker->pAddress = pAddress;
    pMemTracker->size = size;
    pMemTracker->pPrivate = pPrivate;
    /*
      Get the allocation backtrace
    */
    clMemTrackerBTGet(pMemTracker);

    key = hashGenerator(pMemTracker->pAddress);

    /*Add this to the hash list*/
    CL_MEM_TRACKER_LOCK();

    HASH_ADD(key,&pMemTracker->hash);

    /*Log this request*/
    if(logFlag == CL_TRUE)
    {
        clMemTrackerLog(pMemTracker,pMemTracker->ppBT,pMemTracker->btSize,CL_TRUE);
    }
    CL_MEM_TRACKER_UNLOCK();
    out:
    return;
}
        
void clMemTrackerDelete
        (
        const char *id,void *pAddress,ClUint32T size,ClBoolT logFlag
        )
                            
{
    ClMemTrackerT *pMemTracker;
    ClMemTrackerT defTracker = { 0 };
    
    CL_MEM_TRACKER_INITIALIZED_CHECK(out);

    CL_MEM_TRACKER_LOCK();

    pMemTracker = clMemTrackerFind(pAddress);
    if(!pMemTracker)
    {
        pMemTracker = &defTracker;
        pMemTracker->pAddress = pAddress;
        pMemTracker->size = size;
        pMemTracker->offset = CL_MEM_TRACKER_OFFSET_INVALID;
        strncpy(pMemTracker->id,id,sizeof(pMemTracker->id)-1);
        CL_OUTPUT("%s memfree: Unable to find address:%p,size:%d\n",id,pAddress,size);
        CL_OUTPUT("Invalid/Double Free suspected for process:%d\n",(int)getpid());
        
        clMemTrackerTrace(id,NULL,size);

        goto out_unlock;
    }
    /*Unlink from the hash chain*/
    HASH_DEL(&pMemTracker->hash);

    out_unlock:
    CL_MEM_TRACKER_UNLOCK();

    /*Log the free*/
    if(logFlag == CL_TRUE)
    {
        ClUint32T btSize;
        void *ppBT[BT_DEPTH];
        memset(ppBT,0,sizeof(ppBT));
        btSize = backtrace(ppBT,sizeof(ppBT)/sizeof(ppBT[0]));
        CL_MEM_TRACKER_LOCK();
        clMemTrackerLog(pMemTracker,ppBT,btSize,CL_FALSE);
        CL_MEM_TRACKER_UNLOCK();
    }

    if(pMemTracker != &defTracker)
    {
        free(pMemTracker);
    }
    out:
    return;
}

/*
  Tracker trace that should be mostly invoked on
  mem corruption.
*/

void clMemTrackerTrace
        (
        const char *id,void *pAddress,ClUint32T size
        )

{
    ClMemTrackerT *pMemTracker;
    ClMemTrackerT defTracker;
    ClUint32T btSize;
    void *ppBT[BT_DEPTH];
    
    defTracker.size = size;
    CL_MEM_TRACKER_INITIALIZED_CHECK(out);

    /*Get the trace bt*/
    memset(ppBT,0,sizeof(ppBT));
    btSize = backtrace(ppBT,sizeof(ppBT)/sizeof(ppBT[0]));
    /*
     If you want to dump traces on allocation failures,send a NULL 
     address
    */
    if(pAddress == NULL)
    {
        pMemTracker = &defTracker;
        pMemTracker->pAddress = pAddress;
        strncpy(pMemTracker->id,id,sizeof(pMemTracker->id)-1);
        goto out_trace;
    }
    CL_MEM_TRACKER_LOCK();
    pMemTracker = clMemTrackerFind(pAddress);
    if(pMemTracker == NULL)
    {
        pMemTracker = &defTracker;
        pMemTracker->pAddress = pAddress;
        strncpy(pMemTracker->id,id,sizeof(pMemTracker->id)-1);
        CL_OUTPUT("%s MemFree: Unable to find address:%p,size:%d\n",id,pAddress,size);
        goto out_unlock;
    }

    CL_OUTPUT("\n%s - Dumping allocation backtrace of address:%p,size:%d thats %d feet deep\n",
            id,pAddress,size,pMemTracker->btSize);

    clMemTrackerDumpBT(pMemTracker,pMemTracker->ppBT,pMemTracker->btSize,
            CL_TRUE,0,0);

    CL_OUTPUT("------------%s - Allocation Backtrace End-------\n\n",id);

out_unlock:
    CL_MEM_TRACKER_UNLOCK();

out_trace:
    CL_OUTPUT("%s - Dumping trace backtrace for address:%p,size:%d\n",
            id,pAddress,size);
    clMemTrackerDumpBT(pMemTracker,ppBT,btSize,CL_TRUE,0,0);
    CL_OUTPUT("---------%s - Trace Backtrace END----------\n",id);

out:
    return;
}

/*Tracker walk*/
static ClRcT 
clMemTrackerWalk(void *pArg,ClRcT (*pWalkCallback)(void *pArg,void *pTracker))
{
    register ClUint32T i;
    ClRcT rc = CL_ERR_NULL_POINTER;
    if(pWalkCallback == NULL)
    {
        CL_OUTPUT("pWalkCallback is NULL\n");
        goto out;
    }
    rc = CL_OK;    
    CL_MEM_TRACKER_LOCK();
    for(i = 0; i < HASH_TABLE_SIZE; ++i)
    {
        register struct hashStruct *pTemp;
        for(pTemp = gClMemTrackerData.hashTable[i]; pTemp ; pTemp = pTemp->pNext)
        {
            ClMemTrackerT *pMemTracker ;
            pMemTracker = hashEntry(pTemp,ClMemTrackerT,hash);
            rc = pWalkCallback(pArg,(void*)pMemTracker);
            if(rc != CL_OK)
            {
                break;
            }

        }
    }
    CL_MEM_TRACKER_UNLOCK();
    out:
    return rc;
}

/*
  Given a corrupted chunk,detect the underrun trace.
 */
ClRcT clMemTrackerGet(void *pAddress,ClUint32T *pSize,void **ppPrivate)
{
    ClMemTrackerT *pMemTracker;
    ClRcT rc = CL_ERR_INVALID_PARAMETER;

    if(pAddress == NULL || pSize == NULL || ppPrivate == NULL)
    {
        CL_OUTPUT("Invalid param to clMemTrackerGet\n");
        goto out;
    }
    rc = CL_ERR_NOT_EXIST;
    CL_MEM_TRACKER_LOCK();
    pMemTracker = clMemTrackerFind(pAddress);
    if(pMemTracker == NULL)
    {
        CL_OUTPUT("Chunk %p not found. Should be a double free chunk\n",pAddress);
        goto out_unlock;
    }
    *pSize = pMemTracker->size;
    *ppPrivate = pMemTracker->pPrivate;
    rc = CL_OK;
    out_unlock:
    CL_MEM_TRACKER_UNLOCK();
    out:
    return rc;
}

/*Show leaks in the tracker module*/
static ClRcT clMemTrackerDisplay(void *pUnused,void *pTracker)
{
    ClMemTrackerT *pMemTracker = (ClMemTrackerT*) pTracker;
    CL_OUTPUT("\nID : %s, Mem address: %p, size: %d, private data:%p\n",
           pMemTracker->id,pMemTracker->pAddress,pMemTracker->size,pMemTracker->pPrivate
           );
    CL_OUTPUT("Displaying allocation trace thats %d feet deep\n",pMemTracker->btSize);
    clMemTrackerDumpBT(pMemTracker,pMemTracker->ppBT,pMemTracker->btSize,
            CL_TRUE,0,0);
    CL_OUTPUT("-----------Allocation Trace END----------\n");

    return CL_OK;
}

void clMemTrackerDisplayLeaks(void)
{
    CL_MEM_TRACKER_INITIALIZED_CHECK(out);
    CL_OUTPUT("\nDisplaying allocation trace thats not released...\n");
    clMemTrackerWalk(NULL,clMemTrackerDisplay);
    CL_OUTPUT("\n-----------------Trace END---------------------\n");
    out:
    return;
}

/*Initialize the tracker subsystem*/
ClRcT clMemTrackerInitialize(void)
{
    ClRcT rc = CL_OK;
    ClCharT *pDriftStr = NULL;
    if(gClMemTrackerData.initialized == CL_TRUE)
    {
        goto out;
    }
    /*get the logfile*/
    rc = clMemTrackerFileNameGet(CL_MEM_TRACKER_LOG_FILE,sizeof(CL_MEM_TRACKER_LOG_FILE));
    if(rc != CL_OK)
    {
        goto out;
    }
    unlink(CL_MEM_TRACKER_LOG_FILE);

    CL_MEM_TRACKER_LOCK_INIT();

    pDriftStr = getenv("CL_MEM_TIME");

    if(pDriftStr != NULL)
    {
        CL_MEM_TRACKER_TIME = (ClUint32T)atoi(pDriftStr);
    }

    gClMemTrackerData.initialized = CL_TRUE;
    out:
    return rc;
}

