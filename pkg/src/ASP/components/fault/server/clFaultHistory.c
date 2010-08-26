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
/*******************************************************************************
 * ModuleName  : fault                                                         
 * File        : clFaultHistory.c
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 * This module implements the Fault History database                      
 *******************************************************************************/

/* 
 * 
 * Implement an efficient Fault History database.
 *
 */

/* system includes */
#include <time.h>
#include <string.h>

/* ASP includes */
#include <clOmApi.h>
#include <clDebugApi.h>
#include <clCorApi.h>
#include <clCorUtilityApi.h>
#include <clCorServiceId.h>
#include <clOmCommonClassTypes.h>
#include <clOmApi.h>
#include <clEoApi.h>
#include <clEoConfigApi.h>
#include <clCpmApi.h>
#include <clCommon.h>
#include <clClistApi.h>
#include <clCntApi.h>
#include <clRuleApi.h>
#include <clOsalApi.h>
#include <clTimerApi.h>

/* fault includes */
#include <clFaultHistory.h>
#include <clFaultServerIpi.h>
#include <clFaultDefinitions.h>
#include <clFaultErrorId.h>
#include <clFaultApi.h>
#include <clFaultClientServerCommons.h> 
#include <clFaultLog.h> 

#define MAXSIZE_SINGLE_FAULT_RECORD 225
#define MAX_FAULT_BUCKETS 10
#define MODULE_STR   __FILE__
#define BUFFER_LENGTH 100

/*
 * gHistoryString is a global pointer of type ClCharT.
 * This is used for getting the fault history packaged
 * into this and pass it back to the debug server 
 * wherein the same will be printed when the control 
 * moves back to the debug server.
 */
ClCharT *gHistoryString;

extern ClEoExecutionObjT gFmEoObj;
static ClInt32T 
clFault2MinHisKeyCompare(ClCntKeyHandleT key1, ClCntKeyHandleT key2);
static void 
clFaultHistoryDeleteCallBack(ClCntKeyHandleT userKey,
                         ClCntDataHandleT userData);
static ClRcT 
clFault2MinHistoryCallback (ClUint32T eoArg, void *arg);
static ClRcT 
_clFaultCreateRBTree(void);

/* Fault history contains 20 minutes history of faults in the system.
 * The history is a circular list, with each node containing 2 min
 * of fault info. Once the list is full, the first node will be dropped.
 * 
 * Each node is a link list of the faults for that 2 minute period.
 * The key for the nodes in the linkList is the ClCorMOId*. So there may
 * be more than one faults for a key.
 */
  
static ClClistT   sfaultHistoryList = 0; /* List of 2 minute history */
static ClTimerHandleT  sFaultMan2MinHistoryTimerH;
static ClCntHandleT   sCurrent2MinBucketH;
static ClUint8T   sBucketCount;  /* should be used only by FM history show */
static ClOsalMutexIdT shfaultHistoryMutex ;
static ClUint32T sNumberRecords;

void timeStamp(ClCharT timeStr [], ClUint32T    len)
{
    time_t  timer ;
    struct  tm *t = NULL;
    ClCharT * pBuff =NULL;

   struct tm *localtimebuf = clHeapAllocate(sizeof(struct tm));;
   if(localtimebuf == NULL)
   {
      CL_DEBUG_PRINT(CL_DEBUG_ERROR,( "\nMemory Allocation failed\n"));
      return ;
   }
   ClCharT *asctimebuf   = clHeapAllocate(BUFFER_LENGTH*sizeof(ClCharT));
   if(asctimebuf == NULL)
   {
      CL_DEBUG_PRINT(CL_DEBUG_ERROR,( "\nMemory Allocation failed\n"));
      return ;
   }

   memset(&timer,0,sizeof(time_t));
    time(&timer);
    t = localtime_r(&timer,localtimebuf);
    pBuff=asctime_r(t,asctimebuf);
    strncpy(timeStr,pBuff,len-1);
    timeStr[strlen(timeStr) - 1] ='\0';
	clHeapFree(localtimebuf);
	clHeapFree(asctimebuf);
}

/**
 *  Sets the number of bin and duration of each bin for fault data.
 *
 *  @param  binInterval  The interval in miliseconds for each fault data bin.
 *  @param  binNumbers  The number of bins.
 *
 */

