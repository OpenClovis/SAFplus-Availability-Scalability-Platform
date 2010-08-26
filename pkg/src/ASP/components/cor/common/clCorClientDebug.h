/*
 * Copyright (C) 2002-2009 by OpenClovis Inc. All  Rights Reserved.
 * 
 * The source code for  this program is not published  or otherwise 
 * divested of  its trade secrets, irrespective  of  what  has been 
 * deposited with the U.S. Copyright office
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


#ifndef _CL_COR_CLIENT_DEBUG_H_
#define _CL_COR_CLIENT_DEBUG_H_

#ifdef __cplusplus
#define 
extern C
{
#endif

#include <clCorClient.h>

    
/**
 * The type definition depicting the cor client CLI options.
 */
typedef enum {
   CL_COR_CLIENT_CLI_INVALID_OP,
   CL_COR_CLIENT_CLI_CREATE_AND_SET_OP,
   CL_COR_CLIENT_CLI_ATTR_SET_OP,
   CL_COR_CLIENT_CLI_ATTR_TYPE_GET_OP
}ClCorClientCliOpT;


/**
 * Structure to carry the attribute list information.
 */ 
struct _ClCorAttrInfo {
    /* ClCorAttrPathT  attrPath; */ /* Not using attribute path as of now */
    ClCorAttrIdT attrId;
    ClInt32T    index ;
    ClCorAttrTypeT attrType;
    ClCorTypeT   corType;
    ClUint32T    size;
    ClPtrT       pValue;
};

typedef struct _ClCorAttrInfo _ClCorAttrInfoT;
typedef _ClCorAttrInfoT * _ClCorAttrInfoPtrT;
/**
 * Structure to carry the information between COR server and client.
 */ 
struct _ClCorCtAndSetInfo
{
    ClCorMOIdT moId;
    ClUint32T num;
    _ClCorAttrInfoT *pAttrInfo;
};

typedef struct _ClCorCtAndSetInfo ClCorCtAndSetInfoT;
typedef ClCorCtAndSetInfoT * ClCorCtAndSetInfoPtrT;


/**
 * Function to marshall the cor client debug cli information.
 */ 
extern ClRcT _clCorClientDebugDataMarshall ( ClCorClientCliOpT cliOp, 
                                      ClUint32T argc, 
                                      ClCharT *argv[], 
                                      ClBufferHandleT inMsgH );

/**
 *  Function to unmarshall the COR client debug cli information.
 */ 
extern ClRcT _clCorClientDebugDataUnMarshall ( ClBufferHandleT inMsgH, 
                                        ClCorClientCliOpT *pCliOp, 
                                        ClUint32T *pArgc, 
                                        ClCharT ***pArgv );

/**
 * Function to unmarshall the create and set structure.
 */ 
extern ClRcT
clCorClntDbgCtNStUnmarshall(ClBufferHandleT msgH, ClCorCtAndSetInfoPtrT pAttrList);



/**
 * Function to marshall the create and set structure.
 */ 
extern ClRcT
clCorClntDbgCtNStMarshall(ClCorCtAndSetInfoPtrT pAttrList, ClBufferHandleT msgH);


/**
 * Function to finalize the Attribute list.
 */ 
void clCorClntDbgAttrListFinalize(ClCorCtAndSetInfoPtrT pAttrList);

#ifdef __cplusplus
}
#endif

#endif
