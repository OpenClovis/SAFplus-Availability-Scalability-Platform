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
#include <stdio.h>
#include <string.h>
#include <search.h>
#include <fcntl.h>
#include <unistd.h>
#ifdef SOLARIS_BUILD
#include <stdlib.h>
#endif

#include "clLogVwMapping.h"
#include "clLogVwParser.h"
#include "clLogVwHeader.h"
#include "clLogVwReader.h"
#include "clLogVwConstants.h"
#include "clLogVwUtil.h"
#include "clLogVwFilter.h"
#include "clLogVwTlv.h"
#include "clCommon.h"
#include "clLogVwErrors.h"

#include <../clLogViewerQnx.h>

#define LOGVW_MAX_ID_LEN 10

/*
 * Glabal id to name mapping ptr which
 * contains stating and current pointers
 * to id String and Name string for glabal
 * hash table
 */
static ClLogVwIdToNameMapT *idToNameMap = NULL;


/**************************************************************************/

static ClRcT clLogVwAddEntryToCompTable(ClLogVwCmpIdT compId);
static ClRcT clLogVwAddEntryToStreamTable(ClLogVwStreamIdT streamId);

static ClRcT clLogVwCreateCompNameMap(void);

static ClRcT clLogVwCreateStreamNameMap(void);

/**************************************************************************/



/*
 * Initializes id to name map global ptr
 * and opens configuration file and 
 * reads the endianess

 *@param -  logicalFileName
            common prefix of the log files

 *@return - rc
            CL_OK if everything is OK
            error code otherwise
 */
ClRcT
clLogVwInitNameMap(ClCharT *logicalFileName)
{
    ClRcT   rc  =   CL_OK;
    ClUint32T len = strlen(logicalFileName)+10;
    ClCharT *cfgFileName = (ClCharT*) calloc(1, len);

    CL_LOGVW_NO_MEMORY_CHK(cfgFileName);

    strncpy(cfgFileName, logicalFileName, len-1);
    strncat(cfgFileName, ".cfg", (len-strlen(cfgFileName)-1));


    idToNameMap = (ClLogVwIdToNameMapT*)
        malloc(sizeof(ClLogVwIdToNameMapT));

    if(NULL == idToNameMap)
    {
        free(cfgFileName);
        fprintf(stdout, "Out of memory at [%s : %d]\n",
                 __FILE__, __LINE__);
        return CL_ERR_NO_MEMORY;
    }

    idToNameMap->compNameStartPtr   = NULL;
    idToNameMap->streamNameStartPtr = NULL;
    idToNameMap->compIdStartPtr     = NULL;
    idToNameMap->streamIdStartPtr   = NULL;
    
    idToNameMap->nextCompIdIndex = CL_LOGVW_COMPID_MAP_INDEX;
    idToNameMap->nextStreamIdIndex = CL_LOGVW_STREAMID_MAP_INDEX;

    idToNameMap->mFd = -1;

    idToNameMap->compHashTb   = NULL;
    idToNameMap->streamHashTb = NULL;

    rc = clCheckCfgFile(cfgFileName);
    if( CL_OK != rc )
    {
        free(cfgFileName);
        return rc;
    }

    if( -1 == (idToNameMap->mFd = open(cfgFileName, O_RDONLY)))
    {
        fprintf(stdout, "\n\nTimeout occured while opening file '%s'.\n",
                cfgFileName);
        perror(""); 

        free(cfgFileName);
        return CL_LOGVW_ERROR;
    }
    free(cfgFileName);
    
    rc = clLogVwCreateStreamNameMap();

    if(CL_OK != rc)
    {
        return rc;
    }
        
    rc = clLogVwCreateCompNameMap();
    
    if(CL_OK != rc)
    {
        return rc;
    }

    if( lseek(idToNameMap->mFd, CL_LOGVW_MTD_ENDIAN_INDEX,
                SEEK_SET) != CL_LOGVW_MTD_ENDIAN_INDEX)
    {
        fprintf(stdout, "Unable to do lseek on meta data file.\n");
        return CL_LOGVW_ERROR;
    }

    if(sizeof(idToNameMap->endianess) !=
        read(idToNameMap->mFd, &(idToNameMap->endianess),
             sizeof(idToNameMap->endianess)))
    {
        return CL_LOGVW_ERROR;
    }

#ifdef SOLARIS_BUILD
    /* Solaris misses hcreate_r call, one single hash talbe is created */

    hcreate(CL_LOGVW_COMP_HASH_TB_SIZE + CL_LOGVW_STREAM_HASH_TB_SIZE);

#else /* not SOLARIS */
    /* Separate hash tables for component and 
     * stream id mapping.
     */
    idToNameMap->compHashTb   = (struct hsearch_data *) 
                                calloc(1, sizeof(struct hsearch_data));

    CL_LOGVW_NO_MEMORY_CHK(idToNameMap->compHashTb);

    hcreate_r(CL_LOGVW_COMP_HASH_TB_SIZE, idToNameMap->compHashTb);

    idToNameMap->streamHashTb = (struct hsearch_data *) 
                                calloc(1, sizeof(struct hsearch_data));
    
    CL_LOGVW_NO_MEMORY_CHK(idToNameMap->streamHashTb);
    
    hcreate_r(CL_LOGVW_STREAM_HASH_TB_SIZE, idToNameMap->streamHashTb);
#endif

    return CL_OK;
}