ClRcT 
clFaultHistoryInit(ClUint32T binNumbers){

   ClRcT          rc = CL_OK;
   ClTimerTimeOutT      timeOut;


   /* fault history containers can be accessed by FM thread, timer thread
    * etc .. So before accessing the containers, the threads shd take the
    * mutex
    */
    rc = clOsalMutexCreateAndLock(&shfaultHistoryMutex);
    if (rc != CL_OK)
    {
        clLogWrite(CL_LOG_HANDLE_APP, 
               CL_LOG_ERROR,   
               CL_FAULT_SERVER_LIB,
               CL_FAULT_LOG_1_MUTEX,
               "Fault History data base");
      return CL_FAULT_ERR_HISTORY_MUTEX_CREATE_ERROR;
    }

   /* 
    * Create a circular queue with number of nodes = binNumbers
    * Each node is a linked list of fault recovery/history record 
       sorted by key composed of category and severity
    */
   /*
    * Create a repetitive timer at frequency interval = binInterval
    */

   rc = clClistCreate( MAX_FAULT_BUCKETS,
                          CL_DROP_FIRST,
                           (ClClistDeleteCallbackT) clFaultHis2MinBucketDelete,
                           (ClClistDeleteCallbackT) clFaultHis2MinBucketDelete,
                           &sfaultHistoryList);

   if (rc != CL_OK ) 
   {
      clLogError("FLT", NULL, "Error in creating Clist, rc [0x%x]", 
              rc);
      goto unlock_exit;
   }


   /* start a 2 minute timer */
    timeOut.tsSec = binNumbers;
    timeOut.tsMilliSec = 0; 

    if ((rc = clTimerCreate(timeOut,
                              CL_TIMER_REPETITIVE,
                              CL_TIMER_SEPARATE_CONTEXT, 
                              (ClTimerCallBackT) clFault2MinHistoryCallback,
                              NULL, 
                              &sFaultMan2MinHistoryTimerH)
                            ) != CL_OK)
   {
     clLogError("FLT", NULL, "Failed to create repetitive timer for \
                            fault manager's 2 min history");
      goto unlock_exit;
    }

   if ((rc = clTimerStart(sFaultMan2MinHistoryTimerH)) != CL_OK)
   {
      clLogError("FLT", NULL,
              "Fault Manager: could not start the 2 \
                             minute history timer bin rc =  0x%x", rc);
      goto unlock_exit;
   }

   /* create the first bucket in the init itself*/
   rc = _clFaultCreateRBTree();
   if(CL_OK != rc)
   {
       clLogError("FLT", NULL,
               "Error in adding 1st rbtree to store fault records, rc [0x%x]", 
               rc);
   }
unlock_exit:
   clFaultHistoryDataUnLock();
   return rc;
}


void 
clFaultHis2MinBucketDelete(ClClistDataT userData)
{
   ClUint32T size=0;
   ClRcT rc = CL_OK;
   ClFH2MinBucketPtr   bucketH = (ClFH2MinBucketPtr) userData;
   /* iterate thru the link list (user data) and free all the fault records*/
   rc = clCntSizeGet(bucketH->bucketHandle, &size);
   if(CL_OK != rc)
   {
       clLogError("FLT", NULL, 
               "Error in getting size of container, rc [0x%x]", rc);
       return;
   }
   clLogDebug("FLT", NULL,
           "Size of RbTree [%d]", size); 

   /* delete the redblack tree container present in the 2 minute bucket */
   clCntDelete(bucketH->bucketHandle);

   /* now delete the user data */
   clHeapFree((void*)bucketH);
}


