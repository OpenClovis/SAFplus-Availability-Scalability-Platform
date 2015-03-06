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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <search.h>

#include "clLogVwParser.h"
#include "clLogVwHeader.h"
#include "clLogVwReader.h"
#include "clLogVwConstants.h"
#include "clLogVwUtil.h"
#include "clLogVwFilter.h"
#include "clLogVwTlv.h"
#include "clLogVwErrors.h"
#include <../clLogViewerQnx.h>

/*
 * Global variable which contains
 * elements required for parsing 
 * log record such as record header
 * and tlv hash map
 */

static  ClLogVwParserT   *gParser    =   NULL;

/**************************************************************************/

static ClRcT 
clLogVwParseTlvSE(ClLogVwByteT *bytes, ClUint32T msgId, ClUint16T messageSize);

static ClRcT
clLogVwParseTlvDE(ClLogVwByteT *bytes, ClUint32T msgId, ClUint16T messageSize);

static ClRcT
clLogVwLogParseTlvMsg(ClLogVwByteT *bytes, ClLogVwRecHeaderT *recHeader, ClUint16T messageSize);

static ClRcT
clLogVwDispTlvMsgString(ClCharT **subStrPtr, ClBoolT *dispTlvValue);

static ClRcT
clLogVwGetTlvString(ClUint16T msgId, ClCharT **tlvStr);


static ClRcT
clLogVwPrintTlvValue(ClLogVwByteT *bytes, ClUint16T start, ClUint16T length, 
                     ClUint16T tag);

/**************************************************************************/


/*
 * Allocates memory to global parser ptr
 * and initializes its element

 *@return - rc
            CL_OK if everything is OK
            otherwise error code
 */
ClRcT
clLogVwInitParserData(void)
{
    gParser = (ClLogVwParserT*) malloc(sizeof(ClLogVwParserT));

    CL_LOGVW_NO_MEMORY_CHK(gParser);

    gParser->recHeader  =   NULL;
    
    gParser->recHeader = (ClLogVwRecHeaderT*) malloc(sizeof(ClLogVwRecHeaderT));

    CL_LOGVW_NO_MEMORY_CHK(gParser->recHeader);
    
    gParser->tlvHash        =   NULL;
    gParser->isDataColOn    =   CL_FALSE;

    gParser->headerSize     =   0;

    gParser->recordSize     =   0;

    gParser->outPtr         =   stdout; 

    gParser->tlvHashTb      =   NULL;

    gParser->isTlvFilePresent =   CL_FALSE;

    return CL_OK;
}


/*
 * Frees up allocated memory to global parser ptr

 *@return - rc
            CL_OK if everything is OK
            otherwise error code
 */
ClRcT
clLogVwCleanUpParserData(void)
{
    if( NULL != gParser )
    {
        free(gParser->recHeader);
        free(gParser);
    }
    
    return CL_OK;
}


/*
 * Sets header size, message size 
 * of log record

 *@return - rc
            CL_OK if everything is OK
            otherwise error code
 */
ClRcT
clLogVwSetParserHeaderSize(ClUint16T size)
{
    if( NULL != gParser )
    {
        gParser->headerSize = size;
        return CL_OK;
    }

    return CL_LOGVW_ERROR;
}


/*
 * Sets record size of a log record

 *@return - rc
            CL_OK if everything is OK
            otherwise error code
 */
ClRcT
clLogVwSetParserRecSize(ClLogVwMaxRecSizeT size)
{
    if( NULL != gParser )
    {
        gParser->recordSize = size;
        return CL_OK;
    }

    return CL_LOGVW_ERROR;
}


/*
 * Sets output pointer to a file pointer
 
 *@return - rc
            CL_OK if everything is OK
            otherwise error code
 */
ClRcT
clLogVwSetParserOutPtr(FILE *fp)
{
    if( NULL == gParser )
        return CL_LOGVW_ERROR;

    gParser->outPtr = fp;

    return CL_OK;
}


