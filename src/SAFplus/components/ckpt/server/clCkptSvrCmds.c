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
 * ModuleName  : ckpt                                                          
 * File        : clCkptSvrCmds.c
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
*
*   This file contains Checkpoint server debug CLI commands implementations
*
*
*****************************************************************************/
/* System includes */
#include <string.h>
#include <sys/time.h>

/* Clovis Common includes */
#include <clCommon.h>
#include <clCntApi.h>
#include <clDebugApi.h>
#include <clLogApi.h>
#include <clEoApi.h>
#include <clVersionApi.h>

/* Ckpt specific includes */
#include "clCkptSvr.h"
#include "clCkptUtils.h"
#include "clCkptSvrIpi.h"
#include "clCkptApi.h"
#include "clCkptPeer.h"
#include "clCkptMaster.h"
#include <ipi/clHandleIpi.h>
#include <ckptEockptServerPeerPeerClient.h>

#define CL_CKPT_CLI_ENTRY_LEN    20
extern CkptSvrCbT  *gCkptSvr;
ClHandleT  gCkptDebugReg = CL_HANDLE_INVALID_VALUE;

/**============================================**/
/**   Strictly local to this file              **/
/**============================================**/
/* Function to display the checkpoints */
static ClRcT ckptNameShow( 
        ClCntKeyHandleT     key,
        ClCntDataHandleT    pName,
        void                *dummy,
        ClUint32T           dataLength);


/* Function to dispay preferred nodes information */
static ClRcT  ckptPeerShow (
        ClCntKeyHandleT     key,
        ClCntDataHandleT    pData,
        void                *dummy,
        ClUint32T           dataLength);

/* This routine displays the control plane information of a ckpt */
static ClRcT  ckptCPlaneInfoShow(
        CkptCPlaneInfoT   *pCpInfo,
        ClDebugPrintHandleT msg,
        ClInt64T           time);

/* This routine displays the control plane information of a ckpt */
static ClRcT  ckptDPlaneInfoShow(
        CkptDPlaneInfoT   *pDpInfo,
        ClDebugPrintHandleT msg);

/* Function to display a single checkpoint */
static ClRcT  ckptSingleEntryShow(
        SaNameT    *pName,
        ClDebugPrintHandleT msg);

/* Routine to display the presence list */
static ClRcT  ckptPresenceNodeShow (
        ClCntKeyHandleT    key,
        ClCntDataHandleT   pData,
        void               *dummy,
        ClUint32T          dataLength);


/* Routine to display the peer nodes */
static ClRcT ckptPeerListShow (
        ClDebugPrintHandleT msg);



static ClRcT ckptCliCkptShow(
        int          argc, 
        char         **argv ,
        ClCharT      **ret);

static ClRcT ckptCliPeerListShow(
        int          argc, 
        char         **argv ,
        ClCharT      **ret);
static ClRcT ckptCliCkptInit(
        int          argc, 
        char         **argv ,
        ClCharT      **ret);
static ClRcT ckptCliCkptOpen(
        int          argc, 
        char         **argv ,
        ClCharT      **ret);

static ClRcT ckptCliSectionDelete(
        int          argc, 
        char         **argv ,
        ClCharT      **ret);

static ClRcT ckptCliCkptWrite(
        int          argc, 
        char         **argv ,
        ClCharT      **ret);

static ClRcT ckptCliCkptClose(
        int          argc, 
        char         **argv ,
        ClCharT      **ret);

static ClRcT ckptCliCkptStatusGet(
        int          argc, 
        char         **argv ,
        ClCharT      **ret);

static ClRcT  ckptCliCkptRead(
        int          argc, 
        char         **argv ,
        ClCharT      **ret);

static ClRcT  ckptCliSectionCreate(
        int          argc, 
        char         **argv ,
        ClCharT      **ret);
static ClRcT  ckptCliCkptUnlink(
        int          argc, 
        char         **argv ,
        ClCharT      **ret);

static ClRcT  ckptCliSectionOverwrite(
        int          argc, 
        char         **argv ,
        ClCharT      **ret);

static ClRcT  ckptCliSectionExpirationTimeSet(
        int          argc, 
        char         **argv ,
        ClCharT      **ret);

static ClRcT  ckptCliActiveReplicaSet(
        int          argc, 
        char         **argv ,
        ClCharT      **ret);

static ClRcT  ckptCliCkptRetentionDurationSet(
        int          argc, 
        char         **argv ,
        ClCharT      **ret);

static ClRcT ckptCliIterInit( int argc ,
                       char **argv,
                       ClCharT** ret);
static ClRcT ckptCliIterNext(
        int          argc, 
        char         **argv ,
        ClCharT      **ret);
static ClRcT ckptCliIterFin(
        int          argc, 
        char         **argv ,
        ClCharT      **ret);
static ClRcT ckptCliSync(
        int          argc, 
        char         **argv ,
        ClCharT      **ret);
static ClRcT ckptCliClientFinalize(
        int          argc, 
        char         **argv ,
        ClCharT      **ret);
static ClRcT ckptCliDbShow(int argc,
                    char **argv,
                    ClCharT** ret);
static ClDebugFuncEntryT ckptDebugFuncList[] = 
{
    {
        (ClDebugCallbackT)ckptCliCkptInit, 
        "Init", 
        "Initialize the Checkpoint service"
    },
    {
        (ClDebugCallbackT)ckptCliCkptShow, 
        "ckptShow",
        "Display the Checkpoint information" },
    {
        (ClDebugCallbackT) ckptCliPeerListShow, 
        "ckptPeerShow",
        "Display the list of peer Checkpoint Servers"
    },
    {
        (ClDebugCallbackT)ckptCliCkptOpen,
        "ckptOpen",  
        "Open the Checkpoint"
    },
    {
        (ClDebugCallbackT)ckptCliCkptUnlink,
        "ckptDelete",
        "Delete the Checkpoint "
    },
    {
        (ClDebugCallbackT)ckptCliCkptClose,
        "ckptClose", 
        "Close the Checkpoint "
    },
    {
        (ClDebugCallbackT)ckptCliCkptWrite,
        "ckptWrite",
        "Write the checkpoint data"
    },
    {
        (ClDebugCallbackT)ckptCliCkptRead, 
        "ckptRead",
        "Read the checkpoint data"
    },
    {
        (ClDebugCallbackT)ckptCliSectionCreate,
        "ckptSectionCreate",
        "Create the section in the Checkpoint"
    },
    {
        (ClDebugCallbackT)ckptCliSectionDelete,
        "ckptSectionDelete",
        "Delete the section in the Checkpoint"
    },
    {
        (ClDebugCallbackT)ckptCliSectionOverwrite,
        "ckptSectionOverwrite", 
        "Overwrite the section in the Checkpoint"
    },
    {
        (ClDebugCallbackT)ckptCliSectionExpirationTimeSet,
        "ckptSectionExpirationTimeSet", 
        "Set the expiration time of a section in the Checkpoint"
    },
    {
        (ClDebugCallbackT)ckptCliCkptStatusGet,
        "ckptStatusGet",  
        "Show the status of the Checkpoint"
    },
    {
        (ClDebugCallbackT)ckptCliActiveReplicaSet,
        "ckptActiveReplicaSet",  
        "Set the local node as Active replica for the Checkpoint "
    },
    {
        (ClDebugCallbackT)ckptCliCkptRetentionDurationSet,
        "ckptRetentionDurationSet",  
        "Update the retention duration of the Checkpoint"
    },
    {
        (ClDebugCallbackT)ckptCliIterInit,
        "ckptIterInit", 
        "Initialize the section iterator"
    },
    {
        (ClDebugCallbackT)ckptCliIterNext,
        "ckptIterNext", 
        "Get the next section from iterator"
    },
    {
        (ClDebugCallbackT)ckptCliIterFin,
        "ckptIterFin", 
        "Finalize the iterator"
    },
    {
        (ClDebugCallbackT)ckptCliSync,
        "ckptSync", 
        "Synchronize all the replicas of the Checkpoint"
    },
    {
        (ClDebugCallbackT)ckptCliClientFinalize,
        "ckptClientFin", 
        "Finalize the Checkpoint service"
    },
    {
        (ClDebugCallbackT)ckptCliDbShow,
        "ckptDBShow", 
        "Shows the datastructures of master"
    },
    { NULL, "", ""}
};

ClDebugModEntryT clModTab[] = 
{
	{"CKPT", "CKPT", ckptDebugFuncList, "Checkpoint server commands"}, 	
	{"", "", 0, ""}
};

