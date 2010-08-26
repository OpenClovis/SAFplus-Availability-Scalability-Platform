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
 * ModuleName  : amf
 * File        : clAmsDBPackUnpack.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This header file contains definitions related to packing the AMS DB into XML.
 *
 ***************************** Editor Commands ********************************
 * For vi/vim
 * :set shiftwidth=4
 * :set softtabstop=4
 * :set expandtab
 *****************************************************************************/

#ifndef _CL_AMS_DB_PACK_UNPACK_H_
#define _CL_AMS_DB_PACK_UNPACK_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <clParserApi.h>
#include <clCommon.h>
#include <clAms.h>
#include <clAmsPolicyEngine.h>
#include <clAmsInvocation.h>

typedef struct 
{
    ClParserPtrT    headPtr;
    ClParserPtrT    gAmsPtr;
    ClParserPtrT    nodeNamesPtr;
    ClParserPtrT    appNamesPtr;
    ClParserPtrT    sgNamesPtr;
    ClParserPtrT    suNamesPtr;
    ClParserPtrT    siNamesPtr;
    ClParserPtrT    compNamesPtr;
    ClParserPtrT    csiNamesPtr;
}ClAmsParserPtrT;

extern ClRcT   
clAmsDBPackUnpackMain(
        ClAmsT *ams);

extern ClRcT
clAmsDBNodeMarshall(ClAmsNodeT *node,
                    ClBufferHandleT inMsgHdl,
                    ClUint32T marshallMask,
                    ClUint32T versionCode);

extern ClRcT   
clAmsDBNodeXMLize(
       CL_IN  ClAmsNodeT  *node);

extern ClRcT
clAmsDBNodeDeXMLize(
       CL_IN  ClParserPtrT  nodePtr);

extern ClRcT   
clAmsDBClUint8ToStr(
        CL_IN  ClUint8T  number,
        CL_OUT  ClCharT  **str );

extern ClRcT   
clAmsDBClUint32ToStr(
        CL_IN  ClUint32T  number,
        CL_OUT  ClCharT  **str );

extern ClRcT   
clAmsDBClInt64ToStr(
        CL_IN  ClInt64T  number,
        CL_OUT  ClCharT  **str );

extern ClRcT
clAmsDBListMarshall(ClAmsEntityListT *list,
                    ClAmsEntityListTypeT listType,
                    ClBufferHandleT inMsgHdl,
                    ClUint32T versionCode);

extern ClRcT
clAmsDBListUnmarshall(ClAmsEntityT *entity,
                      ClBufferHandleT inMsgHdl,
                      ClUint32T versionCode);

extern ClRcT   
clAmsDBListToXML(
       CL_IN  ClParserPtrT  tagPtr,
       CL_IN  ClAmsEntityListT  *list,
       CL_IN  ClAmsEntityListTypeT  listType );

extern ClRcT
clAmsDBReadEntityTimer(
        CL_IN  ClParserPtrT  tagPtr,
        CL_OUT  ClAmsEntityTimerT  *entityTimer,
        CL_IN  ClCharT  *timerTag );

extern ClRcT
clAmsDBTimerMarshall(ClAmsEntityTimerT *entityTimer,
                     ClAmsEntityTimerTypeT timerType,
                     ClBufferHandleT inMsgHdl,
                     ClUint32T versionCode);

extern ClRcT
clAmsDBTimerUnmarshall(ClAmsEntityT *entity,
                       ClBufferHandleT inMsgHdl,
                       ClUint32T versionCode);

extern ClRcT
clAmsDBWriteEntityTimer(
        CL_IN  ClParserPtrT  tagPtr,
        CL_IN  ClAmsEntityTimerT  *entityTimer,
        CL_IN  ClCharT  *timerTag );

extern ClRcT clAmsDBMarshall(ClAmsDbT *db, ClBufferHandleT inMsgHdl);

extern ClRcT clAmsDBUnmarshall(ClBufferHandleT inMsgHdl);

extern ClRcT clAmsServerDataMarshall(ClAmsT *ams, ClBufferHandleT inMsgHdl, ClUint32T versionCode);
    
extern ClRcT clAmsServerDataUnmarshall(ClAmsT *ams, ClBufferHandleT inMsgHdl, ClUint32T versionCode);

extern ClRcT clAmsEntityUserDataMarshall(ClBufferHandleT inMsgHdl, ClUint32T versionCode);

extern ClRcT clAmsEntityUserDataUnmarshall(ClBufferHandleT inMsgHdl, ClUint32T versionCode);

extern ClRcT clAmsDBConfigSerialize(ClAmsDbT *amsDB, ClBufferHandleT inMsgHdl);

extern ClRcT clAmsDBConfigDeserialize(ClBufferHandleT inMsgHdl);

extern ClRcT
clAmsDBUnpackMain(
        CL_IN  ClCharT  *fileName);

extern ClRcT
clAmsDBParseEntityNames (
        CL_IN  ClParserPtrT  namesPtr,
        CL_IN  ClRcT  (*fn)  (ClParserPtrT  namePtr) );

extern ClRcT   
clAmsDBNodeListDeXMLize(
        CL_IN  ClParserPtrT  nodePtr);

extern ClRcT   
clAmsDBXMLToList(
       CL_IN  ClParserPtrT  tagPtr,
       CL_IN  const ClCharT  *sourceEntityName,
       CL_IN  ClAmsEntityListTypeT  listType );

