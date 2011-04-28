/*
 * Copyright (C) 2002-2009 by OpenClovis Inc. All  Rights Reserved.
 * 
 * The source code for  this program is not published  or otherwise 
 * divested of  its trade secrets, irrespective  of  what  has been 
 * deposited with the U.S. Copyright office
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

#include <sys/types.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <signal.h>
#include <ctype.h>
#include <clDebugApi.h>
#include <clLogUtilApi.h>
#include <clHandleApi.h>
#include <clLogDbg.h>
#include <clLogApi.h>
#include <clLogApiExt.h>
#include <clEoApi.h>
#include <clEoConfigApi.h>
#include <clCpmExtApi.h>

#define  CL_LOG_DEFAULT_SERVICE_ID   10
/* Parser related macros & declarations */

#define CL_LOG_PRNT_ERR(...) do { fprintf(stderr, __VA_ARGS__); } while(0)

#define CL_LOG_PARSE_SYNTX_ERR(line)\
        CL_LOG_PRNT_ERR("Syntax error while parsing log filter file at line %d\n", line);

#define CL_LOG_FILTER_BITS            (0x5)
#define CL_LOG_FILTER_MASK            ( (1 << CL_LOG_FILTER_BITS) - 1)
#define CL_LOG_RULE_SET(rule,bit)     __RULE_SET((rule)->filterMap,bit)
#define CL_LOG_RULE_CLEAR(rule,bit)   __RULE_CLEAR((rule)->filterMap,bit)
#define CL_LOG_RULE_TEST(rule,bit)    __RULE_TEST((rule)->filterMap,bit)
#define __RULE_SET(map,bit)    ( (map) |= ( 1 << (bit) ) )
#define __RULE_CLEAR(map,bit)  ( (map) &= ~(1 << (bit) ) )
#define __RULE_TEST(map,bit)   ( (map) & ( 1 << (bit) ) )

#define LOG_COMMENT '#'

#define LOG_EAT_SPACE() do { while(isspace(*(p))) ++(p); } while(0)

#define LOG_EAT_WILDCARD(bit) do {              \
        if( pSave[0] == '*' && (p == 1+pSave))  \
        {                                       \
            CL_LOG_RULE_SET(pFilters+i,bit);           \
        }                                       \
}while(0)

#define LOG_TOKEN_ASSERT(c) do {                        \
        if(*p++ != (c))                                 \
        {                                               \
            CL_LOG_PARSE_SYNTX_ERR(line);               \
            line++; continue;                           \
        }                                               \
}while(0)

/* Fix for wrl-pnele-1.2 pthread.h issue */
#ifdef WIND_PTHREAD
    #undef PTHREAD_MUTEX_INITIALIZER
    #define PTHREAD_MUTEX_INITIALIZER { { 0, 0, 0, 0, 0, } }
#endif

static ClLogRulesT gClLogRules  = { .numFilters = 0, .pFilters = NULL };

ClHandleT         CL_LOG_HANDLE_SYS = CL_HANDLE_INVALID_VALUE;
ClHandleT         CL_LOG_HANDLE_APP = CL_HANDLE_INVALID_VALUE;
static  ClCharT  *clLogToFile      = "stdout";
static  ClBoolT  clLogStreamEnable = CL_TRUE;
static  FILE     *clDbgFp          = NULL; 
static  ClLogSeverityT   clLogDefaultSeverity = CL_LOG_SEV_DEBUG;
static  ClBoolT          clLogSeveritySet     = CL_FALSE;
static  ClBoolT          gClLogCodeLocationEnable = CL_FALSE;
static  ClBoolT          clLogTimeEnable      = CL_TRUE;
static  ClLogRulesInfoT  gpLogRulesInfo = {0};
static  ClCharT          gLogFilterFile[CL_MAX_NAME_LENGTH];
static  volatile ClInt32T gClLogParseFilter;
static  ClOsalMutexT gClRuleLock;
static  ClBoolT gClRuleLockValid = CL_FALSE;

#define  CL_LOG_CLNT_RECUR_DETECT(threadId)\
do\
{\
}while(0)
        

#define CL_LOG_RULES_FILE                 logGetLogRulesFile()

#define CL_LOG_PRNT_FMT_STR               "%-26s [%s:%d] (%.*s.%d : %s.%3s.%3s"
#define CL_LOG_PRNT_FMT_STR_CONSOLE       "%-26s [%s:%d] (%.*s.%d : %s.%3s.%3s.%05d : %6s) "

#define CL_LOG_PRNT_FMT_STR_WO_FILE         "%-26s (%.*s.%d : %s.%3s.%3s"
#define CL_LOG_PRNT_FMT_STR_WO_FILE_CONSOLE "%-26s (%.*s.%d : %s.%3s.%3s.%05d : %6s) "