ClRcT ckptDebugRegister(ClEoExecutionObjT* pEoObj)
{
    ClRcT  rc = CL_OK;

    rc = clDebugPromptSet("CKPT");
    if( CL_OK != rc )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("clDebugPromptSet(): rc[0x %x]", rc));
        return rc;
    }
   return clDebugRegister(ckptDebugFuncList,
                          sizeof(ckptDebugFuncList)/sizeof(ckptDebugFuncList[0]),
                          &gCkptDebugReg);
}
                                                                                                                             
ClRcT ckptDebugDeregister(ClEoExecutionObjT* pEoObj)
{
    return clDebugDeregister(gCkptDebugReg);
}

/**=====================================================================**/
/**                                                                     **/
/**=====================================================================**/
/* Node Limits show */
#if 0
ClRcT ckptCliNodeLmtsShow( int argc,
                           char **argv ,
                           ClCharT** ret)
{ 
    ClDebugPrintHandleT msg = 0;
    
    if (argc != 1)
    {
        ckptCliPrint("Usage: ckptNodeLmtsShow\n",ret);
        return CL_OK;
    }
    clDebugPrintInitialize(&msg);
    ckptNodeLimitsShow(msg);
    clDebugPrintFinalize(&msg,ret);
    return CL_OK;
}
#endif
ClRcT  ckptAppNodeInfoShow (ClCntKeyHandleT     key,
                         ClCntDataHandleT   pData,
                         void               *dummy,
                         ClUint32T          dataLength)
{
    ClDebugPrintHandleT msg = *(ClDebugPrintHandleT *)dummy;
    clDebugPrint(msg,"\t [%d:%d]", ((ClCkptAppInfoT *)(ClWordT)
                key)->nodeAddress, ((ClCkptAppInfoT *)(ClWordT)key)->portId);
    return CL_OK;
} 

/* Ckpt Show */
ClRcT ckptCliCkptShow( int argc,
                       char **argv,
                       ClCharT** ret)
{ 
    ClDebugPrintHandleT msg = 0;
    
    if ( (argc != 1) && ( argc != 2))
    {
        ckptCliPrint("Usage: ckptShow [ckptName]\n"
                     "\n ckptName   [STRING]  - Name of the checkpoint to be displayed (optional)\n",
                    ret);
        return CL_OK;
    }
    clDebugPrintInitialize(&msg);
    if (argc == 1) ckptEntriesShow(msg);
    else 
    {
        SaNameT    ckptName;
        saNameSet(&ckptName,argv[1]);
        ckptSingleEntryShow(&ckptName,msg);
    }
    clDebugPrintFinalize(&msg,ret);
    return CL_OK;
}

/* Node Limits show */
ClRcT ckptCliPeerListShow( int argc,
                           char **argv,
                           ClCharT** ret)
{ 
    ClDebugPrintHandleT msg = 0;

    if (argc != 1)
    {
        ckptCliPrint("Usage: ckptPeerShow \n",ret);
        return CL_OK;
    }
    clDebugPrintInitialize(&msg);
    ckptPeerListShow(msg);
    clDebugPrintFinalize(&msg,ret);
    return CL_OK;
}

ClRcT  ckptCliCkptInit(int argc,
                        char **argv,
                        ClCharT **ret)
{
 ClRcT rc = CL_OK;
 ClCkptHdlT  ckptHdl = -1;
 ClVersionT  version;
 if(argc != 1)
 {
         ckptCliPrint("Usage:Init",ret);
         return CL_OK;
 }
 
    version.releaseCode  ='B';
    version.majorVersion = 0x01;
    version.minorVersion = 0x01;
 rc = clCkptInitialize(&ckptHdl,NULL,&version);
 return rc;
}
			
/* Checkpoint open */
ClRcT  ckptCliCkptOpen( int argc,
                        char **argv,
                        ClCharT **ret)
{
    SaNameT                                 ckptName;
    ClCkptOpenFlagsT                        openFlags =0;
    ClCkptCheckpointCreationAttributesT     createAttr; 
    ClRcT                                   rc = 0;
    ClCkptHdlT                              hdl;
    ClBufferHandleT                  inMsg;
    if ((argc < 3) || ((!strcmp(argv[2], "create")) && argc != 9)
        || (strcmp(argv[2], "create") && argc != 3))
    {
        ckptCliPrint("Usage: ckptOpen <name> <openFlag> [<mode> " 
                     "<retentionDuration> <maxSize> "
                     "<maxSections> <maxSectionSize> <maxSecIdSize>]\n"
                     "name              [STRING]  Name of the checkpoint\n"
                     "openFlag          [STRING]  create/read/write\n"
                     "mode              [STRING]  sync/async/collocated[mandatory for ckpt Creation]\n"
                     "retentionDuration [INT]     Retention time (in nano seconds)[mandatory for ckpt Creation]\n"
                     "maxSize           [INT]     Maximum size of checkpoint[mandatory for ckpt Creation]\n"
                     "maxSections       [INT]     Maximum number of sections[mandatory for ckpt Creation]\n"
                     "maxSectionSize    [INT]     Maximum section size[mandatory for ckpt Creation]\n"
                     "maxSecIdSize      [INT]     Maximum Section Id Size[mandatory for ckpt Creation]\n\n"
                     "mode, retentionDuration, maxSize, maxSections, maxSecIdSize, maxSectionSize,\n"
                     " if specified, should not be used if openFlag != create \n" ,ret);
        return CL_OK;
    }
    clDebugPrintInitialize((ClDebugPrintHandleT *)&inMsg);
    clDebugPrint(inMsg,"Opening a checkpoint \n");
    memset(&ckptName, '\0', sizeof(ckptName));
    ckptName.length = strlen(argv[1]);
    memcpy(ckptName.value, argv[1], strlen(argv[1]));
    
    if (!(strcmp(argv[2], "create")))
    {
        memset(&createAttr, '\0', sizeof(ClCkptCheckpointCreationAttributesT));
        if (!(strcmp(argv[3], "sync")))
            createAttr.creationFlags =  CL_CKPT_WR_ALL_REPLICAS;
        else if (!(strcmp(argv[3], "async")))
            createAttr.creationFlags =  CL_CKPT_WR_ACTIVE_REPLICA;
        else if (!(strcmp(argv[3], "collocated")))
            createAttr.creationFlags =  CL_CKPT_CHECKPOINT_COLLOCATED;
        createAttr.checkpointSize  = atoi(argv[5]);
        createAttr.retentionDuration = atoll(argv[4]);
        createAttr.maxSections       = atoi(argv[6]);
        createAttr.maxSectionSize    = atoi(argv[7]);
        createAttr.maxSectionIdSize  = atoi(argv[8]);
        openFlags = CL_CKPT_CHECKPOINT_CREATE;
        rc = clCkptCheckpointOpen(1, &ckptName, &createAttr, openFlags, 3000, &hdl);
        if(rc != CL_OK)
           clDebugPrint(inMsg,"Checkpoint Open failed : rc %x\n",rc);
             
        else
           clDebugPrint(inMsg,"Checkpoint Open completed: hdl %llx\n",hdl);
    }
    else if (!(strcmp(argv[2], "write")))
    {
        openFlags = CL_CKPT_CHECKPOINT_WRITE;
        rc = clCkptCheckpointOpen(1, &ckptName, NULL, openFlags, 3000, &hdl);
        if(rc != CL_OK)
           clDebugPrint(inMsg,"Checkpoint Open failed : rc %x\n",rc);
             
        else
           clDebugPrint(inMsg,"Checkpoint Open completed: hdl %llx\n",hdl);
    }
    else if (!(strcmp(argv[2], "read")))
    {
        openFlags = CL_CKPT_CHECKPOINT_READ;
        rc = clCkptCheckpointOpen(1, &ckptName, NULL, openFlags, 3000, &hdl);
        if(rc != CL_OK)
           clDebugPrint(inMsg,"Checkpoint Open failed : rc %x\n",rc);
             
        else
           clDebugPrint(inMsg,"Checkpoint Open completed: hdl %llx\n",hdl);
    }
    else
    {
        openFlags = 0x08;
        rc = clCkptCheckpointOpen(1, &ckptName, NULL, openFlags, 3000, &hdl);
        if(rc != CL_OK)
           clDebugPrint(inMsg,"Checkpoint Open failed : rc %x\n",rc);
             
        else
           clDebugPrint(inMsg,"Checkpoint Open completed: hdl %llx\n",hdl);
    }   
    clDebugPrintFinalize(&inMsg,ret);
    return CL_OK;
}