/*
 * Allocates memory for component id
 * to name mapping and initializes the
 * start pointers

 *@return - rc
            CL_OK if everything is OK
            error code otherwise
 */
ClRcT
clLogVwCreateCompNameMap(void)
{

    idToNameMap->compNamePtr = (ClCharT*)
        calloc(CL_LOGVW_MAX_MAP_COUNT, CL_LOGVW_COMP_NAME_MAX_LEN);
    
    CL_LOGVW_NO_MEMORY_CHK(idToNameMap->compNamePtr);

    idToNameMap->compIdPtr = (ClCharT*)
        calloc(CL_LOGVW_MAX_MAP_COUNT, 15);

    CL_LOGVW_NO_MEMORY_CHK(idToNameMap->compIdPtr);

    idToNameMap->compNameStartPtr = idToNameMap->compNamePtr;
    idToNameMap->compIdStartPtr = idToNameMap->compIdPtr;

    return CL_OK;
}




/*
 * Allocates memory for stream id
 * to name mapping and initializes the
 * start pointers

 *@return - rc
            CL_OK if everything is OK
            error code otherwise
 */
ClRcT
clLogVwCreateStreamNameMap(void)
{
    idToNameMap->streamNamePtr = (ClCharT*)
        calloc(CL_LOGVW_MAX_MAP_COUNT, CL_LOGVW_STREAM_NAME_MAX_LEN);
    
    CL_LOGVW_NO_MEMORY_CHK(idToNameMap->streamNamePtr);

    idToNameMap->streamIdPtr = (ClCharT*)
        calloc(CL_LOGVW_MAX_MAP_COUNT, 15);
    
    CL_LOGVW_NO_MEMORY_CHK(idToNameMap->streamIdPtr);

    /*Initlialize Start Pointers*/
    idToNameMap->streamNameStartPtr = idToNameMap->streamNamePtr;
    idToNameMap->streamIdStartPtr = idToNameMap->streamIdPtr;

    return CL_OK;
}




/*
 * Searches the componet id entry in comp 
 * hash table and finds the corresponding name

 *@param -  compId
            component id
 
 *@param -  compName
            contains component name 
            after return

 *@return - rc
            CL_OK if everything is OK
            error code otherwise
 */
ClRcT
clLogVwGetCompNameFromId(ClLogVwCmpIdT compId, ClCharT **compName)
{
    ClCharT compIdString[LOGVW_MAX_ID_LEN] = {0};
    
    ENTRY * retVal  = NULL;

    ENTRY item      = {NULL, NULL};

    snprintf(compIdString, LOGVW_MAX_ID_LEN, "%u", compId);

    item.key = compIdString;

#ifdef SOLARIS_BUILD
    retVal = hsearch(item, FIND);
#else
    if( 0 != hsearch_r(item, FIND, &retVal, idToNameMap->compHashTb))
    {
#endif
        if ( NULL != retVal )
        {
            if(NULL != (*compName = retVal->data))
            {
                return CL_OK;
            }
        }
#ifndef SOLARIS_BUILD
    }
#endif
    return CL_LOGVW_ERROR;
}




/*
 * Searches the stream id entry in stream 
 * hash table and finds the corresponding name

 *@param -  streamId
            stream id
 
 *@param -  streamName
            contains stream name 
            after return

 *@return - rc
            CL_OK if everything is OK
            error code otherwise
 */
