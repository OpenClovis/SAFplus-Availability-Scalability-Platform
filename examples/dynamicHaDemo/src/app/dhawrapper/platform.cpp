#include "platform.h"
#include <clAmsMgmtClientApi.h>
#include <clAmsErrors.h>
#include <clCpmApi.h>
#include <clCpmExtApi.h>
#include <clHandleApi.h>
#include <assert.h>
#include <clCommon.h>
#include <saAmf.h>
#include <string.h>
#include <clCmApi.h>

//#include <clCmDefs.h>

#define DBG(x) do { x; } while(0)

#define CSI_TYPE_APP "App_CSITYPE"

#define APP_CNT_CMD "dummyComp"

#define UT

static void entityStop(ClAmsEntityConfigT& ent);
static void entityStart(ClAmsEntityConfigT& ent);

enum
  {
    SevError = 1,
    SevWarn  = 2,
    SevInfo  = 3
  };

#define Log(severity,...) do { printf(__VA_ARGS__); printf("\n"); } while(0)

ClAmsMgmtHandleT    mgmtHandle   = CL_HANDLE_INVALID_VALUE;
ClAmsMgmtCCBHandleT ccbHandle    = CL_HANDLE_INVALID_VALUE;

extern SaAmfHandleT        amfHandle;

static ClVersionT amfApiVersion = {'B', 0x1, 0x1 };


static void checkError(const char* operation, ClRcT rc)
{
  if (rc != 0)
    {
      Log(SevError,"%s: Error 0x%x", operation, rc);
      if ((CL_GET_ERROR_CODE(rc) != CL_ERR_DUPLICATE)&&(CL_GET_ERROR_CODE(rc) != CL_ERR_ALREADY_EXIST))
        {
          throw AmfException(rc);
        }
    }
}

static void initHandles(void)
{
    ClRcT rc;
    rc = clAmsMgmtInitialize(&mgmtHandle, NULL, &amfApiVersion);
    if(rc != CL_OK)
    {
        checkError("AMS mgmt handle init",rc);
    }    
    if ((rc = clAmsMgmtCCBInitialize(mgmtHandle, &ccbHandle)) != CL_OK)
    {
        checkError("AMS mgmt config change handle init",rc);
    }

    //  Create the CSI type by creating an unused CSI.
    
    ClAmsEntityConfigT csi;
    memset(&csi,0,sizeof(csi));
    saNameSet(&csi.name,"unusedCSI");
    csi.type = CL_AMS_ENTITY_TYPE_CSI;
    
    if ((rc = clAmsMgmtCCBEntityCreate(ccbHandle,&csi)) != CL_OK)
    {
        checkError("Create CSI",rc);
    }

    if ((rc = clAmsMgmtCCBCommit(ccbHandle)) != CL_OK)
    {
        checkError("Commit SI/CSI creation", rc);
    }

    if(rc == CL_OK)
    {
        ClAmsEntityConfigT *pEntityConfig = NULL;
        ClUint64T changeMask = 0;
        ClAmsCSIConfigT csiConfig;

        // Grab the CSI object
        if ((rc = clAmsMgmtEntityGetConfig(mgmtHandle,&csi,&pEntityConfig)) != CL_OK)
        {
            checkError("Get CSI configuration", rc);
        }
      
        memcpy(&csiConfig, pEntityConfig, sizeof(csiConfig));
        clHeapFree(pEntityConfig);

        // Specify a unique CSI type
        changeMask |= CSI_CONFIG_TYPE;
        saNameSet(&csiConfig.type, CSI_TYPE_APP);
        csiConfig.type.length += 1;

        if ((rc = clAmsMgmtCCBEntitySetConfig(ccbHandle,&csiConfig.entity,changeMask)) != CL_OK)
        {
            checkError("Set CSI type", rc);
        }
      
        // Commit CSI
        if ((rc = clAmsMgmtCCBCommit(ccbHandle)) != CL_OK)
        {
            checkError("Commit CSI",rc);
        }      
      
    }

  
}

static void saNameAppend(SaNameT* name, const char* suffix)
{
  if (!suffix) return; // Nothing to append
  
  int temp = strlen(suffix);
  int amt2cpy = ((CL_MAX_NAME_LENGTH-name->length) < temp) ? (CL_MAX_NAME_LENGTH-name->length) : temp;
    
  strncpy(&name->value[name->length],suffix,CL_MAX_NAME_LENGTH-name->length);
  name->length += amt2cpy;
  assert(name->length<CL_MAX_NAME_LENGTH-1); // -1 saves room for the null term.
  name->value[name->length]=0;
  
}


static void initEntity(ClAmsEntityConfigT* entity, const char* basename, ClAmsEntityTypeT type,const char* extra=NULL)
{
  //ClRcT rc;
   memset(entity,0,sizeof(*entity));
   saNameSet(&entity->name,basename);
   if (extra)
     {
       saNameAppend(&entity->name,extra);
     }
   entity->type = type;
   switch(type)
     {
     case CL_AMS_ENTITY_TYPE_NODE:  // Node has no name modifications
       break;
     case CL_AMS_ENTITY_TYPE_SG:       
       // SGs for now are the exact name passed.  Otherwise we could: saNameAppend(&entity->name,"_SG");
       break;
     case CL_AMS_ENTITY_TYPE_SU:
       saNameAppend(&entity->name,"_SU");
       break;
     case CL_AMS_ENTITY_TYPE_SI:
       saNameAppend(&entity->name,"_SI");
       break;
     case CL_AMS_ENTITY_TYPE_COMP:
       // As per Jim's request, COMPs are the exact name passed, not: saNameAppend(&entity->name,"_COMP");
       break;
     case CL_AMS_ENTITY_TYPE_CSI:
       // As per Jim's request, CSIs are exact name... saNameAppend(&entity->name,"_CSI");
       break;
     case CL_AMS_ENTITY_TYPE_ENTITY:
     case CL_AMS_ENTITY_TYPE_APP:
     case CL_AMS_ENTITY_TYPE_CLUSTER:
     default:
       assert(0);
       break;
     }
}