static
ClCharT *logGetLogRulesFile(void)
{
    ClCharT *logFilterLoc = getenv("ASP_LOG_FILTER_DIR");

    if (logFilterLoc) 
    {
        strncpy(gLogFilterFile, logFilterLoc, sizeof(gLogFilterFile)-1);
        strncat(gLogFilterFile, "/", 1);
        strncat(gLogFilterFile, "logfilter.txt", sizeof(gLogFilterFile)-1);
    }
    else
    {
        strncpy(gLogFilterFile, "/tmp/logfilter.txt", sizeof(gLogFilterFile)-1);
    }

    return gLogFilterFile;
}

void
clLogSingleRuleCopy(ClLogRuleMemT  *pRuleMem,
                    ClCharT        *pValue)
{
    ClUint32T  length = strlen(pValue);

    if( (strlen(pValue) == 1) && (*pValue == '*') )
    {
        pRuleMem->mask = CL_LOG_NO_MASK;
    }
    else
    {
        strcpy(pRuleMem->str, pValue); 
        pRuleMem->mask   = CL_LOG_FULL_MASK;
        pRuleMem->length = length;
    }
    return ;
}

ClRcT
clLogRulesChkCallback(void  *pData)
{
   struct stat buf = {0}; 

  if( 0 != stat(CL_LOG_RULES_FILE, &buf) )
  {
      return CL_OK;
  }
  if( gpLogRulesInfo.mt_time != 0 )
  {
      if( gpLogRulesInfo.mt_time != buf.st_mtime )
      {
          gpLogRulesInfo.mt_time = buf.st_mtime;
          clLogRulesParse();
      }
  }
  gpLogRulesInfo.mt_time = buf.st_mtime;
  return CL_OK;
}

static __inline__ void clLogRulesDisplay(void)
{
    register ClUint32T i = 0;
    if(gClLogRules.pFilters == NULL) return;
    for(i = 0; i < gClLogRules.numFilters;++i)
    {
        CL_LOG_PRNT_ERR("RULE [%d],Filter Map [0x%x]\n",i+1,gClLogRules.pFilters[i].filterMap);
        CL_LOG_PRNT_ERR("[Node :%s Server :%s Area :%s Context:%s, File: %s,Sev:%d]\n",
               gClLogRules.pFilters[i].node,
               gClLogRules.pFilters[i].server,
               gClLogRules.pFilters[i].area,
               gClLogRules.pFilters[i].context,
               gClLogRules.pFilters[i].file,
               gClLogRules.pFilters[i].severity);
        CL_LOG_PRNT_ERR("[Node Match : %s, Server Match :%s, Area Match : %s, Context Match :%s]\n",
               CL_LOG_RULE_TEST(gClLogRules.pFilters+i,0) ? "NO" :"YES",
               CL_LOG_RULE_TEST(gClLogRules.pFilters+i,1) ? "NO" :"YES",
               CL_LOG_RULE_TEST(gClLogRules.pFilters+i,2) ? "NO" :"YES",
               CL_LOG_RULE_TEST(gClLogRules.pFilters+i,3) ? "NO" :"YES");
    }
}