/* checkpoint write */
ClRcT  ckptCliCkptWrite( int argc,
                         char **argv,
                         ClCharT** ret)
{
    ClCkptHdlT                hdl =0; 
    ClRcT                     rc =0;
    ClUint32T                 secCount =0;
    ClUint32T                 count =0;
    ClUint32T                 tmpCount =0;
    ClUint32T                 error =0;
    ClCkptIOVectorElementT   *pVec = NULL;
    ClCkptIOVectorElementT   *pTmpVec = NULL;
    ClDebugPrintHandleT       inMsg;


    if (argc < 5)
    {
        ckptCliPrint("Usage: ckptWrite <handle> <secName> <offset> <data> [secName offset data] ...\n"
                     "handle            [INT]     Handle of the checkpoint\n"
                     "secName           [STRING]  Name of the section\n"
                     "offset            [INT]     Offset in the section\n"
                     "data              [STRING]  data to be written\n",
                     ret);
        return 0;
    }

    clDebugPrintInitialize(&inMsg);
    clDebugPrint(inMsg,"Writing to a checkpoint \n");
    hdl = atoi(argv[1]);

    secCount = (argc - 2)/3;
    if (NULL== ( pVec = (ClCkptIOVectorElementT *)clHeapAllocate(
                                   secCount * sizeof(ClCkptIOVectorElementT))))
    {
        ckptCliPrint("Memory allocation failed \n",ret);
        return 0;
    }
    memset(pVec, '\0', (sizeof(ClCkptIOVectorElementT) * secCount));
    pTmpVec = pVec;
    for (;count < secCount; count++)
    {
           if(!strcmp(argv[(3 * count)+2],"null"))
           {
              pVec->sectionId.idLen = 0;
              pVec->sectionId.id    = NULL;
           }
           else
           {
              pVec->sectionId.idLen = strlen(argv[(3 * count)+2]);
              pVec->sectionId.id    = (ClUint8T *) clHeapAllocate(pVec->sectionId.idLen);
              if (pVec->sectionId.id == NULL) 
              {
                   ckptCliPrint("Memory allocation failed\n",ret);
                   goto exitOnError;
              }
              memcpy(pVec->sectionId.id, argv[(3 * count) +2], pVec->sectionId.idLen);
              pVec->dataSize   = strlen(argv[(3 * count) + 4]);
              pVec->dataOffset = atoi(argv[(3 * count) + 3]);
              pVec->dataBuffer = (ClPtrT)clHeapAllocate(pVec->dataSize);
              if (pVec->dataBuffer == NULL) 
              {
                   ckptCliPrint("Memory allocation failed\n",ret);
                   goto exitOnError;
              }
              memcpy(pVec->dataBuffer,argv[(3 * count) + 4],pVec->dataSize);
           }   
        pVec++;
    }
    rc = clCkptCheckpointWrite(hdl, pTmpVec, secCount, &error);
    if (rc != CL_OK) 
       clDebugPrint(inMsg,"Checkpoint Write failed : rc %x\n",rc);
    else
        clDebugPrint(inMsg,"Checkpoint write completed \n");
exitOnError:
    pVec = pTmpVec;
    for (tmpCount = 0; tmpCount < count; tmpCount++)
    {
        clHeapFree(pVec->sectionId.id);
        clHeapFree(pVec->dataBuffer);
        pVec++;
    }
    if( count < secCount )
    {
        if( NULL != pVec->sectionId.id )
        {
            clHeapFree( pVec->sectionId.id );
        }
        if( NULL != pVec->dataBuffer )
        {
            clHeapFree( pVec->dataBuffer );
        }
    }
    clHeapFree(pTmpVec);
    clDebugPrintFinalize(&inMsg,ret);
    return 0;
}

/* checkpoint write */
ClRcT  ckptCliCkptRead( int argc,
                        char **argv,
                        ClCharT** ret)
{
    ClCkptHdlT                hdl =0; 
    ClRcT                     rc =0;
    ClUint32T                 secCount =0;
    ClUint32T                 count =0;
    ClUint32T                 error =0;
    ClCkptIOVectorElementT   *pVec = NULL;
    ClCkptIOVectorElementT   *pTmpVec = NULL;
    ClDebugPrintHandleT       inMsg   = 0;
    if (argc < 5 )  
    {
        ckptCliPrint("\n Usage: ckptRead <handle> <secName> <offset> <size> [secName offset size] ...\n"
                     "handle            [INT]     Handle of the checkpoint\n"
                     "secName           [STRING]  Name of the section\n"
                     "offset            [INT]     Offset of the section\n"
                     "size              [INT]     Size of data to be read(specify 0 if not known)\n",
                    ret);
        return CL_OK;
    }

   clDebugPrintInitialize(&inMsg);
   clDebugPrint(inMsg,"Reading checkpoint \n");
   hdl = atoi(argv[1]);

    secCount = (argc - 2)/3;
    if (NULL== ( pVec = (ClCkptIOVectorElementT *)clHeapCalloc(secCount ,sizeof(ClCkptIOVectorElementT))))
    {
        ckptCliPrint("Memory allocation failed \n",ret);
        return 0;
    }
    pTmpVec = pVec;
    count = 0;
    for (;count < secCount ; count++)
    {
        if(!strcmp(argv[(3 * count) + 2],"null"))
        {
          pVec->sectionId.idLen = 0;
          pVec->sectionId.id = NULL;
        } 
        else
        {
             pVec->sectionId.idLen = strlen(argv[ (3 * count) + 2]);
             pVec->sectionId.id    = (ClUint8T *) clHeapAllocate(pVec->sectionId.idLen);
            if (pVec->sectionId.id == NULL) 
            {
                ckptCliPrint("\n Memory allocation failed\n",ret);
                goto exitOnError;
             }
            memset(pVec->sectionId.id,'\0',pVec->sectionId.idLen);
            memcpy(pVec->sectionId.id, argv[(3 * count) + 2], pVec->sectionId.idLen);
        }
       pVec->dataOffset   = atoi(argv[(3 * count) + 3]); 
       pVec->dataSize     = atoi(argv[(3 * count) + 4]);
       if(pVec->dataSize == 0)
       {
          pVec->dataBuffer = NULL;
       }
       else
       {
          pVec->dataBuffer = (ClCharT *)clHeapAllocate(pVec->dataSize + 1);
          memset(pVec->dataBuffer,'\0',pVec->dataSize + 1);
       }
       
/*       pVec->dataBuffer = (ClPtrT) pVec->sectionId.id;*/
       pVec++;
   }
   rc = clCkptCheckpointRead(hdl,pTmpVec,secCount,&error);
   if (rc != CL_OK) 
       clDebugPrint(inMsg,"\n Checkpoint read failed : rc %x\n",rc);
   else 
   {
       clDebugPrint(inMsg,"\n Read completed\n");
       pVec = pTmpVec;
       for(count = 0; count < secCount ; count++)
       {
           clDebugPrint(inMsg,"\n %s\n",(ClCharT *)pVec->dataBuffer);
           clDebugPrint(inMsg,"\n %lld\n",pVec->readSize);
           pVec++;
       }
   }  
   pVec = pTmpVec;
   for (count = 0; count < secCount; count++)
   {
       clHeapFree(pVec->sectionId.id);
       clHeapFree(pVec->dataBuffer);
       pVec++;
   }
exitOnError:
   {
   clHeapFree(pTmpVec);
   clDebugPrintFinalize(&inMsg,ret);
   return 0;
   }
}

