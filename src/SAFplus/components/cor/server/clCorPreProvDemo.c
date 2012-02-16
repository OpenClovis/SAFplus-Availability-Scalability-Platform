/*
 * Copyright (C) 2002-2012 OpenClovis Solutions Inc.  All Rights Reserved.
 *
 * This file is available  under  a  commercial  license  from  the
 * copyright  holder or the GNU General Public License Version 2.0.
 * 
 * The source code for  this program is not published  or otherwise 
 * divested of  its trade secrets, irrespective  of  what  has been 
 * deposited with the U.S. Copyright office.
 * 
 * This program is distributed in the  hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied  warranty  of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 * 
 * For more  information, see  the file  COPYING provided with this
 * material.
 */
/*******************************************************************************
 * ModuleName  : cor
 * File        : clCorPreProvDemo.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This module contains COR's persistecy related functions.
 *****************************************************************************/

#include <string.h>
#include <clCommon.h>
#include <clDebugApi.h>
#include <clLogApi.h>
#include <clCorMetaData.h>
#include <clCorUtilityApi.h>
#include <clCorErrors.h>
#include <clCommonErrors.h>
#include <clTimerApi.h>
#include <clOsalApi.h>
#include <clDbalApi.h>


/* Internal Headers */
#include "clCorDmDefs.h"
#include "clCorDmProtoType.h"
#include "clCorUtilsProtoType.h"
#include "clCorTreeDefs.h"
#include "clCorRmDefs.h"
#include "clCorNiIpi.h"
#include "clCorClient.h"
#include "clCorSync.h"
#include "clCorLog.h"
#include "clCorPvt.h"
#include "clCorDeltaSave.h"
#include "clCorRMDWrap.h"

#include <xdrCorPersisInfo_t.h>

#ifdef MORE_CODE_COVERAGE
#include "clCodeCovStub.h"
#endif

#define MAX_VAL_SIZE 256

#define COR_DATA_STORE    1
#define COR_DATA_RESTORE  2

extern _ClCorServerMutexT gCorMutexes;
extern ClUint32T	gClCorSaveType;
extern ClCorInitStageT   gCorInitStage;
extern ClCharT gClCorPersistDBPath[];
extern ClCharT gClCorMetaDataFileName[];
extern ClUint32T gCorSlaveSyncUpDone;

extern ClVersionT gCorDbVersionT[];
extern ClVersionDatabaseT gCorDbVersionDb; 

/*  Static Variables for now. Very bad to  have these. Remove them ASAP*/

static ClTimerHandleT     sPersTmr =0 ;  /* Timer Handle */
static char  *sPersFileName = NULL;
char topPath[50];

/* Declaration of local functions */
ClRcT
corDataStore(ClCorPersDataTypeT    type,
             ClDBHandleT          hTlv,
             ClDBHandleT          hData);

ClRcT 
corDataReStore(ClCorPersDataTypeT    type, 
               ClDBHandleT          hTlv,
               ClDBHandleT          hData);

ClRcT 
corPersDataOp(ClUint32T  op);

/* Timer Call back */
ClRcT
corPersTmrExpHdl(void *);
#ifdef DEBUG
ClRcT corPreProvDbgInit()
{
    ClRcT ret= CL_OK ;
    ret= dbgAddComponent(COMP_PREFIX, COMP_NAME, COMP_DEBUG_VAR_PTR);
    if (CL_OK != ret)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("dbgAddComponent Failed \n "));
        CL_FUNC_EXIT();
    }
    return ret;
}
#endif

/********************************************************************
   A P Is     E X P O S E D      T O     U S E R S
********************************************************************/
/*
 *    Function to Store/Save the necessary COR related information
 *
 *    This function stores COR related information(Classes information, 
 *    MO Tree structure, Object tree structure for the moment) into a 
 *    data base. The name of the data base is user supplied. It uses
 *    two databases one to store the actual data. The other data base
 *    is to store the information about the data like type and length.
 */
ClRcT  _clCorDataSave()
{
	if(gClCorSaveType != CL_COR_DELTA_SAVE)
	{
		CL_DEBUG_PRINT(CL_DEBUG_TRACE,("Delta Save option is not enabled"));
		return CL_OK;
	}
    return (corPersDataOp(COR_DATA_STORE));
}