static ClRcT 
clFault2MinHistoryCallback (ClUint32T eoArg, void *arg)
{
    ClRcT   rc = CL_OK;

    clLogDebug("FLT", NULL,
           "Timer popped");
   if ( (rc = clFaultHistoryDataLock()) != CL_OK )
   {
       clLogError("FLT", NULL,
               "clFaultHistoryDataLock failed, rc [0x%x]", rc);
       return rc;
   }

    rc = _clFaultCreateRBTree();
    if(CL_OK != rc)
    {
        clLogError("FLT", NULL,
                "Error in creating RBTree, rc [0x%x]", 
                rc);

    }
    clFaultHistoryDataUnLock();
    return rc;
}
/* This creates a new rbTree and appends to the Clist */
static ClRcT 
_clFaultCreateRBTree()
{
   ClRcT   rc = CL_OK;
   ClFH2MinBucketPtr   bucketH = NULL;

   bucketH =(ClFH2MinBucketPtr)clHeapAllocate(sizeof(ClFH2MinBucketT));
   if(!bucketH)
   {
       clLogError("FLT", NULL,
               "Error in allocating memory");
       return CL_ERR_NO_MEMORY;
   }
   memset(bucketH ,0,sizeof(ClFH2MinBucketT));

   timeStamp(bucketH->timeString, sizeof(bucketH->timeString));

   clLogDebug("FLT", NULL,
           "Creating tree to insert new fault records");
   rc = clCntRbtreeCreate(clFault2MinHisKeyCompare,
              clFaultHistoryDeleteCallBack,
              clFaultHistoryDeleteCallBack,
                 CL_CNT_NON_UNIQUE_KEY,
              &(bucketH->bucketHandle));
   if(CL_OK != rc)
   {
       clLogError("FLT", NULL,
               "Error in creating new instance of RbTree, rc [0x%x]", rc);
       return rc;
   }
   
   sCurrent2MinBucketH = bucketH->bucketHandle;

   clLogDebug("FLT", NULL,
           "Adding new instance of RbTree to Clist");
   /* now add this bucket to the sfaultHistoryList */
    if ((rc = clClistLastNodeAdd(sfaultHistoryList,
                                    (ClClistDataT) bucketH)) != CL_OK)
    {
        clLogError("FLT", NULL, 
                "Adding last node to Clist failed, rc [0x%x]", 
                rc);
    }

   return rc;
}


static ClInt32T 
clFault2MinHisKeyCompare(ClCntKeyHandleT key1, ClCntKeyHandleT key2)
{
    clLogDebug("FLT", NULL,
            "Key1 : %lu, Key2 : %lu", (ClWordT)key1, (ClWordT)key2);

    return ((ClWordT)key1 - (ClWordT)key2);
}


static void 
clFaultHistoryDeleteCallBack(ClCntKeyHandleT userKey, 
            ClCntDataHandleT userData){

   clHeapFree((void*)userData);
}


   
ClRcT 
clFaultEventHistoryAdd(ClFaultRecordPtr pFE){
    
   ClRcT         rc = CL_OK;
   ClUint32T      hHandle = CL_FAULT_HISTORY_KEY;
   
   clLogDebug("SER", NULL,
           "Adding record with cat [%d], sev [%d], pCause [%d], spec prob [%d], seq num [%d]",
           pFE->event.category,
           pFE->event.severity,
           pFE->event.cause,
           pFE->event.specificProblem,
           pFE->seqNum);

   rc = clCntNodeAdd( sCurrent2MinBucketH, 
                  (ClCntKeyHandleT)(ClWordT) hHandle,  
                  (ClCntDataHandleT) pFE,
                  NULL);

   if (rc != CL_OK)
   {
        clLogError("FLT", NULL,
                "Error in adding node to RbTree, rc [0x%x]",
                rc);
        return rc;
   
   }
   return rc;
}


#if 0
ClRcT 
clFaultHistoryMoIdRecordsDelete(ClCorMOIdPtrT hMOId){
    

   ClRcT         rc = CL_OK;
   ClHandleT       hHandle;
   ClUint32T       size=0;
   ClUint32T       attrId;
   ClCorAttrPathT*    tempContAttrPath = NULL;
   ClCorObjectHandleT hMSOObj;   
   


   /* get the omHandle for the ClCorMOId */
   
   clCorMoIdServiceSet(hMOId, CL_COR_SVC_ID_FAULT_MANAGEMENT);
   attrId = CL_FML_FAULT_HISTORY_KEY;

   rc = clCorObjectHandleGet(hMOId, &hMSOObj);

   if (CL_OK != rc)
   {
      CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("clFaultHistoryMoIdRecordsDelete: \
               clCorObjectHandleGet failed w/rc :%x \n", rc));
      return rc;
   }

   size = sizeof(ClHandleT);   
   rc = clCorObjectAttributeGet(hMSOObj, 
                        tempContAttrPath, 
                        attrId, 
                        CL_COR_INVALID_ATTR_IDX,  
                        &hHandle, 
                        &size);   

   if (rc != CL_OK)
   {
      CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("\n clCorObjectAttributeGet, not able to get the\
                               fault COR attr value for attrId:%d, rc:%0x \n", attrId, rc));
      return rc;
   }   
   
   
   /* go thru all the 2 min bins and delete the records for this ClCorMOId */
   clClistSizeGet(sfaultHistoryList, &size);

   rc = clClistWalk(sfaultHistoryList, 
                    (ClClistWalkCallbackT) clFaultHistoryRecordDelete, 
                    (void*)hHandle);

   if (rc != CL_OK)
   {
      CL_DEBUG_PRINT(CL_DEBUG_ERROR,("faultHistoryMoIdRecodsDelete: clClistWalk\
                              failed !!! rc => [0x%x]", rc));
      return rc;
   
   }

   return rc;
   
}