static ClRcT clLogParse(const ClCharT *file, ClLogRulesFilterT **ppFilters, ClUint32T *pNumFilters)
{
    ClLogRulesFilterT *pFilters = NULL;
    ClUint32T         i         = 0;
    ClUint32T         line      = 1;
    FILE              *fptr     = NULL;
    ClCharT           buf[1024] = {0};
    ClCharT           *p        = NULL;

    fptr = fopen(file,"r");
    if(!fptr)
    {
        return CL_OK;
    }
    while(fgets(buf, sizeof(buf), fptr))
    {
        ClUint32T bytes  = strlen(buf);
        ClCharT   *pSave = NULL;

        if(buf[bytes-1] == '\n') buf[bytes-1] = 0;
        p = buf;
        /*skip spaces*/
        LOG_EAT_SPACE();
        if(!*p || *p == LOG_COMMENT) { ++line; continue; }
        LOG_TOKEN_ASSERT('(');
        /*allocate in batches of 4. who cares for the little pees :-)*/
        if( !(i & 3))
        {
            pFilters = realloc(pFilters, sizeof(*pFilters) * (i+4));
            CL_ASSERT(pFilters);
            memset(pFilters+i, 0, sizeof(*pFilters) * 4);
        }
        memset(pFilters+i, 0, sizeof(*pFilters));
        LOG_EAT_SPACE();
        pSave = p;
        p += strcspn(p," \t:");
        if(pSave == p)
        {
            CL_LOG_PARSE_SYNTX_ERR(line);
            ++line; continue;
        }
        strncpy(pFilters[i].node,pSave,CL_MIN(p - pSave,CL_LOG_MAX_NAME));
        LOG_EAT_WILDCARD(0);

        LOG_EAT_SPACE();
        LOG_TOKEN_ASSERT(':');

        LOG_EAT_SPACE();

        /* now eat up server,area,ctx,filename*/
        pSave = p;
        p += strcspn(p," \t.");
        if(p == pSave)
        {
            CL_LOG_PARSE_SYNTX_ERR(line);
            ++line; continue;
        }
        strncpy(pFilters[i].server, pSave, CL_MIN(p-pSave, CL_LOG_MAX_NAME));
        LOG_EAT_WILDCARD(1);
        LOG_EAT_SPACE();
        LOG_TOKEN_ASSERT('.');
        
        LOG_EAT_SPACE();

        pSave = p;
        p += strcspn(p," \t.");
        if(p == pSave)
        {
            CL_LOG_PARSE_SYNTX_ERR(line);
            ++line; continue;
        }
        strncpy(pFilters[i].area, pSave, CL_MIN(p-pSave,CL_LOG_MAX_NAME));
        LOG_EAT_WILDCARD(2);
        LOG_EAT_SPACE();
        LOG_TOKEN_ASSERT('.');
        LOG_EAT_SPACE();

        pSave = p;
        p += strcspn(p," \t)");
        if(p == pSave)
        {
            CL_LOG_PARSE_SYNTX_ERR(line);
            ++line; continue;
        }
        strncpy(pFilters[i].context, pSave, CL_MIN(p-pSave,CL_LOG_MAX_NAME));
        LOG_EAT_WILDCARD(3);
        LOG_EAT_SPACE();
        LOG_TOKEN_ASSERT(')');
        LOG_EAT_SPACE();

        LOG_TOKEN_ASSERT('[');
        LOG_EAT_SPACE();

        pSave = p;
        p += strcspn(p," \t]");
        if(p == pSave)
        {
            CL_LOG_PARSE_SYNTX_ERR(line);
            ++line; continue;
        }
        strncpy(pFilters[i].file, pSave, CL_MIN(p-pSave,CL_LOG_MAX_NAME));
        LOG_EAT_WILDCARD(4);
        LOG_EAT_SPACE();
        LOG_TOKEN_ASSERT(']');
        
        LOG_EAT_SPACE();
        LOG_TOKEN_ASSERT('=');
        LOG_EAT_SPACE();
        
        pSave = p;
        p += strcspn(p," \t");
        if(p == pSave)
        {
            CL_LOG_PARSE_SYNTX_ERR(line);
            ++line; continue;

        }
        pFilters[i].severity = clLogSeverityGet(pSave);
        ++i;
        ++line;
    }

    if(ppFilters && pNumFilters)
    {
        *ppFilters = pFilters;
        *pNumFilters = i;
    }
    else
    {
        gClLogRules.pFilters   = pFilters;
        gClLogRules.numFilters = i;
    }
    fclose(fptr);
    return CL_OK;
}