/* Restore provisioning data */
ClRcT _clCorDataRestore()
{
    return (corPersDataOp(COR_DATA_RESTORE));
}

/**
 *       Function to save the COR data at a specified frequency 
 */
ClRcT _clCorDataFrequentSave(char  *fileName, ClTimerTimeOutT  frequency )
{
    ClRcT    rc = CL_OK;

    if (sPersTmr != 0)
    {
        /* Timer is already created   */
        return rc;
    }
    if( NULL == fileName )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "NULL File name as input"));
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);
    }
    if (NULL == (sPersFileName = (char *)clHeapAllocate(strlen(fileName) + 
                                                   1)))/* For Null termination*/
    {
	clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL, 
				CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "No Memory"));
        return CL_COR_SET_RC(CL_COR_ERR_NO_MEM);
    }
    memset(sPersFileName, '\0', (strlen(fileName) + 1));
    strcpy(sPersFileName, fileName);
    /* Create a timer and start it */
    if (CL_OK != (rc = clTimerCreate (frequency,
                                         CL_TIMER_REPETITIVE,
                                         CL_TIMER_SEPARATE_CONTEXT,
                                         corPersTmrExpHdl, 
                                         (void*)sPersFileName,
                                         &sPersTmr)))
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Failed to create timer"));
        return rc;
    }

    /* Start the timer */
    if (CL_OK != (rc = clTimerStart(sPersTmr)))
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Failed to start timer"));
        return rc;
    }
    return CL_OK;
}

ClRcT _clCorDataFrequentSaveStop()
{
    ClRcT    rc = CL_OK;
    
    if (sPersTmr != 0) 
    {
        if (CL_OK != (rc = clTimerDelete(&sPersTmr)))
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Failed to Delete timer"));
            return rc;
        }
    }
    if (sPersFileName) 
        clHeapFree(sPersFileName);
    return CL_OK;
}
/*************************************************************
    U T I L I T Y    R O U T I N E S 
*************************************************************/

