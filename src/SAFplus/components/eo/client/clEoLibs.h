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
 * ModuleName  : eo
File        : clEoLibs.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *      This header contains the definitions used by the by various 
 *      library related functionality.
 *      
 *
 *****************************************************************************/


#ifndef _CL_EO_LIBS_H_
#define _CL_EO_LIBS_H_

#ifdef __cplusplus
extern "C" {
#endif

    /* 
     * Includes 
     */
#include <clEoApi.h>
#include <clEoIpi.h>
#include <clLogApi.h>
#include <clDebugApi.h>
#include <clIocParseConfig.h>
#include <clEoConfigApi.h>

    /* 
     * Defines 
     */

#define CL_EOLIB_MAX_LOG_MSG  (500)
#define __CL_EO_LIB_FUNC(type,fun) ((type)(fun))
#define CL_EO_LIB_INITIALIZE_FUNC(fun) __CL_EO_LIB_FUNC(ClEoEssentialLibInitializeFuncT,fun)
#define CL_EO_LIB_FINALIZE_FUNC(fun) __CL_EO_LIB_FUNC(ClEoEssentialLibFinalizeFuncT,fun)

    /* 
     * TypeDefs 
     */


    typedef ClRcT (*ClEoEssentialLibInitializeFuncT) (const ClPtrT pConfig);
    typedef ClRcT (*ClEoEssentialLibFinalizeFuncT) (void);

    typedef struct ClEoEssentialLibInfo {
        ClCharT                         libName[CL_EO_MAX_NAME_LEN];
        ClEoEssentialLibInitializeFuncT pLibInitializeFunc;
        ClEoEssentialLibFinalizeFuncT   pLibFinalizeFunc;
        ClPtrT                          pConfig;
        ClUint32T                       noOfWaterMarks;
        ClEoActionInfoT*                pActionInfo;

#define LIB_NAME(_libId) gEssentialLibInfo[(_libId)].libName
#define LIB_ACTION(_libId) gEssentialLibInfo[(_libId)].pActionInfo
    } ClEoEssentialLibInfoT;

#define isBitMapSet(_libId, _wmId, _MASK)                               \
    (((NULL != LIB_ACTION(_libId)) && (LIB_ACTION(_libId)[(_wmId)].bitMap & (_MASK)))? CL_TRUE : CL_FALSE)

#define isCustomSet(_libId, _wmId)  isBitMapSet(_libId, _wmId, CL_EO_ACTION_CUSTOM)
#define isEventSet(_libId, _wmId)  isBitMapSet(_libId, _wmId, CL_EO_ACTION_EVENT)
#define isLogSet(_libId, _wmId)  isBitMapSet(_libId, _wmId, CL_EO_ACTION_LOG)
#define isNotificationSet(_libId, _wmId)  isBitMapSet(_libId, _wmId, CL_EO_ACTION_NOT)

    __inline__ static ClEoLibIdT eoCompId2LibId(ClInt32T compId)
    {
        switch(compId)
        {
        case CL_CID_OSAL:
            return CL_EO_LIB_ID_OSAL;

        case CL_CID_MEM:
            return CL_EO_LIB_ID_MEM;

        case CL_CID_HEAP:
            return CL_EO_LIB_ID_HEAP;

        case CL_CID_BUFFER:
            return CL_EO_LIB_ID_BUFFER;

        case CL_CID_POOL:
            return CL_EO_LIB_ID_POOL;

        case CL_CID_CPM:
            return CL_EO_LIB_ID_CPM;
            
        case CL_CID_IOC:
            return CL_EO_LIB_ID_IOC;

        default:
            CL_ASSERT(0);
            return CL_EO_LIB_ID_MAX;
        }
    }

    __inline__ static ClRcT eoLibIdValidate(ClEoLibIdT libId)
    {
        return (libId >= CL_EO_LIB_ID_OSAL && libId < CL_EO_LIB_ID_MAX)? CL_OK : CL_EO_RC(CL_EO_ERR_LIB_ID_INVALID);
    }

    __inline__ static ClRcT eoWaterMarkIdValidate(ClEoLibIdT libId, ClWaterMarkIdT wmId)
    {
        switch(libId)
        {
        case CL_EO_LIB_ID_MEM:
        default:
            return (wmId >= CL_WM_LOW && wmId < CL_WM_MAX)? CL_OK : CL_EO_RC(CL_EO_ERR_WATER_MARK_ID_INVALID); 
        }

        return CL_OK;
    }

    extern ClBoolT clEoLibInitialized;

    /*
     * Check if EO library has been initialized. If not return
     * ok for now. FIXME
     */
#define CL_EO_LIB_VERIFY() do { \
    if(CL_TRUE != clEoLibInitialized) \
    { \
        clLogTrace(CL_LOG_AREA_UNSPECIFIED,CL_LOG_CONTEXT_UNSPECIFIED,"The EO Library not initialized\n"); \
        return CL_OK; \
    } \
} while(0) 

    
    extern ClEoActionInfoT gMemWaterMark[CL_WM_MAX];
    extern ClEoActionInfoT gIocWaterMark[CL_WM_MAX];
    
    extern ClBoolT gIsNodeRepresentative;

    extern ClEoEssentialLibInfoT gEssentialLibInfo[];

    extern ClEoActionInfoT gClIocRecvQActions;

#if 0 /* DEPRECATED 3.1 */    
    extern ClIocConfigT *gpClEoIocConfig;
#endif
    
    extern ClRcT clEoQueueWaterMarkInfo(ClEoLibIdT libId, ClWaterMarkIdT wmId, 
                                        ClWaterMarkT *pWaterMark, ClEoWaterMarkFlagT wmType, ClEoActionArgListT argList);

    extern ClRcT clEoQueueLogInfo(ClEoLibIdT libId,ClLogSeverityT severity, const ClCharT *pMsg);

    extern ClRcT clEoIocCommPortWMNotification(ClCompIdT compId, ClIocPortT port,
                                               ClIocNodeAddressT node,
                                               ClIocNotificationT *pIocNotification);

    extern ClRcT clEoProcessIocRecvPortNotification(ClEoExecutionObjT* pThis, ClBufferHandleT eoRecvMsg, ClUint8T priority, ClUint8T protoType, ClUint32T length, ClIocPhysicalAddressT srcAddr);

#ifdef __cplusplus
}

#endif


#endif /* _CL_EO_LIBS_H_ */