ClRcT 
clFaultHistoryRecordDelete(ClFH2MinBucketT* bucketH, ClHandleT hOm){

   ClRcT      rc = CL_OK;
   ClUint32T   size = 0;
   
   /* delete all the records for this omHandle */
   clCntSizeGet(bucketH->bucketHandle, &size);

   rc = clCntAllNodesForKeyDelete(bucketH->bucketHandle, hOm);

   if (rc != CL_OK)
   {
      CL_DEBUG_PRINT(CL_DEBUG_ERROR,("clFaultHistoryRecordDelete: clCntAllNodesForKeyDelete\
                              failed !!! rc => [0x%x]", rc));
   
   }

   return rc;
   
}
#endif
ClRcT 
clFaultProcessFaultRecordQuery(ClCorMOIdPtrT hMOId, 
                           ClUint8T category, 
                           ClUint8T severity, 
                           ClAlarmSpecificProblemT specificProblem,
                           ClAlarmProbableCauseT cause,
                           ClFaultBucket_e bucketInfo, 
                           ClFaultRecordPtr pFE, 
                           ClUint8T* recordFound)
{
   ClRcT         rc = CL_OK;
   ClClistNodeT   nodeH;
   ClFH2MinBucketPtr   bucketH;
   ClFH2MinBucketPtr   bucketDataH;
   ClUint32T       size = 0;
   ClUint32T      idx = 0;
   ClClistNodeT   prevNodeH;
   ClHandleT       hHandle = CL_FAULT_HISTORY_KEY;
   
   *recordFound = 0;
   
   bucketH = (ClFH2MinBucketPtr)clHeapAllocate(sizeof(ClFH2MinBucketT));
   if(!bucketH)
   {
       clLogError("FLT", NULL,
               "Failed to allocate memory");
       return CL_ERR_NO_MEMORY;
   }

   CL_ASSERT(bucketH && pFE);

   rc = clEoMyEoObjectSet(&gFmEoObj);

   if (gFmEoObj.rmdObj == 0)
      CL_DEBUG_PRINT(CL_DEBUG_ERROR,("gFmEoObj->rmdObj is NULL ..... \n"));

   rc = clClistSizeGet(sfaultHistoryList, &size);
   if(CL_OK != rc)
   {
       clLogError("FLT", NULL,
               "Error in getting size of the container, rc [0x%x]",
               rc);
       return rc;
   }

   if (bucketInfo == CL_LATEST_RECORD_IN_CURRENT_BUCKET)
   {
       /* Check only in the current bucket. */
       rc = clFaultMOIDRecordsInBucketSearch(pFE, 
                                             hHandle, 
                                             sCurrent2MinBucketH, 
                                             category, 
                                             severity, 
                                             specificProblem,
                                             cause,
                                             recordFound);
       if (*recordFound)
       {
            clLogInfo("FLT", "FIND", "Fault Record category [%d], severity [%d] found in the current bucket.",
                    category, severity);
          clHeapFree((void*)bucketH);
          return CL_OK;
       }
   }
   else if (bucketInfo == CL_ALL_BUCKETS)
   {
        /* Search in all the buckets. */
        rc = clClistLastNodeGet(sfaultHistoryList, &nodeH);
        if (rc != CL_OK)
        {
           clLogError("FLT", NULL,
                   "Error in getting the last node from Clist, rc [0x%x]",
                   rc);
          clHeapFree((void*)bucketH);
          return rc;
        }

        for (idx = 0; idx < size; idx++)
        {
            rc = clClistDataGet(sfaultHistoryList, 
                                nodeH, 
                                (ClClistDataT*) &bucketDataH);
            if (rc != CL_OK)
            {
              clLogError("FLT", NULL,
                      "Error in getting data of last node, rc [0x%x]", 
                      rc);
              clHeapFree((void*)bucketH);
              return rc;
            }

            memcpy(bucketH, bucketDataH, sizeof(ClFH2MinBucketT));

            rc = clFaultMOIDRecordsInBucketSearch(pFE, 
                                               hHandle, 
                                               bucketH->bucketHandle, 
                                               category, 
                                               severity, 
                                               specificProblem,
                                               cause,
                                               recordFound);

            if (*recordFound)
            {
                rc = CL_OK;
                break;
            }
           
            prevNodeH = 0;

            rc = clClistPreviousNodeGet(sfaultHistoryList, 
                                           nodeH, 
                                           &prevNodeH);

            if (rc != CL_OK)
            {
               CL_DEBUG_PRINT(CL_DEBUG_ERROR,("clFaultProcessFaultRecordQuery:\
                                          clClistPreviousNodeGet \
                                          failed !!! rc => [0x%x]", rc));
               clHeapFree((void*)bucketH);
               return rc;
            }         

            CL_ASSERT(prevNodeH);            
            
            nodeH = prevNodeH;
        }
   }

   if (*recordFound)
   {
      clLogDebug("FLT", "FIND", "Fault record found for Category : [%d], Severity : [%d], Seq Num : [%d]",
               (pFE->event).category, (pFE->event).severity, pFE->seqNum);
   }
   
   clHeapFree((void*)bucketH);   

   return rc;
}


