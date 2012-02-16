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
 * ModuleName  : eo
 * File        : clEoMain.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *          This file contains the implementation of the EO main function.
 *
 *
 ****************************************************************************/

#include <sys/types.h>
#include <unistd.h>

#include <clCommon.h>
#include <clDebugApi.h>
#include <clEoIpi.h>
#include <clLogApi.h>

/*
 * Local and master addresses. 
 */
extern ClUint32T clAspLocalId; /* Defined in utils */
ClUint32T clEoWithOutCpm;

/** Name of the node.  Loaded from the same-named environment variable.  */
ClCharT ASP_NODENAME[CL_MAX_NAME_LENGTH]="";
/** Name of the component.  Loaded from the same-named environment variable.  */
ClCharT ASP_COMPNAME[CL_MAX_NAME_LENGTH]="";
/** Address of the node.  Loaded from the same-named environment variable.  */
ClUint32T ASP_NODEADDR=0;

/** Working dir where programs are run. Loaded from the same-named environment variable.  */
ClCharT ASP_RUNDIR[CL_MAX_NAME_LENGTH]="";
/** Dir where logs are stored. Loaded from the same-named environment variable.  */
ClCharT ASP_LOGDIR[CL_MAX_NAME_LENGTH]="";
/** Dir where ASP binaries are located. Loaded from the same-named environment variable.  */
ClCharT ASP_BINDIR[CL_MAX_NAME_LENGTH]="";
/** Dir where xml config are located. Loaded from the same-named environment variable.  */
ClCharT ASP_CONFIG[CL_MAX_NAME_LENGTH]="";
/** Dir where db files are to be stored. Loaded from the same-named environment variable.  */
ClCharT ASP_DBDIR[CL_MAX_NAME_LENGTH]="";
/** Dir where application binaries are located. Derived from ASP_BINDIR and argv[0].  Deprecated.  Use ASP_APP_BINDIR */
ClCharT CL_APP_BINDIR[CL_MAX_NAME_LENGTH]="";
/** Dir where application binaries are located. Derived from ASP_BINDIR and argv[0]. */
ClCharT ASP_APP_BINDIR[CL_MAX_NAME_LENGTH]="";

/** Variable to check if the current node is a system controller node.  Loaded from the same-named environment variable.  */
ClBoolT SYSTEM_CONTROLLER = CL_FALSE; 
/** Variable to check if the current node is a SC capable node.  Loaded from the same-named environment variable.  */
ClBoolT ASP_SC_PROMOTE = CL_FALSE;

#define LOG_AREA "EO"
#define LOG_CTXT "INI"

void clLoadEnvVars()
{
    ClCharT missing[512];
    ClCharT * temp=NULL;
    ClInt32T i = 0;
 
    missing[0] = 0;

    ClCharT* envvars[] = { "ASP_NODENAME", "ASP_COMPNAME", "ASP_RUNDIR", "ASP_LOGDIR", "ASP_BINDIR", "ASP_CONFIG", "ASP_DBDIR","ASP_APP_BINDIR", 0 };
    ClCharT* storage[] = { ASP_NODENAME ,  ASP_COMPNAME ,  ASP_RUNDIR ,  ASP_LOGDIR ,  ASP_BINDIR ,  ASP_CONFIG , ASP_DBDIR, ASP_APP_BINDIR, 0 };

   
    for (i=0; envvars[i] != 0; i++)
      {
        temp = getenv(envvars[i]);
        if (temp) strncpy(storage[i],temp,CL_MAX_NAME_LENGTH-1);
        else 
          {
              strcat(missing,envvars[i]);
              strcat(missing," ");
          }
      }

    strcpy(CL_APP_BINDIR,ASP_APP_BINDIR);
    
    temp = getenv("ASP_NODEADDR");
    if (temp) ASP_NODEADDR = atoi(temp);
    else strcat(missing,"ASP_NODEADDR ");

    SYSTEM_CONTROLLER = clParseEnvBoolean("SYSTEM_CONTROLLER");
    ASP_SC_PROMOTE = clParseEnvBoolean("ASP_SC_PROMOTE");

    if (missing[0])
      {
          clLog(CL_LOG_CRITICAL, LOG_AREA, LOG_CTXT,
                "The following required environment variables are not set: %s. Exiting", missing);
          exit(1);  
      }
}

ClRcT clEoInitialize(ClInt32T argc, ClCharT *argv[])
{
#ifdef CL_COMPONENT_TESTING
    ClCharT *testing_mode = NULL;
#endif
    ClRcT rc = CL_OK;

    ClCharT *iocAddressStr;

    clLog(CL_LOG_INFO, LOG_AREA, LOG_CTXT,
          "Process [%s] started. PID [%d]", argv[0], (int)getpid());
    
    
    /*
     * This needs to be enabled when we are ready to ship 
     */
#ifdef CL_COMPONENT_TESTING
    /*
     * Parse the command line options. 
     */
    {
        /*
         * this is testing mode 
         */
        if (argc == 3)
        {
            myCh = atoi(argv[1]);
            mySl = atoi(argv[2]);
        }
        else
        {
            clLog(CL_LOG_CRITICAL, LOG_AREA, LOG_CTXT,
                "Usage: %s <chassis-id> <slot-id>", argv[0]);
            return 1;
        }

        clMyId = (myCh << 24);  /* move chassis ID to the 3rd LSB */
        clMyId |= mySl;         /* slot ID is the LSB */
    }
#else
    {
        /*
         * This is non-testing mode when it is started by CPM. This will
         * execute when CL_COMPONENT_TESTING is set in Makefile or when
         * CLOVIS_TESTING is not set in the environment. 
         */

        iocAddressStr = getenv("ASP_NODEADDR");
        if (iocAddressStr != NULL)
            clAspLocalId = atoi(iocAddressStr);
        else
        {
            clLog(CL_LOG_CRITICAL, LOG_AREA, LOG_CTXT,
                "ASP_NODEADDR environment variable not set, exiting");
            exit(1);
        }

        clEoWithOutCpm = clParseEnvBoolean("ASP_WITHOUT_CPM");
    }
#endif /* CL_COMPONENT_TESTING */

    clLoadEnvVars();
    
    rc = clEoMain(argc, argv);
    if (CL_OK != rc)
    {
        clLog(CL_LOG_CRITICAL, LOG_AREA, LOG_CTXT,
            "Process [%s] exited abnormally. Exit code [%u/0x%x]",
            argv[0], rc, rc);
    }
    else
    {
        clLog(CL_LOG_INFO, LOG_AREA, LOG_CTXT,
            "Process [%s] exited normally", argv[0]);
    }
    if (clDbgNoKillComponents)
    {
      clLog(CL_LOG_CRITICAL, LOG_AREA, LOG_CTXT,
            "In debug mode and 'clDbgNoKillComponents' is set, so this process will pause, not exit.");
      while(1) sleep(10000); /* Using sleep here instead of Osal because Osal is likely shutdown */
    }
    return rc;

}

