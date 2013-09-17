#ifndef RMD_SEVER_TEST_H
#define RMD_SEVER_TEST_H


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#include <clCommon.h>
#include <clOsalApi.h>
#include <clOsalErrors.h>
#include <clBufferApi.h>
#include <clCntApi.h>
#include <clHeapApi.h>
#include <clEoApi.h>
#include <clIocApi.h>
#include <clJobQueue.h>
#include <clIocApiExt.h>
#include <clIocErrors.h>
#include <clIocParseConfig.h>

#define RMD_PORT 31
#define CL_RMD_CLIENT_BIT_SHIFT              6
#define __RMD_CLIENT_TABLE_INDEX(port, funId) ( ((port) << CL_RMD_CLIENT_BIT_SHIFT) | (funId) )
#define __RMD_CLIENT_FILTER_INDEX(port, clientID) (((port) << 16) | ( (clientID) & 0xffff ) )
#define CL_RMD_RC(ERROR_CODE)  CL_RC(CL_CID_RMD, (ERROR_CODE))

/**
 * Hexadecimal of \c CL_EO_MAX_NO_FUNC -1.
 */
#define CL_RMD_FN_MASK                       (0x3F)
#define CL_RMD_CLIENT_ID_MASK                (ClUint32T)(~0U >> CL_RMD_CLIENT_BIT_SHIFT)
/**
 * Defines a mechanism to generate unique RMD function number for the function.
 */
#define CL_EO_GET_FULL_FN_NUM(cl, fn)       (((cl) << CL_EO_CLIENT_BIT_SHIFT) | \
                                                ((fn) << 0))

typedef struct {
/**
 * The requested IOC communication port.
 */
    ClIocPortT              reqIocPort;

/**
 * Indicates the maximum number of EO client.
 */
    ClUint32T               maxNoClients;
}ClRmdConfigT;

void clRmdServerSendErrorNotify(ClBufferHandleT message, ClIocRecvParamT *pRecvParam);
ClRcT clRmdServerClientGetFuncVersion(ClIocPortT port, ClUint32T funcId, ClVersionT *version);
void clRmdServerProtoInstall(ClEoProtoDefT* def);
void clRmdServerProtoSwitch(ClEoProtoDefT* def);
void clRmdServerProtoUninstall(ClUint8T protoid);
void rmdProtoInit(void);
ClRcT clRmdServerMyRmdObjectGet(ClEoExecutionObjT **ppRmdObj);

ClRcT clRmdServerClientInstallTables(ClEoExecutionObjT *pThis,
                              ClEoPayloadWithReplyCallbackTableServerT *table);
ClRcT clRmdServerClientTableRegister(ClEoPayloadWithReplyCallbackTableClientT *clientTable,
                              ClIocPortT clientPort);
ClRcT clRmdServerClientTableDeregister(ClEoPayloadWithReplyCallbackTableClientT *clientTable,
                                ClIocPortT clientPort);
ClRcT clRmdServerClientUninstallTables(ClEoExecutionObjT *pThis,
                                ClEoPayloadWithReplyCallbackTableServerT *table);
ClRcT clRmdServerJobHandler(ClEoJobT *pJob);
ClRcT clRmdServerEnqueueJob(ClBufferHandleT recvMsg, ClIocRecvParamT *pRecvParam);
ClBoolT clRmdServerClientTableFilter(ClIocPortT eoPort, ClUint32T clientID);
ClRcT clRmdServerIocPortGet(ClIocPortT * pRmdServerIocPort);
extern ClRcT clRmdServerWalkWithVersion(ClEoExecutionObjT *pThis, ClUint32T func,
                          ClVersionT *version,
                          ClEoCallFuncCallbackT pFuncCallout,
                          ClBufferHandleT inMsgHdl,
                          ClBufferHandleT outMsgHdl);

extern ClRcT clRmdServerWalk(ClEoExecutionObjT *pThis, ClUint32T func,
        ClEoCallFuncCallbackT pFuncCallout,
        ClBufferHandleT inMsgHdl,
        ClBufferHandleT outMsgHdl);

extern ClRcT rmdSeverInit();


#endif