/* Section create */
ClRcT  ckptCliSectionCreate( int argc, 
                             char **argv,
                             ClCharT** ret)
{
    ClUint8T                                *pSecData = NULL;
    ClCkptSectionCreationAttributesT        createAttr;
    ClCkptHdlT                              hdl =0; 
    ClRcT                                   rc =0;
    ClBufferHandleT                  inMsg;

    if (argc != 4)
    {
        ckptCliPrint("\n Usage :ckptSectionCreate <handle> <secName> <expiry Time>\n"
                     "handle            [INT]     Handle of the checkpoint\n"
                     "secName           [STRING]  Name of the section\n"
                     "expiryTime        [STRING]  Expiry Time (in nano seconds)\n"
                     "                            0 for infinite expiry time\n", ret);
        return CL_OK;
    }
    clDebugPrintInitialize(&inMsg);
    hdl = atoi(argv[1]);
    memset(&createAttr, '\0', sizeof(ClCkptSectionCreationAttributesT));
    createAttr.sectionId = (ClCkptSectionIdT *)clHeapAllocate(
                                                     sizeof(ClCkptSectionIdT));
    if (createAttr.sectionId == NULL)
    {
        ckptCliPrint("Malloc failed \n",ret);
        clLogWrite(CL_LOG_HANDLE_APP,CL_LOG_CRITICAL,CL_LOG_CKPT_SVR_NAME,
                   CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
        return CL_CKPT_ERR_NO_MEMORY;
    }

    if(!strcmp(argv[2], "NULL"))
    {
        createAttr.sectionId->idLen = 0;
        createAttr.sectionId->id = NULL;
    }
    else
    {
        createAttr.sectionId->idLen = strlen(argv[2]);
        createAttr.sectionId->id = (ClUint8T *)clHeapAllocate(strlen(argv[2]));
        if (createAttr.sectionId->id == NULL)
        {
            clHeapFree(createAttr.sectionId);
            ckptCliPrint("Malloc failed \n",ret);
            clLogWrite(CL_LOG_HANDLE_APP,CL_LOG_CRITICAL,CL_LOG_CKPT_SVR_NAME,
                    CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
            return CL_CKPT_ERR_NO_MEMORY;
        }
        memcpy(createAttr.sectionId->id, argv[2], strlen(argv[2]));
    }
    if(atoll(argv[3]) != 0)
    createAttr.expirationTime = atoll(argv[3]);
    else
    createAttr.expirationTime = CL_TIME_END;

    pSecData = (ClUint8T *)clHeapAllocate(sizeof(createAttr));
    memcpy(pSecData, &createAttr, sizeof(createAttr));
    rc = clCkptSectionCreate(hdl, &createAttr, (ClUint8T *)pSecData,
                             sizeof(createAttr));
    clHeapFree(createAttr.sectionId->id);
    clHeapFree(createAttr.sectionId);

    if (rc != CL_OK)
       clDebugPrint(inMsg," Section create failed: rc %x\n ",rc);
    else
       clDebugPrint(inMsg," Section [%s] created\n ", argv[2]);
    clHeapFree(pSecData);
    clDebugPrintFinalize(&inMsg,ret);
    return CL_OK;
}

/* Section Overwrite*/
ClRcT  ckptCliSectionOverwrite( int argc,
                                char **argv,
                                ClCharT** ret)
{
    ClCkptSectionIdT          sectionId;      /* Section identifier */
    ClUint8T                  *pSecData = NULL;
    ClCkptHdlT                hdl =0; 
    ClRcT                     rc =0;
    ClBufferHandleT    inMsg;

    if (argc != 4)
    {
        ckptCliPrint("Usage: ckptSectionOverwrite <handle> <secName> <data>\n"
                     "handle            [INT]     Handle of the checkpoint\n"
                     "secName           [STRING]  Name of the section\n"
                     "data              [STRING]  data to be written\n" ,ret);
        return 0;
    }
    clDebugPrintInitialize(&inMsg);
    hdl = atoi(argv[1]);
    if(!strcmp(argv[2],"null"))
    {
    sectionId.id  = NULL;
    sectionId.idLen = 0;
    }
    else
    {
       sectionId.idLen = strlen(argv[2]);
       sectionId.id = (ClUint8T *)clHeapAllocate(strlen(argv[2]));
       if (sectionId.id == NULL)
       {
          ckptCliPrint(" Malloc failed \n",ret);
          clLogWrite(CL_LOG_HANDLE_APP,CL_LOG_CRITICAL,CL_LOG_CKPT_SVR_NAME,
                   CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
          return CL_CKPT_ERR_NO_MEMORY;
       }
       memcpy(sectionId.id, argv[2], strlen(argv[2]));
    }
    pSecData = (ClUint8T *)clHeapAllocate(strlen(argv[3]));
    memcpy(pSecData,argv[3],strlen(argv[3]));

   rc = clCkptSectionOverwrite(hdl, &sectionId, (ClUint8T *)pSecData, strlen(argv[3]));
   if (rc != CL_OK)
       clDebugPrint(inMsg," Section overwrite failed: rc %x\n ",rc);
   else
       clDebugPrint(inMsg," Section Overwritten\n ");

    clHeapFree(sectionId.id);
    clHeapFree(pSecData);
    clDebugPrintFinalize(&inMsg,ret);
    return CL_OK;
}

ClRcT  ckptCliSectionExpirationTimeSet( int argc,
                                        char **argv,
                                        ClCharT ** ret)
{
    ClCkptSectionIdT                        sectionId;
    ClCkptHdlT                              hdl =0; 
    ClRcT                                   rc =0;
    ClBufferHandleT    inMsg;
    ClTimeT                   expirationTime = 0;

    if (argc < 4)
    {
        ckptCliPrint("Usage : ckptSectionExpirationTimeSet <handle> <secName> <expiry time>\n"
                     "handle            [INT]     Handle of the checkpoint\n"
                     "secName           [STRING]  Name of the section\n"
                     "expiry time       [LONG]    Section expiration time (absolute time in nano sec)\n"
                     "        absolute time = clOsalTimeOfDayGet() + relative time \n"
                     "                            0 for infinite expiry time\n", ret);
        return 0;
    }

  
    hdl = atoi(argv[1]);
    clDebugPrintInitialize(&inMsg);

    memset(&sectionId, '\0', sizeof(ClCkptSectionIdT));
    if(!strcmp(argv[2],"null"))
    {
    sectionId.id  = NULL;
    sectionId.idLen = 0;
    }
    else
    {
       sectionId.idLen = strlen(argv[2]);
       sectionId.id = (ClUint8T *)clHeapAllocate(strlen(argv[2]));
       if (sectionId.id == NULL)
       {
          ckptCliPrint(" Malloc failed \n",ret);
          clLogWrite(CL_LOG_HANDLE_APP,CL_LOG_CRITICAL,CL_LOG_CKPT_SVR_NAME,
                   CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
          return CL_CKPT_ERR_NO_MEMORY;
       }
       memcpy(sectionId.id, argv[2], strlen(argv[2]));
    }

    if(atoll(argv[3]) != 0)
        expirationTime = atoll(argv[3]);
    else
        expirationTime = CL_TIME_END;
    rc = clCkptSectionExpirationTimeSet(hdl, &sectionId, expirationTime);
    if(rc != CL_OK)
       clDebugPrint(inMsg," Section expiration time set failed: rc %x\n ",rc);
    clHeapFree(sectionId.id);
    clDebugPrintFinalize(&inMsg,ret);
    return CL_OK;

}


ClRcT  ckptCliSectionDelete( int argc,
                             char **argv,
                             ClCharT ** ret)
{
    ClCkptSectionIdT                        sectionId;
    ClCkptHdlT                              hdl =0; 
    ClRcT                                   rc =0;
    ClBufferHandleT    inMsg;

    if (argc != 3)
    {
        ckptCliPrint("Usage : ckptSectionDelete <handle> <secName>\n"
                     "handle            [INT]     Handle of the checkpoint\n"
                     "secName           [STRING]  Name of the section\n" ,ret);
        return 0;
    }

  
    hdl = atoi(argv[1]);
    clDebugPrintInitialize(&inMsg);

    memset(&sectionId, '\0', sizeof(ClCkptSectionIdT));
    sectionId.idLen = strlen(argv[2]);
    sectionId.id = (ClUint8T *)clHeapAllocate(strlen(argv[2]));
    if (sectionId.id == NULL)
    {
        ckptCliPrint("Malloc failed \n",ret);
        clLogWrite(CL_LOG_HANDLE_APP,CL_LOG_CRITICAL,CL_LOG_CKPT_SVR_NAME,
                   CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
        return CL_CKPT_ERR_NO_MEMORY;
    }
    memcpy(sectionId.id, argv[2], strlen(argv[2]));

    rc = clCkptSectionDelete(hdl, &sectionId);
    if(rc != CL_OK)
       clDebugPrint(inMsg," Section delete failed: rc %x\n ",rc);
    clHeapFree(sectionId.id);
    clDebugPrintFinalize(&inMsg,ret);
    return CL_OK;

}

ClRcT  ckptCliCkptClose( int argc,
                         char **argv,
                         ClCharT** ret)
{
    ClCkptHdlT                       hdl = 0; 
    ClRcT                            rc = 0;
    ClBufferHandleT           inMsg;


    if (argc != 2)
    {
        ckptCliPrint("Usage: ckptClose <handle>\n"
                     "handle            [INT]  Handle of the checkpoint\n",ret);
        return 0;
    }

    hdl = atoi(argv[1]);
    clDebugPrintInitialize(&inMsg);
    rc = clCkptCheckpointClose(hdl);
    if(rc != CL_OK)
       clDebugPrint(inMsg,"ckpt close failed: rc %x\n ",rc);
    else
      clDebugPrint(inMsg,"\n clCkptCheckpointClose Completed \n");
      
    clDebugPrintFinalize(&inMsg,ret);
    return CL_OK;
}
/* This routine deletes a checkpoint */
ClRcT  ckptCliCkptUnlink( int argc,
                          char **argv,
                          ClCharT** ret)/*FIXME this function name changed from ckpt to tCkpt*/
{
    SaNameT                                 ckptName;
    ClRcT                                   rc =0;
    ClDebugPrintHandleT                     inMsg = 0;
                                                                                                 
    if (argc != 2)
    {
        ckptCliPrint("Usage: ckptDelete <name>\n"
                     "name              [STRING]  Name of the checkpoint\n",ret);
        return 0;
    }
    clDebugPrintInitialize(&inMsg);
                                                                                                     
    clDebugPrint(inMsg,"Deleting a checkpoint\n");
    memset(&ckptName, '\0', sizeof(ckptName));
    ckptName.length = strlen(argv[1]);
    memcpy(ckptName.value, argv[1], strlen(argv[1]));
    rc = clCkptCheckpointDelete(1, &ckptName);
    if(rc != CL_OK)
       clDebugPrint(inMsg," ckpt delete failed: rc %x\n ",rc);
    else
       clDebugPrint(inMsg,"clCkptCheckpointDelete Completed\n");
    clDebugPrintFinalize(&inMsg,ret);
    return CL_OK;
}

ClRcT ckptCliCkptStatusGet( int argc ,
                            char **argv,
                            ClCharT** ret)
{
   /* SaNameT          ckptName;*/
    ClRcT            rc = 0 ;
    ClCkptHdlT       hdl = 0;
    ClCkptCheckpointDescriptorT  ckptStatus;
    ClDebugPrintHandleT msg = 0;

    if(argc != 2)
    {
        ckptCliPrint("Usage: ckptStatusGet <handle>\n"
                     "handle              [INT]  Handle of the checkpoint\n",ret);
        return CL_OK;
    }
    hdl = atoi(argv[1]);
    rc = clCkptCheckpointStatusGet(hdl,&ckptStatus);
    if (rc != CL_OK) 
    {
        ckptCliPrint(" Failed to get a checkpoint status \n", ret);
        return CL_OK;
    }
    
    clDebugPrintInitialize(&msg);    
   /* clDebugPrint(msg,"Name                 :  %s\n" , ckptName.value);*/
    clDebugPrint(msg,"Creation Flag        :  %d\n",
             ckptStatus.checkpointCreationAttributes.creationFlags);
    clDebugPrint(msg,"Size                 :  %u\n" ,
             (ClUint32T)ckptStatus.checkpointCreationAttributes.checkpointSize);
    clDebugPrint(msg,"Retention Duration   :  %u\n" ,
          (ClUint32T)ckptStatus.checkpointCreationAttributes.retentionDuration);
    clDebugPrint(msg,"Max No of Sections   :  %u\n" ,
            ckptStatus.checkpointCreationAttributes.maxSections);
    clDebugPrint(msg,"Max SectionSize      :  %u\n" ,
            (ClUint32T)ckptStatus.checkpointCreationAttributes.maxSectionSize);
    clDebugPrint(msg,"SectionIdSize        :  %u\n" ,
           (ClUint32T)ckptStatus.checkpointCreationAttributes.maxSectionIdSize);
    clDebugPrint(msg,"No of Sections       :  %u\n" ,
            (ClUint32T)ckptStatus.numberOfSections);
    clDebugPrint(msg,"Memory Used          :  %u\n" ,
            (ClUint32T)ckptStatus.memoryUsed);
    clDebugPrintFinalize(&msg,ret);
    return CL_OK;
}

ClRcT  ckptCliActiveReplicaSet( int argc,
                                char **argv,
                                ClCharT** ret)
{
    ClCkptHdlT                       hdl = 0;
    ClRcT                            rc = 0;
    ClBufferHandleT           inMsg;
                                                                                                                             
                                                                                                                             
    if (argc != 2)
    {
        ckptCliPrint("Usage: ckptActiveReplicaSet <handle> \n"
                     "handle            [INT]  Handle of the checkpoint\n",ret);
        return 0;
    }
                                                                                                                             
    hdl = atoi(argv[1]);
    clDebugPrintInitialize(&inMsg);
    rc = clCkptActiveReplicaSet(hdl);
    if(rc != CL_OK)
       clDebugPrint(inMsg,"ActiveReplicaSet failed: rc %x\n ",rc);
    else
       clDebugPrint(inMsg," ActiveReplicaSet Completed \n");
                                                                                                                             
    clDebugPrintFinalize(&inMsg,ret);
    return CL_OK;
}

/* This routine updates the retention duration of the checkpoint */
ClRcT  ckptCliCkptRetentionDurationSet( int argc,
                          char **argv,
                          ClCharT** ret)
{
    ClRcT                  rc    = 0;
    ClDebugPrintHandleT    inMsg = 0;
    ClTimeT                time;
    ClCkptHdlT             hdl   = 0;
                                                                                                 
    if (argc != 3)
    {
        ckptCliPrint("Usage: ckptRetentionDurationSet <handle> <time>\n"
                     "handle              [INT] Handle of the checkpoint\n"
                     "time                [INT] Retention time in millisec",ret);
        return 0;
    }
    clDebugPrintInitialize(&inMsg);
                                                                                                     
    hdl  = atoi(argv[1]);
    time = atoi(argv[2]);
    time = time * 1000 *1000;
    rc  =  clCkptCheckpointRetentionDurationSet(hdl, time);
    if(rc != CL_OK)
       clDebugPrint(inMsg," ckptRetentionDurationSet failed: rc %x\n ",rc);
    else
       clDebugPrint(inMsg," ckptRetentionDurationSet Completed \n");
    clDebugPrintFinalize(&inMsg,ret);
    return CL_OK;
}


/**==============================================================**/
/**                                                              **/
/**==============================================================**/

/* 
   Function to display checkpoint entries 
 */
ClRcT  ckptEntriesShow(ClDebugPrintHandleT msg)
{
    ClRcT       rc = CL_OK;  /* Return code */
    /* 
      Algorithm:
        1. Validations
             - Check if Ckpt Server ready to take requests or not
               if ((not_initialised) return CL_CKPT_ERR_NOT_INITIALIZED
        2. Walk thru all the nodes and print each name
     */

    /* Step 1  - Validations*/
    CL_CKPT_SVR_EXISTENCE_CHECK;
    /*CKPT_LOCK(gCkptSvr->ckptSem);*/
    
    rc = clCntWalk(gCkptSvr->ckptHdlList,ckptNameShow, &msg,sizeof(msg));
    CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
            (" Checkpoint names walk failed rc[0x %x]\n", rc), rc);
/*    CKPT_UNLOCK(gCkptSvr->ckptSem);*/ /* Good citizen */
    return rc;

exitOnError:
    {
        return rc;
    }
}

/* Function to display a single checkpoint */
ClRcT  ckptSingleEntryShow( SaNameT    *pName,
                            ClDebugPrintHandleT msg)
{
    ClRcT               rc      = CL_OK;  /* Return code */
    ClCntDataHandleT    dataHdl = 0;
    ClUint32T           cksum   = 0;
    /* 
      Algorithm:
        1. Validations
             - Check if Ckpt Server ready to take requests or not
               if ((not_initialised) return CL_CKPT_ERR_NOT_INITIALIZED
        2. Get the corresponding node and print 
     */

    /* Step 1  - Validations*/
    CL_CKPT_SVR_EXISTENCE_CHECK;
    /*CKPT_LOCK(gCkptSvr->ckptSem);*/

    clCksm32bitCompute ((ClUint8T *)pName->value,
                        strlen((const ClCharT *)pName->value), &cksum);
    rc = clCntNonUniqueKeyFind(gCkptSvr->ckptHdlList, 
                               (ClCntKeyHandleT)(ClWordT)cksum, 
                               (ClPtrT)pName, ckptHdlNonUniqueKeyCompare, &dataHdl);
    if (rc == CL_OK) ckptNameShow(0,dataHdl,&msg, sizeof(msg));
/*    CKPT_UNLOCK(gCkptSvr->ckptSem);  Good citizen */
    return rc;
}

/* Display the name of a checkpoint */
ClRcT ckptNameShow( ClCntKeyHandleT     key,
                    ClCntDataHandleT    pData,
                    void                *dummy,
                    ClUint32T           dataLength)
{
    ClCkptHdlT           ckptHdl = *(ClCkptHdlT *)pData;
    CkptT                *pCkpt = NULL;
    ClDebugPrintHandleT  msg = *(ClDebugPrintHandleT *)dummy;
    ClRcT                rc = CL_OK;
    ClUint32T            actAddr  = 0;
    ClUint32T            destAddr  = 0;
    ClUint32T            refCount = 0;
    ClUint8T             flag     = 0;
    ClInt64T             time     = 0;

    /* Get the required info from master */
    {
        rc = clCkptMasterAddressGet(&destAddr);
        if(rc != CL_OK)
             destAddr = gCkptSvr->localAddr;
        rc = ckptIdlHandleUpdate(destAddr,gCkptSvr->ckptIdlHdl,0);     
        rc = VDECL_VER(clCkptMasterStatusInfoGetClientSync, 4, 0, 0)( gCkptSvr->ckptIdlHdl,
                                    ckptHdl,
                                    &time,
                                    &actAddr,
                                    &refCount,
                                    &flag);
    }
    rc = clHandleCheckout(gCkptSvr->ckptHdl,ckptHdl,(void **)&pCkpt); 
    if (pCkpt != NULL)
    {
        clDebugPrint(msg,
                "====================================================\n");
        clDebugPrint(msg,"Checkpoint Name :%s\n", pCkpt->ckptName.value);
        clDebugPrint(msg,"Primary Svr Addr:%d\n", actAddr);

         if (pCkpt->pCpInfo != NULL)
            ckptCPlaneInfoShow(pCkpt->pCpInfo,msg,time);

         if (pCkpt->pDpInfo != NULL)
            ckptDPlaneInfoShow(pCkpt->pDpInfo,msg);

         clDebugPrint(msg,"\n\n\t Information at primary owner .....");
         clDebugPrint(msg,"\n\t\t Reference Count    : %d", refCount);
         clDebugPrint(msg,"\n\t\t Delete             : %s",
                     (flag)? "Pending": "Not requested");

         clDebugPrint(msg,
                 "\n====================================================\n");
        
    }
    rc = clHandleCheckin(gCkptSvr->ckptHdl,ckptHdl);
    return CL_OK;
}


/* This routine displays the control plane information of a ckpt */
ClRcT  ckptCPlaneInfoShow( CkptCPlaneInfoT    *pCpInfo,
                           ClDebugPrintHandleT msg,
                           ClInt64T            time)
{
    
    clDebugPrint(msg,"\n\n\t Control plane information.....");
    if (pCpInfo->updateOption & CL_CKPT_WR_ALL_REPLICAS)
    {
        const ClCharT *type = "Sync";
        if(pCpInfo->updateOption & CL_CKPT_DISTRIBUTED)
        {
            type = "Sync [hot-standby]";
        }
        clDebugPrint(msg,"\n\t\t Update Option: %s", type);
    }
    else if (pCpInfo->updateOption & CL_CKPT_WR_ACTIVE_REPLICA)
    {
        const ClCharT *type = "Async";
        if(pCpInfo->updateOption & CL_CKPT_CHECKPOINT_COLLOCATED)
        {
            type = "Async collocated";
            if(pCpInfo->updateOption & CL_CKPT_DISTRIBUTED)
            {
                type = "Async collocated [hot-standby]";
            }
        }
        else if(pCpInfo->updateOption & CL_CKPT_DISTRIBUTED)
        {
            type = "Async [hot-standby]";
        }
        clDebugPrint(msg,"\n\t\t Update Option: %s", type);
    }
    else if (pCpInfo->updateOption & CL_CKPT_WR_ACTIVE_REPLICA_WEAK)
    {
        clDebugPrint(msg,"\n\t\t Update Option: Async-Weak");
    }
    else if (pCpInfo->updateOption & CL_CKPT_CHECKPOINT_COLLOCATED)
    {
        clDebugPrint(msg,"\n\t\t Update Option: Collocated");   
    }
    else if (pCpInfo->updateOption & CL_CKPT_WR_ALL_SAFE)
    {
        clDebugPrint(msg,"\n\t\t Update Option: Safe Sync");   
    }
    else if(pCpInfo->updateOption & CL_CKPT_DISTRIBUTED)
    {
        clDebugPrint(msg,"\n\t\t Update Option: Async hot-standby");
    }
    else 
    {
        clDebugPrint(msg,"\n\t\t Update Option: None");   
    }
    clDebugPrint(msg,"\n\t\t Retention Duration (in millisec) :%lld\n",
                 time/(CL_CKPT_NANO_TO_MICRO * CL_CKPT_NANO_TO_MICRO));
    clDebugPrint(msg, "\n\t\t Checkpoint ID: %#llx\n", pCpInfo->id);

    /* Display the list of nodes which have this ckpt */
    if (pCpInfo->presenceList != 0)
    {
        clDebugPrint(msg,"\t\t Presence List .....\n");   
        clDebugPrint(msg,"\t\t");   
        clCntWalk(pCpInfo->presenceList, ckptPresenceNodeShow,&msg,sizeof(msg));
        clDebugPrint(msg,"\n");   
    }
    if (pCpInfo->appInfoList != 0)
    {
        clDebugPrint(msg,"\t\t Application Open List .....\n");   
        clDebugPrint(msg,"\t\t");   
        clCntWalk(pCpInfo->appInfoList, ckptAppNodeInfoShow,&msg,sizeof(msg));
        clDebugPrint(msg,"\n");   
    }
    return CL_OK;
}

/* This routine displays the control plane information of a ckpt */
ClRcT  ckptDPlaneInfoShow( CkptDPlaneInfoT    *pDpInfo,
                           ClDebugPrintHandleT msg)
{
    ClRcT              rc       = CL_OK;
    CkptSectionT       *pSec    = NULL;
    ClCntNodeHandleT   secNode  = CL_HANDLE_INVALID_VALUE;
    ClCntNodeHandleT   nextNode = CL_HANDLE_INVALID_VALUE;
    ClCkptSectionKeyT  *pKey    = NULL;
    
    clDebugPrint(msg,"\n\n\t Data plane information.....");
    clDebugPrint(msg,"\n\t\t Max Size           : %d",
            (ClUint32T)pDpInfo->maxCkptSize); 
    clDebugPrint(msg,"\n\t\t Max Section Size   : %d",
            (ClUint32T)pDpInfo->maxScnSize); 
    clDebugPrint(msg,"\n\t\t Sections Count     : %d",
            pDpInfo->numScns); 
    clDebugPrint(msg,"\n\t\t Max Sections       : %d",
            pDpInfo->maxScns); 
    clDebugPrint(msg,"\n\t\t Max SectionId Size : %d",
            (ClUint32T) pDpInfo->maxScnIdSize); 

    rc = clCntFirstNodeGet(pDpInfo->secHashTbl, &secNode);
    if( CL_OK != rc )
    {
        clLogInfo(CL_CKPT_AREA_PEER, CL_CKPT_CTX_CKPT_OPEN, 
                "No sections have been created");
        return rc;
    }
    while( secNode != 0 )
    {
        ClBoolT         firstSec = CL_TRUE;
        rc = clCntNodeUserDataGet(pDpInfo->secHashTbl, secNode, 
                (ClCntDataHandleT *) &pSec);
        if( CL_OK != rc )
        {
            clLogError(CL_CKPT_AREA_PEER, CL_CKPT_CTX_CKPT_OPEN, 
                    "Failed to get data while packing section");
            return rc;
        }
        rc = clCntNodeUserKeyGet(pDpInfo->secHashTbl, secNode, 
                                 (ClCntKeyHandleT *) &pKey);
        if( CL_OK != rc )
        {
            clLogError(CL_CKPT_AREA_PEER, CL_CKPT_CTX_CKPT_OPEN, 
                    "Failed to get data while packing section");
            return rc;
        }
            ClCharT    *pTmpSecId = clHeapAllocate(pKey->scnId.idLen +1);
            if (pTmpSecId != NULL) 
            {
                memset(pTmpSecId, '\0', pKey->scnId.idLen +1);
                memcpy(pTmpSecId, pKey->scnId.id, pKey->scnId.idLen);
                if (firstSec == CL_TRUE)
                {
                    clDebugPrint(msg,"\n\t\t Section Id   :"
                            "Section Data :Data Length  :Last Updated (sec):\n");
                    firstSec = CL_FALSE;
                }
                clDebugPrint(msg,"\t\t %s", pTmpSecId);
                clDebugPrint(msg,"\t\t %p", pSec->pData);
                clDebugPrint(msg,"\t\t %d", (ClUint32T) pSec->size);
                clDebugPrint(msg,"\t\t %d", (ClUint32T) (pSec->lastUpdated/(1000*1000*1000)));  /* lastUpdated is in nanoseconds */
                clHeapFree(pTmpSecId);
                pTmpSecId = NULL;
            }
        rc = clCntNextNodeGet(pDpInfo->secHashTbl, secNode, &nextNode);
        if( CL_OK != rc )
        {
            break;
        }
        secNode = nextNode;
    }
    return CL_OK;
}


/* Routine to display the peer nodes */
ClRcT ckptPeerListShow (ClDebugPrintHandleT msg)
{
    clDebugPrint(msg,"\n List of peers.....\n");
    clCntWalk(gCkptSvr->peerList, ckptPeerShow,&msg,sizeof(msg));
    return CL_OK;
}

ClRcT  ckptPeerShow (ClCntKeyHandleT     key,
                     ClCntDataHandleT   pData,
                     void              *dummy,
                     ClUint32T          dataLength)
{
    ClDebugPrintHandleT msg = *(ClDebugPrintHandleT *)dummy;
    clDebugPrint(msg,"\t\t %p \n", key);
    return CL_OK;
} 

/* Routine to display the presence list */
ClRcT  ckptPresenceNodeShow (ClCntKeyHandleT     key,
                         ClCntDataHandleT   pData,
                         void               *dummy,
                         ClUint32T          dataLength)
{
    ClDebugPrintHandleT msg = *(ClDebugPrintHandleT *)dummy;
    clDebugPrint(msg,"\t %p ", key);
    return CL_OK;
} 


/* To print all the printfs*/
void ckptCliPrint(char *str,ClCharT ** ret)
{
     *ret = clHeapAllocate(strlen(str)+1);
      if(NULL == *ret)
      {
           CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Malloc Failed \r\n"));
           return;
      }
      snprintf(*ret, strlen(str)+1, str);
     return;
}             
ClRcT ckptCliIterInit( int argc ,
                       char **argv,
                       ClCharT** ret)
{
    ClRcT            rc = 0 ;
    ClCkptHdlT       hdl = 0;
    ClDebugPrintHandleT inMsg = 0;
    ClCkptSectionsChosenT secChosen = 0;
    ClHandleT        secHdl = -1;
    ClTimeT          expiryTime = 0;
   /* ClIdlHandleT     ckptIdlHdl = 0;*/

    if(argc < 3)
    {
        ckptCliPrint("Usage: ckptIterInit <handle> <secChosen> [time]\n"
                     "handle              [INT]  Handle of the checkpoint\n"
                     "secChosen           [INT]  1-->sections without expiration time\n"
                     "                           2-->sections with expiration time lesser than or equal to given time\n"
                     "                           3-->sections with expiration time greater than or equal to given time\n"
                     "                           4-->corrupted sections\n"
                     "                           5-->all sections\n"
                     "time                [INT]  expiration time in nanosec\n"
                     ,ret);
        return CL_OK;
    }
    rc = clDebugPrintInitialize(&inMsg);
    hdl = atoi(argv[1]);
    switch( atoi(argv[2]) )
    {
       case 1:
               secChosen = CL_CKPT_SECTIONS_FOREVER;
               break;
       case 2:
               secChosen = CL_CKPT_SECTIONS_LEQ_EXPIRATION_TIME;
               break;
       case 3:
               secChosen = CL_CKPT_SECTIONS_GEQ_EXPIRATION_TIME; 
               break;
       case 4:
               secChosen = CL_CKPT_SECTIONS_CORRUPTED;
               break;
       case 5:
               secChosen = CL_CKPT_SECTIONS_ANY;
               break;
       default:
               rc = CL_CKPT_ERR_NOT_EXIST;
               clDebugPrint(inMsg,"The given does not exists [%x]\n",rc);
               break;	
    }
    if(argc > 3)
    {
       expiryTime = atoll(argv[3]);	
    }   
    rc = clCkptSectionIterationInitialize(hdl,secChosen,expiryTime,&secHdl);
    if(rc != CL_OK)
        clDebugPrint(inMsg,"Section Iteration failed: rc %x \n",rc);
    else
        clDebugPrint(inMsg,"Section Iteration is completed: secHandle: %lld\n",secHdl);
    clDebugPrintFinalize(&inMsg,ret);
    return rc;
}

ClRcT ckptCliIterNext( int argc ,
                       char **argv,
                       ClCharT** ret)
{
    ClRcT            rc = 0 ;
    ClDebugPrintHandleT inMsg = 0;
    ClHandleT        secHdl = -1;
    ClCkptSectionDescriptorT   secDescriptor;
   /* ClIdlHandleT     ckptIdlHdl = 0;*/

    if(argc != 2)
    {
        ckptCliPrint("Usage: ckptSecNext <secHandle>\n"
                     "secHandle      [INT]  Handle of the section\n",ret);
        return CL_OK;
    }
    rc = clDebugPrintInitialize(&inMsg);
    secHdl = atoi(argv[1]);
    memset(&secDescriptor, '\0', sizeof(ClCkptSectionDescriptorT));
    rc = clCkptSectionIterationNext(secHdl,&secDescriptor);
    if(rc != CL_OK)
        rc = clDebugPrint(inMsg,"Section Iteration failed: rc %x \n",rc);
    else
    {
        clDebugPrint(inMsg,"Section Iteration is completed\n");
        clDebugPrint(inMsg,"Section Id   : %s\n",secDescriptor.sectionId.id);
        clDebugPrint(inMsg,"Section Size : %lld\n",secDescriptor.sectionSize);
        if(secDescriptor.sectionId.id != NULL) 
           clHeapFree(secDescriptor.sectionId.id);
    }   
    clDebugPrintFinalize(&inMsg,ret);
    return rc;
}
ClRcT ckptCliIterFin( int argc ,
                       char **argv,
                       ClCharT** ret)
{
    ClRcT               rc     = 0 ;
    ClDebugPrintHandleT inMsg  = NULL;
    ClHandleT           secHdl = CL_HANDLE_INVALID_VALUE;
   /* ClIdlHandleT     ckptIdlHdl = 0;*/

    if(argc != 2)
    {
        ckptCliPrint("Usage: ckptSecFin <secHandle>\n"
                     "secHandle        [INT]  Handle of the section\n",ret);
        return CL_OK;
    }
    rc = clDebugPrintInitialize(&inMsg);
    secHdl = atoi(argv[1]);
    rc = clCkptSectionIterationFinalize(secHdl);
    if(rc != CL_OK)
        clDebugPrint(inMsg,"Section Iteration  finalize failed: rc %x \n",rc);
    else
        clDebugPrint(inMsg,"Section Iteration finalize is completed\n");
    clDebugPrintFinalize(&inMsg,ret);
    return CL_OK;
}
ClRcT ckptCliSync( int argc ,
                       char **argv,
                       ClCharT** ret)
{
    ClRcT               rc      = 0 ;
    ClDebugPrintHandleT inMsg   = NULL;
    ClHandleT           hdl     = CL_HANDLE_INVALID_VALUE;
    ClTimeT             timeout = 0;
   /* ClIdlHandleT     ckptIdlHdl = 0;*/

    if(argc != 3)
    {
        ckptCliPrint("Usage: ckptSync <handle> <timeout>\n"
                     "handle        [INT]  Handle of the ckpt\n"
                     "timout        [INT]  timeout in millisec\n",ret);
        return CL_OK;
    }
    rc = clDebugPrintInitialize(&inMsg);
    hdl = atoi(argv[1]);
    timeout = atoi(argv[2]);
    rc = clCkptCheckpointSynchronize(hdl,timeout);
    if(rc != CL_OK)
        clDebugPrint(inMsg,"Ckpt Sync failed: rc %x \n",rc);
    else
        clDebugPrint(inMsg,"Ckpt Sync is completed\n");
    clDebugPrintFinalize(&inMsg,ret);
    return rc;
}

ClRcT ckptCliClientFinalize( int argc ,
                       char **argv,
                       ClCharT** ret)
{
    ClRcT               rc     = 0 ;
    ClDebugPrintHandleT inMsg  = NULL;

    if(argc != 1)
    {
        ckptCliPrint("Usage: ckptClientFin \n",ret);
        return CL_OK;
    }
    rc = clDebugPrintInitialize(&inMsg);
    rc = clCkptFinalize(1);
    if(rc != CL_OK)
        clDebugPrint(inMsg,"Ckpt Client finalize failed: rc %x \n",rc);
    else
        clDebugPrint(inMsg,"Ckpt Client finalize is completed\n");
    clDebugPrintFinalize(&inMsg,ret);
    return CL_OK;
}
ClRcT ckptMasterXlationTablePrint(ClCntKeyHandleT    userKey,
        ClCntDataHandleT   hashTable,
        ClCntArgHandleT    userArg,
        ClUint32T          dataLength)
{
    ClRcT                   rc          = CL_OK;
    CkptXlationDBEntryT    *pInXlation  = (CkptXlationDBEntryT *)hashTable;
    CKPT_NULL_CHECK(pInXlation);
    clOsalPrintf("CkptName   :%20.*s\n",pInXlation->name.length,
                 pInXlation->name.value);
    clOsalPrintf("checksum   :%20.d\n",pInXlation->cksum);
    clOsalPrintf("CkptHandle :%20.d\n",pInXlation->mastHdl);
    return rc;
}
ClRcT ckptMasterReplicaListPrint(ClCntKeyHandleT    userKey,
        ClCntDataHandleT   hashTable,
        ClCntArgHandleT    userArg,
        ClUint32T          dataLength)
{
    clOsalPrintf("%d   ",userKey);
    return CL_OK;
}
ClRcT ckptMasterDBEntryPrint(ClHandleDatabaseHandleT databaseHandle,
                             ClHandleT handle, void *pCookie)
{
    ClRcT                  rc              = CL_OK;
    CkptMasterDBEntryT     *pStoredData    = NULL;

    rc = clHandleCheckout(gCkptSvr->masterInfo.masterDBHdl,
            handle,(void **)&pStoredData);
    CKPT_ERR_CHECK_BEFORE_HDL_CHK(CL_CKPT_SVR,CL_DEBUG_ERROR,
            ("Ckpt: Failed to allocate the memory rc[0x %x]\n", rc), rc);
    if(pStoredData == NULL )
    {
       rc = CL_CKPT_ERR_INVALID_STATE;
       CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
            ("Ckpt:Proper data is not there rc[0x %x]\n", rc), rc);
    }
    clOsalPrintf("Name       :%s\t",pStoredData->name.value);
    clOsalPrintf("Handle     :%d\t",handle);
    clOsalPrintf("ActiveAdddr:%d\t",pStoredData->activeRepAddr);
    clOsalPrintf("PrevActAddr:%d\t",pStoredData->prevActiveRepAddr);
    clOsalPrintf("ActHandle  :%d\t",pStoredData->activeRepHdl);
    clOsalPrintf("Refcount   :%d\t",pStoredData->refCount);
    clOsalPrintf("isDelete   :%d\n",pStoredData->markedDelete);
    clOsalPrintf("List of Replicas =>  ");
    /* Pack the no. size of replica list first */
    if(pStoredData->replicaList != 0)
    {
        rc = clCntWalk(pStoredData->replicaList, ckptMasterReplicaListPrint,
            NULL,0);
       CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
            ("Ckpt:Proper data is not there rc[0x %x]\n", rc), rc);
    }
    clOsalPrintf("\n");
exitOnError:
    clHandleCheckin(gCkptSvr->masterInfo.masterDBHdl, handle);
exitOnErrorBeforeHdlCheckout:
    return rc;
}
ClRcT   ckptClientDBEntryPrint(ClHandleDatabaseHandleT databaseHandle,
                               ClHandleT handle , void *pCookie)
{
    CkptMasterDBClientInfoT *pClientData = NULL;
    ClRcT                    rc          = CL_OK;

    rc = clHandleCheckout( gCkptSvr->masterInfo.clientDBHdl,
            handle,(void **)&pClientData);
    CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
            ("Ckpt: Failed to allocate the memory rc[0x %x]\n", rc), rc);
    if(pClientData == NULL)
    {
       rc = CL_CKPT_ERR_INVALID_STATE;
       clHandleCheckin(gCkptSvr->masterInfo.clientDBHdl,handle);
       CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
            ("Ckpt:Proper data is not there rc[0x %x]\n", rc), rc);
    }
clOsalPrintf("%#llX\t%#llX\n",handle, pClientData->masterHdl);;
    clHandleCheckin(gCkptSvr->masterInfo.clientDBHdl, handle);
exitOnError:
  return rc;
}
ClRcT ckptMasterPeerListHdlPrint(ClCntKeyHandleT    userKey,
        ClCntDataHandleT   hashTable,
        ClCntArgHandleT    userArg,
        ClUint32T          dataLength)
{ 
    ClRcT       rc   = CL_OK;
    ClUint32T   flag = (ClUint32T)(ClWordT)userArg;            
    if(flag) 
    {
        CkptNodeListInfoT *pData  = (CkptNodeListInfoT*)hashTable;

    CKPT_NULL_CHECK(pData);
    clOsalPrintf("%llX\t[%d:%d]\n",
         pData->clientHdl,pData->appAddr,pData->appPortNum);
    }
    else
    {
        clOsalPrintf(" %#llX | ", *(ClHandleT *)userKey);
    }
    return rc;
}
ClRcT ckptMasterPeerListPrint(ClCntKeyHandleT    userKey,
        ClCntDataHandleT   hashTable,
        ClCntArgHandleT    userArg,
        ClUint32T          dataLength)
{
    ClRcT                  rc            = CL_OK;
    CkptPeerInfoT         *pPeerInfo     = (CkptPeerInfoT *) hashTable;
    ClUint32T              flag          = 0;

    CKPT_NULL_CHECK(pPeerInfo);

    clOsalPrintf("NodeAddr   :%20.d\n",pPeerInfo->addr);
    clOsalPrintf("Credential :%20.d\n",pPeerInfo->credential);
    clOsalPrintf("Available  :%20.d\n",pPeerInfo->available);
    clOsalPrintf("ReplicaCnt :%20.d\n",pPeerInfo->replicaCount);
    clOsalPrintf("NodeInfo:\n");
    /* Pack the no. of handle per peer first */
    if(pPeerInfo->ckptList != 0)
    {
        clOsalPrintf("ClientHdl [appAddr:portId]\n");
        flag = 1;
        rc = clCntWalk(pPeerInfo->ckptList, ckptMasterPeerListHdlPrint,
                       (ClPtrT)(ClWordT)flag, sizeof(flag));
    }        

    if(pPeerInfo->mastHdlList != 0)
    {
        flag = 0;
        clOsalPrintf("MastHdlList:\n");
        rc = clCntWalk(pPeerInfo->mastHdlList, ckptMasterPeerListHdlPrint,
                       (ClPtrT)(ClWordT)flag, sizeof(flag));
        clOsalPrintf("\n");
    }        
    return rc;
}

