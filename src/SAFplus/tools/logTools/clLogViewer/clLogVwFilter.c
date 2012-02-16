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
#include <string.h>
#include <search.h>
#include <time.h>

#include "clLogVwFilter.h"
#include "clLogVwHeader.h"
#include "clLogVwMapping.h"
#include "clLogVwErrors.h"
#include "clLogVwUtil.h"
#include <../clLogViewerQnx.h>



/*
 * Global filter ptr which contains
 * header field index, comparison operator
 * and value specified in filter criterion
 */
static ClLogVwFilterInfoT *filterInfo = NULL;

/**************************************************************************/

static ClRcT
clLogVwSetFilterFlag(ClBoolT value);



/**************************************************************************/


/*
 * Sets the filter flag to 'value'
 * which indicates that whether
 * filter is set or not
 
 *@param -  value
            CL_TRUE or CL_FALSE
 
 *@return - rc
            CL_OK if everything is OK
            error code otherwise
 */

static ClRcT
clLogVwSetFilterFlag(ClBoolT value)
{
    if(NULL != filterInfo)
    {
        filterInfo->filterFlag = value;
    }
    else
    {
        return CL_LOGVW_ERROR;
    }
    return CL_OK;
}



/*
 * Gets the filter flag
 
 *@param -  flag
            contains value of filter flag
            after return
 
 *@return - rc
            CL_OK if everything is OK
            error code otherwise
 */
ClRcT
clLogVwGetFilterFlag(ClBoolT *flag)
{
    if(NULL != filterInfo)
    {
        *flag = filterInfo->filterFlag;
    }
    else
    {
        *flag = CL_FALSE;
    }
    return CL_OK;
}



/*
 * Frees up the allocated memory
 
 *@return - rc
            CL_OK if everything is OK
            error code otherwise
 */
ClRcT
clLogVwCleanUpFilterInfo(void)
{
    if(NULL != filterInfo)
    {
        free(filterInfo);
    }
    return CL_OK;
}



/*
 * Allocates memory and parses the filter argument
 * sets the filter flag to true is the filter
 * argument is correct

 * Sets the reacord field index on which filter 
 * is to be provided

 * Sets the comparison type and value for filter
 
 *@param -  filterArg
            argument provided in -f option
 
 *@return - rc
            CL_OK if everything is OK
            error code otherwise
 */
ClRcT
clLogVwInitFilterInfo(const ClCharT *filterArg)
{
    ClRcT   rc  =   CL_OK;
    
    ClCharT *optrPtr = (ClCharT*) (filterArg + 1);
    
    filterInfo = (ClLogVwFilterInfoT*) malloc(sizeof(ClLogVwFilterInfoT));

    CL_LOGVW_NO_MEMORY_CHK(filterInfo);
    
    filterInfo->fldIndex=   -1; 
    filterInfo->fldVal  =   -1;
    filterInfo->fldName =   NULL;
    
    filterInfo->compareType = -1;
    filterInfo->filterFlag  = CL_FALSE;

    switch(filterArg[0]) 
    {
        case 's': filterInfo->fldIndex = 1;
                  break;
        case 'r': filterInfo->fldIndex = 2;
                  break;
        case 'c': filterInfo->fldIndex = 3;
                  break;
        case 'v': filterInfo->fldIndex = 4;
                  break;
        case 'm': filterInfo->fldIndex = 5;
                  break;
        case 't': filterInfo->fldIndex = 6;
                  break;

        default: fprintf(stdout, "Invalid Filter flag.\n");          
                 return CL_LOGVW_ERROR;

    }

    switch(optrPtr[0])
    {
        case '=': 
            filterInfo->compareType = 0;
            break;

        case '<': 
            if( '=' == optrPtr[1] )
            {
                filterInfo->compareType =2;
                optrPtr++;
            }
            else
                filterInfo->compareType = 1;
            break;

        case '>': 
            if( '=' == optrPtr[1] )
            {
                filterInfo->compareType = 4;
                optrPtr++;
            }
            else
                filterInfo->compareType = 3;
            break;

        case '!': 
            if( '=' == optrPtr[1] )
            {
                filterInfo->compareType = 5;
                optrPtr++;
            }
            else
            {
                fprintf(stdout, "Invalid Comparision operator in filter.\n");
                return CL_LOGVW_ERROR;
            }
            break;
        default:     
            fprintf(stdout, "Invalid Comparision operator in filter.\n");
            return CL_LOGVW_ERROR;

    }

    /**
     * Reversing the severity comparison type
     */
    if(1 == filterInfo->fldIndex)
    {
        switch(filterInfo->compareType)
        {

            case 1 : 
                filterInfo->compareType = 3;
                break;

            case 2 : 
                filterInfo->compareType = 4;
                break;

            case 3 : 
                filterInfo->compareType = 1;
                break;

            case 4 : 
                filterInfo->compareType = 2;
                break;

        }
    }

    if( 0 >= strlen(optrPtr + 1))
    {
        fprintf(stdout, "Filter 'value' not specified in filter criterion.\n");
        return CL_LOGVW_ERROR;
    }
    
    rc = clLogVwSetFilterValue(optrPtr + 1);
    
    if(CL_OK != rc)
    {
        fprintf(stdout, "Found incorrect filter value.\n");
        return rc;
    }
    
    rc = clLogVwSetFilterFlag(CL_TRUE);

    if(CL_OK != rc)
    {
        fprintf(stdout, "Unable to set filter flag.\n");
        return rc;
    }
    return rc;
}



