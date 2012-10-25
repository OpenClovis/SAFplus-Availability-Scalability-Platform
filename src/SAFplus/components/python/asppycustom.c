// gcc -c custom.c -o custom.o -I/usr/include/python2.5
// gcc -shared custom.o -o pyasp.so

#include <Python.h>

#define CL_AMS_MGMT_FUNC

#include "clIocApi.h"
#include "clOsalApi.h"
//#include "clTipcUserApi.h"
#include "clIocApiExt.h"
#include "clCpmExtApi.h"
#include "clLogApi.h"
#include "clAmsMgmtClientApi.h"
#include "clAmsMgmtCommon.h"

static PyObject* emptyDict = NULL;
static ClAmsMgmtHandleT amsHandle = 0;
/* static FILE* logfile=NULL; */

/*#define DbgLog(...)   clAppLog(CL_LOG_HANDLE_APP, CL_LOG_SEV_INFO, 10, CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED, __VA_ARGS__) */
/*#define DbgLog(...) do { if(logfile) { fprintf(logfile,__VA_ARGS__); fprintf(logfile,"\n"); fflush(logfile); } } while(0)*/
#define DbgLog(...)

#define STRING_HA_STATE(S)                                                  \
(   ((S) == CL_AMS_HA_STATE_ACTIVE)             ? "Active" :                \
    ((S) == CL_AMS_HA_STATE_STANDBY)            ? "Standby" :               \
    ((S) == CL_AMS_HA_STATE_QUIESCED)           ? "Quiesced" :              \
    ((S) == CL_AMS_HA_STATE_QUIESCING)          ? "Quiescing" :             \
    ((S) == CL_AMS_HA_STATE_NONE)               ? "None" :                  \
                                                  "Unknown" )


static PyObject* GetWorkOnServiceGroup(PyObject *self, PyObject *args)
{
  char buf[32*1024];
  const int bufLen = sizeof(buf);
  int curLen = 0;
  ClRcT rc;
  ClAmsEntityBufferT siList,csiList;
  const char* arg0=NULL;
  unsigned int siit;
  unsigned int csiit;
  unsigned int nvpit;
  
  ClAmsMgmtHandleT hdl = amsHandle;
  if (!hdl)
    {
      PyErr_SetString(PyExc_SystemError,"AMF Handle not initialized");
      return NULL;
    }

  if (!PyArg_ParseTuple(args, "s", &arg0))
    return NULL;
  DbgLog("GetWorkOnNode arg is: %s",arg0);

  ClAmsEntityT svcgrp;
  svcgrp.type = CL_AMS_ENTITY_TYPE_SG;
  clNameSet(&svcgrp.name, arg0);
  svcgrp.name.length+=1;

  curLen += snprintf(buf+curLen, bufLen-curLen, "{");
  
  siList.entity = NULL;
  rc = clAmsMgmtGetSGSIList(hdl, &svcgrp, &siList);
  if (CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_EXIST)
    {
      sprintf(buf,"Service Group %s does not exist",arg0);
      PyErr_SetString(PyExc_KeyError,buf);
      return NULL;
    }
  else if (rc != CL_OK)
    {
      sprintf(buf,"clAmsMgmtGetSGSIList error 0x%x",rc);
      PyObject* errData = Py_BuildValue("is",rc,buf);
      PyErr_SetObject(PyExc_SystemError,errData);
      //PyErr_SetString(PyExc_SystemError,buf);
      return NULL;
    }

  for (siit = 0; siit < siList.count; siit++)
    {
      ClAmsEntityT*  siEnt = &siList.entity[siit];

      csiList.entity = NULL;
      rc = clAmsMgmtGetSICSIList(hdl, siEnt, &csiList);
      DbgLog("4: %s", buf);

      curLen += snprintf(buf+curLen, bufLen-curLen, "%s'%.*s':{",(siit==0) ? "":",", siEnt->name.length,siEnt->name.value);
      //curLen += snprintf(buf+curLen, bufLen-curLen, "'csiHaState':'%s'", STRING_HA_STATE(si->haState));
          
      if (rc == CL_OK) for (csiit = 0; csiit < csiList.count; csiit++)
        {
          ClAmsCSINVPBufferT  nvpList;
          nvpList.nvp = NULL;
          rc = clAmsMgmtGetCSINVPList(hdl, &csiList.entity[csiit],&nvpList);
          DbgLog("5: %s", buf);
          if (rc == CL_OK) for (nvpit = 0; nvpit < nvpList.count; nvpit++)
            {
              ClAmsCSINameValuePairT* nvp = &nvpList.nvp[nvpit];
              curLen += snprintf(buf+curLen, bufLen-curLen, "%s '%.*s':'%.*s'",((csiit==0)&&(nvpit==0)) ? "":",",nvp->paramName.length,nvp->paramName.value,nvp->paramValue.length,nvp->paramValue.value);
              DbgLog("6: %s", buf);

            }
          //curLen += snprintf(buf+curLen, bufLen-curLen, "}");
          if (nvpList.nvp) clHeapFree(nvpList.nvp);
        }

      curLen += snprintf(buf+curLen, bufLen-curLen, "}");
      if (csiList.entity) clHeapFree(csiList.entity);
    }

  curLen += snprintf(buf+curLen, bufLen-curLen, "}");
  if (siList.entity) clHeapFree(siList.entity);

  clAppLog(CL_LOG_HANDLE_APP, CL_LOG_SEV_INFO, 10, CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,
           "AmfDump: %s", buf);
  PyObject* ret = PyRun_String(buf,Py_eval_input,emptyDict,emptyDict);
  return ret;
}