/*
 * Sets the data column flag
 * to 'flag' value

 *@return - rc
            CL_OK if everything is OK
            otherwise error code
 */
ClRcT
clLogVwSelDataColumn(ClBoolT flag)
{
    if( NULL != gParser )
    {
        gParser->isDataColOn = flag;
        return CL_OK;
    }

    return CL_LOGVW_ERROR;
}


/*
 * Parses the log record and
 * displays the message part of the record

 * If filter flag is provided
 * it checks if record is passing 
 * through the filter
 
 * It displays the message part of
 * record 
 * It calls the tlv parser if there
 * is any tlv value in messsage part
 
 *@param -  bytes
            raw bytes of a log record

 *@return - rc
            CL_OK if everything is OK
            otherwise error code
 */
ClRcT
clLogVwParseOneRecord(ClLogVwByteT *bytes)
{

    ClLogVwMaxRecSizeT  messageSize =   0;

    ClUint16T   binaryMsgIdx        =   0;

    ClRcT       rc              =   CL_OK;

    ClBoolT     filterFlag      =   CL_FALSE;

    ClBoolT     filterRes       =   CL_FALSE;

    ClBoolT     selColFlag      =   CL_FALSE;

    ClUint32T   binaryMsglen        =   0;

    ClUint32T   binaryMsgStartIdx   =   0;


    /*Get the header info*/
    rc = clLogVwInitRecHeader(bytes, gParser->recHeader);

    if(CL_OK != rc) 
    {
        return rc;
    }

    rc = clLogVwGetSelColumnsFlag(&selColFlag);

    if(CL_OK != rc) 
    {
        return rc;
    }

    if( 0 >= gParser->recHeader->timeStamp )
    {
        //fprintf(stdout, "\nBlank record...\n");//......
        return CL_OK;
    }

    rc = clLogVwGetFilterFlag(&filterFlag);

    if(CL_OK != rc) 
    {
        return rc;
    }

    if(CL_TRUE == filterFlag)
    {
        rc = clLogVwApplyFilter(gParser->recHeader, &filterRes);

        if(CL_OK != rc) 
        {
            return rc;
        }

        if(CL_FALSE == filterRes)
        {
            //fprintf(stdout, "\nRecord didn't pass the filter.\n");
            return CL_OK;
        }
    }

    rc = clLogVwDispLogRecHeader(gParser->recHeader);
    if(CL_OK != rc) 
    {
        fprintf(stdout, "[Error : parsing of record failed!!] \n");
        CL_LOGVW_PRINT_ASCII_REC_ERROR();
        return rc;
    }

    messageSize = gParser->recordSize - gParser->headerSize; 

    if(CL_TRUE == selColFlag &&
            CL_FALSE == gParser->isDataColOn)
    {
        return CL_OK; 
    }

    if(CL_LOGVW_MSGID_ASCII == gParser->recHeader->msgId)
    {
        fprintf(stdout, "[ERROR : Record is of ASCII type (log viewer supports only Binary and TLV records) ] \n");
        return rc;
    }

    if(CL_LOGVW_MSGID_BINARY == gParser->recHeader->msgId)
    {

        if(CL_FALSE ==
                clLogVwIsSameEndianess(gParser->recHeader->endianess))
        {

            rc = clLogVwGetRevSubBytes(bytes, gParser->headerSize,
                    sizeof(binaryMsglen));
        }
        memcpy((ClCharT*)&binaryMsglen, bytes + gParser->headerSize,
                sizeof(binaryMsglen));

        if((binaryMsglen + sizeof(binaryMsglen)) > messageSize)
        {
            binaryMsglen = messageSize - sizeof(binaryMsglen);
        }

        binaryMsgStartIdx = gParser->headerSize + sizeof(binaryMsglen);

        fprintf(gParser->outPtr, "( ");
        for(binaryMsgIdx = binaryMsgStartIdx; 
                binaryMsgIdx < (binaryMsgStartIdx + binaryMsglen);
                binaryMsgIdx++)
        {
            fprintf(gParser->outPtr, "0x%02hhx ", (bytes)[binaryMsgIdx]);
        }

    }
    else
    {

        fprintf(gParser->outPtr, "(");
        //process TLV message

        if(CL_FALSE == gParser->isTlvFilePresent)
        {
            fprintf(stdout, "[ Error: Unable to parse tlv message : "
                    "TLV file not specified! ]" ); 
            return CL_LOGVW_ERROR;
        }

        rc = clLogVwLogParseTlvMsg(bytes + gParser->headerSize,
                gParser->recHeader, 
                messageSize);

        if(CL_OK != rc) 
        {
            fprintf(stdout, "[Error in tlv parsing.]\n");
            return rc;
        }

    }

    fprintf(gParser->outPtr, ") ");

    return rc;
}