void removeNode(const char* safName)
{
  ClRcT rc;
  ClAmsEntityConfigT entity;

  if (ccbHandle==CL_HANDLE_INVALID_VALUE) initHandles();
  initEntity(&entity,safName,CL_AMS_ENTITY_TYPE_NODE);

  entityStop(entity);

  ClCpmSlotInfoT slotInfo;  
  saNameSet(&slotInfo.nodeName,safName);
  if ((rc = clCpmSlotGet(CL_CPM_NODENAME, &slotInfo))  != CL_OK) checkError("Get node address",rc);
  
  if ((rc = clCpmNodeShutDown(slotInfo.nodeIocAddress))  != CL_OK) checkError("shutdown ASP on node",rc);

  ClAmsNodeStatusT* ns;

  bool done = false;
  do
    {
      ns = clAmsMgmtNodeGetStatus( mgmtHandle ,safName);
      sleep(1);
      if (!ns) done = true;
      else
        {
          if (ns->isClusterMember == CL_AMS_NODE_IS_NOT_CLUSTER_MEMBER) done=true;
          clHeapFree(ns);
        }
    } while (!done);

  if ((rc = clAmsMgmtCCBEntityDelete(ccbHandle,&entity))  != CL_OK) checkError("Delete node",rc);      
  if ((rc = clAmsMgmtCCBCommit(ccbHandle) ) != CL_OK) checkError("Committing deletion of node", rc);
}


// Create a new Node in the AMF
void addNode(const char* safName)
{
  ClRcT rc;
  ClAmsEntityConfigT entity;

  if (ccbHandle==CL_HANDLE_INVALID_VALUE) initHandles();
  initEntity(&entity,safName,CL_AMS_ENTITY_TYPE_NODE);
  

  if ((rc = clAmsMgmtCCBEntityCreate(ccbHandle,&entity)) != CL_OK)
    {
      checkError("Create node",rc);
    }
  
  
  if ((rc = clAmsMgmtCCBCommit(ccbHandle)) != CL_OK)
    {
      checkError("Commit create node",rc);
    }

  entityStart(entity);  
}

// Create a new application container in the AMF
void addAppCnt(const char* safName,SafConfig* cfg)
{
  ClRcT rc;
  ClAmsEntityConfigT sg;

  // Create the SG object
  if (ccbHandle==CL_HANDLE_INVALID_VALUE) initHandles();
  initEntity(&sg,safName,CL_AMS_ENTITY_TYPE_SG);

  if ((rc = clAmsMgmtCCBEntityCreate(ccbHandle,&sg)) != CL_OK)
    {
      checkError("Create SG",rc);      
    }

  if ((rc = clAmsMgmtCCBCommit(ccbHandle)) != CL_OK)
    {
      checkError("Commit create SG",rc);      
    }

  // Now configure it -- for now just use the defaults
  if (1)
    {
      ClAmsSGConfigT      sgConfig;
      ClAmsEntityConfigT* pEntityConfig = NULL;
      ClUint64T           changeMask = 0;
      
      if ((rc = clAmsMgmtEntityGetConfig(mgmtHandle,&sg,&pEntityConfig)) != CL_OK)
        {
        checkError("Get SG config",rc);      
        }

      memcpy(&sgConfig, pEntityConfig, sizeof(sgConfig));
      clHeapFree(pEntityConfig);

      changeMask |= SG_CONFIG_REDUNDANCY_MODEL | SG_CONFIG_NUM_PREF_ACTIVE_SUS
        | SG_CONFIG_NUM_PREF_STANDBY_SUS | SG_CONFIG_NUM_PREF_INSERVICE_SUS
        | SG_CONFIG_MAX_ACTIVE_SIS_PER_SU | SG_CONFIG_MAX_STANDBY_SIS_PER_SU
        | SG_CONFIG_INSTANTIATE_DURATION;

      sgConfig.redundancyModel     = CL_AMS_SG_REDUNDANCY_MODEL_TWO_N;
      sgConfig.numPrefActiveSUs    = 1;  // Just a very large number, so the check does not occur in the custom model
      sgConfig.numPrefStandbySUs   = 1;  
      sgConfig.numPrefInserviceSUs = 2;
      sgConfig.maxActiveSIsPerSU   = 1;
      sgConfig.maxStandbySIsPerSU  = 1;
      sgConfig.instantiateDuration = 1000;
      
      //Recovery policy
      if (cfg && cfg->compRestartCountMax > 0 && cfg->compRestartDuration > 0)
        {
          sgConfig.compRestartCountMax = cfg->compRestartCountMax;
          sgConfig.compRestartDuration = cfg->compRestartDuration;	  
          changeMask |= SG_CONFIG_COMP_RESTART_DURATION | SG_CONFIG_COMP_RESTART_COUNT_MAX;
        }
      
      if ((rc = clAmsMgmtCCBEntitySetConfig(ccbHandle,&sgConfig.entity,changeMask) ) != CL_OK)
        {
        checkError("Set SG config", rc);
        }
      
      
      if ((rc = clAmsMgmtCCBCommit(ccbHandle) ) != CL_OK)
        {
        checkError("Commit change SG config",rc);
        }
    }

  // Add the scheduler application in.
  // In particular this defines the CSI TYPE in the SG so it will work when used during component addition.
  //acAddApp(safName,"scheduler","");
 
}

void removeAppCnt(const char* name)
{
  ClRcT rc;
  ClAmsEntityConfigT sg;

  // Create the SG object
  if (ccbHandle==CL_HANDLE_INVALID_VALUE) initHandles();
  initEntity(&sg,name,CL_AMS_ENTITY_TYPE_SG);
  entityStop(sg);
    
  if ((rc = clAmsMgmtCCBEntityDelete(ccbHandle,&sg))  != CL_OK) checkError("Delete SG",rc);
  if ((rc = clAmsMgmtCCBCommit(ccbHandle) ) != CL_OK) checkError("Committing deletion of SG", rc);
}

void suStart(const char* name)
{
  ClAmsEntityConfigT entity;
  initEntity(&entity,name,CL_AMS_ENTITY_TYPE_SU);  
  entityStart(entity);
}

void suStop(const char* name)
{
  ClAmsEntityConfigT entity;
  initEntity(&entity,name,CL_AMS_ENTITY_TYPE_SU);    
  entityStop(entity);  
}

