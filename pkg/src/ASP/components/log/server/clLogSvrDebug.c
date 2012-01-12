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
#include <string.h>

#include <clDebugApi.h>
#include <clLogApi.h>
#include <clLogErrors.h>
#include <clLogDump.h>
#include <clLogServerDump.h>
#include <clLogStreamOwnerEo.h>
#include <clLogServer.h>
#include <clCpmApi.h>

ClVersionT  gVersion     = {'B', 1, 1};
ClHandleT   gLogDebugReg = CL_HANDLE_INVALID_VALUE;

static ClRcT 
clLogDebugCliInitialize(ClUint32T   argc, 
                        ClCharT     **argv, 
                        ClCharT     **ppRet);

static ClRcT 
clLogDebugCliStreamOpen(ClUint32T   argc, 
                        ClCharT     **argv, 
                        ClCharT     **ppRet);

static ClRcT 
clLogDebugCliWrite(ClUint32T   argc, 
                   ClCharT     **argv, 
                   ClCharT     **ppRet);

static ClRcT 
clLogDebugCliFilterSet(ClUint32T   argc, 
                       ClCharT     **argv, 
                       ClCharT     **ppRet);

static ClRcT 
clLogDebugCliSeverityGet(ClUint32T   argc, 
                         ClCharT     **argv, 
                         ClCharT     **ppRet);

static ClRcT 
clLogDebugCliStreamClose(ClUint32T   argc, 
                         ClCharT     **argv, 
                         ClCharT     **ppRet);

static ClRcT 
clLogDebugCliFinalize(ClUint32T   argc, 
                      ClCharT     **argv, 
                      ClCharT     **ppRet);


#if 0
static ClRcT
clLogDebugInitDataDisplay(ClUint32T  argc,
                          ClCharT    **argv,
                          ClCharT    **ppRet);

static ClRcT
clLogDebugStreamDataDisplay(ClUint32T  argc,
                            ClCharT    **argv,
                            ClCharT    **ppRet);
#endif

ClRcT
clLogStreamOwnerDump(ClUint32T  argc,
                     ClCharT    **argv,
                     ClCharT    **ppRet);

ClRcT
clLogServerEoDump(ClUint32T  argc,
                  ClCharT    **argv,
                  ClCharT    **ppRet);

static ClRcT
clLogReturnStringFmt(ClCharT** ppRet,
                     ClCharT*  pErrMsg,
                     ...);

static ClRcT
clLogStreamListDump(ClUint32T argc, ClCharT **argv, ClCharT **ppRet);

static ClDebugFuncEntryT logDebugFuncList[] =
{
    {
        (ClDebugCallbackT)clLogDebugCliInitialize,
        "init", 
        "Initialize the Log service"
    },
    {
        (ClDebugCallbackT)clLogDebugCliStreamOpen,
        "streamOpen", 
        "Opens an application stream for logging"
    },
    {
        (ClDebugCallbackT)clLogDebugCliWrite,
        "write", 
        "Writes a log record"
    },
    {
        (ClDebugCallbackT)clLogDebugCliFilterSet,
        "filterSet", 
        "Set filter for the log streams"
    },
    {
        (ClDebugCallbackT)clLogDebugCliStreamClose,
        "streamClose", 
        "Closes an application stream"
    },
    {
        (ClDebugCallbackT)clLogDebugCliFinalize,
        "finalize", 
        "Finalizes the Log Service"
    },
    {
        (ClDebugCallbackT)clLogStreamOwnerDump,
        "SODump", 
        "Dumps the stream owner data."
    },
    {
        (ClDebugCallbackT)clLogServerEoDump,
        "ServerDump", 
        "Dumps the server EOx data."
    },
    {
        (ClDebugCallbackT)clLogDebugCliSeverityGet,
        "severityGet", 
        "Gets the curent severity for the streamname"
    },
    {
        (ClDebugCallbackT)clLogStreamListDump,
        "streamListGet", 
        "Gets the streams in the cluster"
    },
#if 0
    {
        (ClDebugCallbackT)clLogDebugInitDataDisplay,
        "initDump", 
        "Dumps the data for an init handle"
    },
    {
        (ClDebugCallbackT)clLogDebugStreamDataDisplay,
        "initDump", 
        "Dumps the data for an init handle"
    },


    {
        (ClDebugCallbackT)clLogDebugCliHdlrRegister,
        "handlerRegister", 
        "Registers a handler with a stream"
    },
    {
        (ClDebugCallbackT)clLogDebugCliRecordAck,
        "recordAck", 
        "Acknowledges a previously delivered record set"
    },
#endif
    { NULL, "", ""}
};