ClRcT
clLogVwGetStreamNameFromId(ClLogVwStreamIdT streamId, ClCharT **streamName)
{
    ClCharT streamIdtring[LOGVW_MAX_ID_LEN] = {0};

    ENTRY item    = {NULL, NULL};
    ENTRY *retVal = NULL;

    snprintf(streamIdtring, LOGVW_MAX_ID_LEN, "%hu", streamId);
    
    item.key = streamIdtring;

#ifdef SOLARIS_BUILD
    retVal = hsearch(item, FIND);
#else
    if( 0 != hsearch_r(item, FIND, &retVal, idToNameMap->streamHashTb))
    {
#endif
        if ( NULL != retVal )
        {
            if(NULL != (*streamName = retVal->data))
            {
                return CL_OK;
            }
        }
#ifndef SOLARIS_BUILD
    }
#endif

    return CL_LOGVW_ERROR;
}




/*
 * Finds stream id corresponding to stream name
 * from stream hash table
 
 *@param -  name
            stream name
 
 *@param -  streamId
            contains stream id after return
 
 *@return - rc
            CL_OK if everything is OK
            error code otherwise
 */
ClRcT
clLogVwGetStreamIdFromName(ClCharT *name, ClLogVwStreamIdT *streamId)
{

    ENTRY       item        =   {NULL, NULL};
    ENTRY*      retVal      =   NULL;
    ClCharT*    tempPtr     =   NULL;

    tempPtr = idToNameMap->streamIdStartPtr;

    CL_LOGVW_NULL_CHK(tempPtr);
    
    while( 0 != strcmp(tempPtr, ""))
    {
        item.key = tempPtr;

#ifdef SOLARIS_BUILD
    retVal = hsearch(item, FIND);
#else
        if( 0 != hsearch_r(item, FIND, &retVal, idToNameMap->streamHashTb))
        {
#endif
            if ( NULL != retVal )
            {
                if( (NULL != retVal->data) &&
                        0 == strcmp(retVal->data, name))
                {
                    *streamId = atoi(tempPtr);
                    return CL_OK;
                }
            }
#ifndef SOLARIS_BUILD
        }
#endif
       tempPtr += strlen(tempPtr) + 1;
    }

    return CL_LOGVW_ERROR;
}

ClRcT
clLogVwGetCompIdFromName(ClCharT *name, ClLogVwCmpIdT *compId)
{

    ENTRY       item        =   {NULL, NULL};
    ENTRY*      retVal      =   NULL;
    ClCharT*    tempPtr     =   NULL;

    tempPtr = idToNameMap->compIdStartPtr;

    CL_LOGVW_NULL_CHK(tempPtr);
    
    while( 0 != strcmp(tempPtr, ""))
    {
        item.key = tempPtr;

#ifdef SOLARIS_BUILD
    retVal = hsearch(item, FIND);
#else
        if( 0 != hsearch_r(item, FIND, &retVal, idToNameMap->compHashTb))
        {
#endif
            if ( NULL != retVal )
            {
                if( (NULL != retVal->data) &&
                        0 == strcmp(retVal->data, name))
                {
                    *compId = atoi(tempPtr);
                    return CL_OK;
                }
            }
#ifndef SOLARIS_BUILD
        }
#endif
       tempPtr += strlen(tempPtr) + 1;
    }

    return CL_LOGVW_ERROR;
}



/*
 * Adds entry to hash table for component
 * id to name by reading the metadata file

 *@return - rc
            CL_OK if everything is OK
            error code otherwise
 */