/* COR Peristent Data store/Restore */
ClRcT corPersDataOp(ClUint32T  op)
{
     ClDBHandleT    hTlv = 0;  /* Db handle for storing TLVs */
     ClDBHandleT    hData = 0; /* Db handle for storing persistent data */
     char           *dataFile = NULL; /* Filename for storing actual data */
     ClDBTypeT      dbFlag = CL_DB_CREAT;
     ClRcT          rc = CL_OK; /* Return code */
     ClUint32T      fileNameSize = 0;    

	/* Need to put the lock here */	
	clOsalMutexLock(gCorMutexes.gCorServerMutex);

     CL_FUNC_ENTER();

     fileNameSize = strlen (gClCorPersistDBPath) + strlen("/") + 
                        strlen (gClCorMetaDataFileName) + strlen(".nv") +1 ;

     /* File name should be NUUL terminated string */
     if (NULL == (dataFile = clHeapAllocate ( fileNameSize )))
     {
		clOsalMutexUnlock(gCorMutexes.gCorServerMutex);
        clLogError("PRS", "COM", CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
        return CL_COR_SET_RC(CL_COR_ERR_NO_MEM);
     }
     
     memset(dataFile, 0, (fileNameSize));
     strcpy(dataFile, gClCorPersistDBPath);
     strcat(dataFile, "/");
     strcat(dataFile, gClCorMetaDataFileName);

     clLogInfo("PRS", "CLS", 
             "The DB file name to store class information: [%s]",
             dataFile);
	
     if (op == COR_DATA_RESTORE )
     {
        FILE* file = NULL;

        dbFlag = CL_DB_OPEN;

        file = fopen(dataFile, "rb");
        if (!file)   
        {
            /* Kludge for now!!! DBAL Should support Readonly mode */
            clLogNotice("PRS", "RES", 
                    "Failed while restoring the Db[%s] as it doesn't exist", dataFile);
            clHeapFree(dataFile);
		    clOsalMutexUnlock(gCorMutexes.gCorServerMutex);
            return CL_COR_SET_RC(CL_ERR_NOT_EXIST);
        }

        fclose(file);
     }

     /* Open the DB to store TLV info */
     if (CL_OK !=  (rc  =  clDbalOpen( dataFile, dataFile, dbFlag, 50*1024, 50*1024, &hTlv)))
     {
		clOsalMutexUnlock(gCorMutexes.gCorServerMutex);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Failed to Create DB [RC %x]", rc));
        clHeapFree(dataFile);
        return CL_COR_SET_RC(CL_ERR_NOT_EXIST);
     }

     strcat(dataFile, ".nv");
     
     clLogInfo("PRS", "CLS", 
             "The DB file name to store class name value details: [%s]", dataFile);

     /* Open the DB to store actual data */
     if (CL_OK !=  (rc  =  clDbalOpen( dataFile, dataFile, dbFlag, 50*1024, 50*1024, &hData)))
     {
		clOsalMutexUnlock(gCorMutexes.gCorServerMutex);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Failed to Create DB"));
        clHeapFree(dataFile);
        return rc;
     }
    
     /* Free the datafile header */
     clHeapFree(dataFile);

     if (op == COR_DATA_STORE )
     {
         /* Store the Class information */
         if (CL_OK != (rc = corDataStore(CL_COR_PERS_CLASS_DATA, hTlv, hData)))
         {
			clOsalMutexUnlock(gCorMutexes.gCorServerMutex);
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Failed to Write Class DB"));
            return rc;
         }

         /* Store the Class relationship information */
         if (CL_OK != (rc = corDataStore(CL_COR_PERS_HIER_DATA, hTlv, hData)))
         {
			clOsalMutexUnlock(gCorMutexes.gCorServerMutex);
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Failed to Write MO Tree DB"));
            return rc;
         }

         /* Restore the NI table Configuration */
         if (CL_OK != (rc = corDataStore(CL_COR_PERS_NI_DATA, hTlv, hData)))
         {
			 clOsalMutexUnlock(gCorMutexes.gCorServerMutex);
             CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Failed to Write NI DB"));
             return rc;
         }
     }
     else
     {
         /* Restore the Class information */
         clLogInfo("RES", "CLS", "Restore the class information");
         if (CL_OK != (rc =corDataReStore(CL_COR_PERS_CLASS_DATA, hTlv, hData)))
         {
			clOsalMutexUnlock(gCorMutexes.gCorServerMutex);
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Failed to Read Class info DB"));
            return rc;
         }

         clLogInfo("RES", "CLS", "Restore the class tree information");
         /* Restore the Class relationship information */
         if (CL_OK != (rc = corDataReStore(CL_COR_PERS_HIER_DATA, hTlv, hData)))
         {
			clOsalMutexUnlock(gCorMutexes.gCorServerMutex);
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Failed to Read MO Tree DB"));
            return rc;
         }

         clLogInfo("RES", "CLS", "Restore the NI table information");
         /* Restore the NI Table Configuration */
         if (CL_OK != (rc = corDataReStore(CL_COR_PERS_NI_DATA, hTlv, hData)))
         {
			 clOsalMutexUnlock(gCorMutexes.gCorServerMutex);
             CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Failed to Read RM DB"));
             return rc;
         }
     }

     if (CL_OK != (rc = clDbalClose(hTlv)))
     {
	    clOsalMutexUnlock(gCorMutexes.gCorServerMutex);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Failed to Close DB"));
        return rc;
     }

     if (CL_OK != (rc = clDbalClose(hData)))
     {
	    clOsalMutexUnlock(gCorMutexes.gCorServerMutex);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Failed to Close DB"));
        return rc;
     }

	 clOsalMutexUnlock(gCorMutexes.gCorServerMutex);
     return rc;
}

int
corMOTreePackProv( void **   pBuf)
{
    ClUint32T   sz = COR_MOTREE_BUF_SZ, moTreeSize = 0;
    ClBufferHandleT moTreeBuffer = 0;
    ClRcT   rc = CL_OK;

    /* Sanity checks */    
    if (pBuf == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\nNull pointer passed"));
        return -1;
    }
    rc = clBufferCreate(&moTreeBuffer);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while creating the buffer for packing the MOTree. rc[0x%x]", rc));
        return -1;
    }

    if (CL_OK != (rc =corMOTreePack(NULL, &moTreeBuffer)))
    {
        clBufferDelete(&moTreeBuffer);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\nFailed to pack the MO tree"));
        return -1;
    }

    rc = clBufferLengthGet(moTreeBuffer, &moTreeSize);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while getting the size of the packed data. rc[0%x]", rc));
        clBufferDelete(&moTreeBuffer);
        return -1;
    }

    rc = clBufferFlatten(moTreeBuffer, (ClUint8T **)pBuf);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while doing the flatten, rc[0x%x]\n", rc));
        clBufferDelete(&moTreeBuffer);
        return -1;
    }

    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Packing the buffer %d, moBufSize %d\n", moTreeSize, sz));

    clBufferDelete(&moTreeBuffer);
    return moTreeSize;
}

