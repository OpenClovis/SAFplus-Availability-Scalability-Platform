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
 * ModuleName  : include
 * File        : clCmIpi.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This file contains the interfaces for CM used internally by other ASP
 * componets.  
 *
 *
 *****************************************************************************/

#ifndef _CL_CM_IPI_H_
#define _CL_CM_IPI_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <clCommon.h>

/******************************************************************************
 *  Constant and Macro Definitions
 *****************************************************************************/

/******************************************************************************
 *  Data Types 
 *****************************************************************************/
typedef enum{
    CL_CM_BLADE_SURPRISE_EXTRACTION=0,  /** Node was pulled */
    CL_CM_BLADE_REQ_EXTRACTION,         /** Node's latch was popped or ACPI button pushed */
    CL_CM_BLADE_REQ_INSERTION,          /** Node's latch wad closed */
    CL_CM_BLADE_NODE_ERROR_REPORT,      /** A sensor reported a severe error -- fail AIS services to the redundant */
    CL_CM_BLADE_NODE_ERROR_CLEAR        /** The severe sensor error was cleared */
}ClCmCpmMsgTypeT;

typedef struct 
{
    /* The event type */
    ClCmCpmMsgTypeT cmCpmMsgType;
    /** The slot number (node identifier) with the problem */
    ClUint32T physicalSlot;
    /** For future Use */
    ClUint32T subSlot;
    /** Not to be used by AMF, can be used by the hardware manager to identify the problem */
    ClUint32T resourceId;
}ClCmCpmMsgT;

/**
 * Type of messages from from CPM to CM.
 */
typedef enum{
    CL_CPM_CANCEL_USER_ACTION=0,
    CL_CPM_ALLOW_USER_ACTION
}ClCpmCmMsgTypeT;


/**
 * Messages Response from from CPMG to CM.
 */
typedef struct{

/**
 *  Request Message to CPMG.
 */
    ClCmCpmMsgT cmCpmMsg;

/**
 * Response Msg.
 */
    ClCpmCmMsgTypeT cpmCmMsgType;
}ClCpmCmMsgT;


/*****************************************************************************
 *  Functions
 *****************************************************************************/
/* CPM IPI */
extern ClRcT clCpmHotSwapEventHandle(ClCmCpmMsgT *pCmCpmMsg);
    
#ifdef __cplusplus
}
#endif

#endif /* _CL_CM_IPI_H_ */
