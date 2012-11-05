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
 * ModuleName  : event                                                         
 * File        : clEvtUtils.c
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 *          This module provides the utility functions used in EM.
 *****************************************************************************/

#include "clTimerApi.h"
#include "clOsalApi.h"

#include "clEventApi.h"
#include "clEventExtApi.h"
#include "clEventUtilsIpi.h"

#ifdef MORE_CODE_COVERAGE
# include "clCodeCovStub.h"
#endif




ClInt32T clEvtUtilsNameCmp(ClNameT *pName1, ClNameT *pName2)
{
    ClUint32T result;

    result = pName1->length - pName2->length;

    CL_FUNC_ENTER();

    if (0 == result)
    {
        result = memcmp(pName1->value, pName2->value, pName1->length);
        CL_FUNC_EXIT();
        return result;
    }
    CL_FUNC_EXIT();
    return result;
}

void clEvtUtilsNameCpy(ClNameT *pNameDst, const ClNameT *pNameSrc)
{

    CL_FUNC_ENTER();

    pNameDst->length = pNameSrc->length;
    memcpy(pNameDst->value, pNameSrc->value, CL_MIN(pNameSrc->length + 1, 
                                                    (ClUint32T)sizeof(pNameDst->value)));
    CL_FUNC_EXIT();
    return;
}


ClUint32T clEvtUtilsIsLittleEndian(void)
{
    ClInt32T i = 1;

    return (*((char *) &i) ? CL_TRUE : CL_FALSE);
}

ClRcT clEvtUtilsFilter2Rbe(const ClEventFilterArrayT *pFilterArray,
                           ClRuleExprT **pRbeExpr)
{
    ClRcT rc = CL_OK;

    ClSizeT noOfFilters = 0;
    ClSizeT patternSize = 0;

    ClUint32T i = 0;
    ClUint16T len = 0;
    ClUint16T offset = 0;
    ClUint16T extraOffset = 0;
    ClUint32T remainder = 0;
    ClUint32T mask = 0xffffffff;
    ClUint32T *pPatternWord = NULL;

    ClEventFilterT *pFilters = NULL;
    ClEventPatternT *pFilterPattern = NULL;
    ClRuleExprT **rbeExprArray = NULL;


    CL_FUNC_ENTER();


    if (NULL == pFilterArray)
    {
        *pRbeExpr = NULL;
        CL_FUNC_EXIT();
        return CL_OK;
    }

    noOfFilters = pFilterArray->filtersNumber;
    rbeExprArray = clHeapAllocate(sizeof(ClRuleExprT *) * noOfFilters);

    for (i = 0; i < noOfFilters;
         i++, extraOffset += len + (0 == remainder ? 0 : 1))
    {
        pFilters = &(pFilterArray->pFilters[i]);
        pFilterPattern = &pFilters->filter;
        patternSize = pFilterPattern->patternSize;
        pPatternWord = (ClUint32T *) pFilterPattern->pPattern;

        /*
         * If Pass All simply mark the expression as NULL for it returns true
         * always exactly as desire 
         */
        if (CL_EVENT_PASS_ALL_FILTER & pFilters->filterType)
        {
            rbeExprArray[i] = NULL;
            continue;
        }
        if (!pPatternWord || !patternSize)
        {
            rbeExprArray[i] = NULL;
            continue;
        }

        len = patternSize / 4;
        remainder = patternSize % 4;

        /*
         * Allocate the appropriate amount of memory 
         */
        rc = clRuleExprAllocate(len + (0 == remainder ? 0 : 1),
                                &rbeExprArray[i]);
        rc = clRuleExprOffsetSet(rbeExprArray[i], extraOffset);

        /*
         * Fillin corresponding flag in RBE expres sion 
         */
        switch ((ClInt32T)pFilters->filterType)
        {
            case CL_EVENT_EXACT_FILTER:
                rc = clRuleExprFlagsSet(rbeExprArray[i],
                                        CL_RULE_MATCH_EXACT |
                                        CL_RULE_EXPR_CHAIN_AND);
                mask = 0xffffffff;  /* Initialize mask appropriately */
                break;

            case CL_EVENT_NON_ZERO_MATCH:
                rc = clRuleExprFlagsSet(rbeExprArray[i],
                                        CL_RULE_NON_ZERO_MATCH);
                mask = 0xffffffff;  /* Initialize mask appropriately */
                break;

            default:
                break;
        }

        for (offset = 0; offset < len; offset++)
        {
            rc = clRuleExprMaskSet(rbeExprArray[i], offset + extraOffset, mask);
            rc = clRuleExprValueSet(rbeExprArray[i], offset + extraOffset,
                                    mask & (*pPatternWord++));
        }

        if (0 != remainder)
        {
            /*
             * Alter the mask depending on remainder & endianness 
             */
            mask >>= (4 - remainder) * 8;   /* Clear (4-remainder) bytes */

            if (CL_TRUE != clEvtUtilsIsLittleEndian())
            {
                mask = CL_RULE_SWAP32(mask);
            }

            rc = clRuleExprMaskSet(rbeExprArray[i], offset + extraOffset, mask);
            rc = clRuleExprValueSet(rbeExprArray[i], offset + extraOffset,
                                    mask & (*pPatternWord));
        }

        if (0 != i)
        {
            rc = clRuleExprAppend(rbeExprArray[0], rbeExprArray[i]);
        }
    }

    *pRbeExpr = rbeExprArray[0];
    clHeapFree(rbeExprArray);
    CL_FUNC_EXIT();
    return CL_OK;
}