ClDebugModEntryT clModTab[] =
{
	{ "LOG", "LOG", logDebugFuncList, "Log Commands"},
	{ "", "", 0, ""}
};
	
ClRcT 
clLogDebugRegister(void)
{
    ClRcT  rc = CL_OK;

    rc = clDebugPromptSet("LOG");
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clDebugPromptSet(): rc[0x %x]", rc));
        return rc;
    }

    return clDebugRegister(logDebugFuncList, 
                           sizeof(logDebugFuncList)/sizeof(logDebugFuncList[0]),
                           &gLogDebugReg);
}

ClRcT
clLogDebugDeregister(void)
{
    return clDebugDeregister(gLogDebugReg);
}

static ClRcT 
clLogDebugCliInitialize(ClUint32T   argc, 
                        ClCharT     **argv, 
                        ClCharT     **ppRet)
{
    ClRcT           rc   = CL_OK;
    ClLogHandleT    hLog = CL_HANDLE_INVALID_VALUE;

    if( argc != 1 )
    {
        clLogReturnStringFmt(ppRet, "Usage: init\n");
        return CL_OK;
    }

    rc = clLogInitialize(&hLog, NULL, &gVersion);
    if( CL_OK != rc )
    {
        clLogReturnStringFmt(ppRet, "log initialize failed: rc[0x%x]\n", rc);
        return rc;
    }
    clLogReturnStringFmt(ppRet, "log initialize succeeded: handle "
                                "returned[0x%x]\n", hLog);
    
    return CL_OK;
}



static ClRcT 
clLogDebugCliStreamOpen(ClUint32T   argc, 
                        ClCharT     **argv, 
                        ClCharT     **ppRet)
{
    ClRcT                   rc         = CL_OK;
    ClLogStreamHandleT      hStream    = CL_HANDLE_INVALID_VALUE;
    ClLogStreamAttributesT  streamAttr = {0};
    ClNameT                 streamName = {0};
    ClLogStreamOpenFlagsT   openFlags  = 0;

    if( argc < 5  || argc > 16 || 
            (argc >= 5 && argc < 16 && 0 != atoi(argv[4])) )
    {
        clLogReturnStringFmt(ppRet, "Usage: streamOpen <log handle> <streamName> "
            "<streamScope> <open flags> \n\t\t<fileName> <fileLoc> <fileUnitSize>"
            " <recordSize> <haProperty>\n\t\t<fileFullAction> <maxFilesRotated> <flushFreq>"
            " <flushInterval> \n\t\t<waterMark lowLimit> <waterMark highLimit>\n"
            "\t\tlog handle[DEC]         : Handle to Log service\n"
            "\t\tstreamName[STRING]      : Name of the stream\n"
            "\t\tstreamScope[DEC]        : Scope of the stream: 0 = GLOBAL;1 = LOCAL\n"
            "\t\topen flags[DEC]         : Mode in which stream is to be opened: 0 ="
                                         " OPEN;1 = CREATE\n"
            "\t\tIn case of <open flags> = 0, rest of the arguments are not needed\n"
            "\t\t(will be ignored if present)\n" 
            "\t\tfileName[STRING]        : Name of the logical log file \n"
            "\t\tfileLoc[STRING]         : Location of the file\n"
            "\t\tfileUnitSize[DEC]       : Size of one file unit\n"
            "\t\trecordSize[DEC]         : Max Record Size\n"
            "\t\thaProperty[BOOL]        : High availability property\n"
            "\t\tfileFullAction[DEC]     : 0 = ROTATE;1 = WRAP; 2 = HALT\n"
            "\t\tmaxFilesRotated[DEC]    : Number of file units in a log file\n"
            "\t\tflushFreq[DEC]          : Flush record frequency\n"
            "\t\tflushInterval[DEC]      : Flush time interval\n"
            "\t\twaterMark lowLimit[DEC] : Watermark low limit\n"
            "\t\twaterMark highLimit[DEC]: Watermark high limit\n");
        return CL_OK;
    }

    openFlags = atoi(argv[4]);

    streamName.length = strlen(argv[2]);
    strncpy(streamName.value, argv[2], streamName.length);
    
    if(0 == openFlags)
    {
        rc = clLogStreamOpen(atoi(argv[1]), streamName, atoi(argv[3]), NULL,
                             openFlags, 0, &hStream);
    }
    else
    {
        streamAttr.fileName            = argv[5];
        streamAttr.fileLocation        = argv[6];
        streamAttr.fileUnitSize        = atoi(argv[7]);
        streamAttr.recordSize          = atoi(argv[8]);
        streamAttr.haProperty          = atoi(argv[9]);
        streamAttr.fileFullAction      = atoi(argv[10]);
        streamAttr.maxFilesRotated     = atoi(argv[11]);
        streamAttr.flushFreq           = atoi(argv[12]);
        streamAttr.flushInterval       = atoi(argv[13]);
        streamAttr.waterMark.lowLimit  = atoi(argv[14]);
        streamAttr.waterMark.highLimit = atoi(argv[15]);

        rc = clLogStreamOpen(atoi(argv[1]), streamName, atoi(argv[3]), &streamAttr,
                openFlags, 0, &hStream);
    }
    if( CL_OK != rc )
    {
        clLogReturnStringFmt(ppRet, "streamOpen failed: rc[0x%x]\n", rc);
        return rc;
    }
    if(0 == openFlags && argc > 5)
    {
        clLogReturnStringFmt(ppRet, "Ignoring stream attributes because <open flags> = 0\n"
                             "streamOpen succeeded: stream handle=0x%x\n",
                             hStream);
    }
    else
    {
        clLogReturnStringFmt(ppRet, "streamOpen succeeded: stream handle=0x%x\n",
                hStream);
    }
    
    return CL_OK;
}