static void entityStart(ClAmsEntityConfigT& ent)
{
    ClRcT rc;

  do
    {
    rc = clAmsMgmtEntityUnlock(mgmtHandle,&ent);
    } while(CL_GET_ERROR_CODE(rc) == CL_ERR_TRY_AGAIN);

  // Already running so nothing to do
  if (CL_GET_ERROR_CODE(rc) == CL_ERR_NO_OP) return;

  do
    {
    rc = clAmsMgmtEntityLockAssignment(mgmtHandle,&ent);
    } while(CL_GET_ERROR_CODE(rc) == CL_ERR_TRY_AGAIN);

  do
    {
    rc = clAmsMgmtEntityUnlock(mgmtHandle,&ent);
    } while(CL_GET_ERROR_CODE(rc) == CL_ERR_TRY_AGAIN);

  if ((rc)&&(CL_GET_ERROR_CODE(rc) != CL_ERR_NO_OP))
    {
      checkError("Start entity", rc);
    }
  
}

static void entityStop(ClAmsEntityConfigT& ent)
{
    ClRcT rc = CL_OK;

    if(ent.type == CL_AMS_ENTITY_TYPE_CSI) return;

    if(ent.type != CL_AMS_ENTITY_TYPE_SI)
    {
        do
        {
            rc = clAmsMgmtEntityLockInstantiation(mgmtHandle,&ent);
        } while(CL_GET_ERROR_CODE(rc) == CL_ERR_TRY_AGAIN);
    }

    // Already shut down so nothing to do
    if (CL_GET_ERROR_CODE(rc) == CL_ERR_NO_OP) return;

    do
    {
        rc = clAmsMgmtEntityLockAssignment(mgmtHandle,&ent);
    } while(CL_GET_ERROR_CODE(rc) == CL_ERR_TRY_AGAIN);

    if(ent.type != CL_AMS_ENTITY_TYPE_SI)
    {
        do
        {
            rc = clAmsMgmtEntityLockInstantiation(mgmtHandle,&ent);
        } while(CL_GET_ERROR_CODE(rc) == CL_ERR_TRY_AGAIN);
    }

    if (rc != CL_OK && 
        CL_GET_ERROR_CODE(rc) != CL_ERR_NO_OP &&
        CL_GET_ERROR_CODE(rc) != CL_AMS_ERR_INVALID_ENTITY)
    {
        checkError("Shutdown entity",rc);
    }
    
}

void nodeStart(const char* nodeName)
{
  ClAmsEntityConfigT n;

  initEntity(&n,nodeName,CL_AMS_ENTITY_TYPE_NODE);  
  entityStart(n);
}

void nodeStop(const char* nodeName)
{
  ClAmsEntityConfigT n;

  initEntity(&n,nodeName,CL_AMS_ENTITY_TYPE_NODE);  
  entityStop(n);  
}

void acStart(const char* name)
{
  ClAmsEntityConfigT n;

  initEntity(&n,name,CL_AMS_ENTITY_TYPE_SG);  
  entityStart(n);
}

void acStop(const char* name)
{
  ClAmsEntityConfigT n;

  initEntity(&n,name,CL_AMS_ENTITY_TYPE_SG);  
  entityStop(n);  
}


