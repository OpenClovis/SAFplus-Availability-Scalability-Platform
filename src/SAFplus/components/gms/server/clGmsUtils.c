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
 * File        : clGmsUtils.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This file implements the utility functions for GMS component.
 *****************************************************************************/

#include <dlfcn.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <sys/types.h>
#ifndef VXWORKS_BUILD
#include <grp.h>
#include <pwd.h>
#endif
#include <clDebugApi.h>
#include <clGms.h>
#include <clLogApi.h>
#include <ezxml.h>
#include <errno.h>
#include <ctype.h>
#include <clParserApi.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <clGmsUtils.h>
#include <clGmsTipc.h>
#ifdef SOLARIS_BUILD
#include <unistd.h>
#include <stropts.h>
#include <sys/sockio.h>
#endif


/* Utility function to print the messages on to the CLI Console */
void 
_clGmsCliPrint (char  **retstr, char *format , ... )
{
	va_list ap;
	va_start( ap , format );
	vsprintf(*retstr + strlen(*retstr) , format , ap );
	va_end( ap );
	return;
}

void 
_clGmsCliPrintExtended(ClCharT **retstr, ClInt32T *maxBytes, ClInt32T *curBytes, const ClCharT *format, ...)
{
    va_list ap;
    ClInt32T len;
    ClCharT c;
    va_start(ap, format);
    len = vsnprintf(&c, 1, format, ap);
    va_end(ap);
    if(!len) return ;
    ++len;
    if(!*maxBytes) *maxBytes = CL_MAX(512, len<<1);
    if(!*retstr || (*curBytes + len) >= *maxBytes)
    {
        if(!*retstr) *curBytes = 0;
        *maxBytes *= ( *curBytes ? 2 : 1 );
        *retstr = clHeapRealloc(*retstr, *maxBytes);
        CL_ASSERT(*retstr != NULL);
    }
    va_start(ap, format);
    *curBytes += vsnprintf(*retstr + *curBytes, *maxBytes - *curBytes, format, ap);
    va_end(ap);
}

/*
   GmsPluginLoader
   ---------------
   Loads the plugin in to the Gms process space and stores the algorithm in to
   the Gms leader election algorithm database in the GMS global structure  
   indexing by the groupid. If the plugin cant be loaded successfully then the
   default algorithm is used for that group . Default algorithm is documented
   in the clGmsEngine.c File. 
 */
void *pluginHandle;

#ifndef VXWORKS_BUILD 
void 
_clGmsLoadUserAlgorithm(const ClUint32T groupid , 
                        char* const    pluginPath)
{
    const ClGmsLeaderElectionAlgorithmT algorithm = NULL;


    /* load the plugin in to process address space */ 
    pluginHandle = dlopen(pluginPath, RTLD_NOW | RTLD_GLOBAL );
    if(pluginHandle == NULL)
    {
        clLogMultiline(ERROR,GEN,NA,
                 "Unable to open the leader election plugin :[%s],"
                 "reason: [%s] ",pluginPath,dlerror()
                 );
        return;
    }

    /* Get the algorithm from the plugin */ 
    *(void**)&algorithm =dlsym( 
            pluginHandle, 
            LEADER_ELECTION_ALGORITHM);
    if(algorithm == NULL)
    {
        clLog(ERROR,GEN,NA,
                 "Unable to find the algorithm in plugin:[%s] %s",
                 pluginPath,dlerror());
        dlclose(pluginHandle);
        return;
    }

    /* Store the algorithm handle in the groups slot of the GmsConfiguration
     *  structure */ 
    clLogMultiline (INFO,GEN,NA, 
             "Group [%d] is using [%s] for leader election; Loaded from [%s]", 
            groupid, LEADER_ELECTION_ALGORITHM, pluginPath);
    gmsGlobalInfo.config.leaderAlgDb[groupid] = algorithm;

    return;
}

#else

void 
_clGmsLoadUserAlgorithm(const ClUint32T groupid , 
                        char* const    pluginPath)
{
    pluginHandle = NULL;
    return;
}

#endif

/*
   GetIfAddress
   -------------
   Gets the interface address given the name of the interface. 
 */

