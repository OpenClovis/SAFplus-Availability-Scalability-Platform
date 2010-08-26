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
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#include <clArchHeaders.h>

#include "clLogVwHeader.h"
#include "clLogVwUtil.h"
#include "clLogVwMapping.h"
#include "clLogVwErrors.h"
#include "clLogVwParser.h"
#include "clLogVwConstants.h"



/*
 * Global variable which contains 
 * header elements and their indexes
 */
static  ClLogVwHeaderT   *gHeader =   NULL;

/*
 * Provides severity id to name mapping
 */
static ClCharT *severityNames[20] = {

        "SEV_OFF",
        "EMRGN",
        "ALERT",
        "CRITIC",
        "ERROR",
        "WARN",
        "NOTICE",
        "INFO",
        "DEBUG",
        "DEBUG2",
        "DEBUG3",
        "DEBUG4",
        "TRACE",
        "DEBUG6",
        "DEBUG7",
        "DEBUG8",
        "DEBUG9",
        "SEV_MAX"
};

#define CL_LOGVW_SEV_MAX 0x11


/*
 * Allocates memory and initializes
 * elements of global header ptr

 * @return - rc
             CL_OK if everything is OK
             otherwise error code
 */
ClRcT
clLogVwInitHeader(void)
{

    gHeader = (ClLogVwHeaderT*) malloc(sizeof(ClLogVwHeaderT));
    
    CL_LOGVW_NO_MEMORY_CHK(gHeader);

    gHeader->timeStampString    =   NULL;

    gHeader->timeStampString = (ClCharT*) calloc(1, 100);
    
    CL_LOGVW_NO_MEMORY_CHK(gHeader->timeStampString);

    gHeader->endianessIndex = 0;
    gHeader->severityIndex = gHeader->endianessIndex + sizeof(ClLogVwFlagT);
    gHeader->streamIdIndex = gHeader->severityIndex + sizeof(ClLogVwSeverityT);
    gHeader->compIdIndex = gHeader->streamIdIndex + sizeof(ClLogVwStreamIdT); 
    gHeader->serviceIdIndex = gHeader->compIdIndex + sizeof(ClLogVwCmpIdT);
    gHeader->msgIdIndex = gHeader->serviceIdIndex + sizeof(ClLogVwServiceIdT);
    gHeader->timeStampIndex = gHeader->msgIdIndex + sizeof(ClLogVwMsgIdT);


    gHeader->selColumnsFlag =   CL_FALSE;
    gHeader->isSeverityOn   =   CL_FALSE;
    gHeader->isStreamIdOn   =   CL_FALSE;
    gHeader->isCompIdOn     =   CL_FALSE;
    gHeader->isServiceIdOn  =   CL_FALSE;
    gHeader->isTimeStampOn  =   CL_FALSE; 
    gHeader->isMsgIdOn      =   CL_FALSE;

    gHeader->outPtr = stdout;

    return CL_OK;
}


/*
 * Sets output pointer to a file pointer
 
 *@return - rc
            CL_OK if everything is OK
            otherwise error code
 */
ClRcT
clLogVwSetHeaderOutPtr(FILE *fp)
{
    if( NULL == gHeader )
        return CL_LOGVW_ERROR;

    gHeader->outPtr = fp;

    return CL_OK;
}


/*
 * Sets the selected columns to display
 * after parsing the string 'columnInfoString' 

 * Sets 'selColumnsFlag' to CL_TRUE if 
 * everything is OK

 *@param -  columnInfoString
            string which contains column numbers
            separated by ','

 *@return - rc
            CL_OK if everything is OK
            otherwise error code
 */
