#include <clDebugApi.h>
#include <clCorTxnApi.h>
#include <clCorErrors.h>
#include <clOsalApi.h>
#include <clCorUtilityApi.h>

#include "clCorSimpleApi.h"

#include <clTxnErrors.h>
#include <clCpmApi.h>
#include <clCpmExtApi.h>


static ClRcT clCorReadRef(ClCorObjectReferenceT *ref,void* myObj, ClCor2CT* objDef  );

static ClCorObjectReferenceT *clCorObjRefGetByMoid(const void *moid)
{
    ClRcT rc;
    ClCorObjectReferenceT *ref = NULL;
    //ClTimerTimeOutT delay = { 0 ,100 };
    //int retries = 0;

    CL_ASSERT(moid);
    
    ref = clHeapCalloc(1, sizeof(*ref));
    CL_ASSERT(ref != NULL);

    /* GAS to do:  Add clMoidCopy API */
    memcpy(&ref->moId,moid, sizeof(ClCorMOIdT));
    
    ref->objHandle=0;
    rc = clCorObjectHandleGet(&ref->moId, &ref->objHandle);  /* Get a handle to obj if it exists */
    return ref;    
}


ClCorObjectReferenceT *clCorObjRefGet(const ClCharT *name,int type)
{
    ClNameT objectName = {0};
    ClRcT rc;
    ClCorObjectReferenceT *ref = NULL;
    ClTimerTimeOutT delay = { 0 ,100 };
    int retries = 0;

    CL_ASSERT(name);
    
    ref = clHeapCalloc(1, sizeof(*ref));
    CL_ASSERT(ref != NULL);
    
    clNameSet(&objectName, name);

    do 
    {        
        rc = clCorMoIdNameToMoIdGet(&objectName, &ref->moId);
        if (rc != CL_OK)
        {        
            clLogWarning("COR", "MOIDCVT", "COR moid conversion returned [%#x].  Retry #%d", rc,retries);
            if (CL_GET_ERROR_CODE(rc) == CL_ERR_TIMEOUT) clOsalTaskDelay(delay);
            retries ++;    
        }   
    } while(CL_GET_ERROR_CODE(rc) == CL_ERR_TIMEOUT);
    
    if(rc != CL_OK) return 0;
    if (type != CL_COR_INVALID_SRVC_ID)
    {        
      rc = clCorMoIdServiceSet(&ref->moId, type);
      CL_ASSERT(rc == CL_OK);
      if(rc != CL_OK) return 0;
    }

    ref->objHandle=NULL;
    rc = clCorObjectHandleGet(&ref->moId, &ref->objHandle);  /* Get a handle to obj if it exists */

    return ref;
}

void clCorObjRefRelease(ClCorObjectReferenceT *ref)
{
    CL_ASSERT(ref);
    clCorObjectHandleFree(ref->objHandle);
    memset(ref,0,sizeof(ClCorObjectReferenceT));
    clHeapFree(ref);
}


ClRcT clCorDelete(const ClCharT *name)
{
    ClRcT rc=CL_OK;
    int type;
    
    ClCorObjectReferenceT* hdl;
    
    /* Delete all the MSOs */
    for (type = CL_COR_SVC_ID_MAX; type > CL_COR_SVC_ID_DUMMY_MANAGEMENT; type--)
    {
        /* Most of the msos probably don't exist which is why we ignore errors */
        hdl = clCorObjRefGet(name,type);
        if (hdl)
        {            
            clCorObjectDelete(CL_COR_SIMPLE_TXN, hdl->objHandle);        
            clCorObjRefRelease(hdl);
        }        
    }

    /* Delete the MO */
    hdl = clCorObjRefGet(name,CL_COR_INVALID_SRVC_ID);
    if (hdl) /* Else Deleting something that's already deleted, so return CL_OK */
    {        
    rc = clCorObjectDelete(CL_COR_SIMPLE_TXN, hdl->objHandle);        
    clCorObjRefRelease(hdl);
    }
    
    return rc;
}

ClRcT clCorHandleRelease(ClCorHandleT hdl)
{
    ClRcT rc = clCorObjectDelete(CL_COR_SIMPLE_TXN, hdl->objHandle);
    if (rc != CL_OK)
    {
        return rc;
    }
    
    return CL_OK;    
}

