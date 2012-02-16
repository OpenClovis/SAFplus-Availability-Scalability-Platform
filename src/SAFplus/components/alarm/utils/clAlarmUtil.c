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
#include <string.h>
#include <clDebugApi.h>
#include <clCorUtilityApi.h>

#include <xdrClAlarmUtilPayLoadListT.h>
#include <clAlarmUtils.h>
#include <xdrClAlarmHandleInfoIDLT.h>
#include <clAlarmUtil.h>

/*
 *  This API takes payload as input and convert it in a flat buffer format.
 *  parameters:
 *     pPayLoadList (IN): payload that will be passed in alarm as additional data.
 *     pSize (OUT)      : size of the returned buffer
 *     ppBuf (OUT)      : output buffer
 */
ClRcT clAlarmUtilPayloadFlatten(CL_IN ClAlarmUtilPayLoadListPtrT pPayLoadList, 
	CL_OUT ClUint32T *pSize, CL_OUT ClUint8T  **ppBuf)
{
    ClRcT rc = CL_OK;
    ClBufferHandleT msgHandle;

    if(NULL == pPayLoadList)
    {
	CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("payLoadList is passed as NULL.\n") );
	return CL_ERR_NULL_POINTER;
    }
    if(NULL == pSize)
    {
	CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("payLoadList is passed as NULL.\n") );
	return CL_ERR_NULL_POINTER;
    }
    if(NULL == ppBuf)
    {
	CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("(*buf) is passed as NULL.\n") );
	return CL_ERR_NULL_POINTER;
    }
    rc = clBufferCreate(&msgHandle);
    if (rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Could not create buffer for alarm utility payload message. rc = [0x%x]\n",rc));
        return rc;
    }
    rc = clXdrMarshallClAlarmUtilPayLoadListT( (void *)pPayLoadList, msgHandle, 0);
    if (rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Could not marshal the alarm payload data. rc = [0x%x]\n",rc));
        clBufferDelete(&msgHandle);
        return rc;
    }
    rc = clBufferFlatten(msgHandle, ppBuf);
    if (CL_OK != rc)
    {
	clBufferDelete(&msgHandle);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("clBufferFlatten failed with rc = [0x%x]\n",rc));
	return rc;
    }
    rc = clBufferLengthGet(msgHandle, pSize);
    if (CL_OK != rc)
    {
	clBufferDelete(&msgHandle);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("clBufferLengthGet failed with rc = [0x%x]\n",rc));
	return rc;
    }
    clBufferDelete(&msgHandle);
    return rc;
}

/*
 *  This API takes byte stream buffer as input and convert it in a ClAlarmUtilPayLoadListT structure.
 *  parameters:
 *     pBuf (IN)        : byte stream buffer
 *     size (IN)         : size of the byte stream buffer
 *     pPayLoadList (OUT): payload passed in alarm as additional data.
 */
ClRcT clAlarmUtilPayLoadExtract(CL_IN ClUint8T  *pBuf, CL_IN ClUint32T size, 
	CL_OUT ClAlarmUtilPayLoadListT **ppPayloadList)
{
    ClRcT rc = CL_OK;
    ClBufferHandleT msgHandle;

    if(NULL == pBuf)
    {
	CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Buffer is passed as NULL.\n") );
	return CL_ERR_NULL_POINTER;
    }
    rc = clBufferCreate(&msgHandle);
    if (rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Could not create buffer for alarm utility payload message. rc = [0x%x]\n",rc));
        return rc;
    }
    rc = clBufferNBytesWrite(msgHandle, pBuf, size);
    if (CL_OK != rc)
    {
	clBufferDelete(&msgHandle);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("clBufferNBytesRead failed with rc = [0x%x]\n",rc));
	return rc;
    }
    *ppPayloadList = (ClAlarmUtilPayLoadListPtrT) clHeapAllocate(sizeof(ClAlarmUtilPayLoadListT));
    if(NULL == *ppPayloadList)
    {
	clBufferDelete(&msgHandle);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Memory allocation for *ppPayloadList failed. rc = [0x%x]\n",rc));
	return rc;
    }
    rc = clXdrUnmarshallClAlarmUtilPayLoadListT(msgHandle, *ppPayloadList);
    if (CL_OK != rc)
    {
	clBufferDelete(&msgHandle);
	clHeapFree(*ppPayloadList);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("clBufferFlatten failed with rc = [0x%x]\n",rc));
	return rc;
    }
    clBufferDelete(&msgHandle);
    return rc;
}

/*
 * Free the memory allocated for the buffer.
 */