ClRcT
clLogVwSelectColumns(ClCharT *columnInfoString)
{
    ClUint16T   stringLen   =   strlen(columnInfoString);

    ClCharT     *pColStr    =   NULL;

    ClCharT     *nColStr    =   columnInfoString;

    ClInt32T    colNum      =   0;

    ClBoolT     isEnd       =   CL_FALSE;

    ClRcT       rc          =   CL_OK;
    

    ClCharT     *valString  =   (ClCharT*)  calloc(1, stringLen + 2);

    CL_LOGVW_NO_MEMORY_CHK(valString);

    if(NULL == gHeader)
    {
        free(valString);
        return CL_LOGVW_ERROR;
    }

    if(0 == strlen(columnInfoString))
    {
        fprintf(stdout, "Error : -c option found but no column number(s)\n");

        free(valString);
        return CL_LOGVW_ERROR;
    }
    
    while( CL_FALSE == isEnd )
    {
        pColStr = nColStr;

        nColStr = index(pColStr, ',');
        
        if(NULL == nColStr)
        {
            nColStr = columnInfoString + stringLen;
            isEnd   = CL_TRUE;
        }

        memset(valString, '\0', stringLen);
        
        memcpy(valString, pColStr, nColStr - pColStr);

        nColStr += 1;

        colNum = atoi(valString);

        switch(colNum)
        {
            case 1: gHeader->isSeverityOn   =   CL_TRUE;

                break;
                    
            case 2: gHeader->isStreamIdOn   = CL_TRUE;

                break;
                    
            case 3: gHeader->isCompIdOn     =   CL_TRUE;

                break;

            case 4: gHeader->isServiceIdOn  =   CL_TRUE;

                break;

            case 5: gHeader->isMsgIdOn      =   CL_TRUE;

                break;
         
            case 6: gHeader->isTimeStampOn  =   CL_TRUE;

                break;

                               
            case 7: rc = clLogVwSelDataColumn(CL_TRUE);        
                    
                    if(CL_OK != rc)
                    {
                        free(valString);
                        return rc;
                    }
                    
                    break;

            default : 
                fprintf(stdout, "Error : Found invalid column number : %d in "
                        "-c option.\n", colNum);

                free(valString);
                return CL_LOGVW_ERROR;

        }

    }

    gHeader->selColumnsFlag = CL_TRUE;

    free(valString);
    return rc;

}




/*
 * Sets the selColumnsFlag to 'flag' value 
 * which indiactes whether -c option is provided
 * or not

 *@param -  flag
            boolean value 

 *@return - rc
            CL_OK if everything is OK
            otherwise error code
 */
ClRcT
clLogVwGetSelColumnsFlag(ClBoolT *flag)
{
    if(NULL == gHeader)
    {
        return CL_LOGVW_ERROR;
    }

    *flag = gHeader->selColumnsFlag;

    return CL_OK;
}




/*
 * Initializes header fields streamId, compId,
 * serviceId, timeStamp, msgId
 * from raw bytes of the record buffer
 * 

 *@param -  bytes
            raw bytes of a log record
           
           
 *@param -  recHeader 
            it contain initialized header fields
            after return

 *@return - rc
            CL_OK if everything is OK
            otherwise error code
 */