void 
clFaultMOIdRecordCheck(ClFaultRecordPtr pFE, 
                       ClUint8T catBitMap , 
                       ClUint8T sevBitMap,
                       ClAlarmSpecificProblemT specificProblem,
                       ClAlarmProbableCauseT cause,
                       ClUint8T* recordMatches)
{

    clLogDebug("FLT", NULL,
            "Bucket : Cat [%d], sev [%d] specific prob [%d] cause [%d], \
            Client record: Cat [%d] sev [%d] specific prob [%d] cause [%d]", 
            (pFE->event).category,
            (pFE->event).severity,
            (pFE->event).specificProblem,
            (pFE->event).cause,
            catBitMap, sevBitMap,
            specificProblem, cause); 

   if ( catBitMap == (pFE->event).category &&
         sevBitMap == (pFE->event).severity)
         *recordMatches = 1;   
 
   if (*recordMatches)
   {
       clLogInfo("FLT", "FIND", "Record matches for Category : [%d], Severity : [%d].",
               (pFE->event).category, (pFE->event).severity);
   }            

   return;
}


ClRcT 
clFaultMOIDRecordsInBucketSearch(ClFaultRecordPtr pFE, 
                        ClHandleT hOM,
                        ClCntHandleT hBucket, 
                        ClUint8T category, 
                        ClUint8T severity, 
                        ClAlarmSpecificProblemT specificProblem,
                        ClAlarmProbableCauseT cause,
                        ClUint8T* recordFound)
{
   ClCntNodeHandleT   nodeH;
   ClRcT         rc = CL_OK;
   ClUint32T       size = 0;
   ClUint32T      idx = 0;
   ClCntNodeHandleT*   nextNodeH ;
   ClFaultRecordPtr      ptmpFaultRecord = NULL;
   ClUint8T         matchFound = 0;
   ClFaultRecordPtr       frDataH;
   
   rc = clCntSizeGet(hBucket, &size);
   if(CL_OK != rc)
   {
       clLogError("FLT", NULL,
               "Error in getting size of container, rc [0x%x]",
               rc);
       return rc;
   }

   clLogDebug("FLT", NULL,
           "Size of rbtree [%d]", size);
   size = 0;

   rc = clCntNodeFind(hBucket, (ClPtrT)(ClWordT)hOM,  &nodeH);
   if (rc != CL_OK)
   {
       clLogError("FLT", NULL,
               "Error in finding node in RbTree, rc [0x%x]. This can be because of first fault occurence", rc);
       return rc;
   }

   /* now get the data from the node handle */

   rc = clCntNodeUserDataGet(hBucket, 
                      nodeH, 
                      (ClCntDataHandleT*) &frDataH);
   if (rc != CL_OK)
   {
      clLogError("FLT", NULL,
              "Error while getting user data, rc [0x%x]", rc);
      return rc;
   }

   clLogDebug("SER", NULL,
           "After getting data from rbtree, cat [%d], sev [%d], cause [%d], prob [%d]",
           frDataH->event.category,
           frDataH->event.severity,
           frDataH->event.cause,
           frDataH->event.specificProblem);

   /* now copy the fault record data */
   memcpy(pFE, frDataH, sizeof(ClFaultRecordT));

   /* check for the first node. if needed go thru other nodes as well */

   clFaultMOIdRecordCheck(pFE, 
                          category, 
                          severity, 
                          specificProblem,
                          cause,
                          recordFound);

   ptmpFaultRecord = (ClFaultRecordPtr)clHeapAllocate(sizeof(ClFaultRecordT));

   CL_ASSERT(ptmpFaultRecord);
   
   if (*recordFound)
   {

      matchFound = 1;
      *recordFound = 0;
      memcpy(ptmpFaultRecord, pFE, sizeof(ClFaultRecordT));
   }

   rc = clCntKeySizeGet(hBucket, (ClPtrT)(ClWordT)hOM, &size);

   if (rc != CL_OK)
   {
       clLogError("FLT", NULL,
               "Error in getting size of container, rc [0x%x]", rc);
      clHeapFree((void*)ptmpFaultRecord);
      return rc;
   }

   nextNodeH = (ClCntNodeHandleT*)clHeapAllocate(sizeof(ClCntNodeHandleT));

   CL_ASSERT(nextNodeH);

   for (idx=0; idx < (size-1) ; idx++)
   {
      /* get the next node */
      rc = clCntNextNodeGet(hBucket, nodeH, nextNodeH);

      if (rc != CL_OK)
      {
         CL_DEBUG_PRINT(CL_DEBUG_ERROR,("clFaultMOIDRecordsInBucketSearch: clCntNextNodeGet\
                                 failed !!! rc => [0x%x]", rc));
         clHeapFree((void*)nextNodeH);
         clHeapFree((void*)ptmpFaultRecord);
         return rc;
      }

      rc = clCntNodeUserDataGet(hBucket, 
                         *nextNodeH, 
                                 (ClCntDataHandleT*) &frDataH);

      if (rc != CL_OK)
      {
         CL_DEBUG_PRINT(CL_DEBUG_ERROR,("clFaultMOIDRecordsInBucketSearch: \
                                  clCntNodeUserDataGet in for loop \
                                  failed !!! rc => [0x%x]", rc));
         clHeapFree((void*)nextNodeH);   
         clHeapFree((void*)ptmpFaultRecord);
         return rc;
      }

      memcpy(pFE, frDataH, sizeof(ClFaultRecordT));

      CL_ASSERT(pFE);
      clFaultMOIdRecordCheck(pFE, 
                             category, 
                             severity, 
							 specificProblem,
							 cause,
							 recordFound);

      if (*recordFound)
      {

         matchFound = 1;
         *recordFound = 0;
         memcpy(ptmpFaultRecord, pFE, sizeof(ClFaultRecordT));
      
      }      
      nodeH = *nextNodeH;
   }

   if (matchFound)
   {
      clLogInfo("FLT", NULL,
              " Got a matching record");
      memcpy(pFE, ptmpFaultRecord, sizeof(ClFaultRecordT));
      *recordFound = 1;
   }
   
   clHeapFree((void*)nextNodeH);   
   clHeapFree((void*)ptmpFaultRecord);

   return rc;
}