// Start this app container running on these nodes (1+N redundancy).  This will
// start a SAF-aware process on each node specified.
void acExtend(const char* appCnt, const char* nodeNames[], int numNodes,const char* nameModifier,SafConfig* cfg)
{
  ClRcT rc;
  ClAmsEntityConfigT  sg;
  ClAmsSGConfigT      sgConfig;
  ClAmsEntityConfigT* pEntityConfig = NULL;
  ClUint64T           changeMask = 0;
  char eName[SA_MAX_NAME_LENGTH];

  if (ccbHandle==CL_HANDLE_INVALID_VALUE) initHandles();
  initEntity(&sg,appCnt,CL_AMS_ENTITY_TYPE_SG);

  strncpy(eName,appCnt,SA_MAX_NAME_LENGTH-1);
  eName[SA_MAX_NAME_LENGTH-1] = 0;
  if (nameModifier != NULL)
    {
      strncat(eName,nameModifier,SA_MAX_NAME_LENGTH-1);
      eName[SA_MAX_NAME_LENGTH-1] = 0;
    }
  
    
  if ((rc = clAmsMgmtEntityGetConfig(mgmtHandle,&sg,&pEntityConfig)) != CL_OK)
    {
      checkError("Get SG configuration", rc);
    }

  memcpy(&sgConfig, pEntityConfig, sizeof(sgConfig));
  clHeapFree(pEntityConfig);
  //Recovery policy
  if (cfg && cfg->compRestartCountMax > 0 && cfg->compRestartDuration > 0)
    {
      sgConfig.compRestartCountMax = cfg->compRestartCountMax;
      sgConfig.compRestartDuration = cfg->compRestartDuration;	  
      changeMask |= SG_CONFIG_COMP_RESTART_DURATION | SG_CONFIG_COMP_RESTART_COUNT_MAX;
      if ((rc = clAmsMgmtCCBEntitySetConfig(ccbHandle,&sgConfig.entity,changeMask) ) != CL_OK)
        {
          checkError("Set SG config", rc);
        }
    }
  
  // Create SU and component per node
  for (int i=0;i<numNodes;i++)
    {
      ClAmsEntityConfigT ent;
      initEntity(&ent,eName,CL_AMS_ENTITY_TYPE_SU,nodeNames[i]);
      if ((rc = clAmsMgmtCCBEntityCreate(ccbHandle,&ent)) != CL_OK)
        {
          checkError("Create SU", rc);
        }
      
      initEntity(&ent,eName,CL_AMS_ENTITY_TYPE_COMP,nodeNames[i]);
      if ((rc = clAmsMgmtCCBEntityCreate(ccbHandle,&ent)) != CL_OK)
        {
          checkError("Create Comp", rc);
        }     
      
    }
  
  if ((rc = clAmsMgmtCCBCommit(ccbHandle)) != CL_OK)
    {
      checkError("Commit creation of SU and Component", rc);
    }

  SaNameT            supportedCSIType;

  saNameSet(&supportedCSIType, CSI_TYPE_APP);
  supportedCSIType.length += 1;

  // Configure components
  for (int i=0;i<numNodes;i++)
    {
      ClAmsEntityConfigT su;
      ClAmsEntityConfigT comp;
      ClAmsCompConfigT   compConfig;
      ClUint64T          bitMask = 0;
      
      initEntity(&su,eName,CL_AMS_ENTITY_TYPE_SU,nodeNames[i]);
      initEntity(&comp,eName,CL_AMS_ENTITY_TYPE_COMP,nodeNames[i]);

      if ((rc = clAmsMgmtEntityGetConfig(mgmtHandle,&comp,&pEntityConfig)) != CL_OK)
        {
          checkError("Retrieve component config", rc);
        }
      memcpy(&compConfig, pEntityConfig, sizeof(compConfig));
      clHeapFree(pEntityConfig);

      bitMask |= COMP_CONFIG_CAPABILITY_MODEL | COMP_CONFIG_TIMEOUTS;

      compConfig.capabilityModel = CL_AMS_COMP_CAP_X_ACTIVE_OR_Y_STANDBY;
      compConfig.timeouts.instantiate = 30000;
      compConfig.timeouts.terminate = 30000;
      compConfig.timeouts.cleanup = 30000;
      compConfig.timeouts.quiescingComplete = 30000;
      compConfig.timeouts.csiSet = 30000;
      compConfig.timeouts.csiRemove = 30000;
      compConfig.timeouts.instantiateDelay = 1000;
      if (!strncmp(nodeNames[i], "sc", 2))
      {    	  
    	  if (cfg && cfg->compRestartCountMax == 0)
    	  {
    		  bitMask |= COMP_CONFIG_IS_RESTARTABLE | COMP_CONFIG_RECOVERY_ON_TIMEOUT;
    		  compConfig.isRestartable = false;
    		  compConfig.recoveryOnTimeout = CL_AMS_RECOVERY_NODE_FAILOVER;	     
    	  }    		
    	  else
    	  {    			      	 
    		  bitMask  |= COMP_CONFIG_RECOVERY_ON_TIMEOUT;
    		  compConfig.recoveryOnTimeout = CL_AMS_RECOVERY_NO_RECOMMENDATION;	      	 
    	  }
      }
      else
      {
    	  if (cfg && cfg->compRestartCountMax == 0)
    	  {
    		  bitMask |= COMP_CONFIG_IS_RESTARTABLE | COMP_CONFIG_RECOVERY_ON_TIMEOUT;
    		  compConfig.isRestartable = false;
    		  compConfig.recoveryOnTimeout = CL_AMS_RECOVERY_COMP_FAILOVER;	     
    	  }    		
    	  else
    	  {    			      	 
    		  bitMask  |= COMP_CONFIG_RECOVERY_ON_TIMEOUT;
    		  compConfig.recoveryOnTimeout = CL_AMS_RECOVERY_NO_RECOMMENDATION;	      	 
    	  }
      }

      if ((rc = clAmsMgmtCCBEntitySetConfig(ccbHandle,&compConfig.entity,bitMask) ) != CL_OK)
        {
        checkError("Set component config", rc);
        }
      
      bitMask = COMP_CONFIG_INSTANTIATE_COMMAND;
      if (cfg&&cfg->binName)
        {
        snprintf(compConfig.instantiateCommand, sizeof(compConfig.instantiateCommand), cfg->binName);
        }
      else
        {
        snprintf(compConfig.instantiateCommand, sizeof(compConfig.instantiateCommand), APP_CNT_CMD);
        }
            
      if ((rc = clAmsMgmtCCBEntitySetConfig(ccbHandle,&compConfig.entity,bitMask) ) != CL_OK)
        {
        checkError("Set component instantiate command", rc);          
        }      
    }  

  if ((rc = clAmsMgmtCCBCommit(ccbHandle)) != CL_OK)
    {
      checkError("Commit configure of components", rc);
    }

    // Configure components
  for (int i=0;i<numNodes;i++)
    {
      ClAmsEntityConfigT su;
      ClAmsEntityConfigT comp;
      ClAmsCompConfigT   compConfig;
      ClUint64T          bitMask = 0;
      
      initEntity(&su,eName,CL_AMS_ENTITY_TYPE_SU,nodeNames[i]);
      initEntity(&comp,eName,CL_AMS_ENTITY_TYPE_COMP,nodeNames[i]);

      if ((rc = clAmsMgmtEntityGetConfig(mgmtHandle,&comp,&pEntityConfig)) != CL_OK)
        {
          checkError("Retrieve component config", rc);
        }
      memcpy(&compConfig, pEntityConfig, sizeof(compConfig));
      clHeapFree(pEntityConfig);

      bitMask |= COMP_CONFIG_SUPPORTED_CSI_TYPE | COMP_CONFIG_NUM_MAX_ACTIVE_CSIS | COMP_CONFIG_NUM_MAX_STANDBY_CSIS;

      if(compConfig.pSupportedCSITypes)
        clHeapFree(compConfig.pSupportedCSITypes);
            
      compConfig.numSupportedCSITypes = 1;
      compConfig.pSupportedCSITypes   = &supportedCSIType;
      compConfig.numMaxActiveCSIs     = 10000;
      compConfig.numMaxStandbyCSIs    = 10000;
      
      if ((rc = clAmsMgmtCCBEntitySetConfig(ccbHandle,&compConfig.entity,bitMask) ) != CL_OK)
        {
          checkError("Setting supported CSI types", rc);
        }            
    }  

  if ((rc = clAmsMgmtCCBCommit(ccbHandle)) != CL_OK)
    {
      checkError("Configuring components supported CSI type", rc);
    }

  
  // Configure SUs
  for (int i=0;i<numNodes;i++)
    {
      ClUint64T bitMask = 0;
      ClAmsEntityConfigT su;
      ClAmsEntityConfigT comp;
      ClAmsSUConfigT suConfig;
      initEntity(&su,eName,CL_AMS_ENTITY_TYPE_SU,nodeNames[i]);
      initEntity(&comp,eName,CL_AMS_ENTITY_TYPE_COMP,nodeNames[i]);

      if ((rc = clAmsMgmtEntityGetConfig(mgmtHandle,&su,&pEntityConfig)) != CL_OK)
        {
          checkError("Get SU", rc);
        }
      
      memcpy(&suConfig, pEntityConfig, sizeof(suConfig));
      clHeapFree(pEntityConfig);

      bitMask |= SU_CONFIG_NUM_COMPONENTS |  SU_CONFIG_ADMIN_STATE;
      // set number of components (processes) per SU to 1
      suConfig.numComponents = 1;
      suConfig.adminState = CL_AMS_ADMIN_STATE_LOCKED_I;
      
      if ((rc = clAmsMgmtCCBEntitySetConfig(ccbHandle,&suConfig.entity,bitMask)) != CL_OK)
        {
           checkError("Configure SU", rc);
         
        }
      // attach the comp to the su
      if ((rc = clAmsMgmtCCBSetSUCompList(ccbHandle,&su,&comp)) != CL_OK)
        {
          checkError("Link COMP to SU", rc);          
        }      
    }

  if ((rc = clAmsMgmtCCBCommit(ccbHandle)) != CL_OK)
    {
      checkError("Commit SU configuration", rc);          
    }

  
  // Add the SU to SG's and Node's list
  for (int i=0;i<numNodes;i++)
    {
      ClAmsEntityConfigT su;
      ClAmsEntityConfigT node;
      initEntity(&su,eName,CL_AMS_ENTITY_TYPE_SU,nodeNames[i]);
      initEntity(&node,nodeNames[i],CL_AMS_ENTITY_TYPE_NODE);
      
      if ((rc = clAmsMgmtCCBSetSGSUList(ccbHandle,&sg,&su)) != CL_OK)
        {
          checkError("Add SU to SG", rc);
        }

      if ((rc = clAmsMgmtCCBSetNodeSUList(ccbHandle,&node,&su)) != CL_OK)
        {
          checkError("Add SU to Node", rc);
        }
    }
  
  if ((rc = clAmsMgmtCCBCommit(ccbHandle) ) != CL_OK)
    {
      checkError("Committing addition of SU to node and SG", rc);
    }

  // Now that all is configured, start them up
  for (int i=0;i<numNodes;i++)
    {
      ClAmsEntityConfigT su;      
      initEntity(&su,eName,CL_AMS_ENTITY_TYPE_SU,nodeNames[i]);
      entityStart(su);
    }
  
}


