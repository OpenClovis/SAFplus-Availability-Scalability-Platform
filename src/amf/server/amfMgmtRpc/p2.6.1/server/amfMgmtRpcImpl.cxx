#include "amfMgmtRpc.hxx"
#include <clMgtDatabase.hxx>
#include <SafplusAmf.hxx>
#include <SAFplusAmfModule.hxx>
#include <CapabilityModel.hxx>
#include <Recovery.hxx>
#include <AdministrativeState.hxx>
#include <clDbalBase.hxx>
#include <clHandleApi.hxx>
#include <boost/unordered_map.hpp>

#define DBAL_PLUGIN_NAME "libclSQLiteDB.so"

#define B2S(b) b?std::string("True"):std::string("False")

#define MGMT_CALL(fn)                                                    \
do {                                                                    \
    ClRcT returnCode = CL_OK;                                           \
                                                                        \
    returnCode = (fn);                                                                        \
                                                                        \
    if (returnCode != CL_OK)                                            \
    {                                                                   \
        logError("MGMT","RPC",                                  \
            "ALERT [%s:%d] : Fn [%s] returned [0x%x]\n",               \
             __FUNCTION__, __LINE__, #fn, returnCode);                 \
        return returnCode;                                              \
    }                                                                   \
} while (0)

extern SAFplus::MgtDatabase amfDb;
extern SAFplusAmf::SAFplusAmfModule cfg;

//namespace SAFplus {

typedef std::pair<const SAFplus::Handle,SAFplus::DbalPlugin*> DbalMapPair;
typedef boost::unordered_map <SAFplus::Handle, SAFplus::DbalPlugin*> DbalHashMap;
DbalHashMap amfMgmtMap;

// RPC Operation IDs
const int AMF_MGMT_OP_COMPONENT_CREATE             = 1;
const int AMF_MGMT_OP_COMPONENT_UPDATE             = 2;
const int AMF_MGMT_OP_COMPONENT_DELETE             = 3;
const int AMF_MGMT_OP_SG_CREATE                    = 4;
const int AMF_MGMT_OP_SG_UPDATE                    = 5;
const int AMF_MGMT_OP_SG_DELETE                    = 6;
const int AMF_MGMT_OP_NODE_CREATE                  = 7;
const int AMF_MGMT_OP_NODE_UPDATE                  = 8;
const int AMF_MGMT_OP_NODE_DELETE                  = 9;
const int AMF_MGMT_OP_SU_CREATE                    = 10; 
const int AMF_MGMT_OP_SU_UPDATE                    = 11;
const int AMF_MGMT_OP_SU_DELETE                    = 12;
const int AMF_MGMT_OP_SI_CREATE                    = 13;
const int AMF_MGMT_OP_SI_UPDATE                    = 14;
const int AMF_MGMT_OP_SI_DELETE                    = 15;
const int AMF_MGMT_OP_CSI_CREATE                   = 16;
const int AMF_MGMT_OP_CSI_UPDATE                   = 17;
const int AMF_MGMT_OP_CSI_DELETE                   = 18;
const int AMF_MGMT_OP_CSI_NVP_DELETE               = 19;
const int AMF_MGMT_OP_NODE_SU_LIST_DELETE          = 20;
const int AMF_MGMT_OP_SG_SU_LIST_DELETE            = 21;
const int AMF_MGMT_OP_SG_SI_LIST_DELETE            = 22;
const int AMF_MGMT_OP_SU_COMP_LIST_DELETE          = 23;
const int AMF_MGMT_OP_SI_CSI_LIST_DELETE           = 24;
const int AMF_MGMT_OP_NODE_LOCK_ASSIGNMENT         = 25;
const int AMF_MGMT_OP_SG_LOCK_ASSIGNMENT           = 26;
const int AMF_MGMT_OP_SU_LOCK_ASSIGNMENT           = 27;
const int AMF_MGMT_OP_SI_LOCK_ASSIGNMENT           = 28;
const int AMF_MGMT_OP_NODE_LOCK_INSTANTIATION      = 29;
const int AMF_MGMT_OP_SG_LOCK_INSTANTIATION        = 30;
const int AMF_MGMT_OP_SU_LOCK_INSTANTIATION        = 31;
const int AMF_MGMT_OP_NODE_UNLOCK                  = 32;
const int AMF_MGMT_OP_SG_UNLOCK                    = 33;
const int AMF_MGMT_OP_SU_UNLOCK                    = 34;
const int AMF_MGMT_OP_SI_UNLOCK                    = 35;
const int AMF_MGMT_OP_NODE_REPAIR                  = 36;
const int AMF_MGMT_OP_COMP_REPAIR                  = 37;
const int AMF_MGMT_OP_SU_REPAIR                    = 38;

/*
enum RpcOperation
{
  AMF_MGMT_OP_COMPONENT_CREATE = 1,
  AMF_MGMT_OP_COMPONENT_UPDATE,
  AMF_MGMT_OP_COMPONENT_DELETE,
  AMF_MGMT_OP_SG_CREATE,
  AMF_MGMT_OP_SG_UPDATE,
  AMF_MGMT_OP_SG_DELETE,
  AMF_MGMT_OP_NODE_CREATE,
  AMF_MGMT_OP_NODE_UPDATE,
  AMF_MGMT_OP_NODE_DELETE,
  AMF_MGMT_OP_SU_CREATE,
  AMF_MGMT_OP_SU_UPDATE,
  AMF_MGMT_OP_SU_DELETE,
  AMF_MGMT_OP_SI_CREATE,
  AMF_MGMT_OP_SI_UPDATE,
  AMF_MGMT_OP_SI_DELETE,
  AMF_MGMT_OP_CSI_CREATE,
  AMF_MGMT_OP_CSI_UPDATE,
  AMF_MGMT_OP_CSI_DELETE,
  AMF_MGMT_OP_CSI_NVP_DELETE,
  AMF_MGMT_OP_NODE_SU_LIST_DELETE,
  AMF_MGMT_OP_SG_SU_LIST_DELETE,
  AMF_MGMT_OP_SG_SI_LIST_DELETE,
  AMF_MGMT_OP_SU_COMP_LIST_DELETE,
  AMF_MGMT_OP_SI_CSI_LIST_DELETE,
  AMF_MGMT_OP_NODE_LOCK_ASSIGNMENT,
  AMF_MGMT_OP_SG_LOCK_ASSIGNMENT,
  AMF_MGMT_OP_SU_LOCK_ASSIGNMENT,
  AMF_MGMT_OP_SI_LOCK_ASSIGNMENT,
  AMF_MGMT_OP_NODE_LOCK_INSTANTIATION,
  AMF_MGMT_OP_SG_LOCK_INSTANTIATION,
  AMF_MGMT_OP_SU_LOCK_INSTANTIATION,
  AMF_MGMT_OP_NODE_UNLOCK,
  AMF_MGMT_OP_SG_UNLOCK,
  AMF_MGMT_OP_SU_UNLOCK,
  AMF_MGMT_OP_SI_UNLOCK,
  AMF_MGMT_OP_NODE_REPAIR,
  AMF_MGMT_OP_COMP_REPAIR,
  AMF_MGMT_OP_SU_REPAIR,
};
*/


namespace SAFplus {
namespace Rpc {
namespace amfMgmtRpc {

  ClRcT addEntityConfigToDatabase(const std::string& xpath, const std::string& entityName)
  {
    // check if the xpath exists in the DB
    std::string val;
    std::vector<std::string>child;
    ClRcT rc = amfDb.getRecord(xpath,val,&child);
    if (rc == CL_OK)
    {
       std::vector<std::string>::iterator it;
       it = std::find(child.begin(),child.end(),entityName);
       if (it != child.end())
       {
          return CL_ERR_ALREADY_EXIST;
       }
       child.push_back(entityName);
       rc = amfDb.setRecord(xpath,val,&child);
    }
    return rc;
  }

  ClRcT addEntityToDatabase(const char* xpath, const std::string& entityName)
  {
    // check if the xpath exists in the DB
    logTrace("MGMT","---", "enter [%s] with params [%s] [%s]",__FUNCTION__, xpath, entityName.c_str());
    std::string val;
    std::vector<std::string>child;
    std::string strXpath(xpath);
    ClRcT rc = amfDb.getRecord(strXpath,val,&child);
    logTrace("MGMT","---", "get record rc=[0x%x]", rc);
    if (rc == CL_OK)
    {
       std::vector<std::string>::iterator it;
       it = std::find(child.begin(),child.end(),entityName);
       if (it != child.end())
       {
          return CL_ERR_ALREADY_EXIST;
       }
       // adding: /safplusAmf/ServiceUnit[@name="suname"]
       std::string entname = entityName; //su0
       entname.insert(0,"[@name=\""); // [@name="su0
       entname.append("\"]"); // [@name="su0"]
       child.push_back(entname);
       rc = amfDb.setRecord(xpath,val,&child);
       logTrace("MGMT","---", "set record rc=[0x%x]", rc);
    }
    return rc;
  }

  // /safplusAmf/<entity:Component,Node,ServiceGroup...>[@name=entityName]/tagName --> value
 
  ClRcT updateEntityFromDatabase(const char* xpath, const std::string& entityName, const char* tagName, const std::string& value, std::vector<std::string>* child=nullptr)
  {
    // check if the xpath exists in the DB
    logDebug("MGMT","---", "enter [%s] with params [%s] [%s] [%s]",__FUNCTION__, xpath, entityName.c_str(), value.c_str());
    //std::string val;
    //std::vector<std::string>child;
    std::string strXpath(xpath);
    ClRcT rc = CL_OK;// = amfDb.getRecord(strXpath,val,&child);
    //logInfo("HUNG","---", "get record rc=[0x%x]", rc);
    if (rc == CL_OK)
    {
       /*std::vector<std::string>::iterator it;
       it = std::find(child.begin(),child.end(),entityName);
       if (it != child.end())
       {
          return CL_ERR_ALREADY_EXIST;
       }*/
       //std::string entname = entityName;
       strXpath.append("[@name=\"");
       strXpath.append(entityName);
       strXpath.append("\"]/");
       strXpath.append(tagName);
       rc = amfDb.setRecord(strXpath,value,child);
       logDebug("MGMT","---", "set record with xpath [%s], value [%s]  rc=[0x%x]", strXpath.c_str(), value.c_str(), rc);
    }
    
    return rc;
  }

  ClRcT updateEntityAsListTypeFromDatabase(const char* xpath, const std::string& entityName, const char* tagName, const std::vector<std::string>& values)//, const std::vector<std::string>* child=nullptr)
  {
    // check if the xpath exists in the DB
    logDebug("MGMT","---", "enter [%s] with params [%s] [%s]",__FUNCTION__, xpath, entityName.c_str());
    if (values.size()==0) return CL_ERR_INVALID_PARAMETER;
      
    std::string val;
    std::vector<std::string>child;
    std::string strXpath(xpath);
    strXpath.append("[@name=\"");
    strXpath.append(entityName);
    strXpath.append("\"]/");
    strXpath.append(tagName);
    std::string bkXpath = strXpath;
    ClRcT rc = amfDb.getRecord(strXpath,val,&child);    
    if (rc == CL_OK)
    {  
       std::vector<std::string> newValues;
       // / normally, safplusAmf/Node[@name="node0"]/serviceUnits ->  childs: [[1],[2]]
       if (child.size()==0 && val.length()<=0) // there is only one value of the list e.g.safplusAmf/ServiceUnit[@name="su1"]/components -> [] children [0] 
       {
          if (values.size()==1)
          {
            rc = amfDb.setRecord(bkXpath, values[0]);
            logDebug("MGMT","UDT.LIST", "set record with xpath [%s], value [%s], children [0] rc=[0x%x]", bkXpath.c_str(), values[0].c_str(), rc);
            return rc;
          }
       }
       else if(child.size()==0 && val.length()>0) // there is only one value of the list e.gsafplusAmf/ServiceUnit[@name="su1"]/components -> [c1] children [0]
       {
          newValues.push_back(val);
       }       
       int nextIdx = 0;       
       std::vector<std::string>::const_iterator it = values.begin();
       int count=0;
       for (;it != values.end();it++)
       {         
         std::vector<std::string>::iterator it2 = child.begin();
         for (;it2 != child.end();it2++)
         {
           strXpath.append(*it2);           
           std::vector<std::string> v;
           rc = amfDb.getRecord(strXpath,val,&v); //result: [/safplusAmf/Node[@name="node0"]/serviceUnits[1]] -> [su0] children [0]
           logDebug("MGMT","UDT.LIST", "getting record for key [%s]: val [%s], rc [0x%x]",strXpath.c_str(), val.c_str(),rc);
           if (rc==CL_OK)
           {
             if ((*it).compare(val)!=0)
             {
               count++;
             }
           }
         }
         if (count==(int)child.size())
         {
           newValues.push_back(*it);
         }
         count = 0;
       }
       it = newValues.begin(); 
       if (it != newValues.end())
       {         
         if (child.size()>0)
         {
           std::string lastIdx = *(child.end()-1);
           int i = lastIdx.find("[");
           assert(i>=0 && i<lastIdx.length());
           std::string strIdx = lastIdx.substr(i+1, lastIdx.length()-1-(i+1));
           nextIdx = atoi(strIdx.c_str());
         }
         int j = ++nextIdx;
         logDebug("MGMT","UDT.LIST", "next index for key [%s] is [%d]",bkXpath.c_str(),nextIdx);
         for(;it!=newValues.end();it++)
         {
           char temp[5];
           snprintf(temp,4,"[%d]",nextIdx++);                      
           logDebug("MGMT","UDT.LIST", "appending [%s] to children [%d] of [%s]",temp, (int)child.size(), bkXpath.c_str());
           child.push_back(std::string(temp));
         }
         rc = amfDb.setRecord(bkXpath,std::string(),&child);
         logDebug("MGMT","UDT.LIST", "setting record with xpath [%s], value [], children [%d] rc=[0x%x]", bkXpath.c_str(), (int)child.size(), rc);
         if (rc!=CL_OK)
         {
           logError("MGMT","UDT.LIST", "first stage: updating list for xpath [%s], entity [%s], tag name [%s] failed rc = [0x%x]", xpath, entityName.c_str(), tagName, rc);
           return rc;
         }
         for(it = newValues.begin(); it!=newValues.end();it++)
         {
           char temp[5];
           snprintf(temp,4,"[%d]",j++);
           std::string key = bkXpath;
           key.append(std::string(temp)); //[/safplusAmf/Node[@name="node0"]/serviceUnits[1]] -> [su0] children [0]
           rc = amfDb.setRecord(key,*it);
           logDebug("MGMT","UDT.LIST", "setting record with xpath [%s], value [%s]  rc=[0x%x]", key.c_str(), (*it).c_str(), rc);
           if (rc!=CL_OK)
           {
             logError("MGMT","UDT.LIST", "last stage: updating list for xpath [%s], entity [%s], tag name [%s] failed rc = [0x%x]", xpath, entityName.c_str(), tagName, rc);
             return rc;
           }
         }
       }       
    }
    else if (rc == CL_ERR_NOT_EXIST) // Does not exist
    {
       
    }
    else
    {
       logWarning("MGMT","UDT.LIST", "getting record for [%s] got unknown error code [0x%x]", strXpath.c_str(), rc);
    }
    return rc;
  }

  ClRcT getDbalObj(const void* reqBuf, DbalPlugin** pd)
  {
    Handle hdl;
    memcpy(&hdl,reqBuf,sizeof(Handle));    
    const DbalHashMap::iterator& contents = amfMgmtMap.find(hdl);
    if (contents == amfMgmtMap.end())
    {
      logError("MGMT","RPC", "handle [%" PRIx64 ":%" PRIx64 "] not found", hdl.id[0],hdl.id[1]);
      return CL_ERR_NOT_EXIST;
    }
    logDebug("MGMT","RPC", "got dbal obj for handle [%" PRIx64 ":%" PRIx64 "]", hdl.id[0],hdl.id[1]);
    *pd = contents->second;
    return CL_OK;
  }

  ClRcT compCommit(const ComponentConfig& comp)
  {
    ClRcT rc = CL_OK;
    logTrace("MGMT","RPC", "server is processing component create/update commit name [%s]", comp.name().c_str());
    if (comp.name().length()==0)
    {
      return CL_ERR_INVALID_PARAMETER;      
    }
    rc = addEntityToDatabase("/safplusAmf/Component",comp.name());
    if (rc == CL_ERR_ALREADY_EXIST)
    {
       logNotice("MGMT","SU.COMMIT","xpath for entity [%s] already exists", comp.name().c_str());
    }
    if (comp.has_capabilitymodel())
    {
      SAFplusAmf::CapabilityModel cm = static_cast<SAFplusAmf::CapabilityModel>(comp.capabilitymodel());
      std::stringstream ssCm;
      ssCm<<cm;
      std::string strCm;
      ssCm>>strCm;
      MGMT_CALL(updateEntityFromDatabase("/safplusAmf/Component",comp.name(),"capabilityModel",strCm));
    }
    if (comp.has_maxactiveassignments())
    {
      uint32_t maxActiveAssignments  = comp.maxactiveassignments();
      char temp[10];
      snprintf(temp, 9, "%d", maxActiveAssignments);
      std::string strMaxActiveAssignments = temp;
      MGMT_CALL(updateEntityFromDatabase("/safplusAmf/Component",comp.name(),"maxActiveAssignments",strMaxActiveAssignments));
    }
    if (comp.has_maxstandbyassignments())
    {
      uint32_t maxStandbyAssignments  = comp.maxstandbyassignments();
      char temp[10];
      snprintf(temp, 9, "%d", maxStandbyAssignments);
      std::string strMaxStandbyAssignments = temp;
      MGMT_CALL(updateEntityFromDatabase("/safplusAmf/Component",comp.name(),"maxStandbyAssignments",strMaxStandbyAssignments));
    }
    int cmdEnvSize = 0;
    if ((cmdEnvSize=comp.commandenvironment_size())>0)
    {
      std::string value;
      std::vector<std::string> cmdEnvs;
      for (int i=0;i<cmdEnvSize;i++)
      {
         cmdEnvs.push_back(comp.commandenvironment(i));
      }
      MGMT_CALL(updateEntityFromDatabase("/safplusAmf/Component",comp.name(),"commandEnvironment",value,&cmdEnvs));
    }
    if (comp.has_instantiate())
    {      
      if (comp.instantiate().has_execution())
      {
        const Execution& exe = comp.instantiate().execution();
        if (exe.has_command() && exe.has_timeout())
        {
          const std::string& cmd = exe.command();
          uint64_t timeout = exe.timeout();
          char temp[20];
          snprintf(temp, 19, "%" PRId64, timeout);
          std::string strTimeout = temp;
          std::vector<std::string> v;
          v.push_back(std::string("command"));
          v.push_back(std::string("timeout"));
          std::string value;
          MGMT_CALL(updateEntityFromDatabase("/safplusAmf/Component",comp.name(),"instantiate",value,&v));          
          MGMT_CALL(updateEntityFromDatabase("/safplusAmf/Component",comp.name(),"instantiate/command",cmd));
          MGMT_CALL(updateEntityFromDatabase("/safplusAmf/Component",comp.name(),"instantiate/timeout",strTimeout));
        }
      }
    }
    if (comp.has_terminate())
    {
      if (comp.terminate().has_execution())
      {        
        const Execution& exe = comp.terminate().execution();
        if (exe.has_command() && exe.has_timeout())
        {
          const std::string& cmd = exe.command();
          uint64_t timeout = exe.timeout();
          char temp[20];
          snprintf(temp, 19, "%" PRId64, timeout);
          std::string strTimeout = temp;
          std::vector<std::string> terminates;
          terminates.push_back(cmd);
          terminates.push_back(strTimeout);
          std::string value;
          MGMT_CALL(updateEntityFromDatabase("/safplusAmf/Component",comp.name(),"terminate",value,&terminates));
        }
      }
    }
    if (comp.has_cleanup())
    {
      if (comp.cleanup().has_execution())
      {        
        const Execution& exe = comp.cleanup().execution();
        if (exe.has_command() && exe.has_timeout())
        {
          const std::string& cmd = exe.command();
          uint64_t timeout = exe.timeout();
          char temp[20];
          snprintf(temp, 19, "%" PRId64, timeout);
          std::string strTimeout = temp;
          std::vector<std::string> cleanups;
          cleanups.push_back(cmd);
          cleanups.push_back(strTimeout);
          std::string value;
          MGMT_CALL(updateEntityFromDatabase("/safplusAmf/Component",comp.name(),"cleanup",value,&cleanups));
        }
      }
    }
    if (comp.has_maxinstantinstantiations())
    {
      int maxInstantiations  = comp.maxinstantinstantiations();
      char temp[10];
      snprintf(temp, 9, "%d", maxInstantiations);
      std::string strMaxInstantiations = temp;
      MGMT_CALL(updateEntityFromDatabase("/safplusAmf/Component",comp.name(),"maxInstantInstantiations",strMaxInstantiations));
    }
    if (comp.has_serviceunit())
    {
      const std::string& su  = comp.serviceunit();
      MGMT_CALL(updateEntityFromDatabase("/safplusAmf/Component",comp.name(),"serviceUnit",su));
    }
    if (comp.has_recovery())
    {
      SAFplusAmf::Recovery recovery = static_cast<SAFplusAmf::Recovery>(comp.recovery());
      std::stringstream ssRecovery;
      ssRecovery<<recovery;
      std::string strRecovery;
      ssRecovery>>strRecovery;
      MGMT_CALL(updateEntityFromDatabase("/safplusAmf/Component",comp.name(),"recovery",strRecovery));
    }
    if (comp.has_restartable())
    {
      bool restartable = comp.restartable();
      MGMT_CALL(updateEntityFromDatabase("/safplusAmf/Component",comp.name(),"restartable",B2S(restartable)));
    }
    return rc;
  }

  ClRcT compDeleteCommit(const std::string& compName)
  {
    ClRcT rc = CL_OK;
    logDebug("MGMT","RPC", "server is processing component delete commit name [%s]", compName.c_str());
    if (compName.length()==0)
    {
      return CL_ERR_INVALID_PARAMETER;      
    }
    rc = cfg.safplusAmf.componentList.deleteObj(compName);
    logDebug("MGMT","RPC", "deleting comp name [%s] return [0x%x]", compName.c_str(),rc);
    return rc;
  }

  ClRcT suCommit(const ServiceUnitConfig& su)
  {
    ClRcT rc = CL_OK;
    logDebug("MGMT","RPC", "server is processing su create/update commit name [%s]", su.name().c_str());
    if (su.name().length()==0)
    {
      return CL_ERR_INVALID_PARAMETER;      
    }
    rc = addEntityToDatabase("/safplusAmf/ServiceUnit", su.name());
    if (rc == CL_ERR_ALREADY_EXIST)
    {
       logNotice("MGMT","SU.COMMIT","xpath for entity [%s] already exists", su.name().c_str());
    }
    if (su.has_adminstate())
    {
      SAFplusAmf::AdministrativeState as = static_cast<SAFplusAmf::AdministrativeState>(su.adminstate());
      std::stringstream ssAs;
      ssAs<<as;
      std::string strAs;
      ssAs>>strAs;
      MGMT_CALL(updateEntityFromDatabase("/safplusAmf/ServiceUnit",su.name(),"adminState",strAs));
    }
    if (su.has_rank())
    {
      uint32_t rank  = su.rank();
      char temp[10];
      snprintf(temp, 9, "%d", rank);
      std::string strRank = temp;
      MGMT_CALL(updateEntityFromDatabase("/safplusAmf/ServiceUnit",su.name(),"rank",strRank));
    }
    if (su.has_failover())
    {
      bool failover = su.failover();
      MGMT_CALL(updateEntityFromDatabase("/safplusAmf/ServiceUnit",su.name(),"failover",B2S(failover)));
    }    
    int compsSize = 0;
    if ((compsSize=su.components_size())>0)
    {
      //std::string value;
      std::vector<std::string> comps;
      for (int i=0;i<compsSize;i++)
      {
         comps.push_back(su.components(i));
      }
      MGMT_CALL(updateEntityAsListTypeFromDatabase("/safplusAmf/ServiceUnit",su.name(),"components",comps));
    }
    if (su.has_node())
    {
      const std::string& node  = su.node();
      MGMT_CALL(updateEntityFromDatabase("/safplusAmf/ServiceUnit",su.name(),"node",node));
    }
    if (su.has_servicegroup())
    {
      const std::string& sg  = su.servicegroup();
      MGMT_CALL(updateEntityFromDatabase("/safplusAmf/ServiceUnit",su.name(),"serviceGroup",sg));
    }
    if (su.has_probationtime())
    {
      int pt  = su.probationtime();
      char temp[10];
      snprintf(temp, 9, "%d", pt);
      std::string strPt = temp;
      MGMT_CALL(updateEntityFromDatabase("/safplusAmf/ServiceUnit",su.name(),"probationTime",strPt));
    }   

    return rc;
  }

  ClRcT suDeleteCommit(const std::string& suName)
  {
    ClRcT rc = CL_OK;
    logTrace("MGMT","RPC", "server is processing su delete commit name [%s]", suName.c_str());
    if (suName.length()==0)
    {
      return CL_ERR_INVALID_PARAMETER;      
    }
    rc = cfg.safplusAmf.serviceUnitList.deleteObj(suName);
    logDebug("MGMT","RPC", "deleting su name [%s] returns [0x%x]", suName.c_str(),rc);
    return rc;
  }

  ClRcT handleCommit(const ClDBKeyHandleT recKey, ClUint32T keySize, const ClDBRecordHandleT recData, ClUint32T dataSize)
  {
    int op = 0;
    ClRcT rc = CL_OK;
    memcpy(&op, recKey, keySize);
    switch (op)
    {
    case AMF_MGMT_OP_COMPONENT_CREATE:
      {
        CreateComponentRequest request;
        std::string strRequestData;
        strRequestData.assign((ClCharT*)recData, dataSize);
        request.ParseFromString(strRequestData);
        const ComponentConfig& comp = request.componentconfig();
        rc = compCommit(comp);
        break;
      }
    case AMF_MGMT_OP_COMPONENT_UPDATE:
      {
        UpdateComponentRequest request;
        std::string strRequestData;
        strRequestData.assign((ClCharT*)recData, dataSize);
        request.ParseFromString(strRequestData);
        const ComponentConfig& comp = request.componentconfig();
        rc = compCommit(comp);
        break;
      }
    case AMF_MGMT_OP_COMPONENT_DELETE:
      {
        DeleteComponentRequest request;
        std::string strRequestData;
        strRequestData.assign((ClCharT*)recData, dataSize);
        request.ParseFromString(strRequestData);
        //const std& comp = request.componentconfig();
        rc = compDeleteCommit(request.name());
        break;
      }
    case AMF_MGMT_OP_SG_CREATE:
    case AMF_MGMT_OP_SG_UPDATE:
    case AMF_MGMT_OP_SG_DELETE:
    case AMF_MGMT_OP_NODE_CREATE:
    case AMF_MGMT_OP_NODE_UPDATE:
    case AMF_MGMT_OP_NODE_DELETE:
    case AMF_MGMT_OP_SU_CREATE:
      {
        CreateSURequest request;
        std::string strRequestData;
        strRequestData.assign((ClCharT*)recData, dataSize);
        request.ParseFromString(strRequestData);
        const ServiceUnitConfig& su = request.serviceunitconfig();
        logDebug("MGMT","COMMIT","handleCommit op [%d], entity [%s]", op, su.name().c_str());
        rc = suCommit(su);
        break;
      }
    case AMF_MGMT_OP_SU_UPDATE:
      {        
        UpdateSURequest request;
        std::string strRequestData;
        strRequestData.assign((ClCharT*)recData, dataSize);
        request.ParseFromString(strRequestData);
        const ServiceUnitConfig& su = request.serviceunitconfig();
        logDebug("MGMT","COMMIT","handleCommit op [%d], entity [%s]", op, su.name().c_str());
        rc = suCommit(su);
        break;
      }
    case AMF_MGMT_OP_SU_DELETE:
      {
        DeleteSURequest request;
        std::string strRequestData;
        strRequestData.assign((ClCharT*)recData, dataSize);
        request.ParseFromString(strRequestData);        
        logDebug("MGMT","COMMIT","handleCommit op [%d], entity [%s]", op, request.name().c_str());
        rc = suDeleteCommit(request.name());
        break;
      }
    case AMF_MGMT_OP_SI_CREATE:
    case AMF_MGMT_OP_SI_UPDATE:
    case AMF_MGMT_OP_SI_DELETE:
    case AMF_MGMT_OP_CSI_CREATE:
    case AMF_MGMT_OP_CSI_UPDATE:
    case AMF_MGMT_OP_CSI_DELETE:
    case AMF_MGMT_OP_CSI_NVP_DELETE:
    case AMF_MGMT_OP_NODE_SU_LIST_DELETE:
    case AMF_MGMT_OP_SG_SU_LIST_DELETE:
    case AMF_MGMT_OP_SG_SI_LIST_DELETE:
    case AMF_MGMT_OP_SU_COMP_LIST_DELETE:
    case AMF_MGMT_OP_SI_CSI_LIST_DELETE:
    case AMF_MGMT_OP_NODE_LOCK_ASSIGNMENT:
    case AMF_MGMT_OP_SG_LOCK_ASSIGNMENT:
    case AMF_MGMT_OP_SU_LOCK_ASSIGNMENT:
    case AMF_MGMT_OP_SI_LOCK_ASSIGNMENT:
    case AMF_MGMT_OP_NODE_LOCK_INSTANTIATION:
    case AMF_MGMT_OP_SG_LOCK_INSTANTIATION:
    case AMF_MGMT_OP_SU_LOCK_INSTANTIATION:
    case AMF_MGMT_OP_NODE_UNLOCK:
    case AMF_MGMT_OP_SG_UNLOCK:
    case AMF_MGMT_OP_SU_UNLOCK:
    case AMF_MGMT_OP_SI_UNLOCK:
    case AMF_MGMT_OP_NODE_REPAIR:
    case AMF_MGMT_OP_COMP_REPAIR:
    case AMF_MGMT_OP_SU_REPAIR:
    default:
      logError("MGMT","RPC","invalid rpc operation [%d]",op);
      break;
    }
    return rc;
  }

  

  amfMgmtRpcImpl::amfMgmtRpcImpl()
  {
    //TODO: Auto-generated constructor stub
  }

  amfMgmtRpcImpl::~amfMgmtRpcImpl()
  {
    //TODO: Auto-generated destructor stub
  }

  void amfMgmtRpcImpl::initialize(const ::SAFplus::Rpc::amfMgmtRpc::InitializeRequest* request,
                                ::SAFplus::Rpc::amfMgmtRpc::InitializeResponse* response)
  {
    Handle hdl;
    memcpy(&hdl,request->amfmgmthandle().Get(0).c_str(),sizeof(Handle));
    if (amfMgmtMap.find(hdl) != amfMgmtMap.end())
    {
      logWarning("MGMT","RPC", "handle [%" PRIx64 ":%" PRIx64 "] already existed", hdl.id[0],hdl.id[1]);
      response->set_err(CL_ERR_ALREADY_EXIST);
      return;
    }
    DbalPlugin* p = clDbalObjCreate(DBAL_PLUGIN_NAME);
    char dbname[CL_MAX_NAME_LENGTH];
    hdl.toStr(dbname); 
    ClRcT rc = p->open(dbname, dbname, CL_DB_CREAT, 255, 5000);
    if (rc == CL_OK)
    {
      logDebug("MGMT","RPC", "adding handle [%" PRIx64 ":%" PRIx64 "] and Dbal object to map", hdl.id[0],hdl.id[1]);
      DbalMapPair kv(hdl,p);
      amfMgmtMap.insert(kv);
    }
    response->set_err(rc);
  }

  void amfMgmtRpcImpl::finalize(const ::SAFplus::Rpc::amfMgmtRpc::FinalizeRequest* request,
                                ::SAFplus::Rpc::amfMgmtRpc::FinalizeResponse* response)
  {
    Handle hdl;
    memcpy(&hdl,request->amfmgmthandle().Get(0).c_str(),sizeof(Handle));
    const DbalHashMap::iterator& contents = amfMgmtMap.find(hdl);
    if (contents == amfMgmtMap.end())
    {
      logWarning("MGMT","RPC", "handle [%" PRIx64 ":%" PRIx64 "] not found", hdl.id[0],hdl.id[1]);
      response->set_err(CL_ERR_NOT_EXIST);
      return;
    }
    DbalPlugin* p = contents->second;
    amfMgmtMap.erase(contents);
    delete p;
    response->set_err(CL_OK);
  }

  void amfMgmtRpcImpl::commit(const ::SAFplus::Rpc::amfMgmtRpc::CommitRequest* request,
                                ::SAFplus::Rpc::amfMgmtRpc::CommitResponse* response)
  {
    logDebug("MGMT","COMMIT","server is processing [%s]", __FUNCTION__);
    DbalPlugin* pd = NULL;
    ClRcT rc = getDbalObj(request->amfmgmthandle().Get(0).c_str(), &pd);
    if (rc == CL_OK)
    {
      ClDBKeyHandleT recKey = nullptr;
      ClUint32T keySize = 0;
      ClUint32T nextKeySize = 0;
      //ClUint32T *nextKey = nullptr;
      ClDBRecordHandleT recData = nullptr;
      ClUint32T dataSize = 0;
      /*
      * Iterators key value
      */
      rc = pd->getFirstRecord(&recKey, &keySize, &recData, &dataSize);
      while (rc == CL_OK)
      {
        rc = handleCommit(recKey,keySize,recData,dataSize);
        logDebug("MGMT","COMMIT","handleCommit returns [0x%x]", rc);
        ClDBKeyHandleT curKey = recKey;
        pd->freeRecord(recData);
        rc = pd->getNextRecord(curKey, keySize, &recKey, &nextKeySize, &recData, &dataSize);
        logDebug("MGMT","COMMIT","[%s] getNextRecord returns [0x%x]", __FUNCTION__,rc);
        pd->freeKey(curKey);
      }
      logInfo("MGMT","RPC","read the DB to reflect the changes");
      cfg.read(&amfDb);
      if (CL_GET_ERROR_CODE(rc)==CL_ERR_NOT_EXIST)
      {
        rc = CL_OK;
      }      
    }
    else
    {
      logError("MGMT","COMMIT","no Dbal object found for the specified amfMgmtHandle");
    }
    response->set_err(rc);
  }

  void amfMgmtRpcImpl::createComponent(const ::SAFplus::Rpc::amfMgmtRpc::CreateComponentRequest* request,
                                ::SAFplus::Rpc::amfMgmtRpc::CreateComponentResponse* response)
  {     
    //ComponentConfig* comp = new ComponentConfig();
    //comp->set_name("TestComp");
    //request->set_allocated_componentconfig(comp);
#if 0
    const ComponentConfig& comp = request->componentconfig();
    logInfo("HUNG","---", "server is processing createComponent name [%s]", comp.name().c_str());    
    ClRcT rc = addEntityToDatabase("/safplusAmf/Component",comp.name());
    logInfo("HUNG","---", "addEntityToDB rc=[0x%x]", rc);
    logInfo("HUNG","---","read the DB");
    cfg.read(&amfDb);
    response->set_err(rc);
#endif    
    const ComponentConfig& comp = request->componentconfig();
    logDebug("MGMT","RPC","enter [%s] with param comp name [%s], timeout [%" PRId64 "]",__FUNCTION__,comp.name().c_str(), comp.instantiate().execution().timeout());
    DbalPlugin* pd = NULL;
    ClRcT rc = getDbalObj(request->amfmgmthandle().Get(0).c_str(), &pd);
    if (rc == CL_OK)
    {
      std::string strMsgReq;      
      request->SerializeToString(&strMsgReq);
      rc = pd->insertRecord(ClDBKeyT(&AMF_MGMT_OP_COMPONENT_CREATE),
                           (ClUint32T)sizeof(AMF_MGMT_OP_COMPONENT_CREATE),
                           (ClDBRecordT)strMsgReq.c_str(),
                           (ClUint32T)strMsgReq.length());      
    } 
    response->set_err(rc);
  }


  void amfMgmtRpcImpl::updateComponent(const ::SAFplus::Rpc::amfMgmtRpc::UpdateComponentRequest* request,
                                ::SAFplus::Rpc::amfMgmtRpc::UpdateComponentResponse* response)
  {
    const ComponentConfig& comp = request->componentconfig();
    logTrace("MGMT","RPC","enter [%s] with param comp name [%s]",__FUNCTION__,comp.name().c_str());
    DbalPlugin* pd = NULL;
    ClRcT rc = getDbalObj(request->amfmgmthandle().Get(0).c_str(), &pd);
    if (rc == CL_OK)
    {
      std::string strMsgReq;      
      request->SerializeToString(&strMsgReq);
      rc = pd->insertRecord(ClDBKeyT(&AMF_MGMT_OP_COMPONENT_UPDATE),
                           (ClUint32T)sizeof(AMF_MGMT_OP_COMPONENT_UPDATE),
                           (ClDBRecordT)strMsgReq.c_str(),
                           (ClUint32T)strMsgReq.length());      
    } 
    response->set_err(rc);
  }



  void amfMgmtRpcImpl::deleteComponent(const ::SAFplus::Rpc::amfMgmtRpc::DeleteComponentRequest* request,
                                ::SAFplus::Rpc::amfMgmtRpc::DeleteComponentResponse* response)
  {
    const std::string& compName = request->name();
    logDebug("MGMT","RPC","enter [%s] with param comp name [%s]",__FUNCTION__,compName.c_str());
    DbalPlugin* pd = NULL;
    ClRcT rc = getDbalObj(request->amfmgmthandle().Get(0).c_str(), &pd);
    if (rc == CL_OK)
    {
      std::string strMsgReq;      
      request->SerializeToString(&strMsgReq);
      rc = pd->insertRecord(ClDBKeyT(&AMF_MGMT_OP_COMPONENT_DELETE),
                           (ClUint32T)sizeof(AMF_MGMT_OP_COMPONENT_DELETE),
                           (ClDBRecordT)strMsgReq.c_str(),
                           (ClUint32T)strMsgReq.length());      
    } 
    response->set_err(rc);
  }

  void amfMgmtRpcImpl::createSG(const ::SAFplus::Rpc::amfMgmtRpc::CreateSGRequest* request,
                                ::SAFplus::Rpc::amfMgmtRpc::CreateSGResponse* response)
  {
    //TODO: put your code here
  }

  void amfMgmtRpcImpl::updateSG(const ::SAFplus::Rpc::amfMgmtRpc::UpdateSGRequest* request,
                                ::SAFplus::Rpc::amfMgmtRpc::UpdateSGResponse* response)
  {
    //TODO: put your code here
  }

  void amfMgmtRpcImpl::deleteSG(const ::SAFplus::Rpc::amfMgmtRpc::DeleteSGRequest* request,
                                ::SAFplus::Rpc::amfMgmtRpc::DeleteSGResponse* response)
  {
    //TODO: put your code here
  }

  void amfMgmtRpcImpl::createNode(const ::SAFplus::Rpc::amfMgmtRpc::CreateNodeRequest* request,
                                ::SAFplus::Rpc::amfMgmtRpc::CreateNodeResponse* response)
  {
    //TODO: put your code here
  }

  void amfMgmtRpcImpl::updateNode(const ::SAFplus::Rpc::amfMgmtRpc::UpdateNodeRequest* request,
                                ::SAFplus::Rpc::amfMgmtRpc::UpdateNodeResponse* response)
  {
    //TODO: put your code here
  }

  void amfMgmtRpcImpl::deleteNode(const ::SAFplus::Rpc::amfMgmtRpc::DeleteNodeRequest* request,
                                ::SAFplus::Rpc::amfMgmtRpc::DeleteNodeResponse* response)
  {
    //TODO: put your code here
  }

  void amfMgmtRpcImpl::createSU(const ::SAFplus::Rpc::amfMgmtRpc::CreateSURequest* request,
                                ::SAFplus::Rpc::amfMgmtRpc::CreateSUResponse* response)
  {
    const ServiceUnitConfig& su = request->serviceunitconfig();
    logDebug("MGMT","RPC","enter [%s] with param su name [%s]",__FUNCTION__,su.name().c_str());
    DbalPlugin* pd = NULL;
    ClRcT rc = getDbalObj(request->amfmgmthandle().Get(0).c_str(), &pd);
    if (rc == CL_OK)
    {
      std::string strMsgReq;      
      request->SerializeToString(&strMsgReq);
      rc = pd->insertRecord(ClDBKeyT(&AMF_MGMT_OP_SU_CREATE),
                           (ClUint32T)sizeof(AMF_MGMT_OP_SU_CREATE),
                           (ClDBRecordT)strMsgReq.c_str(),
                           (ClUint32T)strMsgReq.length());      
      logDebug("MGMT","RPC","[%s] insertRecord returns [0x%x]",__FUNCTION__, rc);
    } 
    response->set_err(rc);
  }

  void amfMgmtRpcImpl::updateSU(const ::SAFplus::Rpc::amfMgmtRpc::UpdateSURequest* request,
                                ::SAFplus::Rpc::amfMgmtRpc::UpdateSUResponse* response)
  {
    const ServiceUnitConfig& su = request->serviceunitconfig();
    logDebug("MGMT","RPC","enter [%s] with param su name [%s]",__FUNCTION__,su.name().c_str());
    DbalPlugin* pd = NULL;
    ClRcT rc = getDbalObj(request->amfmgmthandle().Get(0).c_str(), &pd);
    if (rc == CL_OK)
    {
      std::string strMsgReq;      
      request->SerializeToString(&strMsgReq);
      rc = pd->insertRecord(ClDBKeyT(&AMF_MGMT_OP_SU_UPDATE),
                           (ClUint32T)sizeof(AMF_MGMT_OP_SU_UPDATE),
                           (ClDBRecordT)strMsgReq.c_str(),
                           (ClUint32T)strMsgReq.length());      
      logDebug("MGMT","RPC","[%s] insertRecord returns [0x%x]",__FUNCTION__, rc);
    } 
    response->set_err(rc);  
  }

  void amfMgmtRpcImpl::deleteSU(const ::SAFplus::Rpc::amfMgmtRpc::DeleteSURequest* request,
                                ::SAFplus::Rpc::amfMgmtRpc::DeleteSUResponse* response)
  {
    const std::string& suName = request->name();
    logDebug("MGMT","RPC","enter [%s] with param su name [%s]",__FUNCTION__,suName.c_str());
    DbalPlugin* pd = NULL;
    ClRcT rc = getDbalObj(request->amfmgmthandle().Get(0).c_str(), &pd);
    if (rc == CL_OK)
    {
      std::string strMsgReq;      
      request->SerializeToString(&strMsgReq);
      rc = pd->insertRecord(ClDBKeyT(&AMF_MGMT_OP_SU_DELETE),
                           (ClUint32T)sizeof(AMF_MGMT_OP_SU_DELETE),
                           (ClDBRecordT)strMsgReq.c_str(),
                           (ClUint32T)strMsgReq.length());      
      logDebug("MGMT","RPC","[%s] insertRecord returns [0x%x]",__FUNCTION__, rc);
    } 
    response->set_err(rc);
  }

  void amfMgmtRpcImpl::createSI(const ::SAFplus::Rpc::amfMgmtRpc::CreateSIRequest* request,
                                ::SAFplus::Rpc::amfMgmtRpc::CreateSIResponse* response)
  {
    //TODO: put your code here
  }

  void amfMgmtRpcImpl::updateSI(const ::SAFplus::Rpc::amfMgmtRpc::UpdateSIRequest* request,
                                ::SAFplus::Rpc::amfMgmtRpc::UpdateSIResponse* response)
  {
    //TODO: put your code here
  }

  void amfMgmtRpcImpl::deleteSI(const ::SAFplus::Rpc::amfMgmtRpc::DeleteSIRequest* request,
                                ::SAFplus::Rpc::amfMgmtRpc::DeleteSIResponse* response)
  {
    //TODO: put your code here
  }

  void amfMgmtRpcImpl::createCSI(const ::SAFplus::Rpc::amfMgmtRpc::CreateCSIRequest* request,
                                ::SAFplus::Rpc::amfMgmtRpc::CreateCSIResponse* response)
  {
    //TODO: put your code here
  }

  void amfMgmtRpcImpl::updateCSI(const ::SAFplus::Rpc::amfMgmtRpc::UpdateCSIRequest* request,
                                ::SAFplus::Rpc::amfMgmtRpc::UpdateCSIResponse* response)
  {
    //TODO: put your code here
  }

  void amfMgmtRpcImpl::deleteCSI(const ::SAFplus::Rpc::amfMgmtRpc::DeleteCSIRequest* request,
                                ::SAFplus::Rpc::amfMgmtRpc::DeleteCSIResponse* response)
  {
    //TODO: put your code here
  }

  void amfMgmtRpcImpl::deleteCSINVP(const ::SAFplus::Rpc::amfMgmtRpc::DeleteCSINVPRequest* request,
                                ::SAFplus::Rpc::amfMgmtRpc::DeleteCSINVPResponse* response)
  {
    //TODO: put your code here
  }

  void amfMgmtRpcImpl::deleteNodeSUList(const ::SAFplus::Rpc::amfMgmtRpc::DeleteNodeSUListRequest* request,
                                ::SAFplus::Rpc::amfMgmtRpc::DeleteNodeSUListResponse* response)
  {
    //TODO: put your code here
  }

  void amfMgmtRpcImpl::deleteSGSUList(const ::SAFplus::Rpc::amfMgmtRpc::DeleteSGSUListRequest* request,
                                ::SAFplus::Rpc::amfMgmtRpc::DeleteSGSUListResponse* response)
  {
    //TODO: put your code here
  }

  void amfMgmtRpcImpl::deleteSGSIList(const ::SAFplus::Rpc::amfMgmtRpc::DeleteSGSIListRequest* request,
                                ::SAFplus::Rpc::amfMgmtRpc::DeleteSGSIListResponse* response)
  {
    //TODO: put your code here
  }

  void amfMgmtRpcImpl::deleteSUCompList(const ::SAFplus::Rpc::amfMgmtRpc::DeleteSUCompListRequest* request,
                                ::SAFplus::Rpc::amfMgmtRpc::DeleteSUCompListResponse* response)
  {
    //TODO: put your code here
  }

  void amfMgmtRpcImpl::deleteSICSIList(const ::SAFplus::Rpc::amfMgmtRpc::DeleteSICSIListRequest* request,
                                ::SAFplus::Rpc::amfMgmtRpc::DeleteSICSIListResponse* response)
  {
    //TODO: put your code here
  }

  void amfMgmtRpcImpl::lockNodeAssignment(const ::SAFplus::Rpc::amfMgmtRpc::LockNodeAssignmentRequest* request,
                                ::SAFplus::Rpc::amfMgmtRpc::LockNodeAssignmentResponse* response)
  {
    //TODO: put your code here
  }

  void amfMgmtRpcImpl::lockSGAssignment(const ::SAFplus::Rpc::amfMgmtRpc::LockSGAssignmentRequest* request,
                                ::SAFplus::Rpc::amfMgmtRpc::LockSGAssignmentResponse* response)
  {
    //TODO: put your code here
  }

  void amfMgmtRpcImpl::lockSUAssignment(const ::SAFplus::Rpc::amfMgmtRpc::LockSUAssignmentRequest* request,
                                ::SAFplus::Rpc::amfMgmtRpc::LockSUAssignmentResponse* response)
  {
    //TODO: put your code here
  }

  void amfMgmtRpcImpl::lockSIAssignment(const ::SAFplus::Rpc::amfMgmtRpc::LockSIAssignmentRequest* request,
                                ::SAFplus::Rpc::amfMgmtRpc::LockSIAssignmentResponse* response)
  {
    //TODO: put your code here
  }

  void amfMgmtRpcImpl::lockNodeInstantiation(const ::SAFplus::Rpc::amfMgmtRpc::LockNodeInstantiationRequest* request,
                                ::SAFplus::Rpc::amfMgmtRpc::LockNodeInstantiationResponse* response)
  {
    //TODO: put your code here
  }

  void amfMgmtRpcImpl::lockSGInstantiation(const ::SAFplus::Rpc::amfMgmtRpc::LockSGInstantiationRequest* request,
                                ::SAFplus::Rpc::amfMgmtRpc::LockSGInstantiationResponse* response)
  {
    //TODO: put your code here
  }

  void amfMgmtRpcImpl::lockSUInstantiation(const ::SAFplus::Rpc::amfMgmtRpc::LockSUInstantiationRequest* request,
                                ::SAFplus::Rpc::amfMgmtRpc::LockSUInstantiationResponse* response)
  {
    //TODO: put your code here
  }

  void amfMgmtRpcImpl::unlockNode(const ::SAFplus::Rpc::amfMgmtRpc::UnlockNodeRequest* request,
                                ::SAFplus::Rpc::amfMgmtRpc::UnlockNodeResponse* response)
  {
    //TODO: put your code here
  }

  void amfMgmtRpcImpl::unlockSG(const ::SAFplus::Rpc::amfMgmtRpc::UnlockSGRequest* request,
                                ::SAFplus::Rpc::amfMgmtRpc::UnlockSGResponse* response)
  {
    //TODO: put your code here
  }

  void amfMgmtRpcImpl::unlockSU(const ::SAFplus::Rpc::amfMgmtRpc::UnlockSURequest* request,
                                ::SAFplus::Rpc::amfMgmtRpc::UnlockSUResponse* response)
  {
    //TODO: put your code here
  }

  void amfMgmtRpcImpl::unlockSI(const ::SAFplus::Rpc::amfMgmtRpc::UnlockSIRequest* request,
                                ::SAFplus::Rpc::amfMgmtRpc::UnlockSIResponse* response)
  {
    //TODO: put your code here
  }

  void amfMgmtRpcImpl::repairNode(const ::SAFplus::Rpc::amfMgmtRpc::RepairNodeRequest* request,
                                ::SAFplus::Rpc::amfMgmtRpc::RepairNodeResponse* response)
  {
    //TODO: put your code here
  }

  void amfMgmtRpcImpl::repairComponent(const ::SAFplus::Rpc::amfMgmtRpc::RepairComponentRequest* request,
                                ::SAFplus::Rpc::amfMgmtRpc::RepairComponentResponse* response)
  {
    //TODO: put your code here
  }

  void amfMgmtRpcImpl::repairSU(const ::SAFplus::Rpc::amfMgmtRpc::RepairSURequest* request,
                                ::SAFplus::Rpc::amfMgmtRpc::RepairSUResponse* response)
  {
    //TODO: put your code here
  }

}  // namespace amfMgmtRpc
}  // namespace Rpc
}  // namespace SAFplus