/**
 *  Show all the fault records present in the 2 minBucket.
 *
 *
 *   @param bucketH   Handle to the 2 minute bucket
 *   verbose: Specifies how much detail to print, 0 means minimal
 *            1 means detailed.
 *
 *  @returns CL_OK  - Success<br>
 */
static ClRcT
clFaultHistoryRecordShow(ClFH2MinBucketPtr  bucketH, ClCorMOIdPtrT moid )
{
   ClRcT rc = CL_OK;
   ClCntNodeHandleT     watchHdl;
   ClFaultRecordPtr      hRec;   
   ClUint32T      size = 0;

   if ((rc = clCntFirstNodeGet(bucketH->bucketHandle, &watchHdl)) != CL_OK)
   {
      CL_DEBUG_PRINT(CL_DEBUG_INFO,("\n NO History Please "));
      return rc;
   }

   rc = clCntSizeGet(bucketH->bucketHandle, &size);

   if (rc != CL_OK)
   {
      CL_DEBUG_PRINT(CL_DEBUG_ERROR,("clFaultHistoryRecordShow: clCntSizeGet failed !!!\
                             rc => [0x%x]", rc));
      return rc;
   }   
   clLogTrace("SER", NULL, "Size of the tree [%d]" ,size);

      
        /* display the whole of the history of the system if moid not
         * specified */ 
    if  ( moid == NULL ) {   
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n MOID got null"));
       while (watchHdl != 0){
      rc = clCntNodeUserDataGet(bucketH->bucketHandle,
                         watchHdl,
                         (ClCntDataHandleT *) &hRec);

      if (rc != CL_OK){
         CL_DEBUG_PRINT(CL_DEBUG_ERROR,
               ("Cannot get current node from the container\
                      rc = 0x%x\r\n", rc));
         return rc;
      }
      /* display the record information */ 
        clFaultRecordRecursivePack (hRec ); 
        
      rc = clCntNextNodeGet(bucketH->bucketHandle, watchHdl, &watchHdl);
       }
    } /* end if(moid == NULL ) */

     /* compare the moid present in the faultRecord and display the bucket*/ 
    else  {
    clCorMoIdShow(moid);
        
       
       while (watchHdl != 0){
        
      rc = clCntNodeUserDataGet(bucketH->bucketHandle,
                         watchHdl,
                         (ClCntDataHandleT *) &hRec);

      if (rc != CL_OK){
         
             CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Cannot get current node from the container\
                         rc = 0x%x\r\n", rc));
         return rc;
      }
      /* display the record information if moid matches  */ 
      clCorMoIdShow(&(hRec->event).moId);
        if ( clCorMoIdCompare ( moid, &((hRec->event).moId))== 0){
        clFaultRecordRecursivePack (hRec ); 
        }
        
      rc = clCntNextNodeGet(bucketH->bucketHandle, watchHdl, &watchHdl);
       }
    }/* end else */ 
    
    
    CL_FUNC_EXIT();
    return (rc);
}