// Start this app container running on these nodes (1+N redundancy).  This will
// start a SAF-aware process on each node specified.
// Create SU and Comp
void acExtend(const char* appCnt, const char* nodeName, const char* compName,SafConfig* cfg)
{
  ClRcT rc;
  ClAmsEntityConfigT  sg;
  ClAmsEntityConfigT  comp;
  ClAmsEntityConfigT su;
  ClUint64T           changeMask = 0;
  
  ClAmsSGConfigT      sgConfig;
  ClAmsEntityConfigT* pEntityConfig = NULL;

  if (ccbHandle==CL_HANDLE_INVALID_VALUE) initHandles();
  initEntity(&sg,appCnt,CL_AMS_ENTITY_TYPE_SG);
  
    
  if ((rc = clAmsMgmtEntityGetConfig(mgmtHandle,&sg,&pEntityConfig)) != CL_OK)
    {
      checkError("Get SG configuration", rc);
    }

  memcpy(&sgConfig, pEntityConfig, sizeof(sgConfig));
  clHeapFree(pEntityConfig);

  //Fix bug: recovery policy
  if (cfg && cfg->compRestartCountMax > 0 && cfg->compRestartDuration > 0)
    {
      sgConfig.compRestartCountMax = cfg->compRestartCountMax;
      sgConfig.compRestartDuration = cfg->compRestartDuration;
      changeMask |= SG_CONFIG_COMP_RESTART_DURATION | SG_CONFIG_COMP_RESTART_COUNT_MAX;
      if ((rc = clAmsMgmtCCBEntitySetConfig(ccbHandle,&sgConfig.entity,changeMask) ) != CL_OK)
        {
          checkError("Set SG config", rc);
        }
    }
  
  // Create SU and component per node
  if (1)
    {
      initEntity(&su,compName,CL_AMS_ENTITY_TYPE_SU);
      if ((rc = clAmsMgmtCCBEntityCreate(ccbHandle,&su)) != CL_OK)
        {
          checkError("Create SU", rc);
        }
      
      initEntity(&comp,compName,CL_AMS_ENTITY_TYPE_COMP);
      if ((rc = clAmsMgmtCCBEntityCreate(ccbHandle,&comp)) != CL_OK)
        {
          checkError("Create Comp", rc);
        }     
      
    }
  
  if ((rc = clAmsMgmtCCBCommit(ccbHandle)) != CL_OK)
    {
      checkError("Commit creation of SU and Component", rc);
    }

  SaNameT            supportedCSIType;

  saNameSet(&supportedCSIType, CSI_TYPE_APP);
  supportedCSIType.length += 1;

  // Configure components
  if (1)
    {
      ClAmsCompConfigT   compConfig;
      ClUint64T          bitMask = 0;
      
      if ((rc = clAmsMgmtEntityGetConfig(mgmtHandle,&comp,&pEntityConfig)) != CL_OK)
        {
          checkError("Retrieve component config", rc);
        }
      memcpy(&compConfig, pEntityConfig, sizeof(compConfig));
      clHeapFree(pEntityConfig);

      bitMask |= COMP_CONFIG_CAPABILITY_MODEL | COMP_CONFIG_TIMEOUTS;

      compConfig.capabilityModel = CL_AMS_COMP_CAP_X_ACTIVE_OR_Y_STANDBY;
      compConfig.timeouts.instantiate = 30000;
      compConfig.timeouts.terminate = 30000;
      compConfig.timeouts.cleanup = 30000;
      compConfig.timeouts.quiescingComplete = 30000;
      compConfig.timeouts.csiSet = 30000;
      compConfig.timeouts.csiRemove = 30000;
      compConfig.timeouts.instantiateDelay = 1000;
      if (!strncmp(nodeName, "sc", 2))
        {    	  
      	  if (cfg && cfg->compRestartCountMax == 0)
      	  {
      		  bitMask |= COMP_CONFIG_IS_RESTARTABLE | COMP_CONFIG_RECOVERY_ON_TIMEOUT;
      		  compConfig.isRestartable = false;
      		  compConfig.recoveryOnTimeout = CL_AMS_RECOVERY_NODE_FAILOVER;	     
      	  }    		
      	  else
      	  {    			      	 
      		  bitMask  |= COMP_CONFIG_RECOVERY_ON_TIMEOUT;
      		  compConfig.recoveryOnTimeout = CL_AMS_RECOVERY_NO_RECOMMENDATION;	      	 
      	  }
        }
        else
        {
      	  if (cfg && cfg->compRestartCountMax == 0)
      	  {
      		  bitMask |= COMP_CONFIG_IS_RESTARTABLE | COMP_CONFIG_RECOVERY_ON_TIMEOUT;
      		  compConfig.isRestartable = false;
      		  compConfig.recoveryOnTimeout = CL_AMS_RECOVERY_COMP_FAILOVER;	     
      	  }    		
      	  else
      	  {    			      	 
      		  bitMask  |= COMP_CONFIG_RECOVERY_ON_TIMEOUT;
      		  compConfig.recoveryOnTimeout = CL_AMS_RECOVERY_NO_RECOMMENDATION;	      	 
      	  }
        }

      if ((rc = clAmsMgmtCCBEntitySetConfig(ccbHandle,&compConfig.entity,bitMask) ) != CL_OK)
        {
        checkError("Set component config", rc);
        }
      
      bitMask = COMP_CONFIG_INSTANTIATE_COMMAND;
      if (cfg&&cfg->binName)
        {
        snprintf(compConfig.instantiateCommand, sizeof(compConfig.instantiateCommand), cfg->binName);
        }
      else
        {
        snprintf(compConfig.instantiateCommand, sizeof(compConfig.instantiateCommand), APP_CNT_CMD);
        }
            
      if ((rc = clAmsMgmtCCBEntitySetConfig(ccbHandle,&compConfig.entity,bitMask) ) != CL_OK)
        {
        checkError("Set component instantiate command", rc);          
        }      
    }  

  if ((rc = clAmsMgmtCCBCommit(ccbHandle)) != CL_OK)
    {
      checkError("Commit configure of components", rc);
    }

    // Configure components
  if (1)
    {
      ClAmsCompConfigT   compConfig;
      ClUint64T          bitMask = 0;
      

      if ((rc = clAmsMgmtEntityGetConfig(mgmtHandle,&comp,&pEntityConfig)) != CL_OK)
        {
          checkError("Retrieve component config", rc);
        }
      memcpy(&compConfig, pEntityConfig, sizeof(compConfig));
      clHeapFree(pEntityConfig);

      bitMask |= COMP_CONFIG_SUPPORTED_CSI_TYPE | COMP_CONFIG_NUM_MAX_ACTIVE_CSIS | COMP_CONFIG_NUM_MAX_STANDBY_CSIS;

      if(compConfig.pSupportedCSITypes)
        clHeapFree(compConfig.pSupportedCSITypes);
            
      compConfig.numSupportedCSITypes = 1;
      compConfig.pSupportedCSITypes   = &supportedCSIType;
      compConfig.numMaxActiveCSIs     = 10000;
      compConfig.numMaxStandbyCSIs    = 10000;
      
      if ((rc = clAmsMgmtCCBEntitySetConfig(ccbHandle,&compConfig.entity,bitMask) ) != CL_OK)
        {
          checkError("Setting supported CSI types", rc);
        }            
    }  

  if ((rc = clAmsMgmtCCBCommit(ccbHandle)) != CL_OK)
    {
      checkError("Configuring components supported CSI type", rc);
    }

  
  // Configure SUs
  if (1)
    {
      ClUint64T bitMask = 0;
      ClAmsSUConfigT suConfig;

      if ((rc = clAmsMgmtEntityGetConfig(mgmtHandle,&su,&pEntityConfig)) != CL_OK)
        {
          checkError("Get SU", rc);
        }
      
      memcpy(&suConfig, pEntityConfig, sizeof(suConfig));
      clHeapFree(pEntityConfig);

      bitMask |= SU_CONFIG_NUM_COMPONENTS |  SU_CONFIG_ADMIN_STATE;
      // set number of components (processes) per SU to 1
      suConfig.numComponents = 1;
      //suConfig.adminState = CL_AMS_ADMIN_STATE_UNLOCKED;
      suConfig.adminState = CL_AMS_ADMIN_STATE_LOCKED_I;      

      if ((rc = clAmsMgmtCCBEntitySetConfig(ccbHandle,&suConfig.entity,bitMask)) != CL_OK)
        {
           checkError("Configure SU", rc);
         
        }
      // attach the comp to the su
      if ((rc = clAmsMgmtCCBSetSUCompList(ccbHandle,&su,&comp)) != CL_OK)
        {
          checkError("Link COMP to SU", rc);          
        }      
    }

  if ((rc = clAmsMgmtCCBCommit(ccbHandle)) != CL_OK)
    {
      checkError("Commit SU configuration", rc);          
    }

  
  // Add the SU to SG's and Node's list
  if (1)
    {
      ClAmsEntityConfigT node;
      initEntity(&node,nodeName,CL_AMS_ENTITY_TYPE_NODE);

      if ((rc = clAmsMgmtCCBSetSGSUList(ccbHandle,&sg,&su)) != CL_OK)
        {
          checkError("Add SU to SG", rc);
        }

      if ((rc = clAmsMgmtCCBSetNodeSUList(ccbHandle,&node,&su)) != CL_OK)
        {
          checkError("Add SU to Node", rc);
        }

      /*
      if ((rc = clAmsMgmtCCBCommit(ccbHandle) ) != CL_OK)
        {
          checkError("Committing addition of SU to node and SG", rc);
        }
      */

      if ((rc = clAmsMgmtCCBCommit(ccbHandle) ) != CL_OK)
        {
          checkError("Committing addition of SU to node and SG", rc);
        }  
    }

  // Now that all is configured, start them up
  entityStart(su);
}


    // Stop this app container from running on these nodes (1+N redundancy)