ClRcT
clLogRulesParse(void)
{
    ClLogRuleT  *pRules             = NULL;
    ClUint32T   numRules            = 0;
    ClCharT     buffer[ 0xffff + 1] = {0};
    FILE        *fp                 = NULL;
    ClCharT     *pTemp              = NULL;
    ClCharT     *tempStr            = NULL;

    if( NULL == (fp = fopen(CL_LOG_RULES_FILE, "r")) )  
    {
        return CL_OK;
    }
    while( fgets(buffer, sizeof(buffer) - 1, fp ) )
    {
       if( (buffer[0] != '#') && (!isspace(buffer[0])) )
            numRules++;
    }
   if( NULL == ( pRules = calloc(numRules, sizeof(ClLogRuleT))) )
   {
       CL_LOG_PRNT_ERR("calloc failed");
       fclose(fp);
       return CL_OK;
   }
   gpLogRulesInfo.numRules = numRules;
   if( gpLogRulesInfo.pRules != NULL )
   {
       free(gpLogRulesInfo.pRules);
       gpLogRulesInfo.pRules = NULL;
   }
   gpLogRulesInfo.pRules   = pRules;
   
   fseek(fp, SEEK_SET, 0);
   /* Parse and populate the rulesInfo */
   memset(buffer, '\0', sizeof(buffer));
   while( fgets(buffer, sizeof(buffer) -1, fp) )
   {
       if( (buffer[0] == '#') || (isspace(buffer[0])) )
       {
        memset(buffer, '\0', sizeof(buffer));
        continue;
       }
      pTemp = buffer;
      pTemp++;
      
      /* NODENAME */
      strtok_r(pTemp, ":", &tempStr);
      clLogSingleRuleCopy(&(pRules->ruleMems[0]), pTemp);
      /* SVR NAME */
      pTemp = tempStr;
      strtok_r(pTemp, ".", &tempStr);
      clLogSingleRuleCopy(&(pRules->ruleMems[1]), pTemp);
      /* AREA NAME */
      pTemp = tempStr;
      strtok_r(pTemp, ".", &tempStr);
      clLogSingleRuleCopy(&(pRules->ruleMems[2]), pTemp);
      /* CTX NAME */
      pTemp = tempStr;
      strtok_r(pTemp, ")", &tempStr);
      clLogSingleRuleCopy(&pRules->ruleMems[3], pTemp);
      /* FILE NAME */
      pTemp = tempStr;
      pTemp++; 
      strtok_r(pTemp, "]", &tempStr);      
      clLogSingleRuleCopy(&pRules->ruleMems[4], pTemp);
      /* = */
      pTemp = tempStr;
      pTemp++;
      /*Getting severity */
      pTemp[strlen(pTemp) - 1] = '\0';
      pRules->severity = clLogSeverityGet(pTemp);
      pRules++;
      memset(buffer, 0, sizeof(buffer));
   }
#if defined(CL_RULE_DEBUG)
   pRules = gpLogRulesInfo.pRules;   
   for( ClUint32T i = 0; i < numRules; i++ )
   {
       for( ClUint32T j = 0; j < 5; j++ )
       {
           CL_LOG_PRNT_ERR("%s %d %d \t", pRules->ruleMems[j].str,
                 pRules->ruleMems[j].length, pRules->ruleMems[j].mask); 
       }
       pRules++;
   }
#endif
   /*
    *  Even if we are not able to create the timer, we can live without that
    *  not checking the rc.
    */
   
   fclose(fp);

   return CL_OK;
}

ClBoolT
clLogSingleMemEvaluate(ClLogRuleMemT  *pRuleMem,
                       ClCharT        *pStr)
{
    if( pRuleMem->mask == CL_LOG_NO_MASK )
    {
        return CL_TRUE;
    }
    else
    {
        if( pRuleMem->length == strlen(pStr) )
        {
            if( !(strcmp(pRuleMem->str, pStr)) )
            {
                return CL_TRUE;
            }
        }
    }
    return CL_FALSE;
}

static __inline__ ClBoolT clLogRuleValidate(ClLogRulesFilterT  *pFilter,
                                            ClCharT            *pStoredStr,
                                            const ClCharT            *pStr,
                                            ClUint32T          bit,
                                            ClInt32T           *pMatches)
{
    if( CL_LOG_RULE_TEST(pFilter, bit) )
    {
        return CL_TRUE;
    }
    else
    {
        if( !(strcmp( pStoredStr , pStr)) )
        {
            if(pMatches)
                ++(*pMatches);
            return CL_TRUE;
        }
    }
    return CL_FALSE;
}