/*
 * Parses tlv message if the record
 * and the local machine has same endianess

 *@param -  bytes
            raw bytes of the log record

 *@param -  msgId
            message id for id to 
            string mapping

 *@param -  messageSize
            size in bytes of the message part

 *@return - rc
            CL_OK if everything is OK
            otherwise error code
 */
static ClRcT
clLogVwParseTlvSE(ClLogVwByteT *bytes, ClUint32T msgId, ClUint16T messageSize)
{
    ClInt32T    i       =   0;

    ClUint16T   tag     =   0, 
                length  =   0;

    ClRcT       rc      =   CL_OK;

    ClCharT     *subStrPtr = NULL;

    ClBoolT     dispTlvValue = CL_FALSE;

    rc = clLogVwGetTlvString(msgId, &subStrPtr);

    if( CL_OK != rc)
    {
        fprintf(stdout, "[Mapping string not found in TLV file.]");
        return rc;
    }


    for(;;)
    {
        dispTlvValue = CL_FALSE;

        if(i >= messageSize)
        {
            return -1;
        }

        memcpy((ClCharT*)&tag, bytes + i, sizeof(tag));

        if( CL_LOGVW_TAG_END == tag )
            break;

        memcpy((ClCharT*)&length, bytes + i + CL_LOGVW_TLV_LENGTH_INDEX, sizeof(tag));


        if( CL_LOGVW_TAG_STRING == tag)
        {
            rc = clLogVwDispTlvMsgString(&subStrPtr, &dispTlvValue);

            if( CL_OK != rc)
            {
                return rc;
            }
            if(CL_TRUE == dispTlvValue)
            {
                ClCharT *stringValue = (ClCharT*) calloc(1, length +2);

                CL_LOGVW_NO_MEMORY_CHK(stringValue);

                memcpy(stringValue, bytes + i + CL_LOGVW_TLV_VALUE_INDEX, length);

                fprintf(gParser->outPtr, "%s", stringValue);
                free(stringValue);
            }

        }
        else if(CL_LOGVW_TAG_BASIC_SIGNED || CL_LOGVW_TAG_BASIC_UNSIGNED)
        {
            rc = clLogVwDispTlvMsgString(&subStrPtr, &dispTlvValue);

            if( CL_OK != rc)
            {
                return rc;
            }

            if(CL_TRUE == dispTlvValue)
            {
                rc = clLogVwPrintTlvValue(bytes , 
                        i + CL_LOGVW_TLV_VALUE_INDEX,
                        length , 
                        tag);

                if( CL_OK != rc)
                {
                    return rc;
                }
            }
        }
        else
        {/* Invalid tag found */

            return CL_LOGVW_ERROR;
        }

        i += sizeof(tag) + sizeof (length) + length;
    }

    if(NULL != subStrPtr)
    {
        fprintf(gParser->outPtr, "%s", subStrPtr);
    }
    
    fflush(stdout);
    return CL_OK;
}
    


