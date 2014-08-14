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
    CL_CM_BLADE_SURPRISE_EXTRACTION=0,
    CL_CM_BLADE_REQ_EXTRACTION,
    CL_CM_BLADE_REQ_INSERTION,
    CL_CM_BLADE_NODE_ERROR_REPORT,
    CL_CM_BLADE_NODE_ERROR_CLEAR
}ClCmCpmMsgTypeT;

typedef struct 
{
    ClCmCpmMsgTypeT cmCpmMsgType;
    ClUint32T physicalSlot ;
    /*For future Use */
    ClUint32T subSlot ;
    /* Not to be used by CPM , but needed for CM */
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

/* CM IPI */
extern ClRcT clCmCpmResponseHandle(ClCpmCmMsgT * pClCpmCmMsg);
#ifdef __cplusplus
}
#endif

#endif /* _CL_CM_IPI_H_ */