static ClRcT 
clLogDebugCliWrite(ClUint32T   argc, 
                   ClCharT     **argv, 
                   ClCharT     **ppRet)
{
    ClRcT               rc      = CL_OK;

    if( argc < 6 )
    {
        clLogReturnStringFmt(ppRet, "Usage: write <stream handle> <severity> "
            "<serviceId> <msgId> \n\t\t<data>\n"
            "\t\tstream handle[DEC] : Handle to stream opened\n"
            "\t\tseverity[DEC]      : Severity of log record\n"
            "\t\tserviceId[DEC]     : Module id which is generating the log "
                         "record\n"
            "\t\tmsgId[DEC]         : Type of message; 0 = BUFFER, 1 ="
                         " PRINTF_FMT, >1 = TLV format\n"
            "\t\tdata               : According to the msgId; \n"
                         "\t\t\t\t\t\tBUFFER => length + buffer,\n"
                         "\t\t\t\t\t\tPRINTF_FMT => format buffer + arguments,\n"
                         "\t\t\t\t\t\tTLV => TAG + LENGTH + VALUE\n");
        return CL_OK;
    }

    if( CL_LOG_MSGID_BUFFER == atoi(argv[4]) )
    {
        if( argc != 7 )
        {
            clLogReturnStringFmt(ppRet, "Data not according to the msgId\n");
            return CL_ERR_INVALID_PARAMETER;
        }
        rc = clLogWriteAsync(atoi(argv[1]), atoi(argv[2]), atoi(argv[3]),
                             atoi(argv[4]), atoi(argv[5]), argv[6]);
        
    }
    if( CL_LOG_MSGID_PRINTF_FMT == atoi(argv[4]) )
    {
            clLogReturnStringFmt(ppRet, "Not supported by DebugCLI\n");
            return CL_ERR_INVALID_PARAMETER;
        
    }
    else if( CL_LOG_MSGID_PRINTF_FMT < atoi(argv[4]) )
    {
        if( 9 != argc )
        {
            clLogReturnStringFmt(ppRet, "Only one triplet is currently "
            "supported in the TLV format\n");
            return CL_ERR_INVALID_PARAMETER;
        }
        rc = clLogWriteAsync(atoi(argv[1]), atoi(argv[2]), atoi(argv[3]),
                             atoi(argv[4]), atoi(argv[5]), atoi(argv[6]),
                             argv[7], atoi(argv[8]));
    }
    if( CL_OK != rc )
    {
        clLogReturnStringFmt(ppRet, "write failed: rc[0x%x]\n", rc);
        return rc;
    }
    clLogReturnStringFmt(ppRet, "write succeeded\n");
    
    return CL_OK;
}