/* GAS TODO: Combine COR ClCorAttributeValueListT and ClCorAttributeValueDescriptorListT */   
ClCorAttributeValueListT* clCorGetAttValueList(void* myObj,ClCor2CT* objDef)
{
    ClInt32T i;
    CL_ASSERT(myObj);
    CL_ASSERT(objDef);
    
    ClCorAttributeValueT* attrValue = clHeapCalloc(objDef->length,sizeof(ClCorAttributeValueT));
    ClCorAttributeValueListT* attrList = clHeapCalloc(1,sizeof(ClCorAttributeValueListT));
    
    attrList->numOfValues = objDef->length;
    attrList->pAttributeValue = attrValue;
    
    ClCor2CEntryT* entry = objDef->itemDef;
    for (i = 0; i< objDef->length; i++,attrValue++,entry++)
    {        
        attrValue->pAttrPath = NULL;
        attrValue->attrId = entry->attrId;  
        attrValue->index = -1;
        attrValue->bufferPtr = ((char*) myObj) + entry->offset;
        attrValue->bufferSize = entry->length;
    }
    
    return attrList;    
}

void clCorReleaseAttValueList(ClCorAttributeValueListT* obj)
{
    CL_ASSERT(obj);

    clHeapFree(obj->pAttributeValue);
    clHeapFree(obj);    
}

ClCorAttrValueDescriptorListT* clCorGetAttValueDescList(void* myObj,ClCor2CT* objDef,ClCorJobStatusT* jobStatus)
{
    ClInt32T i;
    CL_ASSERT(myObj);
    CL_ASSERT(objDef);
    
    ClCorAttrValueDescriptorT* attrValue = clHeapCalloc(objDef->length,sizeof(ClCorAttributeValueT));
    ClCorAttrValueDescriptorListT* attrList = clHeapCalloc(1,sizeof(ClCorAttributeValueListT));
    
    attrList->numOfDescriptor = objDef->length;
    attrList->pAttrDescriptor = attrValue;
    
    ClCor2CEntryT* entry = objDef->itemDef;
    for (i = 0; i< objDef->length; i++,attrValue++,entry++)
    {        
        attrValue->pAttrPath = NULL;
        attrValue->attrId = entry->attrId;  
        attrValue->index = -1;
        attrValue->bufferPtr = ((char*) myObj) + entry->offset;
        attrValue->bufferSize = entry->length;
        attrValue->pJobStatus = jobStatus;
    }
    
    return attrList;    
}

void clCorReleaseAttValueDescList(ClCorAttrValueDescriptorListT* obj)
{
    CL_ASSERT(obj);

    clHeapFree(obj->pAttrDescriptor);
    clHeapFree(obj);    
}


static ClRcT clCorCommit(ClCorTxnSessionIdT sessionId)
{
    ClTimerTimeOutT delay = { 0 ,100 };
    int retries = 0;
    ClRcT rc;
    do
    {
        rc = clCorTxnSessionCommit(sessionId);
        if (rc != CL_OK)
        {
            
            clLogWarning("COR", "CMT", "COR commit returned [%#x].  Retry # %d", rc,retries);
            if (CL_GET_ERROR_CODE(rc) == CL_ERR_TIMEOUT) clOsalTaskDelay(delay);
            
            retries += 1;
        }
        

    } while((retries<5)&&((CL_GET_ERROR_CODE(rc) == CL_ERR_TIMEOUT)|| (rc == CL_TXN_RC(CL_TXN_ERR_TRANSACTION_ROLLED_BACK))));
    
    return rc;    
}