ClBoolT
clLogRulesTest(ClCharT         *pNodeName, 
               ClCharT         *pSvrName,
               const ClCharT         *pArea,
               const ClCharT         *pContext,
               const ClCharT         *pFileName, 
               ClLogSeverityT  severity,
               ClBoolT *pMatched)
{
    ClLogRulesT  *pRulesInfo = &gClLogRules;
    ClUint32T    cnt         = 0;
    ClBoolT      matchFlag   = CL_FALSE;
    ClInt32T filterIndex = -1;
    ClInt32T maxMatches = 0;
    ClBoolT retVal = CL_FALSE;
    ClBoolT ruleLockValid = gClRuleLockValid;

    *pMatched = CL_FALSE;
    
    if(ruleLockValid)
    {
        /*
         * Skip recursion or deadlock error returns on the mutex
         */
        if(clOsalMutexLockSilent(&gClRuleLock) != CL_OK)
            return CL_FALSE;
    }
    
    if(gClLogParseFilter)
    {
        gClLogParseFilter = 0;
        if(!ruleLockValid)
        {
            ClLogRulesFilterT *firstFilter;
            ClUint32T lastFilters;
            ClLogRulesFilterT *newFilter = NULL;
            ClUint32T numFilters = 0;
            /*
             * We only start taking the lock from the time of the first signal.
             * We do this wih the assumption that a signal trigger to reparse the ruleset is effectively
             * a debug mode where we can take a hit trying to take the lock for a filter check.
             * We do this with a penalty for a first time signal by leaking out the first filter
             * to be safe w.r.t to threads that have sneaked in for a lockless rule test.
             */
            if(clOsalMutexLockSilent(&gClRuleLock) != CL_OK)
                return CL_FALSE;
            ruleLockValid = gClRuleLockValid = CL_TRUE;
            firstFilter = gClLogRules.pFilters;
            lastFilters = gClLogRules.numFilters;
            clLogParse(CL_LOG_RULES_FILE, &newFilter, &numFilters);
            if(newFilter && numFilters)
            {
                if(numFilters < lastFilters)
                {
                    newFilter = realloc(newFilter, sizeof(*newFilter) * lastFilters);
                    CL_ASSERT(newFilter != NULL);
                    memset(newFilter + numFilters, 0, sizeof(*newFilter) * (lastFilters - numFilters));
                }
                gClLogRules.pFilters = newFilter;
                gClLogRules.numFilters = numFilters;
            }
        }
        else
        {
            /*
             * We are here when the lock is valid. So we are safe.
             */
            if(pRulesInfo->pFilters)
            {
                free(pRulesInfo->pFilters);
                pRulesInfo->pFilters = NULL;
            }
            pRulesInfo->numFilters = 0;
            clLogParse(CL_LOG_RULES_FILE, NULL, NULL);
        }
    }

    if( (NULL == pRulesInfo->pFilters) || 
        ( 0   == pRulesInfo->numFilters) )
    {
        /* 
         * With no filters installed, we filter at default severity.
         */
        if(severity <= clLogDefaultSeverity)
        {
            *pMatched = CL_TRUE;
            retVal = CL_TRUE;
        }
        goto out;
    }

    /*
     * Find the filter with the maximum matches.
     */
    for( cnt = 0; cnt < pRulesInfo->numFilters; cnt++ )
    {
        ClInt32T matches = 0;
        if( !clLogRuleValidate(&(pRulesInfo->pFilters[cnt]),
                               pRulesInfo->pFilters[cnt].node, pNodeName, 0, &matches) )
        {
            continue;
        }
        if( !clLogRuleValidate(&(pRulesInfo->pFilters[cnt]), 
                               pRulesInfo->pFilters[cnt].server, pSvrName, 1, &matches) )
        {
            continue;
        }
        if( !clLogRuleValidate(&(pRulesInfo->pFilters[cnt]), 
                               pRulesInfo->pFilters[cnt].area, pArea, 2, &matches) )
        {
            continue;
        }
        if( !clLogRuleValidate(&(pRulesInfo->pFilters[cnt]), 
                               pRulesInfo->pFilters[cnt].context, pContext, 3, &matches) )
        {
            continue;
        }
        if( !clLogRuleValidate(&(pRulesInfo->pFilters[cnt]), 
                               pRulesInfo->pFilters[cnt].file, pFileName, 4, &matches) )
        {
            continue;
        }

        matchFlag = CL_TRUE;

        /*
         * Hunt for the largest match. Incase of a match hit, take the 
         * one with the lowest severity.
         */
        
        if(matches > maxMatches)
        {
            maxMatches = matches;
            filterIndex = cnt;
        }
        else
        {
            /*
             * match clash. take if the severity is lesser than the last hit.
             */
            if(filterIndex < 0 )
                filterIndex = cnt;
            else
            {
                if(pRulesInfo->pFilters[cnt].severity < pRulesInfo->pFilters[filterIndex].severity)
                    filterIndex = cnt;
            }
        }
    }

    if(!matchFlag) 
    {
        goto out;
    }

    *pMatched = CL_TRUE;

    /*
     * If we have a match hit, then check against severity 
     * with the best filter that got matched.
     */
    
    if(severity <= pRulesInfo->pFilters[filterIndex].severity)
    {
        retVal = CL_TRUE;
    }

    out:
    if(ruleLockValid)
    {
        clOsalMutexUnlockSilent(&gClRuleLock);
    }
    return retVal;
}

ClBoolT
clLogRulesValidate(ClCharT         *pNodeName, 
                   ClCharT         *pSvrName,
                   ClCharT         *pArea,
                   ClCharT         *pContext,
                   ClCharT         *pFileName, 
                   ClLogSeverityT  *pLogSeverity)
{
   ClLogRulesInfoT  *pRulesInfo = &gpLogRulesInfo;
   ClLogRuleT       *pRules     = pRulesInfo->pRules;
   ClUint32T        cnt         = 0;
   ClBoolT          matchFlag   = CL_FALSE;

   if( NULL == pRules ) return CL_FALSE;

   for( cnt = 0; cnt < pRulesInfo->numRules; cnt++, pRules++ )
   {
      if( !clLogSingleMemEvaluate(&pRules->ruleMems[0], pNodeName) )
      {
          continue;
      }
      if( !clLogSingleMemEvaluate(&pRules->ruleMems[1], pSvrName) )
      {
          continue;
      }
      if( !clLogSingleMemEvaluate(&pRules->ruleMems[2], pArea) )
      {
          continue;
      }
      if( !clLogSingleMemEvaluate(&pRules->ruleMems[3], pContext) )
      {
          continue;
      }
      if( !clLogSingleMemEvaluate(&pRules->ruleMems[4], pFileName) )
      {
          continue;
      }
      matchFlag = CL_TRUE;
      if( *pLogSeverity > pRules->severity )
      {
          continue;
      }
      goto returnSeverity;
   }
   *pLogSeverity = CL_LOG_SEV_MAX;

   return matchFlag;
returnSeverity: 
   *pLogSeverity = pRules->severity;
   return matchFlag;
}