static ClRcT 
clLogDebugCliFilterSet(ClUint32T   argc, 
                       ClCharT     **argv, 
                       ClCharT     **ppRet)
{
    ClRcT         rc        = CL_OK;
    ClLogFilterT  filter    = {0};
    ClBitmapHandleT msgMap  = 0;
    ClBitmapHandleT compIdMap = 0;
    ClUint32T     msgIdSet  = 0;
    ClUint32T     compIdSet = 0;
    ClUint32T     i         = 0;
    ClUint32T     nextPos   = 0;
    ClNameT streamName = {0};
    ClNameT nodeName = {0};
    ClLogFilterFlagsT filterFlags = CL_LOG_FILTER_ASSIGN;

    if( argc < 3 )
    {
        clLogReturnStringFmt(ppRet, "Usage: filterSet <streamname>"
            " <severityFilter> [ <streamNodeName> <filterFlags> <msgIdSetLength> <msgId> ...]"
            " [<compIdSetLength> <compId> ...]\n"
            "\t\tstream name[STRING] : Name of the stream being opened\n"
            "\t\tseverityFilter     : severity of a particular log record;"
                         " Accepted values: (1-8)\n"
            "\t\tstreamNodeName[STRING]: Name of the node on which the log stream resides\n"
            "\t\tfilterFlags[DEC]   : filter flags; ASSIGN = 1, MERGE_ADD = 2,"
                         " MERGE_DELETE = 3\n"
            "\t\tRemaining arguments are not required if the filter set is nedded only\n"
            "\t\ton severity field. To set filter on msgId(s), provide <msgIdSetLength>\n"
            "\t\tand <msgIdSetLength> number of msgIds.\n"
            "\t\tFilter on compId(s) can be set in similar manner.\n"
            "\t\tmsgIdSetLength     : Number of msgIds to be filtered\n"
            "\t\tmsgId              : msgIds to be filtered\n"
            "\t\tcompIdSetLength    : Number of compIds to be filtered\n"
            "\t\tcompId             : compIds to be filtered\n"
            
            "\n\t\tValid log severity levels for <severityFilter> argument :\n\n"
            "\t\t<severityFilter>\t\tLog severity level\n\n"
            "\t\t\t1\t\t\tCL_LOG_SEV_EMERGENCY \n"
            "\t\t\t2\t\t\tCL_LOG_SEV_ALERT\n"
            "\t\t\t3\t\t\tCL_LOG_SEV_CRITICAL\n"
            "\t\t\t4\t\t\tCL_LOG_SEV_ERROR\n"
            "\t\t\t5\t\t\tCL_LOG_SEV_WARNING\n"
            "\t\t\t6\t\t\tCL_LOG_SEV_NOTICE\n"
            "\t\t\t7\t\t\tCL_LOG_SEV_INFO\n"
            "\t\t\t8\t\t\tCL_LOG_SEV_DEBUG\n"
            "\t\t\t9 or greater\t\tCL_LOG_SEV_TRACE\n"
            );
        return CL_OK;
    }

    if(!strncasecmp(argv[1], "sys", 3))
    {
        clNameSet(&streamName, "LOG_SYSTEM");
    }
    else if(!strncasecmp(argv[1], "app", 3))
    {
        clNameSet(&streamName, "LOG_APPLICATION");
    }
    else
        clNameSet(&streamName, argv[1]);
    filter.severityFilter = atoi(argv[2]);
    if(filter.severityFilter < CL_LOG_SEV_EMERGENCY || filter.severityFilter > CL_LOG_SEV_TRACE)
    {
        clLogReturnStringFmt(ppRet, "Incorrect Value for <severityFilter> (Valid range [1-%d])\n", CL_LOG_SEV_TRACE);
        return CL_ERR_INVALID_PARAMETER;
    }
    if(filter.severityFilter > CL_LOG_SEV_DEBUG)
        filter.severityFilter = CL_LOG_SEV_TRACE;

    filter.severityFilter = (1 << filter.severityFilter) - 1;

    if(argc == 3)
    {
        clCpmLocalNodeNameGet(&nodeName);
        goto filter_set;
    }

    clNameSet(&nodeName, argv[3]);

    nextPos = 4;
    if(argc > 4)
    {
        filterFlags  = atoi(argv[4]);
        nextPos = 5;
    }
    if(argc > 5)
    {
        filter.msgIdSetLength = atoi(argv[5]);
        nextPos = 6;
    }
    if( 0 != filter.msgIdSetLength )
    {
        if(argc < (nextPos + filter.msgIdSetLength ))
        {
            clLogReturnStringFmt(ppRet, "Number of arguments are incorrect\n");
            return CL_ERR_INVALID_PARAMETER;
        }
        rc = clBitmapCreate(&msgMap, 0);
        if(rc != CL_OK)
        {
            clLogReturnStringFmt(ppRet, "MSG id bitmap create returned with [%#x]", rc);
            return rc;
        }
        for(i = 0;i <  filter.msgIdSetLength; i++)
        {
            ClUint32T msgId = atoi(argv[nextPos + i]);
            rc = clBitmapBitSet(msgMap, msgId);
            if(rc != CL_OK)
            {
                clBitmapDestroy(msgMap);
                clLogReturnStringFmt(ppRet, "MSG id bitmap set returned [%#x]", rc);
                return rc;
            }
        }
        rc = clBitmap2BufferGet(msgMap, &msgIdSet, &filter.pMsgIdSet);
        clBitmapDestroy(msgMap);
        if(rc != CL_OK)
        {
            clLogReturnStringFmt(ppRet, "Msg ID bitmap to buffer returned [%#x]", rc);
            return rc;
        }
        nextPos += filter.msgIdSetLength;
        filter.msgIdSetLength = (ClUint16T)msgIdSet;
    }
    if(nextPos == argc)
    {
        goto filter_set;
    }
    filter.compIdSetLength = atoi(argv[nextPos]);
    nextPos++;
    if( 0 != filter.compIdSetLength )
    {
        if(argc < (nextPos + filter.msgIdSetLength))
        {
            clLogReturnStringFmt(ppRet, "Number of arguments are incorrect\n");
            return CL_ERR_INVALID_PARAMETER;
        }
        rc = clBitmapCreate(&compIdMap, 0);
        if(rc != CL_OK)
        {
            clLogReturnStringFmt(ppRet, "Comp id bitmap create returned with [%#x]", rc);
            return rc;
        }
        for(i = 0;i < filter.compIdSetLength; i++)
        {
            ClUint32T compId = atoi(argv[nextPos + i]);
            compId &= CL_LOG_COMPID_CLASS;
            rc = clBitmapBitSet(compIdMap, compId);
            if(rc != CL_OK)
            {
                clLogReturnStringFmt(ppRet, "Comp id bitmap set returned [%#x]", rc);
                clBitmapDestroy(compIdMap);
                return rc;
            }
        }
        rc = clBitmap2BufferGet(compIdMap, &compIdSet, &filter.pCompIdSet);
        clBitmapDestroy(compIdMap);
        if(rc != CL_OK)
        {
            clLogReturnStringFmt(ppRet, "Comp id bitmap to buffer returned [%#x]", rc);
            return rc;
        }
        nextPos += filter.compIdSetLength;
        filter.compIdSetLength = (ClUint16T)compIdSet;
    }
    if( nextPos != argc )
    {
        clLogReturnStringFmt(ppRet, "Number of arguments are incorrect\n");
        return CL_ERR_INVALID_PARAMETER;
    }

filter_set:
    rc = clLogSvrDebugFilterSet(&streamName, &nodeName, &filter, filterFlags);
    if( CL_OK != rc )
    {
        clLogReturnStringFmt(ppRet, "filterSet failed: rc[0x%x]\n", rc);
        return rc;
    }
    clLogReturnStringFmt(ppRet, "filterSet succeeded\n");
    return CL_OK;
}