int
corObjTreePackProv (void **   pBuf)
{
    ClUint32T           bytesWrite = COR_OBJTREE_BUF_SZ, objTreeSize = 0;
    ClBufferHandleT     objTreeBuffer = 0;
    ClRcT               rc = CL_OK;

    /* Sanity checks */    
    if (pBuf == NULL)
    {
         CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\nNull pointer passed"));
         return -1;
    }
    rc = clBufferCreate(&objTreeBuffer);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while creating the buffer for packing the MOTree. rc[0x%x]", rc));
        return rc;
    }

    if (CL_OK != corObjTreePack(NULL, CL_COR_OBJ_PERSIST, &objTreeBuffer))
    {
        clBufferDelete(&objTreeBuffer);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\nFailed to Pack the Object tree\n"));
        return -1;
    }

    rc = clBufferLengthGet(objTreeBuffer, &objTreeSize);
    if(CL_OK != rc)
    {
        clBufferDelete(&objTreeBuffer);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while getting the size of the packed data. rc[0%x]", rc));
        return -1;
    }

    rc = clBufferFlatten(objTreeBuffer, (ClUint8T **)pBuf);
    if(CL_OK != rc)
    {
        clBufferDelete(&objTreeBuffer);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while doing the flatten, rc[0x%x]\n", rc));
        return -1;
    }

    clBufferDelete(&objTreeBuffer);
    return bytesWrite;
}

