/* ------------------------------------------------------------------------
 * System header files below
 *------------------------------------------------------------------------*/
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#if defined(OPENAIS_LINUX)
#include <sys/sysinfo.h>
#endif
#if defined(OPENAIS_BSD) || defined(OPENAIS_DARWIN)
#include <sys/sysctl.h>
#endif
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <sys/uio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
/* ------------------------------------------------------------------------
 * Openais specific header files below
 *------------------------------------------------------------------------*/
#include "totem.h"
#include "../include/mar_gen.h"
#include "../include/list.h"
#include "../include/queue.h"
#include "../lcr/lcr_comp.h"
#include "totempg.h"
#include "main.h"
#include "ipc.h"
#include "mempool.h"
#include "service.h"
#include "print.h"

/* ------------------------------------------------------------------------
 * Clovis specific header files below
 *------------------------------------------------------------------------*/
#include "../../../../components/include/clCommon.h"
#include "../../../../components/include/clVersion.h"
#include "../../../../components/debug/include/clDebugApi.h"
#include "../../../../components/gms/include/clClmApi.h"
#include "../../../../components/gms/include/clTmsApi.h"
#include "../../../../components/gms/common/clGmsCommon.h"
#include "../../../../components/cnt/include/clCntApi.h"
#include "../../../../components/gms/server/clGms.h"
#include "../../../../components/gms/server/clGmsView.h"
#include "../../../../components/osal/include/clOsalApi.h"
#include "../../../../components/gms/server/clGmsEngine.h"
#include "../../../../components/gms/server/clGmsMsg.h"
#include "../../../../components/gms/include/clGmsErrors.h"
#include "../../../../components/utils/include/clVersionApi.h"
#include "../../../../components/include/ipi/clNodeCache.h"
#include "../../../../components/buffer/include/clBufferApi.h"
#include "../../../../components/utils/include/clXdrApi.h"

/* ------------------------------------------------------------------------
 * Clovis data structures as extern declarations
 *------------------------------------------------------------------------*/
extern  ClGmsNodeT          gmsGlobalInfo;
extern  void                _clGmsGetThisNodeInfo(ClGmsClusterMemberT  *);
extern  ClRcT               _clGmsCallClusterMemberEjectCallBack( ClGmsMemberEjectReasonT );
extern  ClGmsCsSectionT     joinCs;
extern  ClBoolT             ringVersionCheckPassed;

#define CHECK_RETURN(x,y)   do { \
    ClRcT  rc = CL_OK;           \
    rc = (x);                    \
    if (rc != (y))               \
        return rc;               \
} while (0)

struct groupMessage {
    ClGmsGroupMemberT       gmsGroupNode;
    ClGmsGroupInfoT         groupData;
};


struct mcastMessage {
    struct groupMessage groupInfo;
    ClUint32T           userDataSize;
};

struct VDECL(req_exec_gms_nodejoin) {
    ClVersionT              version;
    ClGmsMessageTypeT       gmsMessageType;
    ClUint64T               contextHandle;
    ClGmsGroupIdT           gmsGroupId;
    ClGmsMemberEjectReasonT ejectReason;
    union message {
        ClGmsClusterMemberT     gmsClusterNode;
        struct groupMessage     groupMessage;
        struct mcastMessage     mcastMessage;
    } specificMessage;
    ClPtrT                  *dataPtr;
};

extern ClRcT   marshallClVersionT(ClVersionT *version, ClBufferHandleT bufferHandle);
extern ClRcT   marshallClGmsClusterMemberT(ClGmsClusterMemberT *clusterNode, ClBufferHandleT bufferHandle);
extern ClRcT   marshallClGmsGroupMemberT(ClGmsGroupMemberT *groupMember, ClBufferHandleT bufferHandle);
extern ClRcT   marshallClGmsGroupInfoT(ClGmsGroupInfoT *groupInfo, ClBufferHandleT bufferHandle);
extern ClRcT   marshallSyncMessage(struct VDECL(req_exec_gms_nodejoin) *req_exec_gms_nodejoin, ClBufferHandleT bufferHandle);
extern ClRcT   marshallMCastMessage(struct mcastMessage *mcastMsg, ClPtrT  data, ClBufferHandleT bufferHandle);
extern ClRcT   marshallReqExecGmsNodeJoin(struct VDECL(req_exec_gms_nodejoin) *req_exec_gms_nodejoin, ClBufferHandleT bufferHandle);

extern ClRcT   unmarshallClVersionT(ClBufferHandleT bufferHandle, ClVersionT *version);
extern ClRcT   unmarshallClGmsClusterMemberT(ClBufferHandleT bufferHandle, ClGmsClusterMemberT *clusterNode);
extern ClRcT   unmarshallClGmsGroupMemberT(ClBufferHandleT bufferHandle, ClGmsGroupMemberT *groupMember);
extern ClRcT   unmarshallClGmsGroupInfoT(ClBufferHandleT bufferHandle, ClGmsGroupInfoT *groupInfo);
extern ClRcT   unmarshallSyncMessage(ClBufferHandleT bufferHandle, struct VDECL(req_exec_gms_nodejoin) *req_exec_gms_nodejoin);
extern ClRcT   unmarshallMCastMessage(ClBufferHandleT bufferHandle, struct VDECL(req_exec_gms_nodejoin) *req_exec_gms_nodejoin);
extern ClRcT   unmarshallReqExecGmsNodeJoin(ClBufferHandleT bufferHandle, struct VDECL(req_exec_gms_nodejoin) *req_exec_gms_nodejoin);