void
clLogDbgFilePtrAssign(void)
{
    clDbgFp = stdout; 
    if( NULL != clLogToFile )
    {
        if( !strcmp(clLogToFile, "stdout") )
        {
            clDbgFp = stdout;
        }
        else if ( !strcmp(clLogToFile, "stderr") )
        {
            clDbgFp = stderr;
        }
        else if ( !strcmp(clLogToFile, "none") )
        {
            clDbgFp = NULL;
        }
        else
        {
            if( NULL == (clDbgFp = fopen(clLogToFile, "a+")) )
            {
            }
        }
    }
}

ClLogSeverityT
clLogSeverityGet(const ClCharT  *pSevName)
{
    if( NULL == pSevName )
    {
        goto ret_default;
    }
    if( !strcmp(pSevName, "EMERGENCY") )
    {
        return CL_LOG_SEV_EMERGENCY;
    }
    else if( !strcmp(pSevName, "ALERT") )
    {
        return CL_LOG_SEV_ALERT;
    }
    else if( !strcmp(pSevName, "CRITICAL") )
    {
        return CL_LOG_SEV_CRITICAL;
    }
    else if( !strcmp(pSevName, "ERROR") )
    {
        return CL_LOG_SEV_ERROR;
    }
    else if( !strcmp(pSevName, "WARN") )
    {
        return CL_LOG_SEV_WARNING;
    }
    else if( !strcmp(pSevName, "NOTICE") )
    {
        return CL_LOG_SEV_NOTICE;
    }
    else if( !strcmp(pSevName, "INFO") )
    {
        return CL_LOG_SEV_INFO;
    }
    else if( !strcmp(pSevName, "DEBUG") )
    {
        return CL_LOG_SEV_DEBUG;
    }
    else if( !strcmp(pSevName, "TRACE") )
    {
        return CL_LOG_SEV_TRACE;
    }
    CL_LOG_PRNT_ERR("Severity level [%s] is not valid level, setting to [%s]\n",
            pSevName, clLogSeverityStrGet(clLogDefaultSeverity));
ret_default:    
    return clLogDefaultSeverity;
}

void
clLogEnvironmentVariablesGet(void)
{
    ClCharT  *pEnvVar = NULL;
    /* getting file varible for putting debug messages */
    clLogToFile = getenv("CL_LOG_TO_FILE");    

    /* Whether logging should be enabled or not */
    if( NULL == (pEnvVar = getenv("CL_LOG_STREAM_ENABLE")) )
    {
        clLogStreamEnable = CL_TRUE;
    }
    else
    {
        clLogStreamEnable = atoi(pEnvVar);
    }
    
    /* Default severity level while booting up the system */
    if( NULL == (pEnvVar = getenv("CL_LOG_SEVERITY")) )
    {
        clLogDefaultSeverity = CL_LOG_SEV_NOTICE;
    }
    else
    {
        clLogDefaultSeverity = clLogSeverityGet(pEnvVar);
        clLogSeveritySet  = CL_TRUE;
    }

    /* See if they want file/line outputted */
    gClLogCodeLocationEnable = clParseEnvBoolean("CL_LOG_CODE_LOCATION_ENABLE");
    
    /* see if they want the time outputted */
    if( NULL == (pEnvVar = getenv("CL_LOG_TIME_ENABLE")) )
    {
        clLogTimeEnable = CL_TRUE;
    }
    else
    {
        clLogTimeEnable = clParseEnvBoolean("CL_LOG_TIME_ENABLE");
    }

    /* Find the file pointer for the Debug Messages */
    clLogDbgFilePtrAssign();
}