/*
 * Parses tlv message if the record
 * and the local machine has different endianess

 *@param -  bytes
            raw bytes of the log record

 *@param -  msgId
            message id for id to 
            string mapping

 *@param -  messageSize
            size in bytes of the message part

 *@return - rc
            CL_OK if everything is OK
            otherwise error code
 */
static ClRcT
clLogVwParseTlvDE(ClLogVwByteT *bytes, ClUint32T msgId, ClUint16T messageSize)
{
    ClInt32T    i = 0;

    ClUint16T   tag     =   0, 
                length  =   0;

    ClRcT       rc      =   CL_OK;

    ClCharT     *subStrPtr = NULL;

    ClBoolT     dispTlvValue = CL_FALSE;

    rc = clLogVwGetTlvString(msgId, &subStrPtr);

    if( CL_OK != rc)
    { 
        fprintf(stdout, "Mapping string not found in TLV file.");
        return rc;
    }

    for(;;)
    {
        dispTlvValue = CL_FALSE;

        if(i >= messageSize)
        {
            return CL_LOGVW_ERROR;
        }

        rc = clLogVwGetRevSubBytes(bytes, i, sizeof(tag));

        if( CL_OK != rc)
        {
            return rc;
        }

        memcpy((ClCharT*)&tag, bytes + i, sizeof(tag));

        if( CL_LOGVW_TAG_END == tag )
            break;

        rc = clLogVwGetRevSubBytes( bytes, 
                                        i + CL_LOGVW_TLV_LENGTH_INDEX,
                                        sizeof(length));

        if( CL_OK != rc)
        {
            return rc;
        }

        memcpy((ClCharT*)&length, bytes + i + CL_LOGVW_TLV_LENGTH_INDEX,
                sizeof(length));

        if( CL_LOGVW_TAG_STRING == tag)
        {
            rc = clLogVwDispTlvMsgString(&subStrPtr, &dispTlvValue);

            if( CL_OK != rc)
            {
                return rc;
            }
            if(CL_TRUE == dispTlvValue)
            {
                ClCharT *stringValue = (ClCharT*) calloc(1, length +2);

                CL_LOGVW_NO_MEMORY_CHK(stringValue);

                memcpy(stringValue, bytes + i + CL_LOGVW_TLV_VALUE_INDEX, length);

                fprintf(gParser->outPtr, "%s", stringValue);
                free(stringValue);
            }
        }
        else if(CL_LOGVW_TAG_BASIC_SIGNED || CL_LOGVW_TAG_BASIC_UNSIGNED)
        {

            rc = clLogVwDispTlvMsgString(&subStrPtr, &dispTlvValue);

            if( CL_OK != rc)
            {
                return rc;
            }
            if(CL_TRUE == dispTlvValue)
            {
                rc = clLogVwGetRevSubBytes(bytes, 
                        i + CL_LOGVW_TLV_VALUE_INDEX,
                        length );

                if( CL_OK != rc)
                {
                    return rc;
                }
                rc = clLogVwPrintTlvValue(bytes , 
                        i + CL_LOGVW_TLV_VALUE_INDEX, 
                        length, 
                        tag);

                if( CL_OK != rc)
                {
                    return rc;
                }
            }

        }
        else
        {/* Invalid tag found */
            return CL_LOGVW_ERROR;
        }

        i += sizeof(tag) + sizeof (length) + length;
    }

    if(NULL != subStrPtr)
    {
        fprintf(gParser->outPtr, "%s", subStrPtr);
    }

    fflush(stdout);
    return CL_OK;
}



/*
 * Parses tlv message

 *@param -  bytes
            raw bytes of log record

 *@param -  recHeader
            record header

 *@param -  messageSize
            size in bytes of the message part

 *@return - rc
            CL_OK if everything is OK
            otherwise error code
 */