/*
 * Converts the filter value to their
 * corresponding format as in log record
 * except stream name and component name
 * whose entries are retieved dynamically
 * from metadata file
 
 *@param -  name
            string name for the field
 
 *@return - rc
            CL_OK if everything is OK
            error code otherwise
 */
ClRcT
clLogVwSetFilterValue(ClCharT *name)
{
    ClLogVwSeverityT     sevId       =   0;

    ClRcT           rc     =   CL_OK;

    ClLogVwTimeStampT    timeStamp   =   0;

    switch(filterInfo->fldIndex)
    {
        case 1:     
            rc = clLogVwGetSeverityIdFromName(name, &sevId);

            if(CL_OK != rc)
            {
                return rc;
            }

            filterInfo->fldVal = sevId;
            break;

        case 2:     
            filterInfo->fldName = name;
            
            filterInfo->fldVal  = -1;
            rc = clLogVwSetFldValueFromName();

            if( CL_OK != rc)
            {
                fprintf(stdout, "\n\nThe entry corresponding to name '%s' specified "
                        "in filter option is not found in metadata file.\n\n",
                        name);

                return rc;
            }
            break;

        case 3:     
            filterInfo->fldName = name;

            filterInfo->fldVal  = -1;
            rc = clLogVwSetFldValueFromName();

            if( CL_OK != rc)
            {
                fprintf(stdout, "\n\nThe entry corresponding to name '%s' specified "
                        "in filter option is not found in metadata file.\n\n",
                        name);

                return rc;
            }

            break;

        case 4:     
            filterInfo->fldVal = atoll(name);
            break;

        case 5:     
            filterInfo->fldVal = atoll(name);
            if(1 == filterInfo->fldVal)
            {
                fprintf(stdout, "\nERROR : Filter on \"m=1\" is not supported (Message id 1 "
                        "corresponds to ASCII records, log viewer supports only Binary and TLV records).\n");    
                return CL_LOGVW_ERROR;
            }

            break;

        case 6:     
            rc = clLogVwGetTimeStampFromName(name, &timeStamp);

            if(CL_OK != rc)
            {
                return rc;
            }

            filterInfo->fldVal = timeStamp / CL_LOGVW_NANO_SEC; 
            break;

        default:    return CL_LOGVW_ERROR;

    }

    return CL_OK;
}



/*
 * Applies filter on record header fields
 * and checks if it passes through filter
 * i.e. whether it satisfies te filter criterion
 
 *@param -  recHeader
            contains fileds of the record header
 
 *@param -  result
            CL_TRUE if record satisfies the filter
            criterion, CL_FALSE otherwise
 
 *@return - rc
            CL_OK if everything is OK
            error code otherwise
 */
ClRcT
clLogVwApplyFilter(ClLogVwRecHeaderT *recHeader, ClBoolT *result)
{
    ClRcT   rc     =   CL_OK;

    ClBoolT filterRes   =   CL_FALSE;

    if( -1 == filterInfo->fldVal )
    {
        *result = CL_TRUE;
        return CL_LOGVW_ERROR;
    }

    rc = clLogVwIsFiltered(recHeader, &filterRes);

    if( CL_OK != rc)
    {
        *result = CL_FALSE;
        return rc;
    }
    
    *result = filterRes;
    return CL_OK;
}



/*
 * Finds stream id or component id 
 * for stream name or component name
 * and sets the value to fldVal
 
 *@return - rc
            CL_OK if everything is OK
            error code otherwise
 */
ClRcT
clLogVwSetFldValueFromName(void)
{
    ClRcT           rc          =   CL_OK;
    ClLogVwStreamIdT     streamId    =   0;
    ClLogVwCmpIdT        compId      =   0;
    
    switch(filterInfo->fldIndex)
    {
        case 2 :    if(-1 == filterInfo->fldVal)
                    {
                        rc = clLogVwAddEntryToStreamMap();

                        if(CL_OK != rc)
                        {
                            return rc;
                        }
                    }
                    
                    rc = clLogVwGetStreamIdFromName
                             (filterInfo->fldName, &streamId);

                    if(CL_OK != rc)
                    {
                        return rc;
                    }

                    filterInfo->fldVal = streamId;
                    break;
                    
        case 3 :    if(-1 == filterInfo->fldVal)
                    {
                        rc = clLogVwAddEntryToCompMap();

                        if(CL_OK != rc)
                        {
                            return rc;
                        }
                    }
                    
                    rc = clLogVwGetCompIdFromName
                            (filterInfo->fldName, &compId); 
                    
                    if(CL_OK != rc)
                    {
                        return rc;
                    }

                    filterInfo->fldVal = compId; 
                    break;
                    
        default:    return rc;             

    }

    return rc;
}