ClRcT
clLogTimeGet(ClCharT   *pStrTime, ClUint32T maxBytes)
{
    struct timeval  tv = {0};
    ClCharT *tBuf = NULL;
    gettimeofday(&tv, NULL);
    ctime_r((time_t *) &tv.tv_sec, pStrTime);
    pStrTime[strlen(pStrTime) - 1] = 0;
    if( (tBuf = strrchr(pStrTime, ' ')) )
    {
        ClUint32T len;
        if((len = strlen(tBuf)) + 4 < maxBytes )
        {
            memmove(tBuf + 4, tBuf, len);
            snprintf(tBuf, 5, ".%.3d", (ClUint32T)tv.tv_usec/1000);
            tBuf[4] = ' ';
        }
    }
    return CL_OK;
}

ClRcT
clLogHeaderGetWithContext(const ClCharT *pArea, const ClCharT *pContext, 
                          ClCharT *pMsgHeader, ClUint32T maxHeaderSize)
{
    ClRcT rc = CL_OK;
    static ClNameT nodeName = {0};
    ClCharT timeStr[40] = {0};

    if(!pMsgHeader || !maxHeaderSize)
        return CL_ERR_INVALID_PARAMETER;

    if(!nodeName.length)
    {
        rc = clCpmLocalNodeNameGet(&nodeName);
        if(rc != CL_OK)
            return rc;
    }

    if(!pArea)
        pArea = CL_LOG_AREA_UNSPECIFIED;

    if(!pContext)
        pContext = CL_LOG_CONTEXT_UNSPECIFIED;

    clLogTimeGet(timeStr, (ClUint32T)sizeof(timeStr));
    snprintf(pMsgHeader, maxHeaderSize, CL_LOG_PRNT_FMT_STR_WO_FILE, timeStr,
             nodeName.length, nodeName.value, (int)getpid(),
             CL_EO_NAME, pArea, pContext);
    return CL_OK;
}

ClRcT
clLogHeaderGet(ClCharT *pMsgHeader, ClUint32T maxHeaderSize)
{
    return clLogHeaderGetWithContext(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,
                                     pMsgHeader, maxHeaderSize);
}

static void logRuleSigHandler(int sig)
{
    gClLogParseFilter = 1;
}

void clLogDebugFilterInitialize(void)
{
    struct sigaction act;
    clOsalMutexErrorCheckInit(&gClRuleLock);
    memset(&act, 0, sizeof(act));
    act.sa_handler = logRuleSigHandler;
    act.sa_flags = SA_RESTART;
    sigaction(SIGHUP, &act, NULL);
}

void clLogDebugFilterFinalize(void)
{
    gClRuleLockValid = CL_FALSE;
}

static ClRcT
logVMsgWriteDeferred(ClLogStreamHandleT streamHdl,
                     ClLogSeverityT  severity,
                     ClUint16T       serviceId,
                     const ClCharT         *pArea,
                     const ClCharT         *pContext,
                     const ClCharT         *pFileName,
                     ClUint32T       lineNum,
                     ClBoolT          deferred,
                     ClBoolT logToFile,
                     const ClCharT   *pFmtStr,
                     va_list vaargs)
{
    static ClNameT    nodeName            = {0};
    static ClUint32T  msgIdCnt            = 0;
    ClCharT           msg[CL_LOG_MAX_MSG_LEN] = {0};
    ClCharT           msgHeader[CL_MAX_NAME_LENGTH];
    ClUint32T         formatStrLen        = 0;
    ClCharT           *pSevName           = NULL;
    ClCharT           timeStr[40]          = {0};
    ClBoolT           match = CL_FALSE;
    ClBoolT           filterMatch = CL_FALSE;

    if( (severity >= CL_LOG_SEV_MAX) &&
        (severity < CL_LOG_SEV_EMERGENCY) )
    {
        CL_LOG_PRNT_ERR("Passed severity level [%d] is not valid, it should be between [%d] to [%d]\n",
                        severity, CL_LOG_SEV_EMERGENCY, CL_LOG_SEV_MAX);
        return CL_OK;
    }

    clLogTimeGet(timeStr, (ClUint32T)sizeof(timeStr));
    if( 0 == msgIdCnt )
    {
        if( CL_OK != clCpmLocalNodeNameGet(&nodeName) )
        {
            /* how do we return error */
        }
        clLogEnvironmentVariablesGet();
        clLogParse(CL_LOG_RULES_FILE, NULL, NULL);
    }
    msgIdCnt++;
    pSevName = clLogSeverityStrGet(severity);
    
    if( clLogSeveritySet == CL_FALSE )
    {
        match = clLogRulesTest(nodeName.value, CL_EO_NAME,
                               pArea, pContext, pFileName, severity, &filterMatch);
    }
    else
    {
        if( severity <= clLogDefaultSeverity )
        {
            match = CL_TRUE;
        }
    }
    if(gClLogCodeLocationEnable)
        
    {
        formatStrLen = snprintf(msg, CL_LOG_MAX_MSG_LEN - 1, CL_LOG_PRNT_FMT_STR_CONSOLE, 
                                timeStr, pFileName, lineNum, nodeName.length, nodeName.value, (int)getpid(),
                                CL_EO_NAME, pArea, pContext, msgIdCnt, pSevName);
        snprintf(msgHeader, sizeof(msgHeader), CL_LOG_PRNT_FMT_STR,
                 timeStr, pFileName, lineNum, nodeName.length, nodeName.value, (int)getpid(),
                 CL_EO_NAME, pArea, pContext);
        
    }
    else
    {
        formatStrLen = snprintf(msg, CL_LOG_MAX_MSG_LEN - 1,
                                CL_LOG_PRNT_FMT_STR_WO_FILE_CONSOLE, timeStr,  
                                nodeName.length, nodeName.value, (int)getpid(),
                                CL_EO_NAME, pArea, pContext,
                                msgIdCnt, pSevName);
        snprintf(msgHeader, sizeof(msgHeader), CL_LOG_PRNT_FMT_STR_WO_FILE, timeStr,
                 nodeName.length, nodeName.value, (int)getpid(),
                 CL_EO_NAME, pArea, pContext);
    }

    /*
     * Dump to console first to be emitted into nodename log file
     */
    vsnprintf(msg + formatStrLen, CL_LOG_MAX_MSG_LEN - formatStrLen, pFmtStr, vaargs);

    if( NULL != clDbgFp && match == CL_TRUE)
    {
        fprintf(clDbgFp, "%s\n", msg);
        fflush(clDbgFp);
    }

    if(logToFile)
    {
        if(deferred)
        {
            if( CL_OK != clLogWriteDeferredForceWithHeader(streamHdl, severity, serviceId, 
                                                           CL_LOG_MSGID_PRINTF_FMT, msgHeader, "%s", msg + formatStrLen) )
            {
                /* How do we return the error */
            }
        }
        else
        {
            if( CL_OK != clLogWriteDeferredWithHeader(streamHdl, severity, serviceId, 
                                                      CL_LOG_MSGID_PRINTF_FMT, msgHeader, "%s", msg + formatStrLen) )
            {
                /* How do we return the error */
            }
        }
    }

    return CL_OK;
}