ClRcT
clLogVwAddEntryToCompMap(void)
{
    ClCharT buf[CL_LOGVW_COMP_NAME_MAX_LEN] = {0};

    ClLogVwCmpIdT    compId    = 0;
    ClUint16T   length    = 0;

    ClRcT       rc   = CL_OK;

    ClUint16T nextCompIdIndex = idToNameMap->nextCompIdIndex;

    if( lseek(idToNameMap->mFd, nextCompIdIndex,
                SEEK_SET) != nextCompIdIndex)
    {
        fprintf(stdout, "Unable to do lseek on meta data file.\n");
        return CL_LOGVW_ERROR;
    }
    
    if(sizeof(nextCompIdIndex) !=
        read(idToNameMap->mFd, buf,  sizeof(nextCompIdIndex)))
    {
        return CL_LOGVW_ERROR;
    }

    if(clLogVwIsSameEndianess(idToNameMap->endianess))
    {

        memcpy((ClCharT*)&nextCompIdIndex, buf,
                sizeof(nextCompIdIndex));

        while(CL_LOGVW_NEXT_ENTRY_END != nextCompIdIndex )
        {
            idToNameMap->nextCompIdIndex =  nextCompIdIndex;

            if( lseek(idToNameMap->mFd, nextCompIdIndex,
                        SEEK_SET) != nextCompIdIndex)
            {
                fprintf(stdout, "Unable to do lseek on meta data file.\n");
                return CL_LOGVW_ERROR;
            }

            if(sizeof(nextCompIdIndex) !=
                read(idToNameMap->mFd, buf,  sizeof(nextCompIdIndex)))
            {
                return CL_LOGVW_ERROR;
            }

            memcpy((ClCharT*)&nextCompIdIndex, buf, 
                    sizeof(nextCompIdIndex)); 
            
            if(sizeof(length) !=
                read(idToNameMap->mFd, buf, sizeof(length)))
            {
                return CL_LOGVW_ERROR;
            }
            memcpy((ClCharT*)&length, buf, sizeof(length)); 

            if(length !=
                    read(idToNameMap->mFd, buf, length))
            {
                return CL_LOGVW_ERROR;
            }
            memcpy((ClCharT*)idToNameMap->compNamePtr, buf, length);

            if(sizeof(compId) !=
                read(idToNameMap->mFd, buf, sizeof(compId)))
            {
                return CL_LOGVW_ERROR;
            }
            memcpy((ClCharT*)&compId, buf, sizeof(compId)); 

            rc = clLogVwAddEntryToCompTable(compId);

            if(CL_OK != rc)
            {
                return rc;
            }

        }
    }
    else
    {
        rc = clLogVwGetRevSubBytes(buf, 0, sizeof(nextCompIdIndex));

        if(CL_OK != rc)
        {
            return rc;
        }

        memcpy((ClCharT*)&nextCompIdIndex, buf, sizeof(nextCompIdIndex));

        while(CL_LOGVW_NEXT_ENTRY_END != nextCompIdIndex )
        {

            idToNameMap->nextCompIdIndex =  nextCompIdIndex;

            if( lseek(idToNameMap->mFd, nextCompIdIndex,
                        SEEK_SET) != nextCompIdIndex)
            {
                fprintf(stdout, "Unable to do lseek on meta data file.\n");
                return CL_LOGVW_ERROR;
            }
            if(sizeof(nextCompIdIndex) !=
                read(idToNameMap->mFd, buf,  sizeof(nextCompIdIndex)))
            {
                return CL_LOGVW_ERROR;
            }

            rc = clLogVwGetRevSubBytes(buf, 0, sizeof(nextCompIdIndex));

            if(CL_OK != rc)
            {
                return rc;
            }
            memcpy((ClCharT*)&nextCompIdIndex, buf, sizeof(nextCompIdIndex));

            if(sizeof(length) !=
                read(idToNameMap->mFd, buf, sizeof(length)))
            {
                return CL_LOGVW_ERROR;
            }

            rc = clLogVwGetRevSubBytes(buf, 0, sizeof(length));

            if(CL_OK != rc)
            {
                return rc;
            }
            memcpy((ClCharT*)&length, buf, sizeof(length));


            if(length !=
                read(idToNameMap->mFd, buf, length))
            {
                return CL_LOGVW_ERROR;
            }

            rc = clLogVwGetRevSubBytes(buf, 0, length);

            if(CL_OK != rc)
            {
                return rc;
            }
            memcpy((ClCharT*)idToNameMap->compNamePtr, buf, length);
            

            if(sizeof(compId) !=
                read(idToNameMap->mFd, buf, sizeof(compId)))
            {
                return CL_LOGVW_ERROR;
            }

            rc = clLogVwGetRevSubBytes(buf, 0, sizeof(compId));

            if(CL_OK != rc)
            {
                return rc;
            }
            memcpy((ClCharT*)&compId, buf, sizeof(compId));


            rc = clLogVwAddEntryToCompTable(compId);

            if(CL_OK != rc)
            {
                return rc;
            }
        }

    }

    return CL_OK;
}




