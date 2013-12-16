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
 * ModuleName  : gms
 * File        : clGms.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This file implements the GMS function calls that are called from clGmsEo.c 
 * file.
 *****************************************************************************/

#include <string.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <ctype.h>

#include <clCommonErrors.h>
#include <clDebugApi.h>
#include <clBufferApi.h>
#include <clGms.h>
#include <clGmsView.h>
#include <clGmsErrors.h>
#include <clGmsEngine.h>
#include <clLogApi.h>
#include <clGmsCli.h>
#include <clGmsConf.h>
#include <clRmdApi.h>
#include <ezxml.h>
#include <clGmsCkpt.h>

ClGmsCsSectionT joinCs;
ClGmsCsSectionT groupJoinCs;
ClGmsNodeT gmsGlobalInfo;
ClBoolT    readyToServeGroups = CL_FALSE;
ClHandleDatabaseHandleT contextHandleDatabase;
ClBoolT gClTotemRunning = CL_FALSE;

static ClVersionT versions_supported[] = {
	{ 'B', 0x01, 0x01 }
};

static void contextHandleDatabaseDestructor (void *dummy);


/*
   _clGmsServiceInitialize 
   ------------------------
   This function is the main kick off point for the GMS service , All the
   submodules Initializations are done in this function. This function finally
   calls in to the OpenAIS layer and never returns.
 */
void 
_clGmsServiceInitialize ( const int argc  , char* const argv[] )
{

    ClRcT rc = CL_OK;

    if (argc < 2)
    {
        clLog(EMER,GEN,NA,
                "Config file parameter is not specified. Usage: "
                "safplus_gms <gmsConfigFile>");
        exit(-1);
    }

    /* Set the communication parameters */
    gmsGlobalInfo.nodeAddr.iocPhyAddress.nodeAddress
        =  clIocLocalAddressGet();
    gmsGlobalInfo.nodeAddr.iocPhyAddress.portId = CL_IOC_GMS_PORT;

    /*
       Reads the configuration from the Config file and populates the gms
       context structure .
     */

    _clGmsLoadConfiguration ( argv[1] );


    /* Crete view db mutex */
    clGmsMutexCreate(&gmsGlobalInfo.dbMutex);

    /* Create the contextHandleDatabase */
    rc = clHandleDatabaseCreate(contextHandleDatabaseDestructor,
                                &contextHandleDatabase);
    if (rc != CL_OK)
    {
        clLog(EMER,GEN,NA,
                "GMS context handle database creataion failed. Booting aborted");
        exit(-1);
    }
    /*
       Create the View Database and create the view for the Cluster by default
       .
       */
    rc = _clGmsViewInitialize (  );
    if( rc != CL_OK )
    {
        clLog(EMER,GEN,NA,
                "View database creation failed. Booting aborted");
        exit(-1);
    }

    /* Create a table of groupName, groupId */
    rc = _clGmsNameIdDbCreate(&gmsGlobalInfo.groupNameIdDb);
    if (rc != CL_OK)
    {
        clLog(EMER,GEN,NA,
                "GroupName-GroupId database creation failed. Booting aborted");
        exit(-1);
    }

    /* Create the name-id dbMutex */
    rc = clOsalMutexCreate(&gmsGlobalInfo.nameIdMutex);
    if (rc != CL_OK)
    {
        clLog(EMER,GEN,NA,
                    "Unable to create mutex to protect name-id database");
        exit(-1);
    }

    /*
       Create the Join Critical Section to detect the version mismatch . 
       */
    clGmsCsCreate( &joinCs );
    clGmsCsCreate( &groupJoinCs );

	/* Initialize checkpoint metadata */
	rc = clGmsCkptInit();
    if(rc != CL_OK)
    {
        clLog(EMER,GEN,NA,
              "GMS ckpt initialize failed with [%#x]", rc);
        exit(-1);
    }

    /* Set the software version of the GMS server */
    gmsGlobalInfo.config.thisNodeInfo.gmsVersion.releaseCode= 'B';
    gmsGlobalInfo.config.thisNodeInfo.gmsVersion.majorVersion= 0x01;
    gmsGlobalInfo.config.thisNodeInfo.gmsVersion.minorVersion= 0x01;
    clLog(INFO,GEN,NA,
            "GMS service version [%c.%d.%d] started",
            gmsGlobalInfo.config.thisNodeInfo.gmsVersion.releaseCode,
            gmsGlobalInfo.config.thisNodeInfo.gmsVersion.majorVersion,
            gmsGlobalInfo.config.thisNodeInfo.gmsVersion.minorVersion);



    /* Initialize the version compatibility database */
    gmsGlobalInfo.config.versionsSupported.versionCount = 
        sizeof(versions_supported)/sizeof(ClVersionT);
    gmsGlobalInfo.config.versionsSupported.versionsSupported=
        versions_supported;

    /*
     * Starts the GMS engine and calls in to the openais code and starts
     * listening events on the ring.
     * parameters are log to stderr , log to file , log file .
     */
    clLog(NOTICE,GEN,NA, "GMS server fully up");
    clLog(INFO,GEN,NA, "Starting gms engine");
    rc = _clGmsEngineStart ();
    if( rc != CL_OK )
    {
        clLog(EMER,GEN,NA,
                "Gms engine start failed with rc [0x%x]. Booting aborted",rc);
        exit(-1);
    }
    return;
}