/* Write the given type of data into the DB*/
ClRcT 
corDataStore(ClCorPersDataTypeT    type, 
             ClDBHandleT          hTlv,
             ClDBHandleT          hData)
{
    ClCorPersDataTlvT  persTlv;     /*  TLV that gets stored in the DB*/
    void *            pData = NULL;       /* Actual data to be preserved */
    ClRcT            rc = CL_OK; /* Return code  */
    ClVersionT          version = {0};
    ClBufferHandleT bufHandle = {0};

    memset(&persTlv, '\0', sizeof(persTlv));
    persTlv.type = type;

    version.releaseCode = CL_RELEASE_VERSION;
    version.majorVersion = CL_MAJOR_VERSION;
    version.minorVersion = CL_MINOR_VERSION;

    switch (type)
    {
       case CL_COR_PERS_CLASS_DATA:
           {
               
               clLog(CL_LOG_SEV_DEBUG, "PKG", "DCP", "Packing the DM Class Information. Version [0x%x.0x%x.0x%x]",
                       version.releaseCode, version.majorVersion, version.minorVersion);

               rc = clBufferCreate(&bufHandle);
               if(CL_OK != rc)
               {
                    clLog(CL_LOG_SEV_ERROR, "PKG", "DCP", "Failed while creating the buffer. rc[0x%x]", rc);
                    return rc;
               }

               rc = clXdrMarshallClVersionT((void *) &version, bufHandle, 0);
               if (rc != CL_OK)
               {
                   clBufferDelete(&bufHandle);
                   clLogError("PKG","DCP", "Failed to marshall the version information. rc [0x%x]", rc);
                    return rc;
               }

               rc = dmClassesPack( &bufHandle );
               if(rc != CL_OK)
               {
                   clBufferDelete(&bufHandle);
                   clLog(CL_LOG_SEV_ERROR, "PKG", "DCP", "Failed while packing the dmClass information. rc[0x%x]", rc);
                   return rc;
               }

               rc = clBufferLengthGet(bufHandle, (ClUint32T *)&persTlv.len);
               if(CL_OK != rc)
               {
                   clBufferDelete(&bufHandle);
                   clLog(CL_LOG_SEV_ERROR, "PKG", "DCP", "Failed while getting the length of \
                               class-data from buffer. rc[0x%x]", rc);
                   return rc;
               }
               
                clLog(CL_LOG_SEV_DEBUG, "PKG", "DCP", " Classes packed is of length [%d]", persTlv.len);

                if(persTlv.len <= 0)
                {   
                    clBufferDelete(&bufHandle);
                    clLog(CL_LOG_SEV_ERROR, "PKG", "DCP", "Invalid size of the class buffer packed.");
                    return CL_COR_SET_RC(CL_COR_ERR_INVALID_SIZE);
                }

                /* Flatten the buffer to the memory. */
                rc = clBufferFlatten(bufHandle, (ClUint8T **)&pData);
                if(CL_OK != rc)
                {
                    clBufferDelete(&bufHandle);
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while flattening the class-data. rc[0x%x]", rc));
                    return rc;
                } 

                clBufferDelete(&bufHandle);
           }
           break;
       case CL_COR_PERS_HIER_DATA:    
           {
               clLog(CL_LOG_SEV_DEBUG, "PKG", "MCP", " Packing the MO class tree informtion. Version [0x%x.0x%x.0x%x]",
                       version.releaseCode, version.majorVersion, version.minorVersion);

               rc = clBufferCreate(&bufHandle);
               if (rc != CL_OK)
               {
                   clLogError("PKG", "MCP", "Failed to create the buffer. rc [0x%x]", rc);
                   return rc;
               }

               rc = clXdrMarshallClVersionT((void *)&version, bufHandle, 0);
               if (rc != CL_OK)
               {
                   clLogError("PKG", "MCP", "Failed to marshall ClVersionT. rc [0x%x]", rc);
                   clBufferDelete(&bufHandle);
                   return rc;
               }

               rc = corMOTreePack(NULL, &bufHandle );
               if (rc != CL_OK)
               {
                   clLogError("PKG", "MCP", "Failed to pack the MO Tree. rc [0x%x]", rc);
                   clBufferDelete(&bufHandle);
                   return rc;
               }

               rc = clBufferLengthGet(bufHandle, (ClUint32T *)&persTlv.len);
               if(CL_OK != rc)
               {
                   clBufferDelete(&bufHandle);
                   clLog(CL_LOG_SEV_ERROR, "PKG", "DCP", "Failed while getting the length of \
                               class-data from buffer. rc[0x%x]", rc);
                   return rc;
               }

               clLog(CL_LOG_SEV_DEBUG, "PKG", "MCP", " MO Class Tree packed: Len [%d] ", persTlv.len);

               /* Check the fortunes */
               if (persTlv.len == -1 )
               {
                   /* Memory is already corrupted */
                   clLog(CL_LOG_SEV_ERROR, "PKG", "MCP", " Invalid MO class tree packing. ");
                   return CL_COR_SET_RC(CL_COR_ERR_NO_MEM);
               }

                /* Flatten the buffer to the memory. */
                rc = clBufferFlatten(bufHandle, (ClUint8T **)&pData);
                if(CL_OK != rc)
                {
                    clBufferDelete(&bufHandle);
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while flattening the class-data. rc[0x%x]", rc));
                    return rc;
                } 

                clBufferDelete(&bufHandle);
           }
           break;
       case CL_COR_PERS_OBJ_DATA:  
           {
               clLog(CL_LOG_SEV_DEBUG, "PKG", "OTP", " Packing the MO tree informtion. ");

               persTlv.len = corObjTreePackProv(&pData);

               /* Check the fortunes */
               clLog(CL_LOG_SEV_DEBUG, "PKG", "OTP", " Object Tree packed: Len [%d] ", persTlv.len);

               if (persTlv.len == -1)
               {
                   /* Memory is already corrupted */
                   clLog(CL_LOG_SEV_ERROR, "PKG", "OTP", " Invalid Object tree packing. ");
                   return CL_COR_SET_RC(CL_COR_ERR_NO_MEM);
               }
           }
           break;

       case CL_COR_PERS_RM_DATA:
           {
               ClBufferHandleT rmBufferHandle = 0;

                rc = clBufferCreate(&rmBufferHandle);
               if(rc != CL_OK)
               {
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed while creating the buffer Handle. rc[0x%x]", rc));
                    return rc;
               }
                
               rc = rmRoutePack(&rmBufferHandle);

               /* Check the size */
               if (CL_OK != rc)
               {
                   /* Memory is already corrupted */
                   CL_DEBUG_PRINT(CL_DEBUG_CRITICAL, ( "\nMemory Corrupted\n"));
                   return rc;
               }

               rc = clBufferLengthGet(rmBufferHandle, (ClUint32T *)&persTlv.len);
               if(rc != CL_OK)
               {
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while getting the route list buffer size. rc[0x%x]", rc));
                    clBufferDelete(&rmBufferHandle);
                    return rc;
               }
            
                if(persTlv.len <= 0)
                {   
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Invalid size of the route buffer packed."));
                    clBufferDelete(&rmBufferHandle);
                    return CL_COR_SET_RC(CL_COR_ERR_INVALID_SIZE);
                }

                rc = clBufferFlatten(rmBufferHandle, (ClUint8T **)&pData);
                if(CL_OK != rc)
                {
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while flattening the route list data. rc[0x%x]", rc));
                    clBufferDelete(&rmBufferHandle);
                    return rc;
                } 

                CL_DEBUG_PRINT(CL_DEBUG_TRACE, ( "RM Tree packed: Len %d",persTlv.len));
                clBufferDelete(&rmBufferHandle);
           }
           break;
       case  CL_COR_PERS_NI_DATA:
           {
               ClUint32T  size = 0; 
               void *      tmpData = NULL;

               clLogInfo("PAC", "STO", "Packing the NI table information. Version [0x%x.0x%x.0x%x]",
                       version.releaseCode, version.majorVersion, version.minorVersion);

               if (CL_OK != (rc = corNiTablePack(&tmpData, &size)))
               {
                   CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "corNiTablePack failed"));
                   return rc;
               }

               if(size == 0)
               {
                    clHeapFree(tmpData);
                    return rc;
               }
               
               rc = clBufferCreate(&bufHandle);
               if (rc != CL_OK)
               {
                   clLogError("PKG", "NIT", "Failed create the buffer. rc [0x%x]", rc);
                   clHeapFree(tmpData);
                   return rc;
               }

               rc = clXdrMarshallClVersionT((void *)&version, bufHandle, 0);
               if (rc != CL_OK)
               {
                   clLogError("PKG", "NIT", "Failed to marshall ClVersionT. rc [0x%x]", rc);
                   clHeapFree(tmpData);
                   clBufferDelete(&bufHandle);
                   return rc;
               }

               rc = clBufferNBytesWrite(bufHandle, (ClUint8T *) tmpData, size);
               if (rc != CL_OK)
               {
                   clLogError("PKG", "NIT", "Failed to do buffer write. rc [0x%x]", rc);
                   clHeapFree(tmpData);
                   clBufferDelete(&bufHandle);
                   return rc;
               }

               clHeapFree(tmpData);

               rc = clBufferLengthGet(bufHandle, (ClUint32T *)&persTlv.len);
               if(CL_OK != rc)
               {
                   clBufferDelete(&bufHandle);
                   clLog(CL_LOG_SEV_ERROR, "PKG", "DCP", "Failed while getting the length of \
                               class-data from buffer. rc[0x%x]", rc);
                   return rc;
               }

               if (persTlv.len == -1 )
               {
                   /* Memory is already corrupted */
                   clLog(CL_LOG_SEV_ERROR, "PKG", "MCP", " Invalid MO class tree packing. ");
                   clBufferDelete(&bufHandle);
                   return CL_COR_SET_RC(CL_COR_ERR_NO_MEM);
               }

               /* Flatten the buffer to the memory. */
                rc = clBufferFlatten(bufHandle, (ClUint8T **)&pData);
                if(CL_OK != rc)
                {
                    clBufferDelete(&bufHandle);
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed while flattening the class-data. rc[0x%x]", rc));
                    return rc;
                } 

                clBufferDelete(&bufHandle);

                clLogInfo("PKG", "NIT", "NI table information packed successfully. size [%d]", persTlv.len);
           }
           break;
       default :
           break;
    }

    if (CL_OK != (rc = clDbalRecordInsert(hTlv,
                                       (ClDBKeyT) &persTlv.type,
                                       sizeof(persTlv.type),
                                       (ClDBRecordT)&persTlv,
                                       sizeof(persTlv))))
    {
         CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "DB Record addition failed for MO Tree data"));
         clHeapFree(pData);
         return rc;
    }
    
    if (CL_OK != (rc = clDbalRecordInsert(hData,
                                       (ClDBKeyT) &persTlv.type,
                                       sizeof(persTlv.type),
                                       (ClDBRecordT)pData,
                                       persTlv.len)))
    {
         CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "DB Record addition failed to add actual data"));
         clHeapFree(pData);
         return rc;
    }

    clHeapFree(pData);
    return CL_OK;
}