ClRcT clCorCreate(const ClCharT *name, void* myObj, ClCor2CT* objDef, ClCorHandleT *fd)
{
    ClRcT rc;
    ClCorObjectReferenceT *ref = NULL;
    ClCorTxnSessionIdT sessionId = NULL;
    ClIocPortT          iocPort;
    clEoMyEoIocPortGet(&iocPort);
    ClCpmSlotInfoT      sli;
    sli.nodeIocAddress = clIocLocalAddressGet();
    clCpmSlotGet(CL_CPM_IOC_ADDRESS,&sli);
    ClIocPhysicalAddressT  iocAddr = { sli.nodeIocAddress, iocPort};
    ClCorObjectHandleT objH = NULL;
    
    ref = clCorObjRefGet(name, CL_COR_INVALID_SRVC_ID);
    
    /* Create the managed object (if it does not already exist) */
    rc = clCorObjectCreate(&sessionId, &ref->moId, &objH);
    if((rc != CL_OK) && (CL_GET_ERROR_CODE(rc) != CL_ERR_ALREADY_EXIST))
    {
        clLogError("COR", "CREATE", "Failed to add MO create job to the session. rc [0x%x]", rc);
        CL_ASSERT(0);
    }
    
    rc = clCorCommit(sessionId);
    if((rc != CL_OK) && (CL_GET_ERROR_CODE(rc) != CL_COR_INST_ERR_MO_ALREADY_PRESENT))
    {
        clLogError("COR", "CREATE", "Failed to commit the session. rc [0x%x]", rc);
        CL_ASSERT(0);
    }
    
    rc = clCorObjectHandleToMoIdGet(objH, &ref->moId, NULL);
    if (rc != CL_OK)
    {
        clLogError("COR", "CREATE", "Failed to get moid from object handle. rc [0x%x]", rc);
        return rc;
    }

    clCorObjectHandleFree(&objH);

    sessionId = NULL;
    clCorMoIdServiceSet(&ref->moId, CL_COR_SVC_ID_PROVISIONING_MANAGEMENT);

    /* Create without setting items */
    if (myObj == NULL)
    {
        CL_ASSERT(objDef == NULL);
        rc = clCorObjectCreate(&sessionId, &ref->moId, &ref->objHandle);
    }
    else
    {
        ClCorAttributeValueListT* avl = clCorGetAttValueList(myObj,objDef);        
        rc = clCorObjectCreateAndSet(&sessionId,&ref->moId,avl,&ref->objHandle);
        clCorReleaseAttValueList(avl);        
    }

    CL_ASSERT(rc==CL_OK);
    rc = clCorCommit(sessionId);

    /* 
     * Its ok to create an item that already exists.
     * CL_ERR_INVALID_PARAMETER is returned if duplicate key values 
     * are detected.
     */
    if ((CL_GET_ERROR_CODE(rc) == CL_COR_INST_ERR_MSO_ALREADY_PRESENT) || 
            (CL_GET_ERROR_CODE(rc) == CL_ERR_INVALID_PARAMETER) ) 
        rc = CL_OK;
    
    if (rc != CL_OK)
    {
        clLogError("PORT", "CREATE", "COR commit returned [%#x]", rc);
        CL_ASSERT(rc == CL_OK);
        return rc;
    }

    ClRcT rc1 = clCorOIRegister(&ref->moId, &iocAddr);
    CL_ASSERT(rc1 == CL_OK);
    
    if (rc == CL_OK)
    {   
        *fd = ref;
        return rc;
    }
    else
    {
        clCorObjRefRelease(ref);
        *fd = NULL;
        return rc;            
    }
}

ClRcT clCorSetPrimary(const ClCharT *name, ClBoolT state)
{
    ClRcT rc;
    
    if (state)
        rc = clCorNIPrimaryOISet(name);
    else
        rc = clCorNIPrimaryOIClear(name);
    return rc;
}

ClRcT clCorSetPrimaryByMoid(void* moId, ClBoolT state)
{
    ClRcT rc;
    ClIocPortT          iocPort;
    clEoMyEoIocPortGet(&iocPort); 
    ClCpmSlotInfoT sli;
    sli.nodeIocAddress = clIocLocalAddressGet();
    clCpmSlotGet(CL_CPM_IOC_ADDRESS,&sli);
    ClIocPhysicalAddressT  iocAddr = { sli.nodeIocAddress, iocPort};
    ClTimerTimeOutT delay = { 0 ,100 };
    int retries = 0;

    rc = clCorOIRegister(moId, &iocAddr);
    CL_ASSERT(rc == CL_OK);

    do 
    {        
    
        if (state)
            rc = clCorPrimaryOISet(moId,&iocAddr);
        else
            rc = clCorPrimaryOIClear(moId,&iocAddr);
        if (rc != CL_OK)
        {        
            clLogWarning("COR", "OISET", "COR set primary OI [%#x].  Retry #%d", rc,retries);
            if (CL_GET_ERROR_CODE(rc) == CL_ERR_TIMEOUT)  clOsalTaskDelay(delay);
            retries ++;    
        }   
    } while(CL_GET_ERROR_CODE(rc) == CL_ERR_TIMEOUT);
    CL_ASSERT(rc == CL_OK);

    return rc;
}