ClRcT ckptCliDbShow(int argc,
                    char **argv,
                    ClCharT** ret)
{
    ClRcT               rc        =  0 ;
    ClDebugPrintHandleT inMsg     = NULL;
    ClUint32T           ckptCount =  0;

    if(argc != 1)
    {
        ckptCliPrint("Usage: ckptDBShow \n",ret);
        return CL_OK;
    }
    rc = clDebugPrintInitialize(&inMsg);
      
    clOsalPrintf("MasterAddr :%20.d\n",gCkptSvr->masterInfo.masterAddr);
    clOsalPrintf("DeputyAddr :%20.d\n",gCkptSvr->masterInfo.deputyAddr);
    clOsalPrintf("ComponentId:%20.d\n",gCkptSvr->masterInfo.compId);
    rc = clCntSizeGet(gCkptSvr->masterInfo.nameXlationDBHdl,&ckptCount);
    clOsalPrintf("No of Ckpts:%20.d\n",ckptCount);    
    clOsalPrintf("----------------Xlation Entries------------------------- \n");                 
    rc = clCntWalk( gCkptSvr->masterInfo.nameXlationDBHdl,
                     ckptMasterXlationTablePrint,NULL,0);
    clOsalPrintf("----------------MasterDB Entries------------------------ \n");                 
    rc = clHandleWalk( gCkptSvr->masterInfo.masterDBHdl,
                       ckptMasterDBEntryPrint,NULL);
    clOsalPrintf("----------------ClientDB Entries------------------------ \n");                 
    clOsalPrintf("ClientHdl MasterHdl\n");
    rc = clHandleWalk( gCkptSvr->masterInfo.clientDBHdl,
                       ckptClientDBEntryPrint,NULL);
    clOsalPrintf("----------------PeerList Entries------------------------ \n");                 
     rc = clCntWalk( gCkptSvr->masterInfo.peerList,
                     ckptMasterPeerListPrint,
                     NULL,0);
    /*Pack Master DB entries */
    clDebugPrintFinalize(&inMsg,ret);
    return rc;
}

