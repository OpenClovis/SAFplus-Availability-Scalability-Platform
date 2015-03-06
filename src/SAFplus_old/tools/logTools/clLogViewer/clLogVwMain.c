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
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <search.h>
#include <clEoApi.h>
#include "clLogVwMain.h"
#include "clLogVwReader.h"
#include "clLogVwFilter.h"
#include "clLogVwUtil.h"
#include "clLogVwParser.h"
#include "clLogVwConstants.h"
#include "clLogVwMapping.h"
#include "clLogVwErrors.h"

#include <../clLogViewerQnx.h>

#ifdef SOLARIS_BUILD
typedef void (*sighandler_t) (int);
#endif

/**************************************************************************/

static void clLogVwShutdownViewer(void);

static ClRcT  clLogVwIsConvertBinToTxt(int argc, char **argv, 
        ClInt16T *LIndex, ClInt16T *CIndex, ClInt16T *tIndex);

/**************************************************************************/

/*
 * SIGINT Signal Handler
 * Gracefully Shuts down the server
 */

void
clLogVwSigIntHandler(void)
{
    fprintf(stdout, "\nCaught  SIGINT signal...Performing graceful shutdown...\n");

    signal(SIGINT, (sighandler_t)clLogVwSigIntHandler); /* reset signal */

    clLogVwShutdownViewer();
}


/*
 * SIGQUIT Signal Handler
 * Gracefully Shuts down the server
 */
void
clLogVwSigQuitHandler(void)
{
    fprintf(stdout, "\nCaught  SIGQUIT signal...Performing graceful shutdown...\n");

    signal(SIGQUIT, (sighandler_t)clLogVwSigQuitHandler); /* reset signal */

    clLogVwShutdownViewer();
}


/*
 * SIGPIPE Signal Handler
 * Gracefully Shuts down the server
 */
void
clLogVwSigPipeHandler(void)
{
    fprintf(stdout, "\nCaught  SIGPIPE signal...Performing graceful shutdown...\n");

    signal(SIGPIPE, (sighandler_t)clLogVwSigPipeHandler); /* reset signal */

    clLogVwShutdownViewer();
}


/*
 * Calls clean up methos
 * and exits the log viewer
 */
void
clLogVwShutdownViewer(void)
{
    clLogVwCleanUpMetaData();
    clLogVwCleanUpFilterInfo();
    clLogVwDestroyTlvMap();
    clLogVwCleanUpNameMap();
    clLogVwCleanUpParserData();

    fprintf(stdout, "\n\nLog Viewer exited normally...\n\n");
    exit(0);
}



/*
 * Displays the help and exit.
 * @return true when optoin is 
 * either "-h" or "--help"
 */