ClRcT clCorRead(const ClCharT *name,void* myObj, ClCor2CT* objDef)
{    
    ClCorObjectReferenceT *ref = NULL;

    ref = clCorObjRefGet(name,CL_COR_SVC_ID_PROVISIONING_MANAGEMENT);
    ClRcT rc = clCorReadRef(ref,myObj,objDef);
    clCorObjRefRelease(ref);
    return rc;
}

ClRcT clCorReadByMoid(void* MoId, void* myObj, ClCor2CT* objDef)
{
    ClCorObjectReferenceT *ref = NULL;

    ref = clCorObjRefGetByMoid(MoId);
    ClRcT rc = clCorReadRef(ref,myObj,objDef);
    clCorObjRefRelease(ref);
    return rc;
}


static ClRcT clCorReadRef(ClCorObjectReferenceT *ref,void* myObj, ClCor2CT* objDef  )
{
    ClRcT rc;
    ClCorBundleHandleT  bHandle = 0;
    ClCorBundleConfigT  bundleConfig = {CL_COR_BUNDLE_NON_TRANSACTIONAL};
    ClCorAttrValueDescriptorListT* avl = 0;
    ClCorJobStatusT     jobStatus = 0;

    if (ref->objHandle == NULL)
        return CL_RC(CL_CID_COR,CL_ERR_DOESNT_EXIST);
    
    avl = clCorGetAttValueDescList(myObj,objDef,&jobStatus);
    
    rc = clCorBundleInitialize(&bHandle, &bundleConfig);
    CL_ASSERT(CL_OK == rc);    
    rc = clCorBundleObjectGet(bHandle, &ref->objHandle, avl);
    CL_ASSERT(CL_OK == rc);    
    rc = clCorBundleApply(bHandle);
    CL_ASSERT(CL_OK == rc);
    
    clCorReleaseAttValueDescList(avl);        
    return CL_OK;
}


ClRcT clCorOpen(const ClCharT *name, ClCorObjectFlagsT flags, ClCorHandleT *fd)
{
    ClRcT rc = CL_OK;
    ClCorObjectReferenceT *ref = NULL;
    CL_ASSERT(name);
    CL_ASSERT(fd);
    
   if(!name || !fd) return CL_COR_SET_RC(CL_ERR_INVALID_PARAMETER);

   ref = clCorObjRefGet(name,CL_COR_SVC_ID_PROVISIONING_MANAGEMENT);

   if (ref->objHandle == NULL)
   {
       if(flags & CL_COR_OBJECT_CREATE)
       {
           rc = clCorObjectCreate(NULL, &ref->moId, &ref->objHandle);
       }
   }

   if(rc != CL_OK)
       goto out_free;

   *fd = (ClCorHandleT) ref;
   return rc;

out_free:
   clCorObjRefRelease(ref);
   *fd = NULL;
   return rc;
}

#if 0
------------------------------
With the above API, creating an object is a one-liner API:

ClCorHandleT handle = 0;

rc = clCorObjectOpen(﻿"\Chassis:0\activealarmstateTable:0",
CL_COR_OBJECT_CREATE, &handle);
#endif

void clCorTreeNodeInit(ClCorTreeNodeT* obj)
{
    obj->sib = 0;
    obj->child =0;
    obj->parent =0;
}

void clCorTreeInit(ClCorTreeT* obj,ClWordT nSize)
{
    clOsalMutexInit(&obj->mutex);
    clMemGroupInit(&obj->allocated,nSize);
    obj->nodeSize = nSize;
}