ClBoolT 
_clGmsGetIfAddress(
        char* const  ifName , 
        struct in_addr* const ifAddress
        )
{
    int sd = -1;
#ifdef SOLARIS_BUILD
    struct lifreq ifr;
    #define IFR_NAME_MEMBER ifr.lifr_name
    #define IFR_ADDR_MEMBER ifr.lifr_addr
    #define SIO_GET_IF_ADDRESS SIOCGLIFADDR
#else
    struct ifreq ifr;
    #define IFR_NAME_MEMBER ifr.ifr_name
    #define IFR_ADDR_MEMBER ifr.ifr_addr
    #define SIO_GET_IF_ADDRESS SIOCGIFADDR
#endif
    char interface[256];
    char *env = NULL;

    errno = 0x0;
    memset(&ifr, 0 , sizeof(ifr));
    env = getenv("ASP_MULTINODE");
    if ((env != NULL) && (!strncmp(env,"1",strlen(env))))
    {
        sprintf(interface, "%s:%d",ifName,gmsGlobalInfo.nodeAddr.iocPhyAddress.nodeAddress);
        strncpy(IFR_NAME_MEMBER ,interface,sizeof(IFR_NAME_MEMBER)-1);
    }
    else 
    {
        strncpy(IFR_NAME_MEMBER , ifName, sizeof(IFR_NAME_MEMBER)-1);
    }
    strncpy(gmsGlobalInfo.config.ifName,IFR_NAME_MEMBER,CL_MAX_NAME_LENGTH-1);

    sd = socket( AF_INET , SOCK_DGRAM  , 0 );
    if( sd < 0 ){
        clLog(EMER,GEN,NA, 
                "Socket open call failed while getting interface details: [%s]", strerror(errno)
                );
        exit(0);
    }

    if( ioctl( sd, SIO_GET_IF_ADDRESS, &ifr ) < 0 ){
        clLogMultiline(EMER,GEN,NA,
                "ioctl() system call failed while getting details of interface [%s]\n"
                "ioctl() returned error [%s]\n"
                "This could be because the interface [%s] is not configured on the system",
                IFR_NAME_MEMBER,strerror(errno),IFR_NAME_MEMBER);
        close(sd);
        exit(0);
    }

    memcpy( 
            ifAddress ,
            &(((struct sockaddr_in*)&IFR_ADDR_MEMBER)->sin_addr),
            sizeof(struct in_addr)
          );
    close(sd);
    return CL_TRUE;
}

