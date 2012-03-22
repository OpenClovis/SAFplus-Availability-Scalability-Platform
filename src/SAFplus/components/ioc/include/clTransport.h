#ifndef _CL_TRANSPORT_H_
#define _CL_TRANSPORT_H_

#include <clCommon.h>
#include <clCommonErrors.h>
#include <clIocApi.h>
#include <clIocIpi.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CL_TRANSPORT_BASE_PORT (18000)

typedef ClRcT (*ClTransportNotifyCallbackT)
(ClIocPhysicalAddressT *compAddr, ClUint32T status, ClPtrT arg);

extern ClInt32T gClTransportId;
/*
 * We aren't going to be having multiple transports requesting at the same time.
 * So keep this simple. No need for bitmaps as these aren't moving targets.
 */
static __inline__ ClInt32T clTransportIdGet(void)
{
    return ++gClTransportId;
}

extern ClRcT clTransportLayerInitialize(void);
extern ClRcT clTransportLayerFinalize(void);
extern ClRcT clTransportInitialize(const ClCharT *type, ClBoolT nodeRep);
extern ClRcT clTransportFinalize(const ClCharT *type, ClBoolT nodeRep);

extern ClRcT clTransportNotifyInitialize(void);
extern ClRcT clTransportNotifyFinalize(void);
extern ClRcT clTransportIpAddressAssign(const ClCharT *type);
extern ClRcT clTransportNotificationInitialize(const ClCharT *type);
extern ClRcT clTransportNotificationFinalize(const ClCharT *type);

extern ClRcT clTransportBind(const ClCharT *type, ClIocPortT port);
extern ClRcT clTransportBindClose(const ClCharT *type, ClIocPortT port);
extern ClRcT clTransportMaxPayloadSizeGet(const ClCharT *type, ClUint32T *pSize);
extern ClRcT clTransportListen(const ClCharT *type, ClIocPortT port);
extern ClRcT clTransportListenStop(const ClCharT *type, ClIocPortT port);
extern ClRcT clTransportServerReady(const ClCharT *type, ClIocAddressT *pAddress);
extern ClRcT 
clTransportMasterAddressGet(const ClCharT *type, ClIocLogicalAddressT la,
                            ClIocPortT port, ClIocNodeAddressT *masterAddress);
extern ClRcT 
clTransportMasterAddressGetDefault(ClIocLogicalAddressT la, ClIocPortT port,
                                   ClIocNodeAddressT *masterAddress);
extern ClRcT clTransportNotifyOpen(ClIocPortT port);
extern ClRcT clTransportNotifyClose(ClIocPortT port);
extern ClRcT clTransportNotifyRegister(ClTransportNotifyCallbackT callback, ClPtrT arg);
extern ClRcT clTransportNotifyDeregister(ClTransportNotifyCallbackT callback);
extern ClRcT
clTransportNotificationOpen(const ClCharT *type, ClIocPortT port);
extern ClRcT
clTransportNotificationClose(const ClCharT *type, ClIocNodeAddressT nodeAddress, ClIocPortT port);

extern ClRcT 
clTransportSend(const ClCharT *type, ClIocPortT port, ClUint32T priority, ClIocAddressT *address, 
                struct iovec *iov, ClInt32T iovlen, ClInt32T flags);
extern ClRcT 
clTransportSendProxy(const ClCharT *type, ClIocPortT port, ClUint32T priority, ClIocAddressT *address, 
                     struct iovec *iov, ClInt32T iovlen, ClInt32T flags, ClBoolT proxy);

extern ClRcT 
clTransportRecv(const ClCharT *type, ClIocCommPortHandleT commPort, ClIocDispatchOptionT *pRecvOption,
                ClUint8T *buffer, ClUint32T bufSize,
                ClBufferHandleT message, ClIocRecvParamT *pRecvParam);
extern ClRcT 
clTransportRecvDefault(ClIocCommPortHandleT commPort, ClIocDispatchOptionT *pRecvOption,
                       ClUint8T *buffer, ClUint32T bufSize,
                       ClBufferHandleT message, ClIocRecvParamT *pRecvParam);

extern ClRcT clTransportTransparencyRegister(const ClCharT *type,
                                             ClIocPortT port, ClIocLogicalAddressT logicalAddr, ClUint32T haState);
extern ClRcT clTransportTransparencyDeregister(const ClCharT *type, 
                                               ClIocPortT port, ClIocLogicalAddressT logicalAddr);
extern ClRcT clTransportMulticastRegister(const ClCharT *type,
                                          ClIocPortT port, ClIocMulticastAddressT mcastAddr);
extern ClRcT clTransportMulticastDeregister(const ClCharT *type,
                                            ClIocPortT port, ClIocMulticastAddressT mcastAddr);
extern ClRcT 
clTransportListenerRegister(ClInt32T fd, ClRcT (*dispatchCallback)(ClInt32T fd, ClInt32T events, void *cookie),
                            void *cookie);
extern ClRcT 
clTransportListenerDeregister(ClInt32T fd);

extern void
clTransportPrivateDataSet(ClInt32T fd, ClIocPortT port, void *private, void **privateLast);

extern void *
clTransportPrivateDataGet(ClInt32T fd, ClIocPortT port);

extern void *
clTransportPrivateDataDelete(ClInt32T fd, ClIocPortT port);

extern ClRcT
clTransportDispatch(ClIocPortT port, ClUint8T *buffer, ClUint32T bufSize);

extern ClRcT clFindTransport(ClIocNodeAddressT dstIocAddress, ClIocAddressT *rdstIocAddress,
        ClCharT **typeXport);

extern ClCharT *clTransportMcastAddressGet();
extern ClUint32T clTransportMcastPortGet();
extern ClUint32T clTransportHeartBeatIntervalGet();
extern ClUint32T clTransportHeartBeatIntervalCompGet();
extern ClUint32T clTransportHeartBeatRetriesGet();
extern ClBoolT clTransportBridgeEnabled(ClIocNodeAddressT node);
extern ClRcT clTransportBroadcastListGet(const ClCharT *hostXport, 
                                         ClIocPhysicalAddressT *hostAddr,
                                         ClUint32T *pNumEntries, ClIocAddressT **ppDestSlots);

#ifdef __cplusplus
}
#endif

#endif