void clCorTreeDelete(ClCorTreeT* obj)
{
    clOsalMutexLock(&obj->mutex);    
    clMemGroupDelete(&obj->allocated);
    clOsalMutexUnlock(&obj->mutex);
    clOsalMutexDelete(&obj->mutex);
}


void clCorObjectTreeNodeInit(ClCorObjectTreeNodeT* obj)
{
    clCorTreeNodeInit(&obj->cmn);
    obj->name.length=0;
    obj->name.value[0] = 0;
    obj->value = NULL;
    obj->size = 0;
}

ClCorObjectTreeNodeT* clCorObjectTreeNewNode(ClCorObjectTreeT* tree)
{
    ClCorObjectTreeNodeT* ret = clMemGroupAlloc(&tree->cmn.allocated,sizeof(ClCorObjectTreeNodeT));
    if (ret)
      clCorObjectTreeNodeInit(ret);
    return ret;    
}


void clCorObjectTreeInit(ClCorObjectTreeT* obj)
{
    clCorTreeInit(&obj->cmn,sizeof(ClCorObjectTreeNodeT));    
    obj->root = clCorObjectTreeNewNode(obj);  /* Create an empty root node to eliminate special root handling in the algorithms */
    clNameSet(&obj->root->name, "*the fake root*");
    
}

void clCorObjectTreeDelete(ClCorObjectTreeT* obj)
{
    clCorTreeDelete(&obj->cmn);    
}


typedef struct
{
    ClCorTreeT*     tree;  
    ClCorTreeNodeT* node;
    ClCorMOIdT*     moId;
    ClCorClassTypeT classId;
} AttrWalkCookie;

    

static ClRcT attrWalkFunc( ClCorAttrPathPtrT pAttrPath, ClCorAttrIdT attrId, ClCorAttrTypeT attrType,
                                    ClCorTypeT attrDataType, void *value, ClUint32T size, ClCorAttrFlagT attrData, 
                                    void *cookie)
{
    char             name[200];
    unsigned int     sz = 200;
    AttrWalkCookie*   awc = (AttrWalkCookie*) cookie;
    ClCorClassTypeT  classId = awc->classId;
    ClRcT rc = CL_OK;

    name[0]=0;
    
    if(!classId)
    {
        clCorMoIdToClassGet(awc->moId,CL_COR_MSO_CLASS_GET,&classId);
        awc->classId = classId; /*cache the classid*/
    }
    rc = clCorClassAttributeNameGet(classId, attrId, name, &sz );
    if (rc != CL_OK)
    {
        printf("Attribute name get failed: error 0x%x\n", rc);
        return rc;        
    }
    printf("Attribute: name: %s classId: %d ID: %d, Attr Type: %d, Type: %d, size: %d, Attr flags: %d\n",name,classId, attrId,attrType,attrDataType,size,attrData);
    
    /* Make a new node and stick it as a child of awc->node */
    if (1)
    {
        ClCorObjectTreeT* tree = (ClCorObjectTreeT*) awc->tree;
        ClCorObjectTreeNodeT* newnode = clCorObjectTreeNewNode(tree);
        ClCorObjectTreeNodeT* node = (ClCorObjectTreeNodeT*) awc->node;
        printf("node: %s\n", node->name.value);
        assert(newnode);
        clNameSet(&newnode->name,name);
        newnode->size = size;
        newnode->ordinality = attrType;
        newnode->type = attrDataType;
        newnode->value = clMemGroupAlloc(&tree->cmn.allocated,size);
        assert(newnode->value);
        memcpy(newnode->value,value,size);
        
        newnode->cmn.parent =(ClCorTreeNodeT*) node;
        newnode->cmn.sib = node->cmn.child;
        node->cmn.child = (ClCorTreeNodeT*) newnode;
    }
    
    return CL_OK;    
}

