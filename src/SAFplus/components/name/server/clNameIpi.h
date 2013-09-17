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
 * ModuleName  : name
 * File        : clNameIpi.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *
 * This file contains Name Service Server Side module       
 * functionality related datastructures and IPIs                         
 *                                                                       
 *
 *
 *****************************************************************************/
                                                                                                                             

                                                                                                                             
#ifndef _CL_NAME_IPI_H_
#define _CL_NAME_IPI_H_
                                                                                                                             
#include <clEoApi.h>
#include <clBufferApi.h>

#include "clNameApi.h"
#include "clNameConfigApi.h"
#include "clNameConfigApi.h"
#include "clNameErrors.h"
#include "clNameCommon.h"

#ifdef __cplusplus
extern "C" {
#endif
   
/*********************************************************************************/
/*********************************   Defines  ************************************/
/*********************************************************************************/
#define CL_NS_MAX_NO_CONTEXTS           (nameSvcMaxNoGlobalContextGet()\
                                        +nameSvcMaxNoLocalContextGet()+2) 
#define CL_NS_BASE_GLOBAL_CONTEXT       0
#define CL_NS_BASE_NODELOCAL_CONTEXT    (CL_NS_BASE_GLOBAL_CONTEXT\
                                        +nameSvcMaxNoGlobalContextGet()+1)
#define CL_NS_DEFAULT_GLOBAL_CONTEXT    CL_NS_BASE_GLOBAL_CONTEXT
#define CL_NS_DEFAULT_NODELOCAL_CONTEXT CL_NS_BASE_NODELOCAL_CONTEXT 
                                                                                                                             
#define CL_NS_DEFAULT_GLOBAL_MAP_COOKIE CL_NS_BASE_GLOBAL_CONTEXT+1000
#define CL_NS_DEFAULT_LOCAL_MAP_COOKIE  CL_NS_BASE_NODELOCAL_CONTEXT+1000


#define CL_NS_NAME              "NameService" /* Service name */
#define CL_NS_PERIODIC_TIMEOUT   1000         /* in ms */
#define NS_MAX_NO_ATTR           5            /* max no of attributes 
                                                in an entry */
#define CL_NS_RESERVED_OBJREF    0            /* Reserved for internal use */ 
#define CL_NS_TEMPORARY_PRIORITY 0x0FFFFFFF   /* Initial priority */
 
#define CL_NS_MAX_NO_OBJECT_REFERENCES  1     /* Max no. of object references
                                                  per name */

/* Following define are needed for generating contetxIds */
#define CL_NS_SLOT_ALLOCATED    1           /* ContextId already in use */
#define CL_NS_SLOT_FREE         0           /* ContextId can be allocated */     


/*********************************************************************************/
/*****************************   Data structures  ********************************/
/*********************************************************************************/

/* Name Service EO related information */
typedef struct 
{
    int state;                 /* state */
    ClEoExecutionObjT* EOId;   /* pointer to execution object */
}ClNameServiceT;


/* Structure of packed information in database syncup process */
typedef struct 
{
    ClUint32T     contextId;  /* Context Id */
    ClUint32T     count;      /* No. of entries in that context */
    ClNameSvcEntryT dbEntry[1]; /* List of entries */
} ClNameSvcDBSyncupT;

typedef ClNameSvcDBSyncupT * ClNameSvcDBSyncupPtrT;


/* Used in packing/unpacking during database syncup */
typedef enum
{
    CL_NS_METADATA_PACK, /* Indicates the packing of metadata
                            (no. of entries that will be sent etc) */
    CL_NS_ENTRIES_PACK,  /* Indicates the packing of NS entries */
}ClNameSvcSyncupWalkT;


/* Cookie in syncup related walk */
typedef struct 
{
    ClNameSvcSyncupWalkT    operation;    /* Whether walk is for packing
                                            metadata or entries */
    ClBufferHandleT  outMsgHandle; /* Buffer that would contain the 
                                            packed information */
    ClUint32T               source;       /* From server or persistent memory */
}ClNameSvcDBSyncupWalkInfoT;