ClBoolT
clLogVwDisplayHelp(ClCharT *help, ClCharT *argv0)
{
    if( 0 != strcmp(help, "--help") 
            && 0 != strcmp(help, "-h") )
    {
        return CL_FALSE;
    }

    fprintf(stdout, "\nTo view log records in real time :\n");

    fprintf(stdout, "\nUsage : %s -l <log_file_prefix> "
            "[-f <filter>] [-t <msg_id_map_file>] "
            "[-p <num_prev_recs>] [-c <column(s)>] [-a] "
            "[-h|--help]\n\n", argv0);

    fprintf(stdout, "\nTo convert a binary log file to its corresponding "
            "text file : \n");
    
    fprintf(stdout, "\nUsage : %s -L <log_file_path> "
            "[-C <cfg_file_path>] [-t <msg_id_map_file>] "
            "[-h|--help]\n\n", argv0);
        
    fprintf(stdout, 
            "By default Log viewer displays following columns "
            "for a log record:\n\n"
            "(1)Severity Name (2)Stream Name (3)Component Name "
            "(4)Service Id (5)Message Id "
            "(6)Time Stamp (7)Message\n\n");

    
    fprintf(stdout,
            "\nOptions :\n\n"
            "\n-l : Complete path to log files \n\n"
            
            "\t<log_file_prefix> is common prefix of log file names.\n"

            "\n\tExample : \n\n\tIf log files are /tmp/logFile.0 /tmp/logFile.1 ..."
            "\n\n\tUsage: %s -l /tmp/logFile\n", argv0);
    
    fprintf(stdout, 
            "\n\n-f : Filter options\n\n"

            "\t<filter> should be specified as:\n\n"

            "\t\"<header field><comparision operator><value>\"\n\n"

            "\tHeader field options:\n\n"
            "\ts        severityName\n"

            "\tr        streamName\n"

            "\tc        compName\n"

            "\tv        serviceId\n"

            "\tm        msgId\n"

            "\tt        timeStamp\n"


            "\n\n\tComparision operator options:\n\n"

            "\t=, !=, <, <=, >, >=\n\n"

            "\n\t'value' should be string for *Name fields, "
            "HH:MM:SS for time stamp, "
            "and integer for rest of the fields. \n\n "

            "\n\tExample : \n"
            "\tfor compName Sys_comp_0...\t"
            "\n\n\tUsage: %s -l <log_file_prefix>  -f \"c=Sys_comp_0\"", argv0);

    fprintf(stdout,
            "\n\n\tfor time after 4.30pm...\t"
            "\n\n\tUsage: %s -l <log_file_prefix>  -f \"t>16:30:00\"", argv0);

    fprintf(stdout, 
            "\n\n-t : Complete path to TLV mapping file \n\n"
            "\tThis file contains the message id to string mapping "
            "for tlv(tag lengh value) messages.\n"
            "\t'%%TLV' of tlv strings are replaced by the tlv value.");

    fprintf(stdout,
            "\n\n-p : Display <num_prev_recs> previous records and exit\n\n"
            "\t<num_prev_recs> is the number of previous records "
            "to be displayed.\n"
            "\n\tIf user specifies a number greater than the maximum "
            "records or number of "
            "\n\trecords actually present in all the "
            "log files then entire log is displayed.");
    
    fprintf(stdout, 
            "\n\n\n-c : Display selected column(s)"
            "\n\n\tUser may choose to display only selected columns. " 
            "This can be achieved by using -c option where "
            "\n\tcolumn numbers are separated by ',' .\n");

    fprintf(stdout, 
            "\n\tThe default columns are :\n"
            "\t(1)Severity_Name (2)Stream_Name (3)Component_Name "
            "(4)Service_Id (5)Message_Id "
            "(6)Time_Stamp (7)Message\n");
    
    fprintf(stdout, "\n\tExample : for columns Stream_name, Service_Id, "
            "Time_Stamp.. "
            "\n\n\tUsage: %s -l <log_file_prefix> -c 2,4,6\n\n", argv0);

    fprintf(stdout, "-a : Display entire log and exit\n\n");

    fprintf(stdout,
            "\n\n-L : converts a log file in binary format to text format.\n\n"
            "\t<log_file_path> is complete path to a log file.\n\n"
            "\n\t-C : <cfg_file_prefix> is prefix to cfg(metadata) file. "
            "It is an optional argument which is required\n"
            "\t\twhen logFile name and prefix of cfg file are different.\n"
            "\n\tNote: If log file contains TLV messages then please specify "
            "tlv file path using '-t' option.\n"

            "\n\tExample : \n\tIf log file is '/tmp/logFile' the corresponding "
            "text file will be '/tmp/logFile.txt'\n"
            "\n\n\tUsage: %s -L /tmp/sys_2007-04-18T18:58:36 \n", argv0);
    fprintf(stdout,
            "\n\n\tUsage: %s -L /tmp/sys_2007-04-18T18:58:37 "
            "-C /tmp/sys_2007-04-18T18:58:36\n\n", argv0);

    fprintf(stdout, "--help | -h : Display help and exit\n\n");

    return CL_TRUE;
}



/*
 * Displays the Usage of log viewer.
 */