/*
 * Adds the compId to compName
 * entry to comp hash table

 *@param -  id
            component id

 *@return - rc
            CL_OK if everything is OK
            error code otherwise
 */
ClRcT
clLogVwAddEntryToCompTable(ClLogVwCmpIdT compId)
{
    ENTRY item      = {NULL, NULL};
    ENTRY * retVal = NULL;

    snprintf(idToNameMap->compIdPtr, (CL_LOGVW_MAX_MAP_COUNT*15), "%u", compId);

    item.key = idToNameMap->compIdPtr;
    item.data = idToNameMap->compNamePtr;
#ifdef SOLARIS_BUILD
    retVal = hsearch(item, ENTER);
#else
    if( 0 == hsearch_r(item, ENTER, &retVal, idToNameMap->compHashTb))
    {
        fprintf(stdout, "Error while making an entry in hash table.\n");
        return CL_LOGVW_ERROR;
    }
#endif

    idToNameMap->compNamePtr += strlen(idToNameMap->compNamePtr) + 1;    
    idToNameMap->compIdPtr += strlen(idToNameMap->compIdPtr) + 1;

    return CL_OK;
}

/*
 * Adds the streamId to streamName
 * entry to stream hash table

 *@param -  id
            component id

 *@return - rc
            CL_OK if everything is OK
            error code otherwise
 */
ClRcT
clLogVwAddEntryToStreamTable(ClLogVwStreamIdT streamId)
{

    ENTRY item      = {NULL, NULL};
    ENTRY * retVal = NULL;

    snprintf(idToNameMap->streamIdPtr, (CL_LOGVW_MAX_MAP_COUNT*15), "%hu", streamId);
    item.key = idToNameMap->streamIdPtr;
    item.data = idToNameMap->streamNamePtr;

#ifdef SOLARIS_BUILD
    retVal = hsearch(item, ENTER);
#else
    if( 0 == hsearch_r(item, ENTER, &retVal, idToNameMap->streamHashTb))
    {
        fprintf(stdout, "Error while making an entry in hash table.\n");
        return CL_LOGVW_ERROR;
    }
#endif

    idToNameMap->streamNamePtr += strlen(idToNameMap->streamNamePtr) + 1;    
    idToNameMap->streamIdPtr += strlen(idToNameMap->streamIdPtr) + 1;
    return CL_OK;
}

/*
 * Adds entry to hash table for stream 
 * id to name by reading the metadata file

 *@return - rc
            CL_OK if everything is OK
            error code otherwise
 */