void clEvtUtilsChannelKeyConstruct(ClUint32T channelHandle,
                                   ClNameT *pChannelName,
                                   ClEvtChannelKeyT *pEvtChannelKey)
{

    CL_FUNC_ENTER();


    CL_EVT_CHANNEL_ID_GET(channelHandle, pEvtChannelKey->channelId);
    clEvtUtilsNameCpy(&pEvtChannelKey->channelName, pChannelName);

    CL_FUNC_EXIT();
    return;
}

ClRcT clEvtUtilsFlatPattern2FlatBuffer(void *pData, ClUint32T noOfPattern,
                                       ClUint8T **ppData, ClUint32T *pDataLen)
{
    ClRcT rc = CL_OK;

    ClUint32T i = 0;
    ClUint32T len = 0;
    ClUint8T noOfPaddingBytes = 0;
    ClUint8T remainder = 0;

    ClSizeT patternSize = 0;
    ClUint8T *pBuff = (ClUint8T *) pData;   /* Place holder for Info */

    ClBufferHandleT messageHandle = 0;


    CL_FUNC_ENTER();

    /*
     * Added for NULL Pattern Bug 3129 
     */
    if (0 == noOfPattern)
    {
        *ppData = NULL;
        pDataLen = 0;

        CL_FUNC_EXIT();
        return CL_OK;
    }

    rc = clBufferCreate(&messageHandle);

    if (NULL == pDataLen)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_EVENT_LIB_NAME,
                   CL_EVENT_LOG_MSG_1_INTERNAL_ERROR, rc);
         return CL_EVENT_ERR_INTERNAL;
    }

    for (i = 0; i < noOfPattern; i++)
    {
        memcpy(&patternSize, pBuff, sizeof(patternSize));

        rc = clBufferNBytesWrite(messageHandle,
                                        (pBuff + sizeof(ClSizeT)),
                                        patternSize);

        /*
         * Pad the remaining bytes with 0s 
         */
        remainder = (patternSize % 4);
        noOfPaddingBytes = 4 - remainder;
        if (0 != remainder)
        {
            ClUint32T zero = 0;

            rc = clBufferNBytesWrite(messageHandle, (ClUint8T *) &zero,
                                            noOfPaddingBytes);
        }

        pBuff += sizeof(ClSizeT) + patternSize;
        len += patternSize + noOfPaddingBytes;
    }

    *ppData = clHeapAllocate(len);
    if (NULL == *ppData)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_EVENT_LIB_NAME,
                   CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
        CL_FUNC_EXIT();
        return CL_EVENT_ERR_NO_MEM;
    }

    *pDataLen = len;
    rc = clBufferNBytesRead(messageHandle, (ClUint8T *) *ppData, &len);

    rc = clBufferDelete(&messageHandle);
    CL_FUNC_EXIT();

    return CL_OK;
}

ClRcT clEvtUtilsFlatPattern2Rbe(void *pData, ClUint32T noOfPattern,
                                ClRuleExprT **pRbeExpr)
{
    ClRcT rc = CL_OK;

    ClUint32T i = 0;
    ClUint32T mask = 0xffffffff;
    ClUint16T offset = 0;
    ClUint16T len = 0;
    ClUint16T remainder = 0;

    ClUint32T *pPattern = NULL; /* Treat the Pattern as an array of words(4
                                 * bytes each) */
    ClSizeT patternSize = 0;
    ClUint8T *pBuff = (ClUint8T *) pData;   /* Place holder for Info */

    ClRuleExprT **rbeExprArray = NULL;


    CL_FUNC_ENTER();

    /*
     * Added for NULL Pattern Bug 3129 
     */
    if (0 == noOfPattern)
    {
        *pRbeExpr = NULL;

        CL_FUNC_EXIT();
        return CL_OK;
    }

    rbeExprArray = clHeapAllocate(sizeof(ClRuleExprT *) * noOfPattern);

    for (i = 0; i < noOfPattern; i++)
    {
        memcpy(&patternSize, pBuff, sizeof(patternSize));

        pPattern = (ClUint32T *) (pBuff + patternSize);

        len = patternSize / 4;
        remainder = patternSize % 4;

        /*
         * Allocate the appropriate amount of memory 
         */
        rc = clRuleExprAllocate(len + (0 == remainder ? 0 : 1),
                                &rbeExprArray[i]);

        mask = 0xffffffff;

        for (offset = 0; offset < len; offset++)
        {
            rc = clRuleExprValueSet(rbeExprArray[i], offset, *pPattern++);
            rc = clRuleExprMaskSet(rbeExprArray[i], offset, mask);
        }

        if (remainder > 0)
        {
            /*
             * Update the Pattern with Padding 
             */
            ClUint32T tmpWord = 0;

            memcpy(&tmpWord, pPattern, remainder);

            rc = clRuleExprValueSet(rbeExprArray[i], offset, tmpWord);


            /*
             * Alter the mask depending on remainder & endianness 
             */
            mask >>= (4 - remainder) * 8;   /* Clear (4-remainder) bytes */

            /*
             * If it is BIG endian need to SWAP the mask 
             */
            if (CL_TRUE != clEvtUtilsIsLittleEndian())
            {
                mask = CL_RULE_SWAP32(mask);
            }

            rc = clRuleExprMaskSet(rbeExprArray[i], offset, mask);
        }

        pBuff += sizeof(ClSizeT) + patternSize;   /* Update the pointer */
    }

    *pRbeExpr = rbeExprArray[0];
    clHeapFree(rbeExprArray);

    CL_FUNC_EXIT();
    return CL_OK;
}