ClRcT
clLogMsgWrite(ClLogStreamHandleT streamHdl,
              ClLogSeverityT  severity,
              ClUint16T       serviceId,
              const ClCharT         *pArea,
              const ClCharT         *pContext,
              const ClCharT         *pFileName,
              ClUint32T       lineNum,
              const ClCharT   *pFmtStr,
              ...)
              
{
    ClRcT rc = CL_OK;
    va_list vaargs;
    va_start(vaargs, pFmtStr);
    rc = logVMsgWriteDeferred(streamHdl, severity, serviceId, pArea, pContext,
                              pFileName, lineNum, CL_FALSE, CL_TRUE, pFmtStr, vaargs);
    va_end(vaargs);
    return rc;
}

ClRcT
clLogMsgWriteDeferred(ClLogStreamHandleT streamHdl,
                      ClLogSeverityT  severity,
                      ClUint16T       serviceId,
                      const ClCharT         *pArea,
                      const ClCharT         *pContext,
                      const ClCharT         *pFileName,
                      ClUint32T       lineNum,
                      const ClCharT   *pFmtStr,
                      ...)
              
{
    ClRcT rc = CL_OK;
    va_list vaargs;
    va_start(vaargs, pFmtStr);
    rc = logVMsgWriteDeferred(streamHdl, severity, serviceId, pArea, pContext,
                              pFileName, lineNum, CL_TRUE, CL_TRUE, pFmtStr, vaargs);
    va_end(vaargs);
    return rc;
}

ClRcT
clLogMsgWriteConsole(ClLogStreamHandleT streamHdl,
                     ClLogSeverityT  severity,
                     ClUint16T       serviceId,
                     const ClCharT         *pArea,
                     const ClCharT         *pContext,
                     const ClCharT         *pFileName,
                     ClUint32T       lineNum,
                     const ClCharT   *pFmtStr,
                     ...)
              
{
    ClRcT rc = CL_OK;
    va_list vaargs;
    va_start(vaargs, pFmtStr);
    rc = logVMsgWriteDeferred(streamHdl, severity, serviceId, pArea, pContext,
                              pFileName, lineNum, CL_TRUE, CL_FALSE, pFmtStr, vaargs);
    va_end(vaargs);
    return rc;
}

ClRcT
clLogDbgFileClose(void)
{
    if( (clDbgFp != stderr) && (clDbgFp != stdout) )
    {
        fclose(clDbgFp);
    }
    return CL_OK;
}