void
clLogVwPrintUsage(ClCharT *execName)
{
    fprintf(stdout, "\nUsage : %s -l <log_file> "
            "[-f <filter>] [-t <msg_id_map_file>] "
            "[-p <num_prev_recs>] [-c <column(s)>] [-a] "
            "[-h|--help]\n\n", execName);

    fprintf(stdout, "OR\n\nUsage : %s -L <log_file_path> "
            "[-C <cfg_file_path>] [-t <msg_id_map_file>] "
            "[-h|--help]\n\n", execName);

    fprintf(stdout, "\nFor  help : Use \'%s -h|--help\'\n\n", execName);
    exit(0);
}


/*
 * Main function.
 * Parses the command line arguments
 * and calls the required initilization
 * functions then it starts the log viewer
 * by reading the records from files.
 */
int main(int argc, char *argv[])
{
    ClInt16T    i       =   0,
                lIndex  =   -1,
                fIndex  =   -1,
                tIndex  =   -1,
                pIndex  =   -1,
                cIndex  =   -1,
                aIndex  =   -1,
                LIndex  =   -1,
                CIndex  =   -1;
                


    ClCharT *logFileFlag    = "-l";
    ClCharT *filterFlag     = "-f";
    ClCharT *tlvFileFlag    = "-t";
    ClCharT *prevFlag       = "-p";
    ClCharT *colFlag        = "-c"; 
    ClCharT *allFlag        = "-a";

    ClCharT *execName       = NULL;
    

    ClBoolT convertFlag     = CL_FALSE;

    ClRcT   rc  =   CL_OK;

    signal(SIGINT, (sighandler_t)clLogVwSigIntHandler);  /*Signal Handler*/
    signal(SIGQUIT, (sighandler_t)clLogVwSigQuitHandler);/*Signal Handler*/
    signal(SIGPIPE, (sighandler_t)clLogVwSigPipeHandler);/*Signal Handler*/

    if(NULL != (execName = strstr(argv[0], CL_LOGVW_EXEC_NAME))
       && (strlen(execName) == strlen(CL_LOGVW_EXEC_NAME)))
    {
        argv[0][strlen(argv[0]) - strlen(CL_LOGVW_EXEC_APPEND)] = '\0';
    }
    else
    {
        fprintf(stdout, "\nLog viewer executable name is not proper.\n\n");
        return -1;
    }

    if( argc > 1 
            && CL_TRUE == clLogVwDisplayHelp(argv[1], argv[0]) )
    {
        return 0;
    }
    
    rc = clLogVwIsConvertBinToTxt(argc, argv, &LIndex, &CIndex, &tIndex);
    if(CL_OK == rc)
    {
        convertFlag = CL_TRUE;
        if(-1 == CIndex)
        {
            lIndex = LIndex;
        }
    }
    else
    {
        for( i = 1; i < argc ; i++)
        {
            if( strlen(argv[i]) > 2  )
            {
                if('-' == *argv[i] &&
                        'l' == *(argv[i] + 1))
                {
                    lIndex = i;
                    argv[i] += 2;
                }
                else if('-' == *argv[i]  &&
                        'f' == *(argv[i] + 1))
                {
                    fIndex = i;
                    argv[i] += 2;
                }
                else if('-' == *argv[i]  &&
                        't' == *(argv[i] + 1))
                {
                    tIndex = i;
                    argv[i] += 2;
                }
                else if('-' == *argv[i]  &&
                        'p' == *(argv[i] + 1))
                {
                    pIndex = i;
                    argv[i] += 2;
                }
                else if('-' == *argv[i]  &&
                        'c' == *(argv[i] + 1))
                {
                    cIndex = i;
                    argv[i] += 2;
                }
                else if(0 == strcmp("--help", argv[i]))
                {
                    if(CL_TRUE == clLogVwDisplayHelp(argv[i], argv[0]) )
                    {
                        return 0;
                    }
                }
                else
                {
                    fprintf(stdout, "\nInvalid flag: %s\n", argv[i]);
                    clLogVwPrintUsage(argv[0]);
                    return -1;
                }

            }
            else/* If argument length is <= 2 */
                if(0 == strcmp(argv[i], logFileFlag))
                {
                    if(argc > i + 1)
                    {
                        lIndex = i + 1;
                        i++;
                    }
                    else
                    {
                        clLogVwPrintUsage(argv[0]);
                        return -1;
                    }
                }
                else if( 0 == strcmp(argv[i], filterFlag))
                {
                    if(argc > i + 1)
                    {
                        fIndex = i + 1;
                        i++;
                    }
                    else
                    {
                        clLogVwPrintUsage(argv[0]);
                        return -1;
                    }
                }
                else if( 0 == strcmp(argv[i], tlvFileFlag))
                {
                    if(argc > i + 1)
                    {
                        tIndex = i + 1;
                        i++;
                    }
                    else
                    {
                        clLogVwPrintUsage(argv[0]);
                        return -1;
                    }
                }
                else if( 0 == strcmp(argv[i], prevFlag))
                {
                    if(argc > i + 1)
                    {
                        pIndex = i + 1;
                        i++;
                    }
                    else
                    {
                        clLogVwPrintUsage(argv[0]);
                        return -1;
                    }
                }
                else if( 0 == strcmp(argv[i], colFlag))
                {
                    if(argc > i + 1)
                    {
                        cIndex = i + 1;
                        i++;
                    }
                    else
                    {
                        clLogVwPrintUsage(argv[0]);
                        return -1;
                    }
                }
                else if( 0 == strcmp(argv[i], allFlag))
                {
                    aIndex  = i;
                }
                else if(0 == strcmp("-h", argv[i]))
                {
                    if(CL_TRUE == 
                            clLogVwDisplayHelp(argv[i], argv[0]) )
                    {
                        return 0;
                    }
                }
                else
                {
                    fprintf(stdout, 
                            "\nInvalid flag: %s\n", argv[i]);
                    clLogVwPrintUsage(argv[0]);
                    return -1;
                }
        }

        if( -1 == lIndex )
        {
            clLogVwPrintUsage(argv[0]);
            return -1;
        }

    }


    if(CL_OK != clLogVwInitHeader())
    {
        fprintf(stdout, "\nError in initializing Header for records.\n");
        clLogVwShutdownViewer();
    }

    if(CL_OK != clLogVwInitParserData())
    {
        fprintf(stdout, "\nError in initializing Parser data.\n");
        clLogVwShutdownViewer();
    }

    if(-1 != lIndex)
    {
        if( CL_OK != clLogVwInitNameMap(argv[lIndex])) 
        {
            fprintf(stdout, "\nError in loading data from cfg file.\n");
            clLogVwShutdownViewer();
        }
    }
    else if(-1 != CIndex)
    {
        if( CL_OK != clLogVwInitNameMap(argv[CIndex])) 
        {
            fprintf(stdout, "\nError in loading data from cfg file.\n");
            clLogVwShutdownViewer();
        }
    }
    
    if( -1 != cIndex )
    {
        if( CL_OK != clLogVwSelectColumns(argv[cIndex]) )
        {
            fprintf(stdout, "\nError in selecting columns.\n");
            clLogVwShutdownViewer();
        }
    }

    if( -1 != tIndex )
    {
        if( CL_OK != clLogVwCreateTlvMap(argv[tIndex]) )
        {
            fprintf(stdout, "\nError in initializing TLV mapping.\n");
            clLogVwShutdownViewer();
        }
    }

    if( -1 != fIndex )
    {
        if( CL_OK != clLogVwInitFilterInfo(argv[fIndex]) )
        {
            fprintf(stdout, "\nError in initializing Filter data.\n");
            clLogVwShutdownViewer();
        }
    }


    if( -1 != lIndex )
    {
        if( CL_OK != clLogVwReadMetaData(argv[lIndex])) 
        {
            clLogVwShutdownViewer();
        }
    }
    else if( -1 != CIndex )
    {
        if( CL_OK != clLogVwReadMetaData(argv[CIndex])) 
        {
            clLogVwShutdownViewer();
        }
    }
    else
    {
        clLogVwPrintUsage(argv[0]);
        return -1;
    }

    if( -1 != pIndex)
    {
        if( -1 != aIndex)
        {
            fprintf(stdout, "\nError: [-a and -p options can't be used at the "
                    "same time]\n");

            clLogVwShutdownViewer();
        }
        if( CL_OK != clLogVwSetPrevRecVal(argv[pIndex], CL_FALSE) ) 
        {
            clLogVwShutdownViewer();
        }
    }

    if( -1 != aIndex )
    {
        if( CL_OK != clLogVwSetPrevRecVal(NULL, CL_TRUE) )
        {
            clLogVwShutdownViewer();
        }
    }

    if(CL_TRUE == convertFlag)
    {
        clLogVwConvertBinFileToTxt(argv[LIndex]);
    }
    else if( CL_OK != clLogVwReadRecords() )
    {
        fprintf(stdout, "ERROR : Log file might have improper data.\n");    
    }

    clLogVwShutdownViewer();

    return 0;
}/* End of main*/