void acRetract(const char* appCnt, const char* nodeName,const char* compName)
{
  ClRcT rc;
  ClAmsEntityConfigT su;
  ClAmsEntityConfigT comp;

  if (ccbHandle==CL_HANDLE_INVALID_VALUE) initHandles();
  
  if (1)
    {
      initEntity(&su,compName,CL_AMS_ENTITY_TYPE_SU);
      entityStop(su);
      initEntity(&comp,compName,CL_AMS_ENTITY_TYPE_COMP);
      // Comps are implicitly stopped when SU is stopped
      //entityStop(comp);

      if ((rc = clAmsMgmtCCBEntityDelete(ccbHandle,&comp))  != CL_OK) checkError("Delete COMP",rc);
      if ((rc = clAmsMgmtCCBEntityDelete(ccbHandle,&su))  != CL_OK) checkError("Delete SU",rc);      
    }

  if ((rc = clAmsMgmtCCBCommit(ccbHandle) ) != CL_OK) checkError("Committing deletion of COMP/SU", rc);
}

    // Stop this app container from running on these nodes (1+N redundancy)
void acRetract(const char* appCnt, const char* nodeNames[], int numNodes,const char* nameModifier)
{
  ClRcT rc;
  ClAmsEntityConfigT su;
  ClAmsEntityConfigT comp;
  char eName[SA_MAX_NAME_LENGTH];

  if (ccbHandle==CL_HANDLE_INVALID_VALUE) initHandles();

  strncpy(eName,appCnt,SA_MAX_NAME_LENGTH-1);
  eName[SA_MAX_NAME_LENGTH-1] = 0;
  if (nameModifier != NULL)
    {
      strncat(eName,nameModifier,SA_MAX_NAME_LENGTH-1);
      eName[SA_MAX_NAME_LENGTH-1] = 0;
    }

  
  for (int i=0;i<numNodes;i++)
    {
      initEntity(&su,eName,CL_AMS_ENTITY_TYPE_SU,nodeNames[i]);
      entityStop(su);
      initEntity(&comp,eName,CL_AMS_ENTITY_TYPE_COMP,nodeNames[i]);
      // comps are implicitly started/stopped when SU is
      //entityStop(comp);

      if ((rc = clAmsMgmtCCBEntityDelete(ccbHandle,&comp))  != CL_OK) checkError("Delete COMP",rc);
      if ((rc = clAmsMgmtCCBEntityDelete(ccbHandle,&su))  != CL_OK) checkError("Delete SU",rc);      
    }

  if ((rc = clAmsMgmtCCBCommit(ccbHandle) ) != CL_OK) checkError("Committing deletion of COMP/SU", rc);
}

    // Add an application to this app container.
    // The effect of this is to create a new SAF Service Instance (SI) and CSI
    // This SI will be assigned to the SG and therefore a process running on the cluster
    // This process shall use the information in the SI to start the appropriate
    // application running.