/* Restore given type of provisioning data */
ClRcT 
corDataReStore(ClCorPersDataTypeT    type, 
               ClDBHandleT          hTlv,
               ClDBHandleT          hData)
{
    ClCorPersDataTlvT *persTlv;     /*  TLV that was stored in the DB*/
    void *            pData = NULL;/* Actual data to be preserved */
    void *           pTempData = NULL;
    ClRcT            rc = CL_OK;
    ClUint32T        size =0;
    ClVersionT       version = {0};
    ClVersionT       dbVersion = {0};
    ClUint32T        temp = 0;

    memset(&persTlv, '\0', sizeof(persTlv));
    /* Read the TLV record */
    if (CL_OK != (rc = clDbalRecordGet(hTlv,
                                       (ClDBKeyT)&type,
                                       sizeof(type),
                                       (ClDBRecordT *)&persTlv,
                                       &size)))
    {
        /*There is no data in the db.*/
        if (size == 0)
            return CL_OK;
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Failed to read DB %x", rc));
        return rc;
    }

    /* Read the Data record */
    if (CL_OK != (rc = clDbalRecordGet(hData,
                                       (ClDBKeyT)&type,
                                       sizeof(type),
                                       (ClDBRecordT *)&pData,
                                       &size)))
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Failed to read DB"));
        clDbalRecordFree(hTlv, (ClDBRecordT)persTlv);
        return rc;
    }
   
    pTempData = pData;

    /* Release code */
    temp = *(ClUint32T *) pData;
    version.releaseCode = (ClUint8T) ntohl(temp);

    pData = (ClUint8T *) pData + sizeof(ClUint32T);
    size = size - sizeof(ClUint32T);

    /* Major Version */
    temp = *(ClUint32T *) pData;
    version.majorVersion = (ClUint8T) ntohl(temp);

    pData = (ClUint8T *) pData + sizeof(ClUint32T);
    size = size - sizeof(ClUint32T);

    /* Minor version */
    temp = *(ClUint32T *) pData;
    version.minorVersion = (ClUint8T) ntohl(temp);

    pData = (ClUint8T *) pData + sizeof(ClUint32T);
    size = size - sizeof(ClUint32T);

    dbVersion = version;

    clCorDbVersionValidate(version, rc);
    if (rc != CL_OK)
    {
        clLogError("DB", "INT", "Cor persistent db version is unsupported. "
                "Version of the db is [0x%x.0x%x.0x%x], but the max version supported is [0x%x.0x%x.0x%x]",
                dbVersion.releaseCode, dbVersion.majorVersion, dbVersion.minorVersion,
                version.releaseCode, version.majorVersion, version.minorVersion);
		return CL_COR_SET_RC(CL_COR_ERR_VERSION_UNSUPPORTED);
    }

    switch (type)
    {
        case CL_COR_PERS_CLASS_DATA:
            {
                clLogInfo("PER", "RES", "COR Class Db version restored [0x%x.0x%x.0x%x].",
                        version.releaseCode, version.majorVersion, version.minorVersion);

                if (CL_OK != (rc = dmClassesUnpack( pData, size)))
                {
                    clLogError("PER", "RES", "Failed to Unpack Classes data. rc [0x%x]", rc);
                }
                
                clLogInfo("PER", "RES", "Unpacked the class information successfully from the db.");
            } 
            break;
        case CL_COR_PERS_HIER_DATA:
            {
                ClUint32T sz = size;  /* temp var to note size unpacked */
                
                clLogInfo("PER", "RES", "COR MO class tree db version restored [0x%x.0x%x.0x%x].",
                        version.releaseCode, version.majorVersion, version.minorVersion);

                /* unpack to the root of the MOTree */
                if (CL_OK != (rc = corMOTreeUnpack( 0, (void *) pData, &sz)))
                {
                    clLogError("PER", "RES", "Failed to Unpack the mo tree information. rc [0x%x]", rc);
                }
                
                clLogInfo("PER", "RES", "Unpacked the class tree information successfully from the db.");
            } 
            break;
        case CL_COR_PERS_NI_DATA:
            {
                clLogInfo("PER", "RES", "COR Class NI table db version restored [0x%x.0x%x.0x%x].",
                        version.releaseCode, version.majorVersion, version.minorVersion);

                if (CL_OK != (rc = corNiTableUnPack((void *) pData, size)))
                {
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Failed to Unpack NI data"));
                }

                clLogInfo("PER", "RES", "Unpacked the NI table information successfully from the db.");
            }
            break;
        default :
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Unknown Record"));
            break;
    }

    clDbalRecordFree(hTlv, (ClDBRecordT)persTlv);
    clDbalRecordFree(hData, (ClDBRecordT)pTempData);

    return (rc);
}