extern ClRcT clAmsMgmtGetSGSIList(
        CL_IN   ClAmsMgmtHandleT handle,
        CL_IN   ClAmsEntityT    *sg,
        CL_OUT  ClAmsEntityBufferT  *siBuffer);


static PyObject* GetWorkOnNode(PyObject *self, PyObject *args)
{
  char buf[32*1024];
  const int bufLen = sizeof(buf);
  int curLen = 0;
  ClRcT rc;
  ClAmsEntityBufferT suList,csiList;
  const char* arg0=NULL;
  ClAmsMgmtHandleT hdl = amsHandle;
  unsigned int suit;
  unsigned int siit;
  unsigned int csiit;
  unsigned int nvpit;
  
  DbgLog("GetWorkOnNode");

  if (!hdl)
    {
      PyErr_SetString(PyExc_SystemError,"AMF Handle not initialized");
      return NULL;
    }
  DbgLog("GetWorkOnNode handle ok");

  if (!PyArg_ParseTuple(args, "s", &arg0))
        return NULL;
  DbgLog("GetWorkOnNode arg is: %s",arg0);

  ClAmsEntityT node;
  node.type = CL_AMS_ENTITY_TYPE_NODE;
  clNameSet(&node.name, arg0);
  node.name.length+=1;

  curLen += snprintf(buf+curLen, bufLen-curLen, "{");
  DbgLog("GetWorkOnNode 0");

  suList.entity = NULL;
  rc = clAmsMgmtGetNodeSUList(hdl, &node, &suList);
  if (CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_EXIST)
    {
      sprintf(buf,"Node %s does not exist",arg0);
      PyObject* errData = Py_BuildValue("is",rc,buf);
      PyErr_SetObject(PyExc_KeyError,errData);
      //PyErr_SetString(PyExc_KeyError,buf);
      return NULL;
    }
  else if (rc != CL_OK)
    {
      sprintf(buf,"clAmsMgmtGetNode error 0x%x",rc);
      //PyErr_SetString(PyExc_SystemError,buf);
      PyObject* errData = Py_BuildValue("is",rc,buf);
      PyErr_SetObject(PyExc_SystemError,errData);
      return NULL;
    }
  DbgLog("1");
  for (suit = 0; suit< suList.count; suit++)
  {
      curLen += snprintf(buf+curLen, bufLen-curLen, "%s'%.*s':{",(suit==0) ? "":",", suList.entity[suit].name.length,suList.entity[suit].name.value);
      DbgLog("2: %s", buf);

      ClAmsSUSIRefBufferT siList;
      siList.entityRef = NULL;
      rc =  clAmsMgmtGetSUAssignedSIsList(hdl, &suList.entity[suit],&siList); 
      DbgLog("3: %s", buf);
      if (rc == CL_OK)
      {
          
          for (siit = 0; siit < siList.count; siit++)
          {
              ClAmsSUSIRefT* si = &siList.entityRef[siit];
              ClAmsEntityT*  siEnt = &si->entityRef.entity;

              csiList.entity = NULL;
              rc = clAmsMgmtGetSICSIList(hdl, siEnt, &csiList);
              DbgLog("4: %s", buf);

              curLen += snprintf(buf+curLen, bufLen-curLen, "%s'%.*s':{",(siit==0) ? "":",", siEnt->name.length,siEnt->name.value);
              curLen += snprintf(buf+curLen, bufLen-curLen, "'csiHaState':'%s'", STRING_HA_STATE(si->haState));
          
              if (rc == CL_OK)
              {
                  for (csiit = 0; csiit < csiList.count; csiit++)
                  {
                      ClAmsCSINVPBufferT  nvpList;
                      nvpList.nvp = NULL;
                      rc = clAmsMgmtGetCSINVPList(hdl, &csiList.entity[csiit],&nvpList);
                      if (rc==CL_OK)
                      {                  
                          DbgLog("5: %s", buf);
                          for (nvpit = 0; nvpit < nvpList.count; nvpit++)
                          {
                              ClAmsCSINameValuePairT* nvp = &nvpList.nvp[nvpit];
                              curLen += snprintf(buf+curLen, bufLen-curLen, "%s '%.*s':'%.*s'",(0) ? "":",",nvp->paramName.length,nvp->paramName.value,nvp->paramValue.length,nvp->paramValue.value);
                              DbgLog("6: %s", buf);

                          }
                          //curLen += snprintf(buf+curLen, bufLen-curLen, "}");
                          if (nvpList.nvp) clHeapFree(nvpList.nvp);
                      }              
                  }
                  if (csiList.entity) clHeapFree(csiList.entity);
              }
              curLen += snprintf(buf+curLen, bufLen-curLen, "}");
          
          }
          if (siList.entityRef) clHeapFree(siList.entityRef);
      }      
      curLen += snprintf(buf+curLen, bufLen-curLen, "}");
  }

  curLen += snprintf(buf+curLen, bufLen-curLen, "}");
  if (suList.entity) clHeapFree(suList.entity);

  clAppLog(CL_LOG_HANDLE_APP, CL_LOG_SEV_INFO, 10, CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,
           "AmfDump: %s", buf);
  PyObject* ret = PyRun_String(buf,Py_eval_input,emptyDict,emptyDict);
  return ret;
}