static ClRcT 
clLogDebugCliSeverityGet(ClUint32T   argc, 
                         ClCharT     **argv, 
                         ClCharT     **ppRet)
{
    ClRcT         rc        = CL_OK;
    ClNameT streamName = {0};
    ClNameT nodeName = {0};
    ClLogSeverityFilterT severity = 0;
    const ClCharT *pSev = NULL;
    struct ClLogSeverityTable
    {
        const ClCharT *str;
        ClLogSeverityT severity;
    }logSeverityTable[] = {
        {"UNKNOWN", 0},
        {"EMERGENCY", CL_LOG_SEV_EMERGENCY},
        {"ALERT", CL_LOG_SEV_ALERT},
        {"CRITICAL", CL_LOG_SEV_CRITICAL},
        {"ERROR", CL_LOG_SEV_ERROR},
        {"WARNING", CL_LOG_SEV_WARNING},
        {"NOTICE", CL_LOG_SEV_NOTICE},
        {"INFO", CL_LOG_SEV_INFO},
        {"DEBUG", CL_LOG_SEV_DEBUG},
    };

    if( argc != 2 && argc != 3 )
    {
        clLogReturnStringFmt(ppRet, "Usage: severityget <streamname> [ <streamNodeName> ]\n");
        return CL_OK;
    }

    if(!strncasecmp(argv[1], "sys", 3))
    {
        clNameSet(&streamName, "LOG_SYSTEM");
    }
    else if(!strncasecmp(argv[1], "app", 3))
    {
        clNameSet(&streamName, "LOG_APPLICATION");
    }
    else
        clNameSet(&streamName, argv[1]);

    if(argc > 2)
        clNameSet(&nodeName, argv[2]);
    else
        clCpmLocalNodeNameGet(&nodeName);

    rc = clLogSvrDebugSeverityGet(&streamName, &nodeName, &severity);
    if( CL_OK != rc )
    {
        clLogReturnStringFmt(ppRet, "filterGet failed: rc[0x%x]\n", rc);
        return rc;
    }
    if(severity > CL_LOG_SEV_DEBUG)
        pSev = "TRACE";
    else
        pSev = logSeverityTable[severity].str;

    clLogReturnStringFmt(ppRet, "Current log level [%d : %s]\n", severity, pSev);
    return CL_OK;
}