/**
 *  count the fault records present in the 2 minBucket.
 *
 *
 *   @param bucketH   Handle to the 2 minute bucket
 *   verbose: Specifies how much detail to print, 0 means minimal
 *            1 means detailed.
 *
 *  @returns CL_OK  - Success<br>
 */
ClRcT
clFaultHistoryRecordCount(ClFH2MinBucketPtr  bucketH )
{
   ClRcT rc = CL_OK;
   ClCntNodeHandleT     watchHdl;
   ClUint32T      size = 0;

   if ((rc = clCntFirstNodeGet(bucketH->bucketHandle, &watchHdl)) != CL_OK)
   {
      CL_DEBUG_PRINT(CL_DEBUG_INFO,("\n NO History Please "));
      return rc;
   }

   rc = clCntSizeGet(bucketH->bucketHandle, &size);

   if (rc != CL_OK)
   {
      CL_DEBUG_PRINT(CL_DEBUG_ERROR,("clFaultHistoryRecordCount: clCntSizeGet failed !!!\
                             rc => [0x%x]", rc));
      return rc;
   }   

   sNumberRecords+=size;
   CL_FUNC_EXIT();
   return (rc);
}

/* displays the fault record */
void clFaultRecordRecursivePack ( ClFaultRecordPtr   rec ){
      ClCharT *tmp=clHeapAllocate(1000);
      ClUint32T categoryStringIndex=0,severityStringIndex=0,probableCauseStringIndex=0;

      categoryStringIndex = clFaultInternal2CategoryTranslate((rec->event).category);
      sprintf (tmp," Category........... %s\n",clFaultCategoryString[categoryStringIndex]);
       strncat(gHistoryString,tmp,strlen(tmp));

      sprintf (tmp," SpecificProblem.... %d\n",(rec->event).specificProblem);
       strncat(gHistoryString,tmp,strlen(tmp));

      severityStringIndex = clFaultInternal2SeverityTranslate((rec->event).severity);
      sprintf (tmp," Severity........... %s\n",clFaultSeverityString[severityStringIndex]);
       strncat(gHistoryString,tmp,strlen(tmp));

      probableCauseStringIndex = (rec->event).cause;
      sprintf (tmp," cause.............. %s\n",clFaultProbableCauseString[probableCauseStringIndex]);
       strncat(gHistoryString,tmp,strlen(tmp));

      sprintf(tmp," SequenceNumber.. %d\n\n",  rec->seqNum);
       strncat(gHistoryString,tmp,strlen(tmp));

      clHeapFree(tmp);
}