static ClRcT
clLogVwLogParseTlvMsg(ClLogVwByteT *bytes, ClLogVwRecHeaderT *recHeader, ClUint16T messageSize)
{
    ClRcT   rc =   CL_OK;

    if(CL_TRUE == 
            clLogVwIsSameEndianess(recHeader->endianess))
    {
        rc = clLogVwParseTlvSE(bytes, recHeader->msgId, messageSize);

        if( CL_OK != rc)
        {
            return rc;
        }
    }
    else
    {
        rc = clLogVwParseTlvDE(bytes, recHeader->msgId, messageSize);
        
        if( CL_OK != rc)
        {
            return rc;
        }
    }

    return CL_OK;

}



/*
 * Displays part of the tlv message 
 * before CL_TLV_DELM and checks whether
 * tlv value should be displayed or not

 *@param -  subStrPtr
            remaining string after replacing 
            last CL_TLV_DELM

 *@param -  value
            value from tlv

 *@param -  tag
            Indicates whether value is string
            or integer

 *@return - rc
            CL_OK if everything is OK
            otherwise error code
 */
static ClRcT
clLogVwDispTlvMsgString(ClCharT **subStrPtr, ClBoolT *dispTlvValue)
{

    if(NULL == subStrPtr || NULL == *subStrPtr)
    {
        return CL_LOGVW_ERROR;
    }
    *dispTlvValue = CL_FALSE;

    ClCharT *msgString = (ClCharT*) calloc(1, strlen(*subStrPtr) + 1);

    CL_LOGVW_NO_MEMORY_CHK(msgString);

    ClCharT *prevPtr = *subStrPtr;

    *subStrPtr = strstr(*subStrPtr, CL_LOGVW_TLV_DELM); 

    if(NULL != *subStrPtr) 
    {
        *dispTlvValue = CL_TRUE;

        memcpy(msgString, prevPtr, *subStrPtr - prevPtr); 

        fprintf(gParser->outPtr, "%s", msgString);
        *subStrPtr = *subStrPtr + strlen(CL_LOGVW_TLV_DELM);
    }
    else
    {
        fprintf(gParser->outPtr, "%s", prevPtr);
        free(msgString);
        return CL_LOGVW_ERROR;
    }

    free(msgString);

    return CL_OK;
}



/*
 * Gets the mapping string for message id

 *@param -  msgId
            message id

 *@param -  tlvStr
            contains tlv mapping string
            for message id 'msgId'
            after return

 *@return - rc
            CL_OK if everything is OK
            otherwise error code
 */