ClRcT WalkAttributes(ClCorObjectHandleT   objHdl,ClCorTreeT* tree,ClCorObjectTreeNodeT* node)
{    
   ClCorMOIdT           moId;
   ClCorServiceIdT      svcId;
   ClNameT              name;
   ClCorObjAttrWalkFilterT walkFilter = {0};

   AttrWalkCookie awc = {0};
   
   clCorObjectHandleToMoIdGet(objHdl, &moId, &svcId);
   
   clCorMoIdToMoIdNameGet(&moId,&name);
   name.value[name.length] = 0;
   printf("Walk attributes MOID NAME: %s, node name: %s\n", name.value,node->name.value);

   // clCorMoIdServiceSet(&moId,CL_COR_SVC_ID_PROVISIONING_MANAGEMENT);
   

   walkFilter.baseAttrWalk = CL_TRUE; //CL_FALSE;
   
   walkFilter.contAttrWalk = CL_TRUE;
   walkFilter.pAttrPath = NULL;
   walkFilter.attrId = CL_COR_INVALID_ATTR_ID;
   walkFilter.index = CL_COR_INVALID_ATTR_IDX;
   walkFilter.cmpFlag =  CL_COR_ATTR_CMP_FLAG_INVALID; 
   walkFilter.attrWalkOption = CL_COR_ATTR_WALK_ALL_ATTR;
   walkFilter.value        = 0; 
   walkFilter.size         = 0;

   //printf ("MOID type: %d", moId.node[moId.depth-1].type);

   awc.tree = tree;
   awc.node = (ClCorTreeNodeT*) node;
   awc.moId = &moId;
   awc.classId = 0;
   clCorObjectAttributeWalk(objHdl, &walkFilter, attrWalkFunc, &awc);
   
   //clCorMoIdShow(&moId);
  return (CL_OK);
}



ClCorObjectTreeNodeT* clCorObjectTreeNodeFindSib(ClCorObjectTreeNodeT* sib,char* str,int len)
{
    while(sib && (strncmp(str,sib->name.value,len)!=0)) sib = (ClCorObjectTreeNodeT*) sib->cmn.sib;
    return sib;
}


ClCorObjectTreeNodeT* clCorObjectTreeTraverse(ClCorObjectTreeT* tree,ClNameT name,ClWordT* rest)
{       
    char* start = name.value;
    ClCorObjectTreeNodeT* cur;
    ClCorObjectTreeNodeT* sib;
    char* end=NULL;
    char* tmp;
    
    cur = tree->root;
    sib = (ClCorObjectTreeNodeT*) cur->cmn.child;
    
    while(sib)
    {
        tmp = strchr(start,'\\'); /* skip the initial \ */
        if (!tmp) break;
        start= tmp+1;
      
        end = strchr(start,'\\');
        if (!end) break; /* we got to the end of name */

        assert(end-start > 0); /* We are consuming something -- otherwise we'll loop infinitely */      
        sib = clCorObjectTreeNodeFindSib(sib,start,end-start);
        if (sib) /* I matched it, so the next match will be to this node's child */
        {
            cur = sib;   /* Remember this node to return it if no children found */
            start = end;
            sib = (ClCorObjectTreeNodeT*) sib->cmn.child;
        }
    }

    *rest = (start-name.value); /* set rest to the length of the consumed chars */
    
    return cur; /* Return the deepest node we found */
}


static ClRcT corObjectTreeFillWalk( void *data, void * cookie)
{
    ClCorServiceIdT      svcId;
    ClCorMOIdT           moId;
    ClCorObjectHandleT   objHdl = *((ClCorObjectHandleT *)data);
    ClCorObjectTreeT*    tree = (ClCorObjectTreeT*) cookie;
    ClNameT              name;
    clCorObjectHandleToMoIdGet(objHdl, &moId, &svcId);
    clCorMoIdToMoIdNameGet(&moId,&name);

    clOsalMutexLock(&tree->cmn.mutex);

    ClWordT rest = 0;
    ClCorObjectTreeNodeT* newnode = NULL;
    ClCorObjectTreeNodeT* node = clCorObjectTreeTraverse(tree,name,&rest);
    if (1) 
    {
        char* start = &name.value[rest];
    
        while(start && (start < &name.value[name.length]))
        {
            //start = strchr(start,'\\');  /* Find the start of the next name (drop the instance) */
            if (!start) break;
            if (start[0] == '\\') start++;
            
            char* end = strchr(start,'\\'); /* find the end of the name */
            if (end != NULL) *end = 0;

            if (strlen(start)) /* There's a real name, not just junk i.e. :instance */
            {                
                /* Create the new node and hook it up as a child of the last node */
                newnode = clCorObjectTreeNewNode(tree);
                assert(newnode);            
                clNameSet(&newnode->name,start);
                newnode->cmn.parent =(ClCorTreeNodeT*) node;
                newnode->cmn.sib = node->cmn.child;
                node->cmn.child = (ClCorTreeNodeT*) newnode;
                node = newnode;
            }
            

            if (end==NULL) break; /* There is no next position */
            /* Move to the next portion */
            start = end+1;
        }
        
        /* Add the attrs to the leaf node */
        if (newnode) WalkAttributes(objHdl,(ClCorTreeT*)tree,newnode);

        
    }
    
    clOsalMutexUnlock(&tree->cmn.mutex); 
    return CL_OK;
}