static ClRcT 
clLogDebugCliStreamClose(ClUint32T   argc, 
                         ClCharT     **argv, 
                         ClCharT     **ppRet)
{
    ClRcT               rc      = CL_OK;
    ClLogStreamHandleT  hStream = CL_HANDLE_INVALID_VALUE;

    if( argc != 2 )
    {
        clLogReturnStringFmt(ppRet, "Usage: streamClose <stream handle>\n "
            "\t\tstream handle[DEC] : Handle to stream opened\n");
        return CL_OK;
    }

    hStream = atoi(argv[1]);
    rc = clLogStreamClose(hStream);
    if( CL_OK != rc )
    {
        clLogReturnStringFmt(ppRet, "streamClose failed: rc[0x%x]\n", rc);
        return rc;
    }
    
    clLogReturnStringFmt(ppRet, "streamClose succeeded\n");
    return CL_OK;
}

static ClRcT 
clLogDebugCliFinalize(ClUint32T   argc, 
                      ClCharT     **argv, 
                      ClCharT     **ppRet)
{

    ClRcT         rc   = CL_OK;
    ClLogHandleT  hLog = CL_HANDLE_INVALID_VALUE;

    if( argc != 2 )
    {
        clLogReturnStringFmt(ppRet, "Usage: finalize <log handle>\n "
            "\t\tlog handle[DEC] : Handle to Log service\n");
        return CL_OK;
    }

    hLog = atoi(argv[1]);
    rc = clLogFinalize(hLog);
    if( CL_OK != rc )
    {
        clLogReturnStringFmt(ppRet, "finalize failed: rc[0x%x]\n", rc);
        return rc;
    }
    
    clLogReturnStringFmt(ppRet, "finalize succeeded\n");
    return CL_OK;
}