static PyObject* GetAllNodes(PyObject *self, PyObject *args)
{
  ClRcT rc;
  ClAmsEntityBufferT nodeList;
  char buf[16*1024];
  const int bufLen = sizeof(buf);
  int curLen = 0;
  unsigned int nodeit;
  
  ClAmsMgmtHandleT hdl = amsHandle;
  if (!hdl)
    {
      PyErr_SetString(PyExc_SystemError,"AMF Handle not initialized");
      return NULL;
    }

  nodeList.entity = NULL;
  rc = clAmsMgmtGetNodeList(hdl, &nodeList);
  if (rc != CL_OK)
    {
      sprintf(buf,"clAmsMgmtGetNodeList error 0x%x",rc);
      PyObject* errData = Py_BuildValue("is",rc,buf);
      PyErr_SetObject(PyExc_SystemError,errData);
      //PyErr_SetString(PyExc_SystemError,buf);
      return NULL;
    }

  curLen += snprintf(buf+curLen, bufLen-curLen, "[");

  for (nodeit = 0; nodeit < nodeList.count; nodeit++)
    {
        ClAmsEntityT* node = &nodeList.entity[nodeit];
        curLen += snprintf(buf+curLen, bufLen-curLen, "%s'%.*s'",(nodeit==0) ? "":",", node->name.length,node->name.value);
    }

  curLen += snprintf(buf+curLen, bufLen-curLen, "]");

  if (nodeList.entity) clHeapFree(nodeList.entity);
  PyObject* ret = PyRun_String(buf,Py_eval_input,emptyDict,emptyDict);
  return ret;  
}