extern ClRcT
clAmsDBReadEntityTimer(
        ClParserPtrT        tagPtr,
        ClAmsEntityTimerT   *entityTimer,
        ClCharT                *timerTag );

extern ClRcT
clAmsDBSGMarshall(ClAmsSGT *sg,
                  ClBufferHandleT inMsgHdl, ClUint32T marshallMask, ClUint32T versionCode);

extern ClRcT   
clAmsDBSGXMLize(
       CL_IN  ClAmsSGT  *sg);

extern ClRcT   
clAmsDBSGDeXMLize(
       CL_IN  ClParserPtrT  sgPtr);

extern ClRcT   
clAmsDBSGListDeXMLize(
       CL_IN  ClParserPtrT  sgPtr);

extern ClRcT
clAmsDBSUMarshall(ClAmsSUT *su,
                  ClBufferHandleT inMsgHdl, ClUint32T marshallMask, ClUint32T versionCode);

extern ClRcT
clAmsDBSUXMLize(
       CL_IN  ClAmsSUT  *su);

extern ClRcT   
clAmsDBSUDeXMLize(
       CL_IN  ClParserPtrT  suPtr);

extern ClRcT   
clAmsDBSUListDeXMLize(
       CL_IN  ClParserPtrT  suPtr);

extern ClRcT
clAmsDBCompMarshall(ClAmsCompT *comp,
                    ClBufferHandleT inMsgHdl, ClUint32T marshallMask, ClUint32T versionCode);

extern ClRcT   
clAmsDBCompXMLize(
       CL_IN  ClAmsCompT  *comp);

extern ClRcT   
clAmsDBCompDeXMLize(
       CL_IN  ClParserPtrT  compPtr);

extern ClRcT   
clAmsDBCompListDeXMLize(
       CL_IN  ClParserPtrT  compPtr);

extern ClRcT
clAmsDBSIMarshall(ClAmsSIT *si,
                  ClBufferHandleT inMsgHdl, ClUint32T marshallMask, ClUint32T versionCode);

extern ClRcT   
clAmsDBSIXMLize(
       CL_IN  ClAmsSIT  *si);

extern ClRcT   
clAmsDBSIDeXMLize(
       CL_IN  ClParserPtrT  siPtr);

extern ClRcT   
clAmsDBSIListDeXMLize(
       CL_IN  ClParserPtrT  siPtr);

extern ClRcT
clAmsDBCSIMarshall(ClAmsCSIT *csi,
                   ClBufferHandleT inMsgHdl, ClUint32T marshallMask, ClUint32T versionCode);

extern ClRcT   
clAmsDBCSIXMLize(
       CL_IN  ClAmsCSIT  *csi);

extern ClRcT   
clAmsDBCSIDeXMLize(
       CL_IN  ClParserPtrT  csiPtr);

extern ClRcT   
clAmsDBCSIListDeXMLize(
       CL_IN  ClParserPtrT  csiPtr);

extern ClRcT   
clAmsInvocationPackUnpackMain (
        ClAmsT *ams);

extern ClRcT 
clAmsDBConstructInvocationList(
        CL_IN  const ClCharT  *compName,
        CL_IN  const ClCharT  *id,
        CL_IN  const ClCharT  *command,
        CL_IN  const ClCharT  *csiTargetOne,
        CL_IN  const ClCharT  *csiName ) ;

extern ClRcT
clAmsDBConstructInvocation(
        CL_IN  const ClNameT  *compName,
        CL_IN  ClInvocationT id,
        CL_IN  ClAmsInvocationCmdT cmd,
        CL_IN  ClBoolT csiTargetOne,
        CL_IN  const ClNameT  *csiName ) ;

extern ClRcT
clAmsInvocationInstanceXMLize (
        ClAmsInvocationT *invocationData );

extern ClRcT
clAmsInvocationInstanceDeXMLize (
       CL_IN  ClParserPtrT  invocationPtr );

extern ClRcT
clAmsInvocationMainMarshall(ClAmsT *ams, ClBufferHandleT msg);

extern ClRcT   
clAmsInvocationMarshall (
                       CL_IN  ClAmsT  *ams,
                       CL_IN  ClCharT *invocationName,
                       ClBufferHandleT msg);

extern ClRcT
clAmsInvocationListMarshall(ClCntHandleT listHandle, ClBufferHandleT msg);

extern ClRcT   
clAmsInvocationUnmarshall(ClAmsT *ams, ClCharT *invocationName, ClBufferHandleT msg);

extern ClRcT
clAmsInvocationListUnmarshall ( ClBufferHandleT msg );

extern ClRcT   
clAmsDBClUint64ToStr(
       CL_IN  ClInt64T  number,
       CL_OUT  ClCharT  **str );

extern ClRcT   
clAmsDBGAmsXMLize(
       CL_IN  ClAmsT  *ams );

extern ClRcT   
clAmsDBGAmsDeXMLize(
       CL_IN  ClParserPtrT gAmsPtr,
       CL_INOUT  ClAmsT  *ams );

extern ClRcT 
clAmsDBWriteEntityStatus(
        ClAmsEntityStatusT  *entityStatus,
        ClParserPtrT  statusPtr );

extern ClRcT 
clAmsDBReadEntityStatus(
        ClAmsEntityStatusT  *entityStatus,
        ClParserPtrT  statusPtr );

extern ClRcT
clAmsDBMarshallEnd(ClBufferHandleT inMsgHdl, ClUint32T versionCode);

#ifdef __cplusplus
}
#endif

#endif /* _CL_AMS_DB_PACK_UNPACK_H_ */