ClRcT
clLogVwAddEntryToStreamMap(void)
{
    ClCharT buf[CL_LOGVW_STREAM_NAME_MAX_LEN] = {0};
    
    ClLogVwStreamIdT    streamId    =   0;
    ClUint16T   length      =   0;

    ClRcT       rc     =   CL_OK;

    ClUint16T nextStreamIdIndex = idToNameMap->nextStreamIdIndex;

    if( lseek(idToNameMap->mFd, nextStreamIdIndex,
                SEEK_SET) != nextStreamIdIndex)
    {
        fprintf(stdout, "Unable to do lseek on meta data file.\n");
        return CL_LOGVW_ERROR;
    }

    if(sizeof(nextStreamIdIndex) !=
            read(idToNameMap->mFd, buf,  sizeof(nextStreamIdIndex)))
    {
        return CL_LOGVW_ERROR;
    }

    if(clLogVwIsSameEndianess(idToNameMap->endianess))
    {
        memcpy((ClCharT*)&nextStreamIdIndex, buf, 
                sizeof(nextStreamIdIndex)); 

        while(CL_LOGVW_NEXT_ENTRY_END != nextStreamIdIndex)
        {

            idToNameMap->nextStreamIdIndex = nextStreamIdIndex;

            if( lseek(idToNameMap->mFd, nextStreamIdIndex,
                        SEEK_SET) != nextStreamIdIndex)
            {
                fprintf(stdout, "Unable to do lseek on meta data file.\n");
                return CL_LOGVW_ERROR;
            }
            
            if(sizeof(nextStreamIdIndex) !=
                read(idToNameMap->mFd, buf,  sizeof(nextStreamIdIndex)))
            {
                return CL_LOGVW_ERROR;
            }

            memcpy((ClCharT*)&nextStreamIdIndex, buf, 
                    sizeof(nextStreamIdIndex)); 

            if(sizeof(length) != 
                read(idToNameMap->mFd, buf, sizeof(length)))
            {
                return CL_LOGVW_ERROR;
            }
            memcpy((ClCharT*)&length, buf, sizeof(length)); 

            if(length != 
                read(idToNameMap->mFd, buf, length))
            {
                return CL_LOGVW_ERROR;
            }
            memcpy((ClCharT*)idToNameMap->streamNamePtr, buf, length);

            if(sizeof(streamId) != 
                read(idToNameMap->mFd, buf, sizeof(streamId)))
            {
                return CL_LOGVW_ERROR;
            }
            
            memcpy((ClCharT*)&streamId, buf, sizeof(streamId)); 

            rc = clLogVwAddEntryToStreamTable(streamId);

            if(CL_OK != rc)
            {
                return rc;
            }

        }
    }
    else
    {
        rc = clLogVwGetRevSubBytes(buf, 0, sizeof(nextStreamIdIndex));

        if(CL_OK != rc)
        {
            return rc;
        }

        memcpy((ClCharT*)&nextStreamIdIndex, buf, sizeof(nextStreamIdIndex));

        while(CL_LOGVW_NEXT_ENTRY_END != nextStreamIdIndex)
        {

            idToNameMap->nextStreamIdIndex =  nextStreamIdIndex;

            if( lseek(idToNameMap->mFd, nextStreamIdIndex,
                        SEEK_SET) != nextStreamIdIndex)
            {
                fprintf(stdout, "Unable to do lseek on meta data file.\n");
                return CL_LOGVW_ERROR;
            }
            
            if(sizeof(nextStreamIdIndex) !=
                read(idToNameMap->mFd, buf,  sizeof(nextStreamIdIndex)))
            {
               return CL_LOGVW_ERROR;
            }
    
            rc = clLogVwGetRevSubBytes(buf, 0, sizeof(nextStreamIdIndex));

            if(CL_OK != rc)
            {
                return rc;
            }
            memcpy((ClCharT*)&nextStreamIdIndex, buf, sizeof(nextStreamIdIndex));

            if(sizeof(length) !=
                read(idToNameMap->mFd, buf, sizeof(length)))
            {
                return CL_LOGVW_ERROR;
            }

            rc = clLogVwGetRevSubBytes(buf, 0, sizeof(length));

            if(CL_OK != rc)
            {
                return rc;
            }
            memcpy((ClCharT*)&length, buf, sizeof(length));

            if(length != 
                read(idToNameMap->mFd, buf, length))
            {
                return CL_LOGVW_ERROR;
            }

            rc = clLogVwGetRevSubBytes(buf, 0, length);

            if(CL_OK != rc)
            {
                return rc;
            }

            memcpy((ClCharT*)idToNameMap->streamNamePtr, buf, length);

            if(sizeof(streamId) !=
                read(idToNameMap->mFd, buf, sizeof(streamId)))
            {
                return CL_LOGVW_ERROR;
            }

            rc = clLogVwGetRevSubBytes(buf, 0, sizeof(streamId));

            if(CL_OK != rc)
            {
                return rc;
            }
            memcpy((ClCharT*)&streamId, buf, sizeof(streamId));


            clLogVwAddEntryToStreamTable(streamId);

        }
    }

    return CL_OK;
}



/*
 * Frees up allocated memory and
 * closes the metadata file descriptor

 *@param -  

 *@return - rc
            CL_OK if everything is OK
            error code otherwise
 */
ClRcT
clLogVwCleanUpNameMap(void)
{
#ifdef SOLARIS_BUILD
    hdestroy();
#else
    hdestroy_r(idToNameMap->compHashTb);
    hdestroy_r(idToNameMap->streamHashTb);
#endif
    
    if(NULL != idToNameMap)
    {
#ifndef SOLARIS_BUILD
        free(idToNameMap->compHashTb);
        free(idToNameMap->streamHashTb);
#endif
        free(idToNameMap->compNameStartPtr);
        free(idToNameMap->compIdStartPtr);
        free(idToNameMap->streamNameStartPtr);
        free(idToNameMap->streamIdStartPtr);

        if( -1 != idToNameMap->mFd)
        {
            if( -1 == close(idToNameMap->mFd) )
            {
                //fprintf(stderr, "\n\nUnable to close the file.\n\n");
            }
        }
        free(idToNameMap);
    }

    
    return CL_OK;
}