static ClRcT
clLogVwGetTlvString(ClUint16T msgId, ClCharT **tlvStr)
{
    ClCharT msgString  [10] = {0};
    
    ENTRY item    = {NULL, NULL};
    ENTRY *retVal = NULL;

    sprintf(msgString, "%hu", msgId);

    item.key = msgString;

#ifdef SOLARIS_BUILD
    retVal = hsearch(item, FIND);
#else
    if( 0 != hsearch_r(item, FIND, &retVal, gParser->tlvHashTb))
    {
#endif
        if ( NULL != retVal )
        {
            if(NULL != (*tlvStr = retVal->data))
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
 * Prints value of tlv from raw bytes of log record

 *@param -  bytes
            raw bytes of log record

 *@param -  start
            strting index of value in bytes

 *@param -  length
            length in bytes of value

 *@param -  tag
            tag of tlv

 *@param -  value
            contains integer value after
            return

 *@return - rc
            CL_OK if everything is OK
            otherwise error code
 */
static ClRcT
clLogVwPrintTlvValue(ClLogVwByteT *bytes, ClUint16T start, ClUint16T length, 
                     ClUint16T tag)
{

    ClUint8T    unsignedByte    =   0;
    ClInt8T     signedByte      =   0;
    ClUint16T   unsignedShort   =   0;
    ClInt16T    signedShort     =   0;
    ClUint32T   unsignedInt     =   0;
    ClInt32T    signedInt       =   0;
    ClUint64T   unsignedLong    =   0;
    ClInt64T    signedLong      =   0;

    ClUint64T   uValue          =   0;
    ClInt64T    sValue          =   0;

    ClRcT       rc              =   CL_OK;


    if(CL_FALSE == clLogVwIsSameEndianess(
                gParser->recHeader->endianess))
    {
        rc = clLogVwGetRevSubBytes(bytes, start, length);

        if(CL_OK != rc)
        {
            return rc;
        }
    }

    switch( tag )
    {
        case CL_LOGVW_TAG_BASIC_SIGNED: 

            switch(length)
            {
                case 1: memcpy((ClCharT*)&signedByte, bytes + start, length);
                        sValue = (ClInt64T) signedByte;
                        break;

                case 2: memcpy((ClCharT*)&signedShort, bytes + start, length);
                        sValue = (ClInt64T) signedShort;
                        break;

                case 4: memcpy((ClCharT*)&signedInt, bytes + start, length);
                        sValue = (ClInt64T) signedInt;
                        break;

                case 8: memcpy((ClCharT*)&signedLong, bytes + start, length);
                        sValue = (ClInt64T) signedLong;
                        break;

                default: return CL_LOGVW_ERROR;
            }
            break;

        case CL_LOGVW_TAG_BASIC_UNSIGNED: 

            switch(length)
            {
                case 1: memcpy((ClCharT*)&unsignedByte, bytes + start, length);
                        uValue = (ClUint64T) unsignedByte;
                        break;

                case 2: memcpy((ClCharT*)&unsignedShort, bytes + start, length);
                        uValue = (ClUint64T) unsignedShort;
                        break;

                case 4: memcpy((ClCharT*)&unsignedInt, bytes + start, length);
                        uValue = (ClUint64T) unsignedInt;
                        break;

                case 8: memcpy((ClCharT*)&unsignedLong, bytes + start, length);
                        uValue = (ClUint64T) unsignedLong;
                        break;

                default: return CL_LOGVW_ERROR;
            }
            break;

        default: return CL_LOGVW_ERROR;
    }

    if(CL_LOGVW_TAG_BASIC_SIGNED == tag)
    {
        fprintf(gParser->outPtr, "%lld", sValue);
    }
    else
    {
        fprintf(gParser->outPtr, "%llu", uValue);
    }

    return CL_OK;
}



/*
 * Creates the hash table entry for
 * message id to tlv string mapping

 * Allocates the memory to message id ptr
 * and tlv string ptr
 
 *@param -  tlvFileName
            file for message id to tlv
            string mapping

 *@return - rc
            CL_OK if everything is OK
            otherwise error code
 */
ClRcT
clLogVwCreateTlvMap(ClCharT *tlvFileName)
{
    ClRcT rc = CL_OK;

    FILE *fp = NULL;

    ClUint16T   mapCount = 0;

    ENTRY item = {NULL, NULL};

    ENTRY *retVal = NULL;

#ifdef SOLARIS_BUILD
    hcreate(CL_LOGVW_TLV_HASH_TB_SIZE);
#else
    /* Separate hash tables for component and 
     * stream id mapping.
     */
    gParser->tlvHashTb  = (struct hsearch_data *) 
                          calloc(1, sizeof(struct hsearch_data));

    CL_LOGVW_NO_MEMORY_CHK(gParser->tlvHashTb);

    hcreate_r(CL_LOGVW_TLV_HASH_TB_SIZE, gParser->tlvHashTb);

    if( NULL == (fp = fopen(tlvFileName, "r")) )
    {
        fprintf(stdout, "Unable to open tlvFile.");
        return CL_LOGVW_ERROR;
    }
#endif

    gParser->tlvHash = (ClLogVwTlvHashMapT*) malloc(sizeof(ClLogVwTlvHashMapT));

    CL_LOGVW_NO_MEM_CHK_WITH_STMT(gParser->tlvHash, fclose(fp));

    gParser->tlvHash->msgIdPtr   =   NULL;
    gParser->tlvHash->stringPtr  =   NULL;

    gParser->tlvHash->msgIdPtr = (ClCharT*) calloc(1, CL_LOGVW_TLV_MAX_COUNT *
            CL_LOGVW_TLV_MAX_MSG_ID_LEN);

    CL_LOGVW_NO_MEM_CHK_WITH_STMT(gParser->tlvHash->msgIdPtr, fclose(fp));

    gParser->tlvHash->stringPtr = (ClCharT*) calloc(1, CL_LOGVW_TLV_MAX_COUNT *
            CL_LOGVW_TLV_MAX_STRING_LEN);

    CL_LOGVW_NO_MEM_CHK_WITH_STMT(gParser->tlvHash->stringPtr, fclose(fp));

    ClCharT *tlvStringPtr = gParser->tlvHash->stringPtr;
    ClCharT *keyPtr = gParser->tlvHash->msgIdPtr;

    mapCount = 0;

    while( EOF != fscanf(fp, "%s", keyPtr) && 
            mapCount < CL_LOGVW_TLV_MAX_COUNT )
    {
        if(atol(keyPtr) < 2 )
        {
            fprintf(stdout, "Message id specified in Tlv file is not valid.\n");
            rc = CL_LOGVW_ERROR;
            goto failure;
        }

        item.key = keyPtr;
        
        if(!fgets(tlvStringPtr, CL_LOGVW_TLV_MAX_STRING_LEN, fp)) tlvStringPtr[0] = 0;

        if( 5 >= strlen(tlvStringPtr) )
        {
            
            fprintf(stdout, "[TLV string for message id '%ld' is not proper.]\n", atol(keyPtr));
            fprintf(stdout, "[Probable cause : 1) No %%TLV found in TLV string"
                " 2) No space between message id and TLV string]\n");
            rc = CL_LOGVW_ERROR;
            goto failure;
        }
        tlvStringPtr[strlen(tlvStringPtr) - 1] = '\0'; //Removing '\n' from the end

        item.data = ++tlvStringPtr;
        
#ifdef SOLARIS_BUILD
    retVal = hsearch(item, FIND);
#else
        if( 0 == hsearch_r(item, ENTER, &retVal, gParser->tlvHashTb))
        {
            fprintf(stdout, "Error while making an entry in hash table.\n");
            rc = CL_LOGVW_ERROR;
            goto failure;
        }
#endif

        if(strlen(keyPtr) >= CL_LOGVW_TLV_MAX_MSG_ID_LEN)
        {
            fprintf(stdout, "\nMessage Id is too long in tlv file.\n");
            rc = -1;
            goto failure;
        }
        keyPtr += strlen(keyPtr) + 1;

        if(strlen(tlvStringPtr) >= CL_LOGVW_TLV_MAX_STRING_LEN)
        {
            fprintf(stdout, "\nTLV message is too long in tlv file.\n");
            rc = CL_LOGVW_ERROR;
            goto failure;
        }
        tlvStringPtr += strlen(tlvStringPtr) + 1;

        mapCount++;
    }

    fclose(fp);

    gParser->isTlvFilePresent = CL_TRUE;

    return CL_OK;

failure:
    fclose(fp);

    return rc;
}



/*
 * Frees up the allocated memory
 * for tlv string mapping

 *@return - rc
            CL_OK if everything is OK
            otherwise error code
 */
ClRcT
clLogVwDestroyTlvMap(void)
{
    if( NULL != gParser &&
            NULL != gParser->tlvHash)
    {
#ifdef SOLARIS_BUILD
    hdestroy();
#else
        hdestroy_r(gParser->tlvHashTb);
#endif
        free(gParser->tlvHash->msgIdPtr);
        free(gParser->tlvHash->stringPtr);
        free(gParser->tlvHash);
    }
    return CL_OK;
}