/* Timer Expiry handler */
ClRcT 
corPersTmrExpHdl(void *fileName)
{
    return CL_OK;
}


ClRcT
VDECL(_corPersisOp) (ClEoDataT cData, ClBufferHandleT  inMsgHandle,
                                  ClBufferHandleT  outMsgHandle)
{
    ClRcT rc = CL_OK;
    corPersisInfo_t* pData = NULL;
   
    CL_FUNC_ENTER();

    clLogTrace("PRS", "EOF", "Inside the cor data store ");

    if(gCorInitStage == CL_COR_INIT_INCOMPLETE)
    {
        clLogError("PRS", "EOF", "The COR server Initialization is in progress....");
        return CL_COR_SET_RC(CL_COR_ERR_TRY_AGAIN);
    }

    pData = clHeapAllocate(sizeof(corPersisInfo_t));
    if(pData == NULL)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "NULL input parameter") );
        return CL_COR_SET_RC(CL_COR_ERR_NULL_PTR);   
    }
	if((rc = VDECL_VER(clXdrUnmarshallcorPersisInfo_t, 4, 0, 0)(inMsgHandle, (void *)pData)) != CL_OK)
	{
		clHeapFree(pData);
		CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Failed to Unmarshall corPersisInfo_t"));
		return rc;
	}
    /* if((rc = clBufferFlatten(inMsgHandle, (ClUint8T **)&pData))!= CL_OK)
    {
         CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Failed to flatten the Message"));
         CL_FUNC_EXIT();
         return rc;
    }*/


    /*if(pData->version > CL_COR_VERSION_NO)
    {
		clHeapFree(pData);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "Version mismatch"));
        CL_FUNC_EXIT();
        return CL_ERR_VERSION_MISMATCH;
    }*/

	clCorClientToServerVersionValidate(pData->version, rc);
    if(rc != CL_OK)
	{
		clHeapFree(pData);	
		return CL_COR_SET_RC(CL_COR_ERR_VERSION_UNSUPPORTED); 
	}

    switch(pData->operation)
    {
        case COR_DATA_SAVE:
            rc = _clCorDataSave();
        break;
        case COR_DATA_RESTORE:
            rc = _clCorDataRestore();
        break;
        case COR_DATA_FREQ_SAVE:
            rc = _clCorDataFrequentSave(gClCorMetaDataFileName, pData->frequency);
        break;
        case COR_DATA_FREQ_SAVE_STOP:
            rc = _clCorDataFrequentSaveStop();
        break;
        default:
                rc = CL_COR_SET_RC(CL_COR_ERR_INVALID_PARAM);
        break;
    }

    if((rc != CL_OK) && (rc != CL_ERR_NOT_EXIST))
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,( "_corPersisOp returned error,rc=%x",rc) );
    }

	clHeapFree(pData);

    if (gCorSlaveSyncUpDone == CL_TRUE)
    {
        ClRcT retCode = CL_OK;
        retCode = clCorSyncDataWithSlave(COR_EO_PERSISTENCY_OP, inMsgHandle);
        if (retCode != CL_OK)
        {
            clLogError("SYNC", "", "Failed to Sync data with slave COR. rc [0x%x]", rc);
        }

        /* Ignore the error code returned. */
    }

    CL_FUNC_EXIT();
    return rc;
}