int OpenAisConfFileCreate(char* ipAddr,
                          char* mcastAddr,
                          char* mcastPort)
{
    FILE *fd = NULL;
    char *env = NULL;
    char logToStderr[OPTION_STR_SIZE] = "no";
    char logToFile[OPTION_STR_SIZE] = "no";
    char logToSyslog[OPTION_STR_SIZE] = "no";
    char logFileName[MAX_FILE_NAME] = "OpenAis.log";
    char confFileName[MAX_FILE_NAME] = "";
    char *name = NULL;
#ifndef VXWORKS_BUILD
    struct group *gp = NULL;
    struct passwd *pw = NULL;
#endif
    char path[MAX_PATH_SIZE] = "";

    memset(gmsGlobalInfo.config.aisUserId, 0, 
           sizeof(gmsGlobalInfo.config.aisUserId));

    memset(gmsGlobalInfo.config.aisGroupId, 0,
           sizeof(gmsGlobalInfo.config.aisGroupId));

#ifndef VXWORKS_BUILD
    pw = getpwuid(getuid());
    if(!pw)
    {
        clLogError(GEN, NA, "UID [%d] not found in passwd db", (int)getuid());
        exit(1);
    }
    name = pw->pw_name;
#else
    name = "root";
#endif
    strncpy(gmsGlobalInfo.config.aisUserId, name,
            sizeof(gmsGlobalInfo.config.aisUserId)-1);

#ifndef VXWORKS_BUILD
    gp = getgrgid(getgid());
    if(!gp)
    {
        clLogError(GEN, NA, "GID [%d] not found in group db", (int)getgid());
        exit(1);
    }
    name = gp->gr_name;
#else
    name = "root";
#endif
    strncpy(gmsGlobalInfo.config.aisGroupId, name,
            sizeof(gmsGlobalInfo.config.aisGroupId)-1);

#ifndef VXWORKS_BUILD
    endpwent();
    endgrent();
#endif

    env = getenv("ASP_RUNDIR");
    if (env != NULL)
    {
        clLogDebug(GEN,NA, "ASP_RUNDIR value %s",env);
        strncpy(path,env,MAX_PATH_SIZE-1);
        setenv("OPENAIS_RUN_DIR", path, 1);
        snprintf(confFileName,MAX_FILE_NAME,"%s/openais_%d.conf",
                path, gmsGlobalInfo.nodeAddr.iocPhyAddress.nodeAddress);
    }
    else
    {
        setenv("OPENAIS_RUN_DIR", ".", 1);
        snprintf(confFileName,MAX_FILE_NAME,"openais_%d.conf",
                gmsGlobalInfo.nodeAddr.iocPhyAddress.nodeAddress);
    }

    env = getenv("ASP_LOGDIR");
    if (env != NULL)
    {
        clLogDebug(GEN,NA, "ASP_LOGDIR value %s",env);
        strncpy(logFileName, env, MAX_FILE_NAME-1);
        strncat(logFileName, "/OpenAis.log", (MAX_FILE_NAME-strlen(logFileName)-1));
    }

    if (strcmp(gmsGlobalInfo.config.aisLogOption, "stderr") == 0)
    {
        /* Logging option to stderr is provided. Enable logging to stderr */
        strncpy(logToStderr, "yes",OPTION_STR_SIZE-1);
    }
    else if (strcmp(gmsGlobalInfo.config.aisLogOption, "file") == 0)
    {
        /* Log to file is provided. Enable file logging */
        strncpy(logToFile,"yes",OPTION_STR_SIZE-1);
    }
    else if (strcmp(gmsGlobalInfo.config.aisLogOption, "syslog") == 0)
    {
        strncpy(logToSyslog,"yes",OPTION_STR_SIZE-1);
    } 
    else
    {
        /* Ignore none entries, and keeping it as a last else block
         * to keep static analyzers happy, instead of finishing with else-if
        */
        if(strcmp(gmsGlobalInfo.config.aisLogOption, "none") )
        {
            clLogMultiline(ERROR,GEN,NA,
                           "Invalid value specified in the config file for 'AisLoggingOption' "
                           "Valid values are 'stderr', 'file', 'syslog' and 'none'");
        }
    }

    fd = fopen(confFileName,"w+");
    if (fd == NULL)
    {
        clLogMultiline(CRITICAL,GEN,NA,
                       "Failed to open \'openais.conf\' file to write config info "
                       "needed by openais. System Returned [%s]",strerror(errno));
		exit(-1);
    }
    fprintf(fd, "totem {\n"
            "\t    version: 2\n"
            "\t    secauth: off\n"
            "\t    threads: 0\n"
#ifdef OPENAIS_TIPC
            "\t    netmtu: 9000\n"
#endif
            "\t    interface {\n"
            "\t\t        ringnumber: 0\n"
#ifndef OPENAIS_TIPC
            "\t\t        bindnetaddr: %s\n"
            "\t\t        mcastaddr: %s\n"
            "\t\t        mcastport: %s\n"
#else
            "\t\t        bindnetaddr: %d:%d\n"
            "\t\t        mcastaddr: %d\n"
            "\t\t        mcastport: %d\n"
#endif
            "\t    }\n"
            "}\n\n"

            "logging {\n"
            "\t    to_stderr: %s\n"
            "\t    to_file: %s\n"
            "\t    to_syslog: %s\n"
            "\t    syslog_facility: local5\n"
            "\t    logfile: %s\n"
            "\t    debug: on\n"
            "\t    timestamp: on\n"
            "}\n\n"

            "amf {\n"
            "\t    mode: disabled\n"
            "}\n\n"

            "aisexec {\n"
            "\t    user: %s\n"
            "\t    group: %s\n"
            "\t    defaultservices: no\n"
            "}\n\n"
            
            "service {\n"
            "\t    name: clovis_gms\n"
            "\t    ver: 0\n"
            "}\n",
#ifndef OPENAIS_TIPC
            ipAddr, mcastAddr, mcastPort,
#else
            CL_OPENAIS_TIPC_UCAST_TYPE,
            gmsGlobalInfo.nodeAddr.iocPhyAddress.nodeAddress,
            CL_OPENAIS_TIPC_MCAST_RANGE, CL_OPENAIS_TIPC_MCAST_TYPE,
#endif
            logToStderr,
            logToFile, logToSyslog, logFileName,
            gmsGlobalInfo.config.aisUserId,
            gmsGlobalInfo.config.aisGroupId);

    setenv("OPENAIS_MAIN_CONFIG_FILE", confFileName, 1);

    
    fclose(fd);

    return 0;
}

int getNameString(ClNameT *name, ClCharT *retName)
{
    ClUint32T   i=0;

    if (name != NULL)
    {
        for (i = 0; i < name->length; i++)
        {
            retName[i]=name->value[i];
        }
        retName[i]='\0';
        return 0;
    }
    return 1;
}