ClRcT clCorObjectTreeFill(ClCorObjectTreeT* tree)
{
    ClRcT rc = clCorObjectWalk(NULL, NULL, corObjectTreeFillWalk, CL_COR_MSO_WALK, (void*) tree);

    return rc;
    
}


void clCorClassTreeNodeInit(ClCorClassTreeNodeT* obj)
{
    obj->name.length=0;
    obj->name.value[0] = 0;
    obj->cmn.sib = 0;
    obj->cmn.child =0;
    obj->cmn.parent =0;
}


ClCorClassTreeNodeT* clCorClassTreeNewNode(ClCorClassTreeT* tree)
{
    ClCorClassTreeNodeT* ret = clMemGroupAlloc(&tree->cmn.allocated,sizeof(ClCorClassTreeNodeT));
    if (ret)
      clCorClassTreeNodeInit(ret);
    return ret;    
}

void clCorClassTreeInit(ClCorClassTreeT* obj)
{
    clOsalMutexInit(&obj->cmn.mutex);
    obj->cmn.nodeSize = sizeof(ClCorClassTreeNodeT);
    clMemGroupInit(&obj->cmn.allocated,sizeof(ClCorClassTreeNodeT));
    obj->root = clCorClassTreeNewNode(obj);  /* Create an empty root node to eliminate special root handling in the algorithms */
    clNameSet(&obj->root->name, "*the fake root*");
}

void clCorClassTreeDelete(ClCorClassTreeT* obj)
{
    clOsalMutexLock(&obj->cmn.mutex);    
    clMemGroupDelete(&obj->cmn.allocated);
    obj->root = NULL;
    clOsalMutexUnlock(&obj->cmn.mutex);
    clOsalMutexDelete(&obj->cmn.mutex);
}



ClCorClassTreeNodeT* clCorClassTreeNodeFindSib(ClCorClassTreeNodeT* sib,char* str,int len)
{
    while(sib && (strncmp(str,sib->name.value,len)!=0)) sib = (ClCorClassTreeNodeT*) sib->cmn.sib;
    return sib;
}


static char *spaces = "                                                                               ";
static const int spacesLen = 80;

void clCorObjectTreeNodePrint(ClCorObjectTreeNodeT* node,ClWordT indent)
{
    char* ispaces;
    if (indent<spacesLen) ispaces = &spaces[spacesLen-indent];
    else ispaces = spaces;
    
    while(node)
    {
        printf("%s%s\n",ispaces,node->name.value);
        clCorObjectTreeNodePrint((ClCorObjectTreeNodeT*) node->cmn.child,indent+2);

        node=(ClCorObjectTreeNodeT*)node->cmn.sib;
    }
    
}

    
void clCorObjectTreePrint(ClCorObjectTreeT* tree)
{
    clOsalMutexLock(&tree->cmn.mutex);
    clCorObjectTreeNodePrint((ClCorObjectTreeNodeT*) tree->root->cmn.child,0);
    clOsalMutexUnlock(&tree->cmn.mutex); 
}