#if 0
static ClRcT 
clLogDebugCliHdlrRegister(ClUint32T   argc, 
                          ClCharT     **argv, 
                          ClCharT     **ppRet)
{
    ClRcT               rc      = CL_OK;
    ClLogStreamHandleT  hStream = CL_HANDLE_INVALID_VALUE;

    if( argc != 6 )
    {
        clLogReturnStringFmt(ppRet, "Usage: handlerRegister <log handle> <streamName> "
            "<streamScope> <nodeName> \n\t\t<handler flags>\n"
            "\t\tlog handle[DEC]         : Handle to Log service\n"
            "\t\tstreamName[STRING]      : Name of the stream\n"
            "\t\tstreamScope[DEC]        : Scope of the stream: 0 = GLOBAL;1 = LOCAL\n"
            "\t\tnodeName[STRING]        : Applicable only if scope = LOCAL; "
                                  "Name of the node where the stream is created\n"
            "\t\thandler flags[DEC]      : 1 = will acknowledge; 0 = will not"
                                  " acknowledge\n");
        return CL_OK;
    }

    rc = clLogHandlerRegister(atoi(argv[1]), argv[2], atoi(argv[3]), argv[4], 
                              atoi(argv[5]), &hStream);
    if( CL_OK != rc )
    {
        clLogReturnStringFmt(ppRet, "handlerRegister failed: rc[0x%x]\n", rc);
        return rc;
    }
    clLogReturnStringFmt(ppRet, "handlerRegister succeeded: handle to "
                                "streamHandler =0x%x\n", hStream);
    
    return CL_OK;
}

static ClRcT 
clLogDebugCliRecordAck(ClUint32T   argc, 
                       ClCharT     **argv, 
                       ClCharT     **ppRet)
{
    ClRcT               rc      = CL_OK;
    ClLogStreamHandleT  hStream = CL_HANDLE_INVALID_VALUE;

ClRcT
clLogHandlerRecordAck(ClLogStreamHandleT  hStream,
                      ClUint64T           sequenceNumber,
                      ClUint32T           numRecords)

    if( argc != 4 )
    {
        clLogReturnStringFmt(ppRet, "Usage: recordAck <streamHandler handle> <sequence number> "
            "<no of records> \n"
            "\t\tstreamHandler handle[DEC] : Handle to stream handler\n"
            "\t\tsequence number[DEC]      : the number\n"
            "\t\tstreamScope[DEC]        : Scope of the stream: 0 = GLOBAL;1 = LOCAL\n"
            "\t\tnodeName[STRING]        : Applicable only if scope = LOCAL; "
                                  "Name of the node where the stream is created\n"
            "\t\thandler flags[DEC]      : 1 = will acknowledge; 0 = will not"
                                  " acknowledge\n");
        return CL_OK;
    }

    rc = clLogHandlerRegister(atoi(argv[1]), argv[2], atoi(argv[3]), argv[4], 
                              atoi(argv[5]), &hStream);
    if( CL_OK != rc )
    {
        clLogReturnStringFmt(ppRet, "handlerRegister failed: rc[0x%x]\n", rc);
        return CL_OK;
    }
    clLogReturnStringFmt(ppRet, "handlerRegister succeeded: handle to "
                                "streamHandler =0x%x\n", hStream);
    
    return CL_OK;
}


static ClRcT
clLogDebugInitDataDisplay(ClUint32T  argc,
                          ClCharT    **argv,
                          ClCharT    **ppRet)
{
    ClRcT  rc = CL_OK;
    
    if( argc != 1 )
    {
        clLogReturnStringFmt(ppRet, "Usage: initDisplay\n");
        return CL_OK;
    }

    rc = clLogInitHandleDataDisplay(ppRet);
    if( CL_OK != rc )
    {
        clLogReturnStringFmt(ppRet, "Could not display the data. rc[0x%x]\n", rc);
        return CL_OK;
    }

    return CL_OK;
}