ClRcT
clLogVwInitRecHeader(ClLogVwByteT *bytes, ClLogVwRecHeaderT *recHeader)
{
    ClRcT       rc                  =   CL_OK;
    ClInt64T    timeStampInNsec     =   0 ;


    if(NULL == bytes || NULL == recHeader)
    {
        return CL_LOGVW_ERROR;
    }
    
    recHeader->endianess = bytes[gHeader->endianessIndex];
    recHeader->endianess &= 0x01;

    memcpy(&recHeader->severity, bytes + gHeader->severityIndex, sizeof(ClLogVwSeverityT) );

    if( CL_TRUE == 
            clLogVwIsSameEndianess(recHeader->endianess) )
    {
        memcpy((ClCharT*)(&recHeader->streamId), 
                bytes + gHeader->streamIdIndex, 
                sizeof(ClLogVwStreamIdT) );

        memcpy((ClCharT*)(&recHeader->compId), 
                bytes + gHeader->compIdIndex, 
                sizeof(ClLogVwCmpIdT) );

        memcpy((ClCharT*)(&recHeader->serviceId), 
                bytes + gHeader->serviceIdIndex, 
                sizeof(ClLogVwServiceIdT));

        memcpy((ClCharT*)(&recHeader->msgId), 
                bytes + gHeader->msgIdIndex, 
                sizeof(ClLogVwMsgIdT));

        memcpy((ClCharT*)(&recHeader->timeStamp), 
                bytes + gHeader->timeStampIndex, 
                sizeof(ClLogVwTimeStampT));

        if( 0 >= recHeader->timeStamp )
        {
            return CL_LOGVW_ERROR;
        }
        timeStampInNsec = recHeader->timeStamp / CL_LOGVW_NANO_SEC ;

        memset(gHeader->timeStampString, '\0', 50);

        strcpy(gHeader->timeStampString, 
                ctime((time_t*)&(timeStampInNsec)) );

        gHeader->timeStampString[strlen(gHeader->timeStampString) - 1] = '\0';

    }
    else 
    {
        rc = clLogVwGetRevSubBytes(bytes, 
                gHeader->streamIdIndex, 
                sizeof(ClLogVwStreamIdT));

        if(CL_OK != rc)
        {
            return rc;
        }

        memcpy((ClCharT*)(&recHeader->streamId), 
                bytes + gHeader->streamIdIndex, 
                sizeof(ClLogVwStreamIdT) );

        rc = clLogVwGetRevSubBytes(bytes, 
                gHeader->compIdIndex, 
                sizeof(ClLogVwCmpIdT));

        if(CL_OK != rc)
        {
            return rc;
        }

        memcpy((ClCharT*)(&recHeader->compId), 
                bytes + gHeader->compIdIndex,  
                sizeof(ClLogVwCmpIdT));

        rc = clLogVwGetRevSubBytes(bytes, 
                gHeader->serviceIdIndex, 
                sizeof(ClLogVwServiceIdT));

        if(CL_OK != rc)
        {
            return rc;
        }

        memcpy((ClCharT*)(&recHeader->serviceId), 
                bytes + gHeader->serviceIdIndex, 
                sizeof(ClLogVwServiceIdT));

        rc = clLogVwGetRevSubBytes(bytes, 
                gHeader->msgIdIndex, 
                sizeof(ClLogVwMsgIdT));

        if(CL_OK != rc)
        {
            return rc;
        }

        memcpy((ClCharT*)(&recHeader->msgId), 
                bytes + gHeader->msgIdIndex, 
                sizeof(ClLogVwMsgIdT));

        rc = clLogVwGetRevSubBytes(bytes, 
                gHeader->timeStampIndex, 
                sizeof(ClLogVwTimeStampT));

        if(CL_OK != rc)
        {
            return rc;
        }

        memcpy((ClCharT*)(&recHeader->timeStamp), 
                bytes + gHeader->timeStampIndex, 
                sizeof(ClLogVwTimeStampT));

        if( 0 >= recHeader->timeStamp )
        {
            return CL_LOGVW_ERROR;
        }
        timeStampInNsec = recHeader->timeStamp / CL_LOGVW_NANO_SEC ;

        memset(gHeader->timeStampString, '\0', 50);

        strcpy(gHeader->timeStampString, 
                ctime((time_t*)&(timeStampInNsec)) );

        gHeader->timeStampString[strlen(gHeader->timeStampString) - 1] = '\0';

    }


    return CL_OK;
}




/*
 * Displays the selected fields of the 
 * record header
 * Converts the time stamp value
 * to date time format

 *@param -  recHeader
            ptr to record header structure

 *@return - rc
            CL_OK if everything is OK
            otherwise error code
 */
ClRcT
clLogVwDispLogRecHeader(ClLogVwRecHeaderT *recHeader)
{
    ClRcT       rc             =   CL_OK;
    
    if( NULL != recHeader )
    {

        if( NULL == gHeader->outPtr)
        {
            fprintf(stdout, "\nError: Out file pointer not set.\n");
            return CL_LOGVW_ERROR;
        }
        else
        {
            fprintf(gHeader->outPtr, "\n");
        }

        if(CL_FALSE == gHeader->selColumnsFlag)
        {
            
            rc = clLogVwDispSeverityName(recHeader->severity);

            if(CL_OK != rc)
            {
                return rc;
            }

            rc = clLogVwDispStreamName(recHeader->streamId);

            if(CL_OK != rc)
            {
                return rc;
            }

            rc = clLogVwDispCompName(recHeader->compId);

            if(CL_OK != rc)
            {
                return rc;
            }

            fprintf(gHeader->outPtr, "%hu ", recHeader->serviceId);

            fprintf(gHeader->outPtr, "%hu ", recHeader->msgId);

            fprintf(gHeader->outPtr, "{%s} ", gHeader->timeStampString);

        }
        else
        {
            if(CL_TRUE == gHeader->isSeverityOn)
            {
                rc = clLogVwDispSeverityName(recHeader->severity);

                if(CL_OK != rc)
                {
                    return rc;
                }
            }

            if(CL_TRUE == gHeader->isStreamIdOn)
            {
                rc = clLogVwDispStreamName(recHeader->streamId);

                if(CL_OK != rc)
                {
                    return rc;
                }
            }

            if(CL_TRUE == gHeader->isCompIdOn)
            {
                rc = clLogVwDispCompName(recHeader->compId);

                if(CL_OK != rc)
                {
                    return rc;
                }
            }

            if(CL_TRUE == gHeader->isServiceIdOn)
            {
                fprintf(gHeader->outPtr,"%hu ", recHeader->serviceId);
            }

            if(CL_TRUE == gHeader->isMsgIdOn)
            {
                fprintf(gHeader->outPtr,"%hu ", recHeader->msgId);
            }

            if(CL_TRUE == gHeader->isTimeStampOn)
            {
                fprintf(gHeader->outPtr,"{%s} ", gHeader->timeStampString);
            }
        }

    }
    else
    {
        fprintf(stdout, "\nError: Found NULL Record Header.\n");
        return CL_LOGVW_ERROR;
    }
    return CL_OK;
}