typedef ClNameSvcDBSyncupWalkInfoT * ClNameSvcDBSyncupWalkInfoPtrT;

    
/* Structure of each Global NS entry */
typedef struct 
{
    ClCntHandleT        hashId;           /* hash key for per context hash 
                              table pointed to by the entry */
    ClUint32T           entryCount;       /* total no of entries in the context.
                              Will be used during database syncup */
    ClUint32T           contextMapCookie; /* Needed during lookup. Maps 
                              one-to-one to a context */   
    ClUint32T           dsIdCnt; /* dsIdCnt for runtime dsId generation */
    
    ClUint32T           dsIdCntMax; /* max given out*/

    ClUint8T            freeDsIdMap[32]; /*small static bitmap of free dataset ids*/

} ClNameSvcContextInfoT;

typedef ClNameSvcContextInfoT * ClNameSvcContextInfoPtrT;


/* Structure passed while searching for a context given a cookie */
typedef struct 
{
    ClUint32T    cookie;      /* cookie */
    ClUint32T    contextId;   /* context Id */
} ClNameSvcCookieInfoT;


/* Structure containing the name service binding details */
typedef struct
{
    ClUint32T            cksum;        /* Cksum of attr list. Needed during
                                         hash key compare function */ 
    ClUint64T            objReference; /* Object Reference */
    ClUint32T            refCount;     /* Count of components providing the
                                         service */
    ClUint32T            dsId;         /* dsId of checkpoint */
    ClNameSvcCompListT   compId;       /* List of components providing the 
                                         service */
    ClUint32T            attrCount;    /* Number of attributes associated with
                                         the entry */
    ClUint32T            attrLen;      /* Size of variable member "attr" */
    ClNameSvcAttrEntryT  attr[1];      /* List of attributes */
} ClNameSvcBindingDetailsT;

typedef ClNameSvcBindingDetailsT* ClNameSvcBindingDetailsPtrT;


/* Structure containing the name service binding information */
typedef struct
{
    ClUint32T           cksum;    /* Cksum of name. Needed during
                                     hash key compare function */ 
    SaNameT             name;     /* Name of the service */
    ClUint32T           refCount; /* Count of components providing the 
                                     service */
    ClUint32T           dsId;     /* dsId of checkpoint */
    ClCntHandleT        hashId;   /* Hash key for binding details */ 
    ClUint32T           priority; /* Highest priority registered by all 
                                     components for this service */
    ClCntNodeHandleT    nodeHdl;  /* Node handle of highest priority service 
                                     provider */
} ClNameSvcBindingT;

typedef ClNameSvcBindingT* ClNameSvcBindingPtrT;


/* Cookie in deregistration related walk */
typedef struct
{
    ClUint32T                contextId;
    ClNameSvcContextInfoPtrT hashTable; /* Per context hash table that
                                       is being walked */
    ClUint32T                compId;    /* Id of the component that has
                                       gone down */
    ClEoIdT                  eoID;     /* EO ID of the component that has
                                          gone down */
    ClIocNodeAddressT        nodeAddress; /* IOC address of the node which
                                             has gone down */
    ClIocPortT               clientIocPort; /* IOC port of the component that
                                               has gone down */
    ClNameSvcBindingPtrT     nameEntry;        /* Binding entry */
    ClCntKeyHandleT          nameEntryUserKey; /* Container Key */
    ClUint32T                operation; /* Whether component or service
                                       deregistration */
} ClNameSvcDeregisInfoT;


/* Cookie in display related walk */
typedef struct
{
    SaNameT                name;      /* Name of the service */
    ClUint32T              operation; /* Whether name or binding display */
    ClBufferHandleT outMsgHdl; /* Will carry displat info */
} ClNameSvcDisplayInfoT;


/* Cookie in all bindings/name get  related walk */
typedef struct
{
    ClBufferHandleT  outMsgHandle; /* Buffer that would contain the
                                            packed information */
    ClNameSvcInfoT          *nsInfo;      /* will contain attribute details */
} ClNameSvcAllBindingsGetT;

/* Key for name lookup */
typedef struct
{
    ClUint32T   cksum;    /* Cksum of name */ 
    SaNameT     name;     /* Name of the service */
}ClNameSvcNameLookupT;