void acAddApp(const char* appCnt,const char* appName, const char* activeXml, const char* standbyXml)
{
  ClRcT rc;
  ClAmsEntityConfigT sg;
  ClAmsEntityConfigT si;
  ClAmsEntityConfigT csi;
  
  if (ccbHandle==CL_HANDLE_INVALID_VALUE) initHandles();
  initEntity(&sg,appCnt,CL_AMS_ENTITY_TYPE_SG);
  initEntity(&si,appName,CL_AMS_ENTITY_TYPE_SI);
  initEntity(&csi,appName,CL_AMS_ENTITY_TYPE_CSI);

  // Create the new entities
  if ((rc = clAmsMgmtCCBEntityCreate(ccbHandle,&si)) != CL_OK)
    {
      checkError("Create SI",rc);
    }
  if ((rc = clAmsMgmtCCBCommit(ccbHandle)) != CL_OK)
    {
      checkError("Commit SI creation", rc);
    }
  
  if ((rc = clAmsMgmtCCBEntityCreate(ccbHandle,&csi)) != CL_OK)
    {
      checkError("Create CSI",rc);
    }

  if ((rc = clAmsMgmtCCBCommit(ccbHandle)) != CL_OK)
    {
      checkError("Commit CSI creation", rc);
    }


  // Configure the new entities
  if (1) // configure the SI
    {
      ClUint64T changeMask = 0;
      ClAmsEntityConfigT *pEntityConfig = NULL;
      ClAmsSIConfigT siConfig;

      // Grab the SI object
      if ((rc = clAmsMgmtEntityGetConfig(mgmtHandle,&si,&pEntityConfig)) != CL_OK)
        {
           checkError("Get SI config", rc);
        }

      memcpy(&siConfig, pEntityConfig, sizeof(siConfig));
      clHeapFree(pEntityConfig);

      // Configure SI state
      siConfig.numCSIs = 1;
      siConfig.numStandbyAssignments = 1; 
      changeMask |= SI_CONFIG_NUM_CSIS | SI_CONFIG_NUM_STANDBY_ASSIGNMENTS;

      if ((rc = clAmsMgmtCCBEntitySetConfig(ccbHandle,&siConfig.entity,changeMask)) != CL_OK)
        {
          checkError("Set SI config", rc);
        }

      // Link SI to the CSI
      if ((rc = clAmsMgmtCCBSetSICSIList(ccbHandle,&si,&csi)) != CL_OK)
        {
           checkError("Change SI config", rc);
        }

      // Commit SI
      if ((rc = clAmsMgmtCCBCommit(ccbHandle)) != CL_OK)
        {
           checkError("Commit SI configuration", rc);
        }      
    }

  if (1) // configure the CSI
    {
      ClAmsEntityConfigT *pEntityConfig = NULL;
      ClUint64T changeMask = 0;
      ClAmsCSIConfigT csiConfig;
      ClAmsCSINVPT nvp;

      // Grab the CSI object
      if ((rc = clAmsMgmtEntityGetConfig(mgmtHandle,&csi,&pEntityConfig)) != CL_OK)
        {
          checkError("Get CSI configuration", rc);
        }
      
      memcpy(&csiConfig, pEntityConfig, sizeof(csiConfig));
      clHeapFree(pEntityConfig);

      // Specify a unique CSI type
      changeMask |= CSI_CONFIG_TYPE;
      saNameSet(&csiConfig.type, CSI_TYPE_APP);
      csiConfig.type.length += 1;

      if ((rc = clAmsMgmtCCBEntitySetConfig(ccbHandle,&csiConfig.entity,changeMask)) != CL_OK)
        {
          checkError("Set CSI type", rc);
        }
      
      // Set the name/value pairs
      saNameSet(&nvp.paramName,"activexml");
      saNameSet(&nvp.paramValue,activeXml);
      if ((rc = clAmsMgmtCCBCSISetNVP(ccbHandle,&csi,&nvp)) != CL_OK)
        {
          checkError("Set CSI NVP", rc);
        }
      // Set the name/value pairs
      saNameSet(&nvp.paramName,"standbyxml");
      saNameSet(&nvp.paramValue,standbyXml);
      if ((rc = clAmsMgmtCCBCSISetNVP(ccbHandle,&csi,&nvp)) != CL_OK)
        {
          checkError("Set CSI NVP", rc);
        }

      // Commit CSI
      if ((rc = clAmsMgmtCCBCommit(ccbHandle)) != CL_OK)
        {
          checkError("Commit CSI",rc);
        }      
      
    }

    if (1) // Link the SI to the SG
    {
      if ((rc = clAmsMgmtCCBSetSGSIList(ccbHandle,&sg,&si)) != CL_OK)
        {
          checkError("Add SI to SG", rc);
        }
      
      if ((rc = clAmsMgmtCCBCommit(ccbHandle) ) != CL_OK)
        {
          checkError("Commit adding SI to SG", rc);
        }
    }
    /*
     * Unlock or start the SI
     */
    if((rc = clAmsMgmtEntityUnlock(mgmtHandle, &si)) != CL_OK 
       &&
       CL_GET_ERROR_CODE(rc) != CL_ERR_NO_OP)
    {
        checkError("SI unlock failed", rc);
    }
}

    // Remove an application from this container.
    // The effect of this is to delete the SAF Service Instance that represents this
    // application