ClRcT 
clLogVwIsConvertBinToTxt(int argc, char **argv, ClInt16T *LIndex, 
                         ClInt16T *CIndex, ClInt16T *tIndex)
{
    ClBoolT     convertFlag =   CL_FALSE;

    ClCharT *cfgFlag        = "-C";
    ClCharT *fileFlag       = "-L";
    ClCharT *tlvFileFlag    = "-t";

    ClInt32T    i   =   0;
    
    for( i = 1; i < argc ; i++)
    {
        if( strlen(argv[i]) > 2  )
        {
            if('-' == *argv[i] &&
               'L' == *(argv[i] + 1))
            {
                convertFlag = CL_TRUE;
                *LIndex = i;
                argv[i] += 2;
            }
            else if('-' == *argv[i]  &&
                    'C' == *(argv[i] + 1))
            {
                convertFlag = CL_TRUE;
                *CIndex = i;
                argv[i] += 2;
            }
            else if('-' == *argv[i]  &&
                    't' == *(argv[i] + 1))
            {
                *tIndex = i;
                argv[i] += 2;
            }
            else if(0 == strcmp("--help", argv[i]))
            {
                if(CL_TRUE == clLogVwDisplayHelp(argv[i], argv[0]) )
                {
                    exit(0);
                }
            }
            else
            {
                if(CL_TRUE == convertFlag)
                {
                    fprintf(stdout, "\nInvalid flag: %s\n", argv[i]);
                    clLogVwPrintUsage(argv[0]);
                }
            }

        }
        else/* If argument length is <= 2 */
            if(0 == strcmp(argv[i], fileFlag))
            {
                if(argc > i + 1)
                {
                    convertFlag = CL_TRUE;
                    *LIndex = i + 1;
                    i++;
                }
                else
                {
                    clLogVwPrintUsage(argv[0]);
                }
            }
            else if( 0 == strcmp(argv[i], tlvFileFlag))
            {
                if(argc > i + 1)
                {
                    *tIndex = i + 1;
                    i++;
                }
                else
                {
                    clLogVwPrintUsage(argv[0]);
                }
            }
            else if( 0 == strcmp(argv[i], cfgFlag))
            {
                if(argc > i + 1)
                {
                    convertFlag = CL_TRUE;
                    *CIndex = i + 1;
                    i++;
                }
                else
                {
                    clLogVwPrintUsage(argv[0]);
                }
            }
            else if(0 == strcmp("-h", argv[i]))
            {
                if(CL_TRUE == 
                        clLogVwDisplayHelp(argv[i], argv[0]) )
                {
                    exit(0);
                }
            }
            else
            {
                if(CL_TRUE == convertFlag)
                {
                    fprintf(stdout, 
                            "\nInvalid flag: %s\n", argv[i]);
                    clLogVwPrintUsage(argv[0]);
                }
            }
    }

    if(-1 == *LIndex)
    {
        return CL_LOGVW_NOT_OK;
    }
    
    return CL_OK;
}

/***********************************
 * Fix for make S=1 *
 ***********************************/

ClEoConfigT clEoConfig;
ClUint8T clEoBasicLibs[1];
ClUint8T clEoClientLibs[1];

/***********************************/