/*
 * Displays the severity name if
 * the entry corresponding to severity id
 * is found in static array severityNames
 * otherwise error

 *@param -  severity
            severity id
            
 *@return - rc
            CL_OK if everything is OK
            otherwise error code
 */
ClRcT
clLogVwDispSeverityName(ClLogVwSeverityT severity)
{
    if( severity < CL_LOGVW_SEV_MAX)
    {
        fprintf(gHeader->outPtr,"%s ", 
                severityNames[severity]);
    }
    else
    {
        return CL_LOGVW_ERROR;
        //fprintf(gHeader->outPtr,"%hu ", severity);
    }
    return CL_OK;
}



/*
 * Displays the stream  name if
 * the entry corresponding to stream id
 * is found in global hash table
 * otherwise stream id

 *@param -  streamId
            stream id
            
 *@return - rc
            CL_OK if everything is OK
            otherwise error code
 */
ClRcT
clLogVwDispStreamName(ClLogVwStreamIdT streamId)
{

    ClCharT *dispName   =   NULL ;

    ClRcT   rc     =   CL_OK;

    rc =  clLogVwGetStreamNameFromId(streamId, &dispName);
    
    if(CL_OK != rc)
    {
        rc = clLogVwAddEntryToStreamMap();

        if(CL_OK != rc)
        {
            return rc;
        }

        rc =  clLogVwGetStreamNameFromId(streamId, &dispName);
        
        if(CL_OK == rc)
        {
            fprintf(gHeader->outPtr,"%s ", dispName);
        }
        else
        {
            fprintf(gHeader->outPtr,"%hu ", streamId);
        }
    }
    else
    {
        fprintf(gHeader->outPtr,"%s ", dispName);
    }

    return CL_OK;
}



/*
 * Displays the component name if
 * the entry corresponding to comp id
 * is found in global hash table
 * otherwise comp id

 *@param -  compId
            component id
            
 *@return - rc
            CL_OK if everything is OK
            otherwise error code
 */
ClRcT
clLogVwDispCompName(ClLogVwCmpIdT compId)
{

    ClCharT *dispName   =   NULL ;

    ClRcT   rc     =   CL_OK;
    
    rc = clLogVwGetCompNameFromId(compId, &dispName);
    
    if(CL_OK != rc)
    {
        rc = clLogVwAddEntryToCompMap();

        if(CL_OK != rc)
        {
            return rc;
        }
        
        rc = clLogVwGetCompNameFromId(compId, &dispName);
        
        if(CL_OK == rc)
        {
            fprintf(gHeader->outPtr,"%s ", dispName);
        }
        else
        {
            fprintf(gHeader->outPtr,"%d ", compId);
        }
    }
    else
    {
        fprintf(gHeader->outPtr,"%s ", dispName);
    }

    return CL_OK;
}



/*
 * Frees up allocated memory
 * 

 *@return - rc
            CL_OK if everything is OK
            otherwise error code
 */
ClRcT
clLogVwCleanUpHeaderData(void)
{
    if(NULL != gHeader)
    {
        free(gHeader->timeStampString);
        free(gHeader);
    }
    return CL_OK;
}



/*
 * Finds severity id corresponding to 
 * a severity name 
 * 

 *@param -  name
            severity name

 *@param -  sevId
            severity id

 *@return - rc
            CL_OK if everything is OK
            otherwise error code
 */
ClRcT
clLogVwGetSeverityIdFromName(ClCharT *name, ClLogVwSeverityT *sevId)
{
    ClInt32T i = 0;
    
    if(NULL == name)
    {
        return CL_LOGVW_ERROR;
    }

    for( i = 0; i < 10; i++)
    {
        if( 0 == strcmp(severityNames[i], name))
        {
            *sevId = i;
            return CL_OK;
        }
    }
    return CL_LOGVW_ERROR;
}