#if 0
static PyObject* GetAmsEntityList(PyObject *self, PyObject *args)
{
  ClAmsEntityListT *lst = (ClAmsEntityListT *) 0 ;
  void *argp1 = 0 ;

  if (!PyArg_ParseTuple(args,(char *)"O:GetAmsEntityListT",&obj0)) return NULL;
  res1 = SWIG_ConvertPtr(obj0, &argp1,SWIGTYPE_p_ClAmsEntityListT, 0 |  0 );
  if (!SWIG_IsOK(res1))
  {
    SWIG_exception_fail(SWIG_ArgError(res1), "in method '" "GetAmsEntityList" "', argument " "1"" of type '" "ClAmsEntityListT *""'"); 
  }
  lst = (ClAmsEntityListT *)(argp1);

  char buf[2048];
  const int bufLen = 2048;
  int curLen = 0;
  
  if (lst->isValid == 0)
  {
      sprintf(buf,"List is not valid");
      PyErr_SetString(PyExc_SystemError,buf);
      return NULL; /* Oops, list is invalid */
  }
  

  ClCntNodeHandleT  nodeHandle = -1;

  curLen += snprintf(buf+curLen, bufLen-curLen, "[");

  for (int count=0, entityRef = (ClAmsEntityRefT *) clAmsCntGetFirst( &entityList->list,&nodeHandle);
       entityRef != NULL; 
       count++, entityRef = (ClAmsEntityRefT *) clAmsCntGetNext( &entityList->list,&nodeHandle) )
  {
      AMS_CHECKPTR_AND_EXIT (!entityRef);        
      curLen += snprintf(buf+curLen, bufLen-curLen, "%s(%d,'%.*s')",(count==0) ? "":",", entityRef->entity.type,entityRef->entity.name.length,entityRef->entity.name.value);
  }

  curLen += snprintf(buf+curLen, bufLen-curLen, "]");
  
  PyObject* ret = PyRun_String(buf,Py_eval_input,emptyDict,emptyDict);
  return ret;
}
#endif