void clAlarmUtilPayloadBufFree(CL_IN ClUint8T  *pBuf)
{
    if(NULL != pBuf)
	clHeapFree(pBuf);
}

/*
 * Free the memory allocated for the pPayloadList.
 */
void clAlarmUtilPayloadListFree(CL_IN ClAlarmUtilPayLoadListT   *pPayloadList)
{
    ClUint32T payLoadcount = 0, tlvCount = 0;
    if((NULL != pPayloadList) && (NULL != pPayloadList->pPayload))
    {
	    for(payLoadcount = 0; payLoadcount < pPayloadList->numPayLoadEnteries; payLoadcount++)
	    {
            if ( (pPayloadList->pPayload + payLoadcount == NULL))
                continue;
	        clCorMoIdFree(pPayloadList->pPayload[payLoadcount].pMoId);
	        for(tlvCount = 0; tlvCount < pPayloadList->pPayload[payLoadcount].numTlvs; tlvCount++ )
	        {
		        if(NULL != pPayloadList->pPayload[payLoadcount].pTlv[tlvCount].value)
		            clHeapFree(pPayloadList->pPayload[payLoadcount].pTlv[tlvCount].value);
	        }
	        clHeapFree(pPayloadList->pPayload[payLoadcount].pTlv);
	    }
    }

    if (NULL != pPayloadList)
        clHeapFree(pPayloadList->pPayload);
    clHeapFree(pPayloadList);
}


ClRcT
clAlarmEventDataGet(ClUint8T *pData, ClSizeT size, ClAlarmHandleInfoT *pAlarmHandleInfo)
{
    ClRcT rc = CL_OK;
    ClBufferHandleT   msgHdl;
    ClAlarmHandleInfoIDLT alarmHandleInfoIdl = {0};

    if(pData == NULL || pAlarmHandleInfo == NULL)
        return CL_ERR_NULL_POINTER;


    if((rc = clBufferCreate(&msgHdl)) != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\n Buffer Message Creation Failed "));
        return (rc);
    }
    if((rc = clBufferNBytesWrite(msgHdl, (ClUint8T *)pData, size))!= CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ( "\n Buffer Message Write Failed with rc : %0x\n",rc));
        clBufferDelete(&msgHdl);
        return (rc);
    }

    rc = VDECL_VER(clXdrUnmarshallClAlarmHandleInfoIDLT, 4, 0, 0)(msgHdl,(void *)&alarmHandleInfoIdl);
    if (CL_OK != rc)
    {
        clBufferDelete(&msgHdl);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Could not write into Buffer\n"));
        return rc;
    }
    CL_DEBUG_PRINT(CL_DEBUG_TRACE,("alarmHandleInfoIdl.alarmInfo.len :%d, \
                alarmHandleInfoIdl.alarmInfo.buff : %s\n",
                (alarmHandleInfoIdl.alarmInfo).len,alarmHandleInfoIdl.alarmInfo.buff));
    pAlarmHandleInfo->alarmHandle = alarmHandleInfoIdl.alarmHandle;
    pAlarmHandleInfo->alarmInfo.probCause = alarmHandleInfoIdl.alarmInfo.probCause;
    pAlarmHandleInfo->alarmInfo.compName.length = (alarmHandleInfoIdl.alarmInfo.compName).length;
    memcpy(pAlarmHandleInfo->alarmInfo.compName.value,(alarmHandleInfoIdl.alarmInfo.compName).value,pAlarmHandleInfo->alarmInfo.compName.length);
    pAlarmHandleInfo->alarmInfo.moId = (alarmHandleInfoIdl.alarmInfo).moId;
    pAlarmHandleInfo->alarmInfo.alarmState = (alarmHandleInfoIdl.alarmInfo).alarmState;
    pAlarmHandleInfo->alarmInfo.category = (alarmHandleInfoIdl.alarmInfo).category;
    pAlarmHandleInfo->alarmInfo.specificProblem = (alarmHandleInfoIdl.alarmInfo).specificProblem;
    pAlarmHandleInfo->alarmInfo.severity = (alarmHandleInfoIdl.alarmInfo).severity;
    pAlarmHandleInfo->alarmInfo.eventTime = (alarmHandleInfoIdl.alarmInfo).eventTime;
    pAlarmHandleInfo->alarmInfo.len = (alarmHandleInfoIdl.alarmInfo).len;
    memcpy(pAlarmHandleInfo->alarmInfo.buff,alarmHandleInfoIdl.alarmInfo.buff,pAlarmHandleInfo->alarmInfo.len);


	clBufferDelete(&msgHdl);
    clHeapFree(alarmHandleInfoIdl.alarmInfo.buff);
    return rc;
}