/**
 *  Show fault history.
 *
 *  This API prints the contents of the fault history.
 *
 *  @param verbose: Specifies how much detail to print, 0 means minimal
 *   1 means detailed.
 *
 *  @returns CL_OK  - Success<br>
 */
ClRcT
clFaultHistoryShow( ClCorMOIdPtrT moid )
{
   ClRcT      rc = CL_OK;
   ClUint32T   size;
   sBucketCount = 0;   

   CL_FUNC_ENTER();

   if ( (rc = clFaultHistoryDataLock()) != CL_OK )
   {
      CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("clFaultHistoryShow, not able to obtain history \
                               data LOCK .... cannot proceed rc: 0x%x \n",
                               rc));
      return rc;
   }

   if ((rc = clClistSizeGet(sfaultHistoryList,
                           &size)) != CL_OK)
   {
      CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("clClistSizeGet failed rc => [0x%x]",
                              rc));
      clFaultHistoryDataUnLock();
      CL_FUNC_EXIT();
      return (rc);
   }
   /* this is used for counting the number of records in total in the fault
    ** history */
   sNumberRecords=0;
   if ((rc = clClistWalk(sfaultHistoryList,
                        (ClClistWalkCallbackT)clFaultHistoryRecordCount,NULL)) != CL_OK)
   {
      CL_DEBUG_PRINT(CL_DEBUG_ERROR, (MODULE_STR "clClistWalk failed\
                                          rc => [0x%x]", rc));
      clFaultHistoryDataUnLock();
      CL_FUNC_EXIT();
      return (rc);
   }
   if(sNumberRecords == 0)
   {
		gHistoryString=(ClCharT *)clHeapAllocate(BUFFER_LENGTH *sizeof(ClCharT));
		if(gHistoryString == NULL)
		{
			CL_DEBUG_PRINT(CL_DEBUG_ERROR,( "\nMemory Allocation failed\n"));
            clFaultHistoryDataUnLock();
			return CL_ERR_NO_MEMORY;
		}
		strcpy(gHistoryString,"There aren't any fault records in the history");
   }
   else
   {
      /* allocating memory for the fault records */
	  gHistoryString=(ClCharT   *)clHeapAllocate((MAXSIZE_SINGLE_FAULT_RECORD*sNumberRecords)*sizeof(ClCharT));
	  if(gHistoryString == NULL)
	  {
	      CL_DEBUG_PRINT(CL_DEBUG_ERROR,( "\nMemory Allocation failed\n"));
          clFaultHistoryDataUnLock();
		  return CL_ERR_NO_MEMORY;
	  }
   }
   if ((rc = clClistWalk(sfaultHistoryList,
                   (ClClistWalkCallbackT)clFaultHistoryRecordShow,
                   (void *)moid )) != CL_OK)
   {
      CL_DEBUG_PRINT(CL_DEBUG_ERROR, (MODULE_STR "clClistWalk failed\
                                          rc => [0x%x]", rc));
      clFaultHistoryDataUnLock();
      CL_FUNC_EXIT();
      return (rc);
   }

   if ( (rc = clFaultHistoryDataUnLock()) != CL_OK )
   {
      CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("clFaultHistoryShow, not able to obtain history data\
                               LOCK .... cannot proceed rc: 0x%x \n", rc));
   }

   CL_FUNC_EXIT();
   return (rc);
}
ClRcT 
clFaultHistoryDataLock(){

   ClRcT rc = CL_OK;

   if (shfaultHistoryMutex != 0)
   {
      rc = clOsalMutexLock(shfaultHistoryMutex);
   }
   else
   {
      CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("FM: clFaultHistoryDataLock, Mutex is not yet \
                               created .... \n"));
       return CL_ERR_MUTEX_ERROR;
   }
   return rc;
}

ClRcT 
clFaultHistoryDataUnLock(){
    
   ClRcT rc = CL_OK;
                                              
   if (shfaultHistoryMutex != 0)
   {
      rc = clOsalMutexUnlock(shfaultHistoryMutex);
   }
   else
   {
      CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("FM: clFaultHistoryDataUnLock, Mutex is\
                        not yet created .... \n"));
      return CL_ERR_MUTEX_ERROR;
   }
   return rc;
}