static PyObject* GetConfiguredEntities(PyObject *self, PyObject *args)
{
  ClRcT rc;
  ClAmsEntityBufferT entList;
  char buf[128*1024];
  const int bufLen = sizeof(buf);
  int curLen = 0;
  int i,j;
  unsigned int entit;
  int parg[CL_AMS_ENTITY_TYPE_CLUSTER+1]={0};

  if (!PyArg_ParseTuple(args, "|iiiiiii", &parg[0],&parg[1],&parg[2],&parg[3],&parg[4],&parg[5],&parg[6]))
  {
  }

  if (parg[0] == 0) /* No args passed, so get all entities */
  {
      for (i = CL_AMS_ENTITY_TYPE_NODE,j=0; i< CL_AMS_ENTITY_TYPE_CLUSTER; i++,j++)
      {
          parg[j] = i;          
      }      
  }

  /* Verify that they are in range */
  for (i = CL_AMS_ENTITY_TYPE_NODE; i< CL_AMS_ENTITY_TYPE_CLUSTER; i++)
      {
          if ((parg[i] < CL_AMS_ENTITY_TYPE_ENTITY) && (parg[i] > CL_AMS_ENTITY_TYPE_CLUSTER)) parg[i] = 0;
      }      

  
  ClAmsMgmtHandleT hdl = amsHandle;
  if (!hdl)
  {
      PyErr_SetString(PyExc_SystemError,"AMF Handle not initialized");
      return NULL;
  }


  int e2l[CL_AMS_ENTITY_TYPE_CLUSTER+1];  /* entity to list translation */
  e2l[CL_AMS_ENTITY_TYPE_ENTITY] = 0;
  e2l[CL_AMS_ENTITY_TYPE_NODE] = CL_AMS_NODE_LIST;
  e2l[CL_AMS_ENTITY_TYPE_APP] = 0;
  e2l[CL_AMS_ENTITY_TYPE_SG] = CL_AMS_SG_LIST;
  e2l[CL_AMS_ENTITY_TYPE_SU] = CL_AMS_SU_LIST;
  e2l[CL_AMS_ENTITY_TYPE_SI] = CL_AMS_SI_LIST;
  e2l[CL_AMS_ENTITY_TYPE_COMP] = CL_AMS_COMP_LIST;
  e2l[CL_AMS_ENTITY_TYPE_CSI] = CL_AMS_CSI_LIST;
  e2l[CL_AMS_ENTITY_TYPE_CLUSTER] = 0;

  curLen += snprintf(buf+curLen, bufLen-curLen, "[");

  int count = 0;
  
  for (i = 0; i<7;i++)
  {
      int listType = e2l[parg[i]];
      if (listType>0)
      {
          entList.entity = NULL;
          rc = clAmsMgmtGetList(hdl, (ClAmsEntityListTypeT) listType, &entList);
          
          if (rc != CL_OK)
          {
              sprintf(buf,"clAmsMgmtGetNodeList error 0x%x",rc);
              //PyErr_SetString(PyExc_SystemError,buf);
              PyObject* errData = Py_BuildValue("is",rc,buf);
              PyErr_SetObject(PyExc_SystemError,errData);
              return NULL;
          }
          
          for (entit = 0; entit < entList.count; entit++,count++)
          {
              ClAmsEntityT* ent = &entList.entity[entit];
              curLen += snprintf(buf+curLen, bufLen-curLen, "%s(%d,'%.*s')",(count==0) ? "":",", parg[i],ent->name.length,ent->name.value);
          }
          if (entList.entity) clHeapFree(entList.entity);
  
      }
  }
  
  curLen += snprintf(buf+curLen, bufLen-curLen, "]");
  
  PyObject* ret = PyRun_String(buf,Py_eval_input,emptyDict,emptyDict);
  return ret;  
}