/*
 * Gets the time stamp value from time stamp string

 * It takes the time in HH:MM:SS form and converts
 * it into nano seconds passed after 1970 ...
 * till current date and mentioned time
 
 *@param -  timeString
            time in HH:MM:SS format

 *@param -  timeStamp
            contains time value in nano seconds
            after return
 
 *@return - rc
            CL_OK if everything is OK
            error code otherwise
 */
ClRcT
clLogVwGetTimeStampFromName(ClCharT *timeString, ClInt64T *timeStamp)
{
    ClRcT   rc =   CL_OK;
    struct tm *tmCurPtr = NULL; 
    
    time_t tVal = time(NULL);
    tmCurPtr = localtime( &tVal );

    struct tm *tmPtr = (struct tm *) malloc(sizeof(struct tm));
    
    CL_LOGVW_NO_MEMORY_CHK(tmPtr);

    if( NULL == strptime(timeString, "%T", tmPtr) )
    {
        free(tmPtr);
        return CL_LOGVW_ERROR;
    }

    tmCurPtr->tm_hour = tmPtr->tm_hour; 
    tmCurPtr->tm_min = tmPtr->tm_min; 
    tmCurPtr->tm_sec = tmPtr->tm_sec;

    *timeStamp = mktime(tmCurPtr);
    if(-1 == *timeStamp)
    {
        fprintf(stdout, "Unable to get the time from filter timestamp\n");
        free(tmPtr);
        return CL_LOGVW_ERROR;
    }
    *timeStamp = (*timeStamp) * CL_LOGVW_NANO_SEC;

    free(tmPtr);
    return rc;
}


/*
 * Checks if record satisfies the filter criterion
 
 *@param -  recHeader
            record header
 
 *@param -  result
            CL_TRUE if record satisfies the filter criterion
            CL_FALSE otherwise
 
 *@return - rc
            CL_OK if everything is OK
            error code otherwise
 */
ClRcT
clLogVwIsFiltered(ClLogVwRecHeaderT *recHeader, ClBoolT *result)
{
    ClRcT   rc          =   CL_OK;

    ClBoolT rangeRes    =   CL_FALSE;
    
    switch(filterInfo->fldIndex)
    {
        case 1: rc = clLogVwCheckRange(recHeader->severity, &rangeRes);
                break;
                
        case 2: rc = clLogVwCheckRange(recHeader->streamId, &rangeRes);
                break;

        case 3: rc = clLogVwCheckRange(recHeader->compId, &rangeRes);
                break;

        case 4: rc = clLogVwCheckRange(recHeader->serviceId, &rangeRes);
                break;

        case 5: rc = clLogVwCheckRange(recHeader->msgId, &rangeRes);
                break;

        case 6: rc = clLogVwCheckRange(recHeader->timeStamp / CL_LOGVW_NANO_SEC, &rangeRes);
                break;

        default :  return CL_LOGVW_ERROR;
    }
    
    if(CL_OK != rc)
    {
        return rc;
    }

    *result = rangeRes;

    return rc;
}



/*
 * Compares the value specified in filter criterion
 * to the current records value
 
 *@param -  value
            value of field in current record

 *@param -  result
            CL_TRUE if value falls in range
            specified in filter criterion
 
 *@return - rc
            CL_OK if everything is OK
            error code otherwise
 */
ClRcT
clLogVwCheckRange(ClInt64T value, ClBoolT *result)
{
    ClRcT   rc  =   CL_OK;
    
    if(NULL == filterInfo)
    {
        return CL_LOGVW_ERROR;
    }

    switch(filterInfo->compareType)
    {
        case 0: if(value == filterInfo->fldVal)   
                    *result = CL_TRUE;

                break;
                
        case 1: if(value < filterInfo->fldVal)   
                    *result = CL_TRUE;

                break;

        case 2: if(value <= filterInfo->fldVal)   
                    *result = CL_TRUE;

                break;

        case 3: if(value > filterInfo->fldVal)   
                    *result = CL_TRUE;

                break;
                
        case 4: if(value >= filterInfo->fldVal)   
                    *result = CL_TRUE;

                break;
                
        case 5: if(value != filterInfo->fldVal)   
                    *result = CL_TRUE;

                break;

        default : return CL_LOGVW_ERROR;        
    }

    return rc;
}