static ClRcT
clLogDebugStreamDataDisplay(ClUint32T  argc,
                            ClCharT    **argv,
                            ClCharT    **ppRet)
{
    ClRcT  rc = CL_OK;
    
    if( argc != 1 )
    {
        clLogReturnStringFmt(ppRet, "Usage: streamDisplay\n");
        return CL_OK;
    }

    rc = clLogStreamHandleDataDisplay(ppRet);
    if( CL_OK != rc )
    {
        clLogReturnStringFmt(ppRet, "Could not display the data. rc[0x%x]\n", rc);
        return CL_OK;
    }

    return CL_OK;
}

#endif

static int logStreamSortCmp(const void *a, const void *b)
{
    const ClLogStreamInfoT *stream1 = (const ClLogStreamInfoT*)a;
    const ClLogStreamInfoT *stream2 = (const ClLogStreamInfoT*)b;
    int cmp = stream1->streamScopeNode.length - stream2->streamScopeNode.length;
    if(cmp == 0)
        cmp = strncmp((const char*)stream1->streamScopeNode.value,
                      (const char*)stream2->streamScopeNode.value, 
                      stream1->streamScopeNode.length);
    return cmp;
}

static ClRcT 
logStreamListDump(ClCharT **ppRet)
{
    ClLogHandleT logHandle = 0;
    ClVersionT version = { 'B', 0x1, 0x1 };
    ClRcT rc = clLogInitialize(&logHandle, NULL, &version);
    ClDebugPrintHandleT msgHandle = 0;
    ClUint32T numStreams = 0;
    ClLogStreamInfoT *logStreams = NULL;
    *ppRet = NULL;
    if(rc != CL_OK)
        goto out;
    rc = clDebugPrintInitialize(&msgHandle);
    if(rc != CL_OK)
        goto out_log_finalize;
    rc = clLogStreamListGet(logHandle, &numStreams, &logStreams);
    if(rc != CL_OK)
        goto out_print_finalize;

    qsort(logStreams, numStreams, sizeof(*logStreams), logStreamSortCmp);
    clDebugPrint(msgHandle, "Displaying [%d] streams\n", numStreams);
    for(ClUint32T i = 0; i < numStreams; ++i)
    {
        clDebugPrint(msgHandle, "Stream name [%.*s], node [%.*s]\n", 
                     logStreams[i].streamName.length, logStreams[i].streamName.value,
                     logStreams[i].streamScopeNode.length, logStreams[i].streamScopeNode.value);
    }
    if(logStreams)
        clHeapFree(logStreams);

    out_print_finalize:
    clDebugPrintFinalize(&msgHandle, ppRet);
    if(rc != CL_OK && *ppRet)
    {
        clHeapFree(*ppRet);
        *ppRet = NULL;
    }
    out_log_finalize:
    clLogFinalize(logHandle);
    out:
    return rc;
}

static ClRcT
clLogStreamListDump(ClUint32T  argc,
                    ClCharT    **argv,
                    ClCharT    **ppRet)
{
    return logStreamListDump(ppRet);
}

ClRcT
clLogStreamOwnerDump(ClUint32T argc,
                     ClCharT **argv,
                     ClCharT **ppRet)
{
    clLogStreamOwnerDataDump(ppRet);
    return CL_OK;
}

ClRcT
clLogServerEoDump(ClUint32T  argc,
                  ClCharT    **argv,
                  ClCharT    **ppRet)
{
    clLogSvrEoDataDump(ppRet);
    return CL_OK;
}

static ClRcT
clLogReturnStringFmt(ClCharT** ppRet,
                     ClCharT*  pErrMsg,
                     ...)
{
    va_list arg;

    va_start(arg, pErrMsg);
    *ppRet = (ClCharT *)clHeapAllocate(strlen(pErrMsg) + 20);
    if( NULL != *ppRet )
    {
        memset(*ppRet, '\0', strlen(pErrMsg) + 20);
        vsnprintf(*ppRet, strlen(pErrMsg) + 20, pErrMsg, arg);
        va_end(arg);
        return CL_OK;
    }
    else
    {
        va_end(arg);
        return CL_LOG_RC(CL_ERR_NULL_POINTER);
    }
}