/*
   _clGmsSetThisNodeInfo
   ----------------------
   Sets the node profile , called from the API handlers when CPM issues a
   cluster join .
   */
void 
_clGmsSetThisNodeInfo(
                const ClGmsNodeIdT                nodeId, 
                const SaNameT* const              nodeName, 
                const ClGmsLeadershipCredentialsT credential
                )
{
    time_t      t                             = {0};
    ClCharT     retName[CL_MAX_NAME_LENGTH+1] = "";
    ClCharT     timeBuffer[256]               = {0};

    CL_ASSERT(nodeName != NULL);

    gmsGlobalInfo.config.thisNodeInfo.nodeId = nodeId;

    gmsGlobalInfo.config.thisNodeInfo.credential = credential;

    strncpy((ClCharT *) gmsGlobalInfo.config.thisNodeInfo.nodeName.value, (const ClCharT *) nodeName->value, CL_MAX_NAME_LENGTH);

    /* NULL terminate in case of overflow, if no overflow, this has no effect */
    gmsGlobalInfo.config.thisNodeInfo.nodeName.value[CL_MAX_NAME_LENGTH-1] = 0;  
    gmsGlobalInfo.config.thisNodeInfo.nodeName.length = strlen((const ClCharT *)nodeName->value);


    gmsGlobalInfo.config.thisNodeInfo.nodeAddress.iocPhyAddress.nodeAddress
                                    = clIocLocalAddressGet();
    gmsGlobalInfo.config.thisNodeInfo.nodeAddress.iocPhyAddress.portId
                                        = (ClIocNodeAddressT) CL_IOC_GMS_PORT;

    if (time(&t) == -1)
    {
        clLog(ERROR,GEN,NA,
                "Failed to get system time");
    }
    gmsGlobalInfo.config.thisNodeInfo.bootTimestamp =
                                                (ClTimeT)(t*CL_GMS_NANO_SEC);

    gmsGlobalInfo.config.thisNodeInfo.memberActive = CL_TRUE;

    getNameString(&gmsGlobalInfo.config.thisNodeInfo.nodeName,retName);

    clLogMultiline(INFO,CLM,NA,"This node Information:\n"
                   "Node ID [%d]\nCredentials [%d]\nNode Name [%s]\nIOC Address [%d:0x%x]\nBoot Timestamp [%s]",
                   nodeId,credential,retName,
                   gmsGlobalInfo.config.thisNodeInfo.nodeAddress.iocPhyAddress.nodeAddress,
                   gmsGlobalInfo.config.thisNodeInfo.nodeAddress.iocPhyAddress.portId,
                   ctime_r((const time_t*)&t,timeBuffer));
    return;
}


/*
   _clGmsGetThisNodeInfo
   ---------------------
   Utility function to get the node information called from various places in
   the code .
   */
void 
_clGmsGetThisNodeInfo(
                ClGmsClusterMemberT * const thisClusterMember)
{
    CL_ASSERT(thisClusterMember != NULL);
    *thisClusterMember =  gmsGlobalInfo.config.thisNodeInfo;
    return;
}

static void contextHandleDatabaseDestructor (void *dummy)
{
    /* Not implemented */
}