/* Cookie for attribute level query */
typedef struct
{
    SaNameT                 *pName;       /* Name of the service */
    ClNameSvcAttrSearchT    *pAttrList;   /* attribute list */
    ClBufferHandleT  outMsgHandle; /* Buffer that would contain the
                                            packed information */
}ClNameSvcAttrLevelQueryT;

/* Enum of Nack types */
typedef enum
{
    CL_NS_NACK_REGISTER,             /* Registration failure */
    CL_NS_NACK_COMPONENT_DEREGISTER, /* Component deregistration failure */
    CL_NS_NACK_SERVICE_DEREGISTER,   /* Service deregistration failure */
    CL_NS_NACK_CONTEXT_CREATE,       /* Context creation failure */
    CL_NS_NACK_CONTEXT_DELETE        /* Contect deletion failure */
}ClNameSvcNackT;

 

/*********************************************************************************/
/**************************************  IPIS   **********************************/
/*********************************************************************************/

ClRcT VDECL(nameSvcRegister)(ClEoDataT data,  ClBufferHandleT  inMsgHandle,
                      ClBufferHandleT  outMsgHandle);
ClRcT VDECL(nameSvcServiceDeregister)(ClEoDataT data,  ClBufferHandleT  inMsgHandle,
                               ClBufferHandleT  outMsgHandle);
ClRcT VDECL(nameSvcComponentDeregister)(ClEoDataT data,  ClBufferHandleT  inMsgHandle,
                                 ClBufferHandleT  outMsgHandle);
ClRcT VDECL(nameSvcContextDelete)(ClEoDataT data,  ClBufferHandleT  inMsgHandle,
                           ClBufferHandleT  outMsgHandle);
ClRcT VDECL(nameSvcQuery)(ClEoDataT data,  ClBufferHandleT  inMsgHandle,
                   ClBufferHandleT  outMsgHandle);

ClRcT VDECL(nameSvcContextCreate)(ClEoDataT data,  ClBufferHandleT  inMsgHandle,
                           ClBufferHandleT  outMsgHandle);
ClRcT nameSvcPerContextInfo(ClUint32T data,  ClBufferHandleT  inMsgHandle,
                            ClBufferHandleT  outMsgHandle);
                                                                                                                             
ClRcT VDECL(nameSvcNack)(ClEoDataT data,  ClBufferHandleT  inMsgHandle,
                  ClBufferHandleT  outMsgHandle);

ClRcT nameSvcSyncupMetadataGet(ClNameSvcContextInfoPtrT pContextInfo);

ClRcT nameSvcSyncupMetadataPack(ClUint32T data,  ClBufferHandleT  inMsgHandle,
                                ClBufferHandleT  outMsgHandle);
 


/*ClRcT nameSvcDatabaseSyncup(ClUint32T eoArg, NameSvcInfo_t *nsInfo, ClUint32T inLen,
                                  ClUint32T* outBuff, ClUint32T *outLen);
*/
ClRcT nameSvcPerContextBindings();
ClRcT nameSvcIsAlive();
ClRcT VDECL(nameSvcDBEntriesPack)(ClEoDataT data,  ClBufferHandleT  inMsgHandle,
                           ClBufferHandleT  outMsgHandle);

ClRcT nameSvcDBEntriesUnpack(ClBufferHandleT msgHdl);
ClRcT _nameSvcDBEntriesPack(ClBufferHandleT  outMsgHandle,
                            ClUint32T flag);
ClRcT _nameSvcAttributeQuery(ClUint32T contextiMapCookie,
                             ClNameSvcAttrSearchT *pAttr,
                             ClBufferHandleT outMsgHandle);
ClRcT nameDebugRegister(ClEoExecutionObjT* pEoObj);
ClRcT nameDebugDeregister(ClEoExecutionObjT* pEoObj);

ClRcT clNameInitialize(ClNameSvcConfigT* pConfig);
ClRcT clNameFinalize(void);


ClUint32T nameSvcMaxNoGlobalContextGet();
ClUint32T nameSvcMaxNoLocalContextGet();	
ClRcT nameSvcContextGet(ClUint32T contextId,
                        ClNameSvcContextInfoPtrT *pCtxData);


#ifdef __cplusplus
}
#endif
                                                                                                                             
#endif /* _CL_NAME_IPI_H_ */

