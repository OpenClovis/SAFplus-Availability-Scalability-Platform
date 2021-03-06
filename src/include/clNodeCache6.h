#ifndef _CL_NODE_CACHE_H_
#define _CL_NODE_CACHE_H_

#include <clCommon6.h>
#include <clCommonErrors6.h>
#include <clIocApi.h>
//#include <clOsalApi.h>

#ifdef __cplusplus
extern "C" {
#endif

#define __SC_CAPABILITY_VAL (0x1)
#define __SC_CAPABILITY_OFFSET (0x0)
#define __SC_CAPABILITY_MASK (__SC_CAPABILITY_VAL << __SC_CAPABILITY_OFFSET )
#define __SC_PROMOTE_CAPABILITY_VAL (0x1)
#define __SC_PROMOTE_CAPABILITY_OFFSET (0x1)
#define __SC_PROMOTE_CAPABILITY_MASK (__SC_PROMOTE_CAPABILITY_VAL << __SC_PROMOTE_CAPABILITY_OFFSET)
#define __PL_CAPABILITY_VAL (0x1)
#define __PL_CAPABILITY_OFFSET (0x2)
#define __PL_CAPABILITY_MASK (__PL_CAPABILITY_VAL << __PL_CAPABILITY_OFFSET)
#define __LEADER_CAPABILITY_VAL (0x1)
#define __LEADER_CAPABILITY_OFFSET (0x3)
#define __LEADER_CAPABILITY_MASK  (__LEADER_CAPABILITY_VAL << __LEADER_CAPABILITY_OFFSET )

#define CL_NODE_CACHE_SC_CAPABILITY(cap) (!!( (cap) & __SC_CAPABILITY_MASK))
#define CL_NODE_CACHE_SC_SOFT_CAPABILITY(cap) (!!( (cap) & ( __SC_CAPABILITY_MASK | __SC_PROMOTE_CAPABILITY_MASK)))
#define CL_NODE_CACHE_SC_PROMOTE_CAPABILITY(cap) (!!( (cap) & __SC_PROMOTE_CAPABILITY_MASK))
#define CL_NODE_CACHE_PL_CAPABILITY(cap) (!!( (cap) & __PL_CAPABILITY_MASK ) )
#define CL_NODE_CACHE_LEADER_CAPABILITY(cap) (!!( (cap) & __LEADER_CAPABILITY_MASK))
#define CL_NODE_CACHE_SC_CAPABILITY_SET(cap) ( (cap) |= __SC_CAPABILITY_MASK )
#define CL_NODE_CACHE_SC_PROMOTE_CAPABILITY_SET(cap) ( (cap) |= __SC_PROMOTE_CAPABILITY_MASK )
#define CL_NODE_CACHE_PL_CAPABILITY_SET(cap) ( (cap) |= __PL_CAPABILITY_MASK )
#define CL_NODE_CACHE_LEADER_CAPABILITY_SET(cap) ( (cap) |= __LEADER_CAPABILITY_MASK )

#define CL_NODE_CACHE_CAP_ASSIGN (0x1)
#define CL_NODE_CACHE_CAP_AND (0x2)
#define CL_NODE_CACHE_CAP_MERGE (0x3)
#define CL_NODE_CACHE_CAP_CLEAR (0x4)
#define CL_NODE_CACHE_NODENAME_MAX (40)

typedef struct ClNodeCacheMember
{
    ClIocNodeAddressT address;
    ClCharT name[CL_NODE_CACHE_NODENAME_MAX];
    ClUint32T version;
    ClUint32T capability;
}ClNodeCacheMemberT;

typedef enum
{
    CL_NODE_CACHE_SLOT_ID,
    CL_NODE_CACHE_NODENAME,
} ClNodeCacheSlotInfoFieldT;

typedef struct ClNodeCacheSlotInfo
{
    ClUint32T           slotId;
    SaNameT             nodeName;
} ClNodeCacheSlotInfoT;

ClRcT clNodeCacheInitialize(ClBoolT createFlag);
ClRcT clNodeCacheFinalize(void);
ClRcT clNodeCacheUpdate(ClIocNodeAddressT nodeAddress, ClUint32T version, ClUint32T capability, SaNameT *nodeName);
ClRcT clNodeCacheViewGetFast(ClNodeCacheMemberT *pMembers, ClUint32T *pMaxMembers);
ClRcT clNodeCacheViewGetFastSafe(ClNodeCacheMemberT *pMembers, ClUint32T *pMaxMembers);
ClRcT clNodeCacheViewGet(ClNodeCacheMemberT *pMembers, ClUint32T *pMaxMembers);
ClRcT clNodeCacheViewGetSafe(ClNodeCacheMemberT *pMembers, ClUint32T *pMaxMembers);
ClRcT clNodeCacheViewGetWithFilter(ClNodeCacheMemberT *pMembers, ClUint32T *pMaxMembers, ClUint32T capabilityMask);
ClRcT clNodeCacheViewGetWithFilterSafe(ClNodeCacheMemberT *pMembers, ClUint32T *pMaxMembers, ClUint32T capabilityMask);
ClRcT clNodeCacheViewGetWithFilterFast(ClNodeCacheMemberT *pMembers, ClUint32T *pMaxMembers, ClUint32T capabilityMask);
ClRcT clNodeCacheViewGetWithFilterFastSafe(ClNodeCacheMemberT *pMembers, ClUint32T *pMaxMembers, ClUint32T capabilityMask);
ClRcT clNodeCacheReset(ClIocNodeAddressT nodeAddress);
ClRcT clNodeCacheSoftReset(ClIocNodeAddressT nodeAddress);
ClRcT clNodeCacheVersionGet(ClIocNodeAddressT nodeAddress, ClUint32T *pVersion);
ClRcT clNodeCacheVersionAndCapabilityGet(ClIocNodeAddressT nodeAddress, 
                                         ClUint32T *pVersion,
                                         ClUint32T *pCapability);
ClRcT clNodeCacheMinVersionGet(ClIocNodeAddressT *pNodeAddress, ClUint32T *pVersion);
ClRcT clNodeCacheCapabilitySet(ClIocNodeAddressT nodeAddress, ClUint32T capability, ClUint32T flag);
ClRcT clNodeCacheMemberGetFast(ClIocNodeAddressT node, ClNodeCacheMemberT *pMember);
ClRcT clNodeCacheMemberGetFastSafe(ClIocNodeAddressT node, ClNodeCacheMemberT *pMember);
ClRcT clNodeCacheMemberGet(ClIocNodeAddressT node, ClNodeCacheMemberT *pMember);
ClRcT clNodeCacheMemberGetSafe(ClIocNodeAddressT node, ClNodeCacheMemberT *pMember);

ClRcT clNodeCacheMemberGetExtended(ClIocNodeAddressT node, ClNodeCacheMemberT *pMember,ClUint32T retries, ClUint32T msecDelay);
ClRcT clNodeCacheMemberGetExtendedSafe(ClIocNodeAddressT node, ClNodeCacheMemberT *pMember,ClUint32T retries, ClUint32T msecDelay);

/** This function authoritatively updates the leader, cleans out old leader markings and posts this information to all other nodes in the cluster */
ClRcT clNodeCacheLeaderUpdate(ClIocNodeAddressT currentLeader);

/* This function to send leader node to cluster */
ClRcT clNodeCacheLeaderSend(ClIocNodeAddressT currentLeader);

/** This function sets the leader without cleaning up any prior leader marking.  This is used to update the database so that split brain can be detected */
ClRcT  clNodeCacheLeaderSet(ClIocNodeAddressT leader);
    
ClRcT clNodeCacheLeaderGet(ClIocNodeAddressT *pCurrentLeader);

ClRcT clNodeCacheSlotInfoGet(ClNodeCacheSlotInfoFieldT flag, ClNodeCacheSlotInfoT *slotInfo);

ClRcT clNodeCacheSlotInfoGetSafe(ClNodeCacheSlotInfoFieldT flag, ClNodeCacheSlotInfoT *slotInfo);

#ifdef __cplusplus
}
#endif

#endif