static PyObject* GetRunningNodeList(PyObject *self, PyObject *args)
{
  char buf[16*1024];
  const int bufLen = sizeof(buf);
  int curLen = 0;
  ClIocNodeAddressT nodeList[CL_IOC_MAX_NODES];
  unsigned int numNodes = CL_IOC_MAX_NODES;
  ClRcT rc;
  unsigned int i;

  for (i=0;i<CL_IOC_MAX_NODES;i++)
    nodeList[i] = i;

  if ((rc = clIocNeighborListGet(&numNodes, nodeList)) != CL_OK)
    {
     char c[100];
     snprintf(c,100,"Error [0x%x] getting nodes", rc);
     //PyErr_SetString(PyExc_SystemError,c);
     PyObject* errData = Py_BuildValue("is",rc,buf);
     PyErr_SetObject(PyExc_SystemError,errData);
     return NULL;
    }
  DbgLog("IocNeighbors: returned: 0x%x Number of Nodes: %d  nodelist: %d %d", rc, numNodes, nodeList[0],nodeList[1]);

#if 0 /* NeighborListGet supposedly returns self */
  nodeList[0] = clIocLocalAddressGet()&CL_IOC_NODE_MASK;
  numNodes++;
#endif

  curLen += snprintf(buf+curLen, bufLen-curLen, "[ ");

  /*
   * Now get node names from ioc address using clCpmSlotGet.
   */

  char* needComma = "";
  for(i = 0; i < numNodes; ++i)
    {
    ClCpmSlotInfoT slotInfo;
    slotInfo.slotId = nodeList[i];
    rc = clCpmSlotGet(CL_CPM_SLOT_ID, &slotInfo);
    if(rc == CL_OK)
      {
          DbgLog("rc: %x, 'slot': %d, 'name': '%.*s'", rc,slotInfo.slotId, slotInfo.nodeName.length, slotInfo.nodeName.value);
       curLen += snprintf(buf+curLen, bufLen-curLen, "%s{ 'slot': %d, 'name': '%.*s' }", needComma,slotInfo.slotId, slotInfo.nodeName.length, slotInfo.nodeName.value);
       needComma = ",";
      }
    else
      {
        clAppLog(CL_LOG_HANDLE_APP, CL_LOG_SEV_ERROR, 10, CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,
           "Cannot get slot information for slot [%d], Error [0x%x]", nodeList[i], rc);
      }
    }

  curLen += snprintf(buf+curLen, bufLen-curLen, "]");

  
  DbgLog("NumNodes=%d, Slot List=%s", numNodes,buf);
  PyObject* ret = PyRun_String(buf,Py_eval_input,emptyDict,emptyDict);
  return ret;
}


static PyObject* GetMasterNode(PyObject *self)
{
    ClIocNodeAddressT nodeAddr=0;
    ClRcT   rc = CL_OK;
    char buf[256] = "";
    

    rc = clCpmMasterAddressGet(&nodeAddr);
    if (rc != CL_OK) {
        DbgLog("Error while getting master address");
        return NULL;
    }

    snprintf(buf,256,"%d",nodeAddr);
    
    PyObject* ret = PyRun_String(buf,Py_eval_input,emptyDict,emptyDict);
    
    return ret;
}

static PyMethodDef PyAspMethods[] = {
    /*    {"GetAmsEntityList", GetAmsEntityList, METH_VARARGS, "Pass an ams entity list from SWIG, and returns a Python list"}, */
    {"GetRunningNodeList",  GetRunningNodeList, METH_VARARGS, "Get nodes that are up.  No arguments"},
    {"GetWorkOnNode",  GetWorkOnNode, METH_VARARGS, "Get AMF database for a particular node.  Pass the string name of the node."},
    {"GetAllNodes",  GetAllNodes, METH_VARARGS, "Get list of names of all configured nodes. No Arguments"},
    {"GetConfiguredEntities",  GetConfiguredEntities, METH_VARARGS, "Get list of names of configured entities.  Pass the entity types (as integers) that you want to get."},
    {"GetWorkOnServiceGroup",  GetWorkOnServiceGroup, METH_VARARGS, "Get AMF database for a particular Service Group instance.  Pass the string name of the desired service group."},
    {"GetMasterNode", GetMasterNode, METH_VARARGS, "Get the master node in the cluster"},
    {NULL, NULL, 0, NULL}        /* Sentinel */
};

PyMODINIT_FUNC
initasppycustom(void)
{
    ClVersionT  version = {'B', 01, 01 };
    ClRcT rc;
    //logfile = fopen("custom.log","a+");
    emptyDict = PyDict_New();

    rc= clAmsMgmtInitialize(&amsHandle, NULL,&version);
    if (rc != CL_OK)
    {
        printf("AMS Mgmt initialize failed rc: %x\n", rc);
    }
    
    
    (void) Py_InitModule("asppycustom", PyAspMethods);
}