void acRemoveApp(const char* appCnt, const char* appName)
{
  ClRcT rc;
  //ClAmsEntityConfigT sg;
  ClAmsEntityConfigT si;
  ClAmsEntityConfigT csi;
  
  if (ccbHandle==CL_HANDLE_INVALID_VALUE) initHandles();
  //initEntity(&sg,appCnt,CL_AMS_ENTITY_TYPE_SG);
  initEntity(&si,appName,CL_AMS_ENTITY_TYPE_SI);
  initEntity(&csi,appName,CL_AMS_ENTITY_TYPE_CSI);

  entityStop(si);
  entityStop(csi);
  
  if ((rc = clAmsMgmtCCBEntityDelete(ccbHandle,&csi))  != CL_OK) checkError("Delete CSI",rc);
  if ((rc = clAmsMgmtCCBEntityDelete(ccbHandle,&si))  != CL_OK) checkError("Delete SI",rc);      
  if ((rc = clAmsMgmtCCBCommit(ccbHandle) ) != CL_OK) checkError("Committing deletion of SI/CSI", rc);
}

void nodeFail(const char* node)
{
  SaNameT nodeName;
  strncpy((char*) nodeName.value,node,SA_MAX_NAME_LENGTH);
  nodeName.value[SA_MAX_NAME_LENGTH-1] = 0;
  nodeName.length = strlen((char*)nodeName.value);
  saAmfComponentErrorReport(amfHandle, &nodeName, 0, SA_AMF_NODE_SWITCHOVER, SA_NTF_IDENTIFIER_UNUSED);  
}

// creates sg, si/csi (work) under the sg
void createApp(const char *app, SafConfig *cfg,
               const char *activecfg, const char *standbycfg)
{
    SafConfig newCfg;
    memset(&newCfg, 0, sizeof(newCfg));
    if(!cfg)
    {
        cfg = &newCfg;
        cfg->binName = APP_CNT_CMD;
        /*
         * Restart 10 times within 10 seconds before escalation to surestart or comp failover
         */
        cfg->compRestartDuration = 10000;
        cfg->compRestartCountMax = 10; 
    }

    addAppCnt(app, cfg);

    char wrk[CL_MAX_NAME_LENGTH];
    wrk[0] = 0;
    strncat(wrk, app, sizeof(wrk)-1);
    strncat(wrk, "_Wrk", sizeof(wrk)-1);
    if(!activecfg) activecfg = "activecfg.xml";
    if(!standbycfg) standbycfg = "standbycfg.xml";

    acAddApp(app, wrk, activecfg, standbycfg);
}

// removes su/comp, si/csi (work) and sg
void removeApp(const char *app, const char *nodenames[], int numNodes)
{
    char wrk[CL_MAX_NAME_LENGTH];
    wrk[0] = 0;
    strncat(wrk, app, sizeof(wrk)-1);
    strncat(wrk, "_Wrk", sizeof(wrk)-1);
    // delete su/comp on the nodes
    acRetract(app, nodenames, numNodes);
    // remove si/csi
    acRemoveApp(app, wrk);
    //remove sg itself
    removeAppCnt(app);
}

// creates su and comps on the 2 nodes under an sg
void extendApp(const char *app, const char *nodeNames[], int numNodes, 
               const char *nameModifier, SafConfig *cfg)
{
    return acExtend(app, nodeNames, numNodes, nameModifier, cfg);
}

// creates su and comps on a given node under an sg
void extendApp(const char *app, const char *node, const char *comp, SafConfig *cfg)
{
    return acExtend(app, node, comp, cfg);
}

/*
 * SG create for 2 nodes. Also creates nodes
 */
void testCreateApp(const char *app, const char *node1, const char *node2, bool createNode)
{
    const char *nodenames[] = {node1, node2};
    if(createNode)
    {
        for(int i = 0; i < 2; ++i)
            addNode(nodenames[i]);
    }
    // create sg, si/csi
    createApp(app);
    // create su/comp on the 2 nodes inside sg
    extendApp(app, nodenames, 2);
    acStart(app);
}

void testRemoveApp(const char *app, const char *node1, const char *node2)
{
    const char *nodenames[] = {node1, node2};
    removeApp(app, nodenames, 2);
}
