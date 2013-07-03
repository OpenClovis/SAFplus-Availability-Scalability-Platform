
#ifndef IOCAPP_H_
#define IOCAPP_H_

/* POSIX Includes */
#include <assert.h>
#include <errno.h>

/* Basic SAFplus Includes */
#include <clCommon.h>

/* SAFplus Client Includes */
#include <clLogApi.h>
#include <clCpmApi.h>
#include <saAmf.h>
#include <clCommon.h>
#include <clCommonErrors.h>
#include <clEoApi.h>
#include <clIocApi.h>
#include <clXdrApi.h>
#include <clOsalApi.h>

#define TEST_IOC_PORT 77

void initializeIoc(ClUint8T sendProtoId, ClUint8T receiveProtoId);
ClRcT recvIocLoop(ClIocCommPortHandleT iocCommPortRev);
ClRcT sendIocLoop(ClIocCommPortHandleT iocCommPortSend,ClUint8T mSendProtoId,ClCharT c);
ClRcT clMgtIocRequestHandle(ClEoExecutionObjT *pThis,
                           ClBufferHandleT eoRecvMsg,
                           ClUint8T priority,
                           ClUint8T protoType,
                           ClUint32T length,
                           ClIocPhysicalAddressT srcAddr);


#endif
