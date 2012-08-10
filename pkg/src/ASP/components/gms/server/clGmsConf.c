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
 * ModuleName  : gms
 * File        : clGmsConf.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This file contains the configuration loader for the GMS server Instance .
 *****************************************************************************/


#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <clLogApi.h>
#include <clParserApi.h>
#include <clGms.h>
#include <clGmsConf.h>

/*
   _clGmsLoadConfiguration
   ------------------------
   Reads the Configuration file searches for "clGmsConfig.xml" in colon
   separated paths of "ASP_CONFIG" environment variable. picks up the first
   occurence. Routine populates the Global Gms Context/Configuration
   structure.
   */
void 
_clGmsLoadConfiguration ( char* const gmsConfigFile )
{
    ClParserPtrT cluster_name = NULL;
    ClParserPtrT max_group_num = NULL;
    ClParserPtrT groupid = NULL;
    ClParserPtrT plugin_path = NULL;
    ClParserPtrT group = NULL;
    ClParserPtrT openais_logging = NULL;
    ClParserPtrT xmlConfig = NULL;
    ClParserPtrT gmsConfig = NULL;
    ClParserPtrT preferredActiveSCNodeName = NULL;
    ClParserPtrT bootElectionTimeout = NULL;
    ClCharT      env[1024] = "";
#ifndef OPENAIS_TIPC
    ClParserPtrT mcast_port = NULL;
    ClCharT      net_addr[64] = "eth0";
    ClParserPtrT mcast_addr = NULL;
    ClCharT     *start = NULL;
#endif
    ClUint32T    i;
    ClCharT     *temp = NULL;
    ClInt32T electionTimeout = 0;

    temp = getenv("ASP_CONFIG");
    if (temp == NULL)
    {
        clLog(EMER,GEN,NA,
              "ASP_CONFIG environment variable is not set. It must be set "
              "to the location of the ASP xml config files.");
        exit(-1);
    }
    strncpy(env, temp,1023);

    xmlConfig = clParserOpenFile(env, gmsConfigFile );
    if (xmlConfig == NULL)
    {
        clLogMultiline(EMER,GEN,NA,
                       "GMS Configuration file [%s] is not present in "
                       "any of the dirs pointed by ASP_CONFIG env variable (%s) ",env, gmsConfigFile);
        exit(-1);
    }

    /* Verify for the gms tag in the config file */
    gmsConfig = clParserChild(xmlConfig, "gms");
    if (gmsConfig == NULL)
    {
        clLogMultiline(EMER,GEN,NA,
                       "Improper config file [%s] detected. \'gms\' "
                       "section entry is missing in the config file",gmsConfigFile);
        exit(-1);
    }


    /* Look for ClusterName to which this node wants to become a member of */
    cluster_name = clParserChild(gmsConfig , "clusterName");

    if (cluster_name == NULL)
    {
        /* Check for backward compatibility */
        cluster_name = clParserChild(gmsConfig , "ClusterName");
        if (cluster_name == NULL)
        {
            clLogMultiline(EMER,GEN,NA,
                           "Improper config file [%s] detected. \'clusterName\' "
                           "entry is not found",gmsConfigFile);
            exit(-1);
        } else {
            clLogMultiline(NOTICE,GEN,NA,
                           "tag names in %s have changed. You are using the old config file."
                           "As per new convention, 'ClusterName' tag is renamed to 'clusterName'",
                           gmsConfigFile);
        }
    }

    strncpy(gmsGlobalInfo.config.clusterName.value,
            cluster_name->txt, 
            sizeof(gmsGlobalInfo.config.clusterName.value)-1);

    gmsGlobalInfo.config.clusterName.length = 
        strlen(gmsGlobalInfo.config.clusterName.value);

    /* Look for Max number of groups */
    max_group_num = clParserChild(gmsConfig, "maxNoOfGroups");

    if (max_group_num == NULL)
    {
        max_group_num = clParserChild(gmsConfig, "MaxNoOfGroups");
        if (max_group_num == NULL)
        {
            clLogMultiline(EMER,GEN,NA,
                           "Improper config file [%s] detected. \'maxNoOfGroups\' "
                           "entry is not found",gmsConfigFile);
            exit(-1);
        } else {
            clLogMultiline(NOTICE,GEN,NA,
                           "tag names in %s have changed. You are using the old config file."
                           "As per new convention, 'MaxNoOfGroups' tag is renamed to 'maxNoOfGroups'",
                           gmsConfigFile);
        }
    }

    /* Verify the value provided to MaxNoOfGroups */
    errno = 0;
    gmsGlobalInfo.config.noOfGroups  = strtol(max_group_num->txt, NULL, 10);
    if (errno != 0)
    {
        clLogMultiline(EMER,GEN,NA,
                       "Improper value is specified for \'maxNoOfGroups\' "
                       "entry in the config file [%s]",gmsConfigFile);
        exit(-1);
    }

#ifndef OPENAIS_TIPC
    /* Look for linkname tag to get interface name on which
     * this instance of GMS will bind to */
    temp = getenv("LINK_NAME");
    if (temp == NULL)
    {
        clLogMultiline(NOTICE,GEN,NA,
                       "LINK_NAME environment variable is not exported. Using 'eth0' "
                       "interface as default");
    } else {
        clLog(DBG,GEN,NA,
              "LINK_NAME env is exported. Value is %s",temp);
        strncpy(net_addr, temp, 63);
    }

    /* Get the IP address from the interface name */
    if (_clGmsGetIfAddress (net_addr, 
                            &gmsGlobalInfo.config.bind_net.sin_addr) != CL_TRUE)
    {
        clLogMultiline(EMER,GEN,NA,
                       "Could not get interface address from given interface name."
                       "Please check if LINK_NAME env variable is exported properly");
        exit(-1);
    }

    /* Read the multicast address and store it in the 
     * GMS GlobalInformation */
    mcast_addr = clParserChild(gmsConfig, "multicastAddress"); 
    if (mcast_addr == NULL)
    {
        mcast_addr = clParserChild(gmsConfig, "MulticastAddress");
        if (mcast_addr == NULL)
        {
            clLog(EMER,GEN,NA,
                  "Improper config file [%s] detected. \'multicastAddress\'"
                  "entry is not found",gmsConfigFile);
            exit(-1);
        } else {
            clLogMultiline(NOTICE,GEN,NA,
                           "tag names in %s have changed. You are using the old config file."
                           "As per new convention, 'MulticastAddress' tag is renamed to 'multicastAddress'",
                           gmsConfigFile);
        }
    }

    for (start = mcast_addr->txt; isspace(*start); start++) 
        /* Do nothing */ ;

#ifndef SOLARIS_BUILD
    /* Get the multicast address value into inet format from text format */
    if (inet_aton(start, &gmsGlobalInfo.config.mcast_addr.sin_addr) == 0)
    {
        clLog(EMER,GEN,NA,
              "Invalid Multicast Address Specified [%s] in config file [%s]",
              start,gmsConfigFile);
        exit(-1);
    }
#endif

    /* Read the multicast port */
    mcast_port = clParserChild(gmsConfig, "multicastPort");
    if (mcast_port == NULL)
    {
        mcast_port = clParserChild(gmsConfig, "MulticastPort");
        if (mcast_port == NULL)
        {
            clLog(EMER,GEN,NA, 
                  "Improper config file [%s] detected. \'multicastPort\' "
                  "entry is not found",gmsConfigFile);
            exit(-1);
        } else {
            clLogMultiline(NOTICE,GEN,NA,
                           "tag names in %s have changed. You are using the old config file."
                           "As per new convention, 'MulticastPort' tag is renamed to 'multicastPort'",
                           gmsConfigFile);
        }
    }
    gmsGlobalInfo.config.mcast_port =atoi(mcast_port->txt);
#endif

    /* Look for openais log option. This can be any of 'stderr', 'file'
     * 'syslog' or 'none'. Default is 'none'
     */
    openais_logging = clParserChild(gmsConfig, "openAisLoggingOption");
    if (openais_logging == NULL)
    {
        clLogMultiline(NOTICE,GEN,NA,
                       "\'openAisLoggingOption\' is not specified in the config file. "
                       "Assuming 'none' as default. So openais logs will not be "
                       "generated for this run");
        strncpy(gmsGlobalInfo.config.aisLogOption,"none",CL_MAX_NAME_LENGTH-1);
    } 
    else 
    {
        clLogMultiline(DBG,GEN,NA,
                       "AIS Logging option provided in the config file is %s",openais_logging->txt);
        strncpy(gmsGlobalInfo.config.aisLogOption,openais_logging->txt,CL_MAX_NAME_LENGTH-1);
    }


    /* Look for the preferredActiveSCNodeName tag in the config file. If this 
     * is defined then give preference to this node during leader election.
     */
    preferredActiveSCNodeName = clParserChild(gmsConfig, "preferredActiveSCNodeName");
    if (preferredActiveSCNodeName == NULL)
    {
        clLogMultiline(NOTICE,GEN,NA,
                       "\'preferredActiveSCNodeName\' tag is is not specified in the config file or\n"
                       "the value is null. So using default leader election algorithm.");
        strncpy(gmsGlobalInfo.config.preferredActiveSCNodeName,"none",CL_MAX_NAME_LENGTH-1);
    } 
    else 
    {
        clLogMultiline(NOTICE,GEN,NA,
                       "preferred Active System controller is [%s]", preferredActiveSCNodeName->txt);
        strncpy(gmsGlobalInfo.config.preferredActiveSCNodeName,preferredActiveSCNodeName->txt,CL_MAX_NAME_LENGTH-1);
    }


    /* See if bootElectionTimeout is set. If not set it to default of 
     * 5 seconds
     */
    gmsGlobalInfo.config.bootElectionTimeout = CL_GMS_DEFAULT_BOOT_ELECTION_TIMEOUT;
    gmsGlobalInfo.config.leaderSoakInterval = CL_GMS_DEFAULT_BOOT_ELECTION_TIMEOUT;
    gmsGlobalInfo.config.leaderReElectInterval = 3; /* 3 seconds for a link split re-elect detection*/
    if( (temp = getenv("CL_ASP_LEADER_SOAK_INTERVAL") ) )
    {
        ClUint32T leaderSoakInterval = atoi(temp);
        if(leaderSoakInterval > 0)
        {
            gmsGlobalInfo.config.leaderSoakInterval = leaderSoakInterval;
        }
    }
    if( (temp = getenv("CL_ASP_LEADER_REELECT_INTERVAL") ) )
    {
        ClUint32T leaderReElectInterval = atoi(temp);
        if(leaderReElectInterval > 0)
        {
            gmsGlobalInfo.config.leaderReElectInterval = leaderReElectInterval;
        }
    }

    /* First check if this is system controller node. If not then use
     * default 5 seconds timeout */
    if((temp = getenv("CL_ASP_BOOT_ELECTION_TIMEOUT")))
    {
        electionTimeout = atoi(temp);
    }
    else
    {
        bootElectionTimeout = clParserChild(gmsConfig, "bootElectionTimeout");
        if (bootElectionTimeout == NULL)
        {
            clLogMultiline(NOTICE,GEN,NA,
                           "\'bootElectionTimeout\' tag is is not specified in the config file or"
                           "the value is null. So using default value of 5 seconds for the boot "
                           "election timeout");
        } 
        else
        {
            electionTimeout = atoi(bootElectionTimeout->txt);
        }

    }
    if(electionTimeout && electionTimeout > CL_GMS_DEFAULT_BOOT_ELECTION_TIMEOUT)
    {
        gmsGlobalInfo.config.bootElectionTimeout = electionTimeout;
        clLogNotice(GEN, NA,
                    "bootElectionTimeout is set to %d seconds", gmsGlobalInfo.config.bootElectionTimeout);
    }

    gmsGlobalInfo.config.leaderAlgDb = 
        clHeapAllocate(sizeof(ClGmsLeaderElectionAlgorithmT)*
                       (gmsGlobalInfo.config.noOfGroups+1));
    if (gmsGlobalInfo.config.leaderAlgDb == NULL )
    {
        clLog(EMER,GEN,NA,
              "Memory allocation failed while loading the leader election algorithm");
        exit(-1);
    }

    /* Initialize the algorithm for each group to be the default */ 
    for ( i = 0 ; i< gmsGlobalInfo.config.noOfGroups ; i++ )
    {
        gmsGlobalInfo.config.leaderAlgDb[i] =
            _clGmsDefaultLeaderElectionAlgorithm;
    }

    group = clParserChild(gmsConfig, "group");

    while (group != NULL)
    {
        groupid = clParserChild(group , "id");
        if (groupid != NULL)
        {
            plugin_path = clParserChild(group,"pluginPath");
            if (plugin_path != NULL)
            {
                if (atoi(groupid->txt) > gmsGlobalInfo.config.noOfGroups )
                {
                    clLogMultiline(ERROR,GEN,NA,
                                   "Invalid group id [%s] specified while loading group"
                                   "specific leader election algorithm",
                                   groupid->txt);
                    continue;
                } 
                _clGmsLoadUserAlgorithm(atoi(groupid->txt), plugin_path->txt);
            }
            else {
                clLogMultiline(CRITICAL,GEN,NA,
                               "Incomplete Group Configuration section in config file. "
                               "Leader Election Algorithm path is not given"); 
            }
        }
        else {
            clLogMultiline(CRITICAL,GEN,NA,
                           "Incomplete Group Configuration section in config file. "
                           "group id is not specified.");
        }
        /* go to the next group if any */ 
        group= group->next;
    }

    /* Once you are done loading gms configuration,
     * generate $ASP_CONF/openais.conf file which
     * will be used by openais */
#ifndef OPENAIS_TIPC
    OpenAisConfFileCreate(inet_ntoa(gmsGlobalInfo.config.bind_net.sin_addr),
                          start,
                          mcast_port->txt);
#else
    OpenAisConfFileCreate(NULL,NULL,NULL);
#endif

    clParserFree(xmlConfig);
    return;
}