void clCorClassTreeNodePrint(ClCorClassTreeNodeT* node,ClWordT indent)
{
    char* ispaces;
    if (indent<spacesLen) ispaces = &spaces[spacesLen-indent];
    else ispaces = spaces;
    
    while(node)
    {
        printf("%s%s\n",ispaces,node->name.value);
        clCorClassTreeNodePrint((ClCorClassTreeNodeT*)node->cmn.child,indent+2);

        node=(ClCorClassTreeNodeT*)node->cmn.sib;
    }
    
}

    
void clCorClassTreePrint(ClCorClassTreeT* tree)
{
    clOsalMutexLock(&tree->cmn.mutex);
    clCorClassTreeNodePrint((ClCorClassTreeNodeT*)tree->root->cmn.child,0);
    clOsalMutexUnlock(&tree->cmn.mutex); 
}



ClCorClassTreeNodeT* clCorClassTreeTraverse(ClCorClassTreeT* tree,ClNameT name,ClWordT* rest)
{       
    char* start = name.value;
    ClCorClassTreeNodeT* cur;
    ClCorClassTreeNodeT* sib;
    char* end=NULL;
    char* nextBegin = NULL;
    char* tmp;
    
    cur = tree->root;
    sib = (ClCorClassTreeNodeT*) cur->cmn.child;
    
    while(sib)
    {
        tmp = strchr(start,'\\'); /* The skip the initial \ */
        if (!tmp) break;
        start= tmp+1;
      
        end = strchr(start,':');      
        nextBegin = strchr(start,'\\');
      
        if ((!end)||(nextBegin&&(nextBegin<end))) end = nextBegin;
        if (!end) break; /* we got to the end of name */

        assert(end-start > 0); /* We are consuming something -- otherwise we'll loop infinitely */      
        sib = clCorClassTreeNodeFindSib(sib,start,end-start);
        if (sib) /* I matched it, so the next match will be to this node's child */
        {
            cur = sib;   /* Remember this node to return it if no children found */
            start = end;
            sib = (ClCorClassTreeNodeT*) sib->cmn.child;
        }
    }

    *rest = (start-name.value); /* set rest to the length of the consumed chars */
    
    return cur; /* Return the deepest node we found */
}


static ClRcT corClassTreeFillWalk( void *data, void * cookie)
{
    ClCorServiceIdT      svcId;
    ClCorMOIdT           moId;
    ClCorObjectHandleT   objHdl = *((ClCorObjectHandleT *)data);
    ClCorClassTreeT* tree = (ClCorClassTreeT*) cookie;
    ClNameT              name;
    clCorObjectHandleToMoIdGet(objHdl, &moId, &svcId);
    clCorMoIdToMoIdNameGet(&moId,&name);

    clOsalMutexLock(&tree->cmn.mutex);

    ClWordT rest = 0;
    ClCorClassTreeNodeT* node = clCorClassTreeTraverse(tree,name,&rest);
    if (1) 
    {
        char* start = &name.value[rest];
    
        while(start && (start < &name.value[name.length]))
        {
            //start = strchr(start,'\\');  /* Find the start of the next name (drop the instance) */
            if (!start) break;
            if (start[0] == '\\') start++;
            
            char* end = strchr(start,':'); /* find the end of the name */
            if (end == NULL) break;
            *end = 0;

            if (strlen(start)) /* There's a real name, not just junk i.e. :instance */
            {                
                /* Create the new node and hook it up as a child of the last node */
                ClCorClassTreeNodeT* newnode = clCorClassTreeNewNode(tree);
                assert(newnode);            
                clNameSet(&newnode->name,start);
                newnode->cmn.parent = (ClCorTreeNodeT*) node;
                newnode->cmn.sib = node->cmn.child;
                node->cmn.child = (ClCorTreeNodeT*) newnode;
                node =  newnode;
            }
            

            /* Move to the next portion */
            start = strchr(end+1,'\\');
        }
        
    }
    
    clOsalMutexUnlock(&tree->cmn.mutex); 
    return CL_OK;
}


ClRcT clCorClassTreeFill(ClCorClassTreeT* tree)
{
    ClRcT rc = clCorObjectWalk(NULL, NULL, corClassTreeFillWalk, CL_COR_MO_WALK, (void*) tree);
    return rc;    
}
