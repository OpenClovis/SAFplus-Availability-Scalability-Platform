#include "amfMgmtRpc.hxx"
#include <clMgtDatabase.hxx>
#include <SafplusAmf.hxx>
#include <Data.hxx>
#include <ComponentServiceInstance.hxx>
#include <Component.hxx>
//#include <EntityId.hxx>
#include <clAmfPolicyPlugin.hxx>
#include <SAFplusAmfModule.hxx>
#include <CapabilityModel.hxx>
#include <Recovery.hxx>
#include <AdministrativeState.hxx>
#include <amfOperations.hxx>
#include <clDbalBase.hxx>
#include <clHandleApi.hxx>
#include <boost/unordered_map.hpp>
#include <iostream>
#include <map>
#include <typeinfo>

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
extern SAFplus::AmfOperations *amfOpsMgmt;
extern SAFplus::RedPolicyMap redPolicies;
extern ClRcT setInstallInfo(const std::string& nodeName, const std::string& safplusInstallInfo);
extern ClRcT getInstallInfo(const std::string& nodeName, std::string& safplusInstallInfo);

//namespace SAFplus {

typedef std::pair<const SAFplus::Handle,SAFplus::DbalPlugin*> DbalMapPair;
typedef boost::unordered_map <SAFplus::Handle, SAFplus::DbalPlugin*> DbalHashMap;
DbalHashMap amfMgmtMap;

typedef std::pair<const std::string,const std::string> KeyValueMapPair;
typedef boost::unordered_map <const std::string, const std::string> KeyValueHashMap;

static unsigned short inc = 0; //increament number of operation number

// RPC Operation IDs
/*
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
*/

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


namespace SAFplus {

//extern ClRcT sgAdjust(const SAFplusAmf::ServiceGroup* sg);

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

#if 0

  //    addEntityToDatabase("/safplusAmf/Component",comp.name())
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

#endif

   //    addEntityToDatabase("/safplusAmf", "ServiceUnit", su.name())
  ClRcT addEntityToDatabase(const char* xpath, const char* tagName, const std::string& entityName)
  {
    // check if the xpath exists in the DB
    logTrace("MGMT","ADD.ENT", "enter [%s] with params [%s] [%s]",__FUNCTION__, xpath, entityName.c_str());
    std::string val;
    std::vector<std::string>child;
    std::string strXpath(xpath);// /safplusAmf
    strXpath.append("/"); // /safplusAmf/
    strXpath.append(tagName); // /safplusAmf/Component
    ClRcT rc = amfDb.getRecord(strXpath,val,&child);
    logDebug("MGMT","ADD.ENT", "get record for xpath [%s], rc=[0x%x]", strXpath.c_str(), rc);
    if (rc == CL_OK)
    {
       // build name of entity : ServiceUnit[@name="abc"]
       std::string temp = "[@name=\""; // [@name="
       temp.append(entityName); // [@name="su0
       temp.append("\"]"); // [@name="su0"]
       //std::string entname = tagName; // ServiceUnit  
       //entname.append("[@name=\""); // ServiceUnit[@name="
       //entname.append(entityName); // ServiceUnit[@name="su0
       //entname.append("\"]"); // ServiceUnit[@name="su0"]
       //entname.append(temp); //ServiceUnit[@name="su0"]
       //Check if it already exists
       std::vector<std::string>::iterator it;
       it = std::find(child.begin(),child.end(),temp);
       if (it != child.end())
       {
          //return CL_ERR_ALREADY_EXIST;
          logInfo("MGMT","ADD.ENT", "entity [%s] already added to database, rc=[0x%x]",temp.c_str(), rc);
       }
       else
       {
         child.push_back(temp);
         MGMT_CALL(amfDb.setRecord(strXpath,val,&child));
#if 0     
         // check /safplusAmf/ServiceUnit ->  childs: [[@name="su0"]]
         child.clear();
         strXpath.append("/"); // /safplusAmf/
         strXpath.append(tagName); // /safplusAmf/ServiceUnit
         rc = amfDb.getRecord(strXpath,val,&child); // child if any :  [[@name="su0"],[@name="su1"],[@name="su2"]] or []
         logInfo("MGMT","ADD.ENT","getting record for xpath [%s] rc [0x%x] ", strXpath.c_str(), rc);
         std::vector<std::string>::iterator it;
         it = std::find(child.begin(),child.end(),temp);
         if (it != child.end())
         {
            logInfo("MGMT","ADD.ENT","child [%s] is already assigned to xpath [%s]", temp.c_str(), strXpath.c_str());
         }
         else
         {
           // Append tagName as a new child of the entity: /safplusAmf/ServiceUnit ->  childs: [[@name="su0"], [@name="su1"]]
           child.push_back(temp);
           rc = amfDb.setRecord(strXpath,val,&child);
           if (rc != CL_OK)
           {
              logError("MGMT","ADD.ENT","setting record for xpath [%s], val [%s], child [%d] failed rc [0x%x]", strXpath.c_str(), val.c_str(), (int)child.size(),rc);
              return rc;
           }
         }
#endif
#if 1
         // adding: /safplusAmf/ServiceUnit[@name="suname"]
         //entname.insert(0,"/"); // /ServiceUnit[@name="su0"]
         //entname.insert(0,xpath); // /safplusAmf/ServiceUnit[@name="su0"]
         strXpath.append(temp);
         val="";
         rc = amfDb.setRecord(strXpath,val);
         logDebug("MGMT","ADD.ENT", "set record with key [%s] rc=[0x%x]", strXpath.c_str(), rc);
#endif
       }
    }
    else
    {
       logError("MGMT","ADD.ENT", "get record for xpath [%s] failed rc=[0x%x]", xpath, rc);
    }
    return rc;
  }
  // call: deleteEntityFromDatabase("/safplusAmf", "Component",  compName);
  ClRcT deleteEntityFromDatabase(const char* xpath, const char* tagName, const std::string& entityName, const char* entityListName)
  {
    logTrace("MGMT","DELL.ENT", "enter [%s] with params [%s] [%s]",__FUNCTION__, xpath, entityName.c_str());
    std::string strXpath(xpath); // /safplusAmf
    strXpath.append("/"); // /safplusAmf/
    strXpath.append(tagName); // /safplusAmf/Component
    std::string temp = "[@name=\""; // [@name="
    temp.append(entityName); // [@name="su0
    temp.append("\"]"); // [@name="su0"]
    logDebug("MGMT","DELL.ENT", "start deleting all records containing [%s]", temp.c_str());
    ClRcT rc = amfDb.deleteAllRecordsContainKey(temp);
    if (rc != CL_OK)
    {
       logError("MGMT","DELL.ENT", "delete records FAILED for xpath(key) containing [%s], rc=[0x%x]", temp.c_str(), rc);
       return rc;
    }
    strXpath.append(temp); // /safplusAmf/Component[@name="c0"]
#if 0
    std::string val;
    std::vector<std::string> child;
    ClRcT rc = amfDb.getRecord(strXpath,val,&child);
    logDebug("MGMT","DELL.ENT", "get record for xpath [%s], rc=[0x%x]", strXpath.c_str(), rc);
    if (rc == CL_OK)
    {      
       std::string entityAttrXpath;

       std::vector<std::string>::iterator it;
       for (it = child.begin(); it!=child.end(); it++);       
       {
          entityAttrXpath = strXpath;
          entityAttrXpath.append("/");
          logDebug("MGMT","DELL.ENT", "appending [%s]", (*it).c_str());
          entityAttrXpath.append(*it);
#endif
#if 0
       for(std::vector<std::string>::const_iterator it = child.cbegin();it!=child.cend();it++)
       {
          entityAttrXpath = strXpath;
          entityAttrXpath.append("/");
          entityAttrXpath.append(*it);
          logDebug("MGMT","DELL.ENT","xpath of attribute is [%s]\n", entityAttrXpath.c_str());
          rc = amfDb.deleteRecord(entityAttrXpath);
          if (rc != CL_OK)
          {
             logError("MGMT","DELL.ENT", "delete record FAILED for xpath [%s], rc=[0x%x]", entityAttrXpath.c_str(), rc);
             return rc; 
          }
       }
#endif
    logDebug("MGMT","DELL.ENT", "start deleting all references for xpath [%s], entity [%s], entityListName [%s]", strXpath.c_str(), entityName.c_str(), entityListName);
    if ((rc = amfDb.deleteAllReferencesToEntity(strXpath, entityName, entityListName))!=CL_OK)
    {
       logError("MGMT","DELL.ENT", "delete all refs with xpath [%s] failed rc=[0x%x]", strXpath.c_str(),rc);
       return rc;
    }

    rc = amfDb.deleteRecord(strXpath);
    if (CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_EXIST) rc = CL_OK; // no problem if the key to delete doesn't exist
    if (rc != CL_OK)
    {
       logError("MGMT","DELL.ENT", "delete record with key [%s] FAILED rc=[0x%x]", strXpath.c_str(),rc);
    }

    return rc;
  }

  // /safplusAmf/<entity:Component,Node,ServiceGroup...>[@name=entityName]/tagName --> value
  // updateEntityFromDatabase("/safplusAmf/ComponentServiceInstance",csi.name(),"serviceInstance",si));
#if 0
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

#endif

  // /safplusAmf/<entity:Component,Node,ServiceGroup...>[@name=entityName]/tagName --> value
  // updateEntityFromDatabase("/safplusAmf/ComponentServiceInstance",csi.name(),"serviceInstance",si));
  // MGMT_CALL(updateEntityFromDatabase("/safplusAmf/Component",comp.name(),"capabilityModel",strCm));
  ClRcT updateEntityFromDatabase(const char* xpath, const std::string& entityName, const char* tagName, const std::string& value, std::vector<std::string>* child=nullptr)
  {
    // check if the xpath exists in the DB
    logDebug("MGMT","UDT.ENT", "enter [%s] with params [%s] [%s] [%s]",__FUNCTION__, xpath, entityName.c_str(), value.c_str());
    std::string val;
    std::vector<std::string>children;
    std::string strXpath(xpath); // /safplusAmf/Component
    strXpath.append("[@name=\""); // /safplusAmf/Component[@name="
    strXpath.append(entityName); // /safplusAmf/Component/[@name="c1
    strXpath.append("\"]");// /safplusAmf/Component[@name="c1"]
    ClRcT rc = amfDb.getRecord(strXpath,val,&children);
    logInfo("MGMT","UDT.ENT", "get record with key [%s] rc=[0x%x]", strXpath.c_str(), rc);    
    if (rc == CL_OK)
    {
       std::string strTagName(tagName);
       std::vector<std::string>::iterator it;
       it = std::find(children.begin(),children.end(),strTagName);
       if (it != children.end())
       {
          logInfo("MGMT","UDT.ENT","child [%s] is already assigned to xpath [%s]", tagName, strXpath.c_str());
       }
       else
       {
         // Append tagName as a new child of the entity
         children.push_back(strTagName);
         rc = amfDb.setRecord(strXpath,val,&children);
         if (rc != CL_OK)
         {
            logError("MGMT","UDT.ENT","setting record for xpath [%s], val [%s], child [%d] failed", strXpath.c_str(), val.c_str(), (int)children.size());
            return rc;
         }
       }
       /*std::vector<std::string>::iterator it;
       it = std::find(child.begin(),child.end(),entityName);
       if (it != child.end())
       {
          return CL_ERR_ALREADY_EXIST;
       }*/
       //std::string entname = entityName;
       //strXpath.append("[@name=\""); // /safplusAmf/Component[@name="
       //strXpath.append(entityName); // /safplusAmf/Component/[@name="c1
       strXpath.append("/");// /safplusAmf/Component[@name="c1"]/
       strXpath.append(tagName); // /safplusAmf/Component[@name="c1"]/capabilityModel
       rc = amfDb.setRecord(strXpath,value,child);
       logDebug("MGMT","UDT.ENT", "set record with xpath [%s], value [%s]  rc=[0x%x]", strXpath.c_str(), value.c_str(), rc);
    }
    else
    {
       logError("MGMT","UDT.ENT","getting record for xpath [%s] failed rc [0x%x]", xpath,rc);
    }
    
    return rc;
  }
 
#if 0
  //    updateEntityFromDatabase("/safplusAmf/ComponentServiceInstance", csi.name(), "data", "val", data));
  ClRcT updateEntityFromDatabase(const char* xpath, const std::string& entityName, const char* tagName1, const char* tagName2, const KeyValueHashMap& kvm)
  {
    logDebug("MGMT","---", "enter [%s] with params [%s] [%s] [%s] [%s]",__FUNCTION__, xpath, entityName.c_str(), tagName1, tagName2);
    /*/safplusAmf/ComponentServiceInstance[@name="csi"]/data[@name="testKey"] ->  childs: [val]
      /safplusAmf/ComponentServiceInstance[@name="csi"]/data[@name="testKey"]/val -> [testVal] children [0]
      /safplusAmf/ComponentServiceInstance[@name="csi"]/data[@name="testKey2"] ->  childs: [val]
      /safplusAmf/ComponentServiceInstance[@name="csi"]/data[@name="testKey2"]/val -> [testVal2] children [0]

       params:
         xpath = /safplusAmf/ComponentServiceInstance
         entityName = csi
         tagName1 = data
         tagName2 = val
         kvm (key/val map): testKey->testVal ; testkey2->testVal2 ...
    */
    if (kvm.size()==0)
    {
      logError("MGMT","UPT.ENT","key value hash map is empty");
      return CL_ERR_INVALID_PARAMETER;
    }
    std::string strXpath(xpath);
    ClRcT rc = CL_OK;
    strXpath.append("[@name=\"");
    strXpath.append(entityName);
    strXpath.append("\"]/");
    strXpath.append(tagName1); // /safplusAmf/ComponentServiceInstance[@name="csi"]/data   
    KeyValueHashMap::const_iterator it = kvm.begin();    
    std::vector<std::string>child;
    child.push_back(std::string(tagName2));
    std::string value;
    for (;it!=kvm.end();it++)
    {
       const std::string& key = it->first;
       const std::string& val = it->second;
       std::string tempXpath = strXpath;
       tempXpath.append("[@name=\"");
       tempXpath.append(key);
       tempXpath.append("\"]"); // /safplusAmf/ComponentServiceInstance[@name="csi"]/data[@name="testKey2"]
       MGMT_CALL(amfDb.setRecord(tempXpath,value,&child));
       logDebug("MGMT","---", "set record with xpath [%s], value [%s], child [%s]  rc=[0x%x]", tempXpath.c_str(), value.c_str(), tagName2, rc);
       tempXpath.append("/");
       tempXpath.append(tagName2); // /safplusAmf/ComponentServiceInstance[@name="csi"]/data[@name="testKey2"]/val
       MGMT_CALL(amfDb.setRecord(tempXpath,val));
       logDebug("MGMT","---", "set record with xpath [%s], value [%s]  rc=[0x%x]", tempXpath.c_str(), value.c_str(), rc);       
    }
    
    return rc;
  }

#endif

  //    updateEntityFromDatabase("/safplusAmf/ComponentServiceInstance", csi.name(), "data", "val", data));
  ClRcT updateEntityFromDatabase(const char* xpath, const std::string& entityName, const char* tagName1, const char* tagName2, const KeyValueHashMap& kvm)
  {
    logDebug("MGMT","---", "enter [%s] with params [%s] [%s] [%s] [%s]",__FUNCTION__, xpath, entityName.c_str(), tagName1, tagName2);
    /*/safplusAmf/ComponentServiceInstance[@name="csi"]/data[@name="testKey"] ->  childs: [val]
      /safplusAmf/ComponentServiceInstance[@name="csi"]/data[@name="testKey"]/val -> [testVal] children [0]
      /safplusAmf/ComponentServiceInstance[@name="csi"]/data[@name="testKey2"] ->  childs: [val]
      /safplusAmf/ComponentServiceInstance[@name="csi"]/data[@name="testKey2"]/val -> [testVal2] children [0]

       params:
         xpath = /safplusAmf/ComponentServiceInstance
         entityName = csi
         tagName1 = data
         tagName2 = val
         kvm (key/val map): testKey->testVal ; testkey2->testVal2 ...
    */
    if (kvm.size()==0)
    {
      logError("MGMT","UPT.ENT","key value hash map is empty");
      return CL_ERR_INVALID_PARAMETER;
    }

    std::string value;
    std::vector<std::string>children;
    std::string strXpath1(xpath); // /safplusAmf/ComponentServiceInstance
    strXpath1.append("[@name=\""); // /safplusAmf/ComponentServiceInstance[@name="
    strXpath1.append(entityName); // /safplusAmf/ComponentServiceInstance[@name="csi
    strXpath1.append("\"]"); // /safplusAmf/ComponentServiceInstance[@name="csi"]
    ClRcT rc = amfDb.getRecord(strXpath1,value,&children);
    logInfo("MGMT","UPT.ENT", "get record for xpath [%s], rc=[0x%x]", strXpath1.c_str(),rc);
    //if (rc == CL_OK)
    //{
    //strXpath1.append("/"); // /safplusAmf/ComponentServiceInstance[@name="csi"]/
    std::string strXpath2(tagName1); // data
    strXpath2.append("[@name=\""); // data[@name="
    //strXpath2.append(entityName); // /safplusAmf/ComponentServiceInstance[@name="csi
    //strXpath1.append("\"]/"); // /safplusAmf/ComponentServiceInstance[@name="csi"]
        
    //strXpath.append("[@name=\"");
    //strXpath.append(entityName);
    //strXpath.append("\"]/");
    //strXpath.append(tagName1); // /safplusAmf/ComponentServiceInstance[@name="csi"]/data

    KeyValueHashMap::const_iterator it = kvm.begin();    
    std::vector<std::string>child;
    //child.push_back(std::string(tagName2));
    //std::string value;

    std::string ent("[@name=\""); // [@name="  ==>  testKey2"]
    std::string dataXpath = strXpath1; // /safplusAmf/ComponentServiceInstance[@name="csi"]
    dataXpath.append("/data"); // /safplusAmf/ComponentServiceInstance[@name="csi"]/data
    for (;it!=kvm.end();it++)
    {
       const std::string& key = it->first;
       const std::string& val = it->second;
       std::string tempXpath = strXpath2; // data[@name="
       //tempXpath.append("[@name=\"");
       tempXpath.append(key); // data[@name="testKey2
       tempXpath.append("\"]"); // data[@name="testKey2"]
       std::vector<std::string>::iterator it;
       it = std::find(children.begin(),children.end(),tempXpath);
       if (it != children.end())
       {
          logInfo("MGMT","UDT.ENT","child [%s] is already assigned to xpath [%s]", tempXpath.c_str(), strXpath1.c_str());
       }
       else
       {
          children.push_back(tempXpath);
          rc = amfDb.setRecord(strXpath1,value,&children);
          if (rc != CL_OK)
          {
             logError("MGMT","UDT.ENT", "set record for xpath [%s], value [%s], child [%d]  rc=[0x%x]", strXpath1.c_str(), value.c_str(), (int)children.size(), rc);
             return rc;
          }
       }
       // check if /safplusAmf/ComponentServiceInstance[@name="csi"]/data  --> child [@name="testKey2"] exists       
       children.clear();
       amfDb.getRecord(dataXpath,value,&children);
       std::string sEnt = ent; // [@name="
       sEnt.append(key); // [@name="testKey2
       sEnt.append("\"]"); // [@name="testKey2"]
       it = std::find(children.begin(),children.end(),sEnt);
       if (it != children.end())
       {
          logInfo("MGMT","UDT.ENT","child [%s] is already assigned to xpath [%s]", sEnt.c_str(), dataXpath.c_str());
       }
       else
       {
          children.push_back(sEnt);
          rc = amfDb.setRecord(dataXpath,value,&children);
          if (rc != CL_OK)
          {
             logError("MGMT","UDT.ENT", "set record for xpath [%s], value [%s], child [%d]  rc=[0x%x]", dataXpath.c_str(), value.c_str(), (int)children.size(), rc);
             return rc;
          }
       }

       // check if /safplusAmf/ComponentServiceInstance[@name="csi"]/data[@name="testKey2"] --> child [val] exists
       std::string strTagName2 = tagName2;
       //strXpath1.append("/"); // /safplusAmf/ComponentServiceInstance[@name="csi"]/       
       tempXpath.insert(0,strXpath1); // /safplusAmf/ComponentServiceInstance[@name="csi"]data[@name="testKey2"]
       tempXpath.insert(strXpath1.length(),"/"); // /safplusAmf/ComponentServiceInstance[@name="csi"]/data[@name="testKey2"]
       children.clear();
       amfDb.getRecord(tempXpath,value,&children);
       it = std::find(children.begin(),children.end(),strTagName2);
       if (it != children.end())
       {
          logInfo("MGMT","UDT.ENT","child [%s] is already assigned to xpath [%s]", tagName2, tempXpath.c_str());
       }
       else
       {
          children.push_back(strTagName2);
          MGMT_CALL(amfDb.setRecord(tempXpath,value,&children));
       }
       tempXpath.append("/"); // /safplusAmf/ComponentServiceInstance[@name="csi"]/data[@name="testKey2"]
       tempXpath.append(tagName2); // /safplusAmf/ComponentServiceInstance[@name="csi"]/data[@name="testKey2"]/val
       rc = amfDb.setRecord(tempXpath,val);
       if (rc == CL_OK)
       {
         logDebug("MGMT","UDT.ENT", "set record with xpath [%s], value [%s]  rc=[0x%x]", tempXpath.c_str(), value.c_str(), rc);       
       }
       else
       {
         break;
       }
    }    
    //}
    
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
    if (CL_GET_ERROR_CODE(rc)==CL_ERR_NOT_EXIST) // not existing, create it
    {
      rc = updateEntityFromDatabase(xpath, entityName, tagName, val);
    }
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
       else if(child.size()==0 && val.length()>0) // there is only one value of the list e.g. safplusAmf/ServiceUnit[@name="su1"]/components -> [c1] children [0]
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
           strXpath = bkXpath;
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
    /*else if (rc == CL_ERR_NOT_EXIST) // Does not exist
    {
       
    }*/
    else
    {
       logWarning("MGMT","UDT.LIST", "getting record for [%s] got unknown error code [0x%x]", strXpath.c_str(), rc);
    }
    return rc;
  }
  
  /*
  function deleteEntityAsListTypeFromDatabase:
     params:
       xpath: /safplusAmf/Node
       entityName: node0
       entityType: {"node","serviceUnit","serviceInstance","componentServiceInstance","component"}
  */
  ClRcT deleteEntityAsListTypeFromDatabase(const char* xpath, const std::string& entityName, const char* entityType, const char* tagName, const std::vector<std::string>& values)//, const std::vector<std::string>* child=nullptr)
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
       std::string value;
       std::vector<std::string> valuesToDelete;
       std::vector<std::string>::iterator it;
       // / normally, safplusAmf/Node[@name="node0"]/serviceUnits ->  childs: [[1],[2]]
       if (child.size()==0 && val.length()<=0) // there is only one value of the list e.g.safplusAmf/ServiceUnit[@name="su1"]/components -> [] children [0] 
       {          
         logError("MGMT","DEL.LIST", "value of xpath [%s] is empty, rc=[0x%x]", bkXpath.c_str(), rc);
         return rc;        
       }
       else if(child.size()==0 && val.length()>0) // there is only one value of the list e.g. safplusAmf/ServiceUnit[@name="su1"]/components -> [c1] children [0]
       {
          if (std::find(values.begin(),values.end(),val)!=values.end())
          {            
            rc = amfDb.setRecord(bkXpath, value);
            logDebug("MGMT","DEL.LIST", "set record with xpath [%s], value [%s], children [0] rc=[0x%x]", bkXpath.c_str(), val.c_str(), rc);
            if (rc == CL_OK) valuesToDelete.push_back(val);
          }
          else
          {
            logWarning("MGMT","DEL.LIST", "there is no any value matched for xpath [%s]-->value [%s]", bkXpath.c_str(), val.c_str());
            return CL_ERR_INVALID_PARAMETER;
          }
       }
       else
       {
         // in this case, val = "", child = [[1],[2],[3],...]         
         it = child.begin();
         int count=0;
         std::map<std::string,std::string> map;
         for (;it != child.end();it++)
         {           
           strXpath.append(*it); //safplusAmf/ServiceUnit[@name="su1"]/components[1]
           std::vector<std::string> v;
           rc = amfDb.getRecord(strXpath,value,&v); //result: safplusAmf/ServiceUnit[@name="su1"]/components[1] -> [c0] children [0]
           logDebug("MGMT","DEL.LIST", "getting record for key [%s]: val [%s], rc [0x%x]",strXpath.c_str(), value.c_str(),rc);
           if (rc==CL_OK)
           {
             if (std::find(values.begin(),values.end(),value)!=values.end())
             {
               valuesToDelete.push_back(value);
               logDebug("MGMT","DEL.LIST", "deleting record for xpath [%s]",strXpath.c_str());
               MGMT_CALL(amfDb.deleteRecord(strXpath));
               count++;               
               //if (count == values.size()) break;
             }             
           }           
           strXpath = bkXpath;
           map[*it] = value;
         }
         for(it=valuesToDelete.begin();it!=valuesToDelete.end();it++)
         {
           for (std::map<std::string,std::string>::iterator itmap = map.begin(); itmap!=map.end();itmap++)
           {
             if (itmap->second.compare(*it)==0)
             {             
               const std::string& sVal = itmap->first;
               //logDebug("MGMT","DEL.LIST", "test if erasing [%s]-->[%s] out of child of [%s]",(*it).c_str(), sVal.c_str(),bkXpath.c_str());
               std::vector<std::string>::iterator it2 = std::find(child.begin(),child.end(),sVal);
               if (it2 != child.end())
               {
                 logDebug("MGMT","DEL.LIST", "erasing [%s] out of child of [%s]",(*it2).c_str(), bkXpath.c_str());
                 child.erase(it2);
               }
               break;
             }
           }           
         }
         if (child.size()>1)
         {
           logDebug("MGMT","DEL.LIST", "setting record for xpath [%s], val [%s], child [%d]",bkXpath.c_str(), val.c_str(), (int)child.size());
           rc = amfDb.setRecord(bkXpath, val, &child);
         }
         else if (child.size()==1)
         {
           std::string& sVal = map[*(child.begin())];
           logDebug("MGMT","DEL.LIST", "setting record for xpath [%s], val [%s], child [0]",bkXpath.c_str(), sVal.c_str());
           rc = amfDb.setRecord(bkXpath, sVal);
         }
         else
         {
           logDebug("MGMT","DEL.LIST", "there is no value to delete for xpath [%s]", bkXpath.c_str());
           if (count == 0) rc = CL_ERR_INVALID_PARAMETER;
         }
       }
       /* Next steps: remove the link between tagName and entity:
          MGMT_CALL(deleteEntityAsListTypeFromDatabase("/safplusAmf/Node",nodeName,"node", "serviceUnits",[su0,su1]));
          /safplusAmf/Node[@name='node0']/serviceUnits --> su0,su1
          ==> need to remove link ServiceUnit to Node: /safplusAmf/ServiceUnit[@name='su0']/node --> node0
                                                       /safplusAmf/ServiceUnit[@name='su1']/node --> node0

       */
       if (rc != CL_OK) return rc;
       std::string linkedEntity;
       if (!strcmp(tagName,"serviceUnits"))
         linkedEntity = "ServiceUnit";
       else if (!strcmp(tagName,"serviceInstances"))
         linkedEntity = "ServiceInstance";
       else if (!strcmp(tagName,"componentServiceInstances"))
         linkedEntity = "ComponentServiceInstance";
       else if (!strcmp(tagName,"components"))
         linkedEntity = "Component";
       else
       {
         logError("MGMT","DEL.LIST", "unknown tagName [%s]", tagName);
         return CL_ERR_INVALID_PARAMETER;
       }
       std::stringstream ss;
       val="";
       for (it=valuesToDelete.begin();it!=valuesToDelete.end();it++)
       {         
         ss << "/safplusAmf/" << linkedEntity << "[@name=\"" << *it << "\"]/" << entityType;
         //logDebug("MGMT","DEL.LIST", "seting record for xpath [%s], val [%s]", ss.str().c_str(), val.c_str());
         rc = amfDb.setRecord(ss.str(), val); //set empty
         logDebug("MGMT","DEL.LIST", "seting record for xpath [%s]: val [%s], rc [0x%x]",ss.str().c_str(), val.c_str(),rc);         
         if (rc != CL_OK) 
           break;
         else 
           ss.str("");
       }
    }
    /*else if (rc == CL_ERR_NOT_EXIST) // Does not exist
    {
       
    }*/
    else
    {
       logError("MGMT","DEL.LIST", "getting record for [%s] got error code [0x%x]", strXpath.c_str(), rc);
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
    logDebug("MGMT","RPC", "server is processing [%s] for entity [%s]", __FUNCTION__, comp.name().c_str());
    if (comp.name().length()==0)
    {
      return CL_ERR_INVALID_PARAMETER;      
    }
    rc = addEntityToDatabase("/safplusAmf","Component",comp.name());
    if (rc == CL_ERR_ALREADY_EXIST)
    {
       logNotice("MGMT","SU.COMMIT","xpath for entity [%s] already exists", comp.name().c_str());
    }
    if (rc != CL_OK) return rc;
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
   
    if (comp.has_instantiationsuccessduration())
    {
      int isd  = comp.instantiationsuccessduration();
      char temp[10];
      snprintf(temp, 9, "%d", isd);
      std::string strIsd = temp;
      MGMT_CALL(updateEntityFromDatabase("/safplusAmf/Component",comp.name(),"instantiationSuccessDuration",strIsd));
    }

    if (comp.has_delaybetweeninstantiation())
    {
      int dbi  = comp.delaybetweeninstantiation();
      char temp[10];
      snprintf(temp, 9, "%d", dbi);
      std::string strDbi = temp;
      MGMT_CALL(updateEntityFromDatabase("/safplusAmf/Component",comp.name(),"delayBetweenInstantiation",strDbi));
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

    if (comp.has_timeouts())
    {
      const Timeouts& timeouts = comp.timeouts();
      if (timeouts.has_terminate())
      {
        const SaTimeT& terminate  = timeouts.terminate();
        std::stringstream ss;
        ss << terminate.uint64();
        MGMT_CALL(updateEntityFromDatabase("/safplusAmf/Component",comp.name(),"timeouts/terminate",ss.str()));
      }
      if (timeouts.has_quiescingcomplete())
      {
        const SaTimeT& qc  = timeouts.quiescingcomplete();
        std::stringstream ss;
        ss << qc.uint64();
        MGMT_CALL(updateEntityFromDatabase("/safplusAmf/Component",comp.name(),"timeouts/quiescingComplete",ss.str()));
      }
      if (timeouts.has_workremoval())
      {
        const SaTimeT& wr  = timeouts.workremoval();
        std::stringstream ss;
        ss << wr.uint64();
        MGMT_CALL(updateEntityFromDatabase("/safplusAmf/Component",comp.name(),"timeouts/workRemoval",ss.str()));
      }
      if (timeouts.has_workassignment())
      {
        const SaTimeT& wa  = timeouts.workassignment();
        std::stringstream ss;
        ss << wa.uint64();
        MGMT_CALL(updateEntityFromDatabase("/safplusAmf/Component",comp.name(),"timeouts/workAssignment",ss.str()));
      }
    }
    if (comp.has_csitype())
    {
      const std::string& csiType  = comp.csitype();
      MGMT_CALL(updateEntityFromDatabase("/safplusAmf/Component",comp.name(),"csiType",csiType));
    }
   
    return rc;
  }

  ClRcT compDeleteCommit(const std::string& compName)
  {
    ClRcT rc = CL_OK;
    logDebug("MGMT","RPC", "server is processing [%s] for entity [%s]", __FUNCTION__, compName.c_str());
    if (compName.length()==0)
    {
      return CL_ERR_INVALID_PARAMETER;      
    }
    SAFplusAmf::Component* comp = dynamic_cast<SAFplusAmf::Component*>(cfg.safplusAmf.componentList[compName]);
    SAFplusAmf::ServiceUnit* su = NULL;
    if (comp)
    {
       su = comp->serviceUnit.value;
    }
    else
    {
       logError("MGMT","RPC", "su obj for comp [%s] not found", compName.c_str());
    }
    ScopedLock<ProcSem> lock(amfOpsMgmt->mutex);
    rc = cfg.safplusAmf.componentList.deleteObj(compName);
    if (rc != CL_OK)
    {
       logError("MGMT","RPC", "delete comp name [%s] failed, rc [0x%x]", compName.c_str(),rc);
       return rc;
    }
    rc = deleteEntityFromDatabase("/safplusAmf", "Component",  compName, "components");
    if (rc != CL_OK) {       
       logError("MGMT","RPC", "delete comp name [%s] failed, rc [0x%x]", compName.c_str(),rc);
       return rc;
    }
    if (su)
    {
       logDebug("MGMT","RPC", "read for su [%s]",su->name.value.c_str());
       su->read();
    }
    
    return rc;
  }

  ClRcT fillCompConfig(const SAFplusAmf::Component* comp,ComponentConfig* compConfig)
  {
    ClRcT rc = CL_OK;
    logDebug("MGMT","RPC", "server is processing [%s] for entity [%s]", __FUNCTION__, comp->name.value.c_str());

    compConfig->set_name(comp->name.value);
    
    compConfig->set_capabilitymodel(static_cast<::SAFplus::Rpc::amfMgmtRpc::CapabilityModel>(comp->capabilityModel.value));

    compConfig->set_maxactiveassignments(comp->maxActiveAssignments.value);

    compConfig->set_maxstandbyassignments(comp->maxStandbyAssignments.value);

    const std::vector<std::string>& cmdEnv = comp->commandEnvironment.value;
    for(std::vector<std::string>::const_iterator it = cmdEnv.begin();it!=cmdEnv.end();it++)
    {
      compConfig->add_commandenvironment(*it);
    }

    Instantiate* instantiate = new Instantiate();
    Execution* exe = new Execution();
    exe->set_command(comp->instantiate.command.value);
    exe->set_timeout(comp->instantiate.timeout.value);
    instantiate->set_allocated_execution(exe);
    compConfig->set_allocated_instantiate(instantiate);

    Terminate* terminate = new Terminate();
    Execution* ter_exe = new Execution();
    ter_exe->set_command(comp->terminate.command.value);
    ter_exe->set_timeout(comp->terminate.timeout.value);
    terminate->set_allocated_execution(ter_exe);
    compConfig->set_allocated_terminate(terminate);

    Cleanup* cleanup = new Cleanup();
    Execution* cleanup_exe = new Execution();
    cleanup_exe->set_command(comp->cleanup.command.value);
    cleanup_exe->set_timeout(comp->cleanup.timeout.value);
    cleanup->set_allocated_execution(cleanup_exe);
    compConfig->set_allocated_cleanup(cleanup);

    compConfig->set_maxinstantinstantiations(comp->maxInstantInstantiations.value);

    compConfig->set_instantiationsuccessduration(comp->instantiationSuccessDuration.value);

    compConfig->set_delaybetweeninstantiation(comp->delayBetweenInstantiation.value);
   
    if (comp->serviceUnit.value) 
    {
       compConfig->set_serviceunit(comp->serviceUnit.value->name.value);
    }
    std::stringstream ss;
    ss<<comp->recovery.value;
    compConfig->set_recovery(static_cast<::SAFplus::Rpc::amfMgmtRpc::Recovery>(comp->recovery.value));

    compConfig->set_restartable(comp->restartable.value);

    Timeouts* timeouts = new Timeouts();
    SaTimeT* terminateTimeout = new SaTimeT();
    terminateTimeout->set_uint64(comp->timeouts.terminate.value);
    SaTimeT* quiescingComplete = new SaTimeT();
    quiescingComplete->set_uint64(comp->timeouts.quiescingComplete.value);
    SaTimeT* workRemoval = new SaTimeT();
    workRemoval->set_uint64(comp->timeouts.workRemoval.value);
    SaTimeT* workAssignment = new SaTimeT();
    workAssignment->set_uint64(comp->timeouts.workAssignment.value);
    timeouts->set_allocated_terminate(terminateTimeout);
    timeouts->set_allocated_quiescingcomplete(quiescingComplete);
    timeouts->set_allocated_workremoval(workRemoval);
    timeouts->set_allocated_workassignment(workAssignment);
    compConfig->set_allocated_timeouts(timeouts);

    compConfig->set_csitype(comp->csiType.value);
 
    return rc;
  }

  ClRcT fillCompStatus(const SAFplusAmf::Component* comp,ComponentStatus* compStatus)
  {
    ClRcT rc = CL_OK;
    logDebug("MGMT","RPC", "server is processing [%s] for entity [%s]", __FUNCTION__, comp->name.value.c_str());

    compStatus->set_name(comp->name.value);

    ProcStats* procStats = new ProcStats();
    ProcessStats* ps = new ProcessStats();
    MemUtilization* memUtil = new MemUtilization();
    DecStatistic* decStat = new DecStatistic();
    decStat->set_current(comp->procStats.memUtilization.current.value);
    memUtil->set_allocated_decstatistic(decStat);
    ps->set_allocated_memutilization(memUtil);

    NumThreads* numThreads = new NumThreads();
    IntStatistic* intStat2 = new IntStatistic();   
    intStat2->set_current(comp->procStats.numThreads.current.value);
    numThreads->set_allocated_intstatistic(intStat2);
    ps->set_allocated_numthreads(numThreads);

    ResidentMem* resMem = new ResidentMem();
    IntStatistic* intStat3 = new IntStatistic();   
    intStat3->set_current(comp->procStats.residentMem.current.value);
    resMem->set_allocated_intstatistic(intStat3);
    ps->set_allocated_residentmem(resMem);
    
    procStats->set_allocated_processstats(ps);
    compStatus->set_allocated_procstats(procStats);    

    ActiveAssignments* activeAssignments = new ActiveAssignments();    
    IntStatistic* intStat4 = new IntStatistic();   
    intStat4->set_current(comp->activeAssignments.current.value);
    activeAssignments->set_allocated_intstatistic(intStat4);
    compStatus->set_allocated_activeassignments(activeAssignments);

    StandbyAssignments* standbyAssignments = new StandbyAssignments();    
    IntStatistic* intStat5 = new IntStatistic();   
    intStat5->set_current(comp->standbyAssignments.current.value);
    standbyAssignments->set_allocated_intstatistic(intStat5);
    compStatus->set_allocated_standbyassignments(standbyAssignments);

    const std::vector<std::string>& assignedCSI = comp->assignedWork.value;
    for(std::vector<std::string>::const_iterator it = assignedCSI.begin();it!=assignedCSI.end();it++)
    {
      compStatus->add_assignedwork(*it);
    }   

    compStatus->set_presencestate(static_cast<::SAFplus::Rpc::amfMgmtRpc::PresenceState>(comp->presenceState.value));
    compStatus->set_operstate(comp->operState.value);
    compStatus->set_readinessstate(static_cast<::SAFplus::Rpc::amfMgmtRpc::ReadinessState>(comp->readinessState.value));
    compStatus->set_hareadinessstate(static_cast<::SAFplus::Rpc::amfMgmtRpc::HighAvailabilityReadinessState>(comp->haReadinessState.value));
    compStatus->set_hastate(static_cast<::SAFplus::Rpc::amfMgmtRpc::HighAvailabilityState>(comp->haState.value));

    compStatus->set_safversion(comp->safVersion.value);
    compStatus->set_compcategory(comp->compCategory.value);
    compStatus->set_swbundle(comp->swBundle.value);
    compStatus->set_numinstantiationattempts(comp->numInstantiationAttempts.value);
    
    SAFplus::Rpc::amfMgmtRpc::Date* date = new SAFplus::Rpc::amfMgmtRpc::Date();
    date->set_uint64(comp->lastInstantiation.value.value);
    compStatus->set_allocated_lastinstantiation(date);

    RestartCount* restartCount = new RestartCount();
    IntStatistic* intStat6 = new IntStatistic();   
    intStat6->set_current(comp->restartCount.current.value);
    restartCount->set_allocated_intstatistic(intStat6);
    compStatus->set_allocated_restartcount(restartCount);

    compStatus->set_processid(comp->processId.value);
    compStatus->set_lasterror(comp->lastError.value);
    compStatus->set_pendingoperation(static_cast<::SAFplus::Rpc::amfMgmtRpc::PendingOperation>(comp->pendingOperation.value));
    
    SAFplus::Rpc::amfMgmtRpc::Date* date2 = new SAFplus::Rpc::amfMgmtRpc::Date();
    date2->set_uint64(comp->pendingOperationExpiration.value.value);
    compStatus->set_allocated_pendingoperationexpiration(date2);
    
    return rc;
  }

  ClRcT suCommit(const ServiceUnitConfig& su)
  {
    ClRcT rc = CL_OK;
    logDebug("MGMT","RPC", "server is processing [%s] for entity [%s]", __FUNCTION__, su.name().c_str());
    if (su.name().length()==0)
    {
      return CL_ERR_INVALID_PARAMETER;      
    }
    rc = addEntityToDatabase("/safplusAmf","ServiceUnit", su.name());
    if (rc == CL_ERR_ALREADY_EXIST)
    {
       logNotice("MGMT","SU.COMMIT","xpath for entity [%s] already exists", su.name().c_str());
    }
    if (rc != CL_OK) return rc;
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

  ClRcT fillSuConfig(const SAFplusAmf::ServiceUnit* su, ServiceUnitConfig* suConfig)
  {
    ClRcT rc = CL_OK;
    logDebug("MGMT","RPC", "server is processing [%s] for entity [%s]", __FUNCTION__, su->name.value.c_str());
    suConfig->set_name(su->name.value);
    suConfig->set_adminstate(static_cast<SAFplus::Rpc::amfMgmtRpc::AdministrativeState>(su->adminState.value));
    suConfig->set_rank(su->rank.value);
    suConfig->set_failover(su->failover.value);

    SAFplus::MgtIdentifierList<SAFplusAmf::Component*>::iterator itcomp = const_cast<SAFplusAmf::ServiceUnit*>(su)->components.listBegin();
    SAFplus::MgtIdentifierList<SAFplusAmf::Component*>::iterator itend = const_cast<SAFplusAmf::ServiceUnit*>(su)->components.listEnd();
    for (; itcomp != itend; itcomp++)
    {      
      SAFplusAmf::Component* comp = dynamic_cast<SAFplusAmf::Component*>(*itcomp);
      suConfig->add_components(comp->name.value);
    }

    suConfig->set_node(su->node.value->name.value);
    suConfig->set_servicegroup(su->serviceGroup.value->name.value);
    suConfig->set_probationtime(su->probationTime.value);   
    
    return rc;
  }

  ClRcT fillSuStatus(const SAFplusAmf::ServiceUnit* su, ServiceUnitStatus* suStatus)
  {
    ClRcT rc = CL_OK;
    logDebug("MGMT","RPC", "server is processing [%s] for entity [%s]", __FUNCTION__, su->name.value.c_str());
    suStatus->set_name(su->name.value);
    suStatus->set_presencestate(static_cast<::SAFplus::Rpc::amfMgmtRpc::PresenceState>(su->presenceState.value));
    suStatus->set_operstate(su->operState.value);
    suStatus->set_readinessstate(static_cast<::SAFplus::Rpc::amfMgmtRpc::ReadinessState>(su->readinessState.value));
    suStatus->set_hareadinessstate(static_cast<::SAFplus::Rpc::amfMgmtRpc::HighAvailabilityReadinessState>(su->haReadinessState.value));
    suStatus->set_hastate(static_cast<::SAFplus::Rpc::amfMgmtRpc::HighAvailabilityState>(su->haState.value));

    suStatus->set_preinstantiable(su->preinstantiable.value);

    SAFplus::MgtIdentifierList<SAFplusAmf::ServiceInstance*>::iterator itsi = const_cast<SAFplusAmf::ServiceUnit*>(su)->assignedServiceInstances.listBegin();
    SAFplus::MgtIdentifierList<SAFplusAmf::ServiceInstance*>::iterator itend = const_cast<SAFplusAmf::ServiceUnit*>(su)->assignedServiceInstances.listEnd();
    for (; itsi != itend; itsi++)
    {      
      SAFplusAmf::ServiceInstance* si = dynamic_cast<SAFplusAmf::ServiceInstance*>(*itsi);
      suStatus->add_assignedserviceinstances(si->name.value);
    } 

    NumActiveServiceInstances* nasi = new NumActiveServiceInstances();
    IntStatistic* intStat = new IntStatistic();   
    intStat->set_current(su->numActiveServiceInstances.current.value);
    nasi->set_allocated_intstatistic(intStat);
    suStatus->set_allocated_numactiveserviceinstances(nasi);

    NumStandbyServiceInstances* nssi = new NumStandbyServiceInstances();
    IntStatistic* intStat2 = new IntStatistic();   
    intStat2->set_current(su->numStandbyServiceInstances.current.value);
    nssi->set_allocated_intstatistic(intStat2);
    suStatus->set_allocated_numstandbyserviceinstances(nssi);

    RestartCount* restartCount = new RestartCount();
    IntStatistic* intStat3 = new IntStatistic();   
    intStat3->set_current(su->restartCount.current.value);
    restartCount->set_allocated_intstatistic(intStat3);
    suStatus->set_allocated_restartcount(restartCount);
    
    return rc;
  }

  ClRcT sgCommit(const ServiceGroupConfig& sg)
  {
    ClRcT rc = CL_OK;
    logDebug("MGMT","RPC", "server is processing [%s] for entity [%s]", __FUNCTION__, sg.name().c_str());
    if (sg.name().length()==0)
    {
      return CL_ERR_INVALID_PARAMETER;      
    }
    rc = addEntityToDatabase("/safplusAmf","ServiceGroup", sg.name());
    if (rc == CL_ERR_ALREADY_EXIST)
    {
       logNotice("MGMT","SG.COMMIT","xpath for entity [%s] already exists", sg.name().c_str());
    }
    if (rc != CL_OK) return rc;
    if (sg.has_adminstate())
    {
      SAFplusAmf::AdministrativeState as = static_cast<SAFplusAmf::AdministrativeState>(sg.adminstate());
      std::stringstream ssAs;
      ssAs<<as;
      std::string strAs;
      ssAs>>strAs;
      MGMT_CALL(updateEntityFromDatabase("/safplusAmf/ServiceGroup",sg.name(),"adminState",strAs));
    }    
    if (sg.has_autorepair())
    {
      bool autoRepair = sg.autorepair();
      MGMT_CALL(updateEntityFromDatabase("/safplusAmf/ServiceGroup",sg.name(),"autoRepair",B2S(autoRepair)));
    }  
    if (sg.has_autoadjust())
    {
      bool autoAdj = sg.autoadjust();
      MGMT_CALL(updateEntityFromDatabase("/safplusAmf/ServiceGroup",sg.name(),"autoAdjust",B2S(autoAdj)));
    }
    if (sg.has_autoadjustinterval())
    {
      SaTimeT autoAdjInterval  = sg.autoadjustinterval();
      char temp[20];
      snprintf(temp, 19, "%" PRId64, autoAdjInterval.uint64());
      std::string strAutoAdjInterval = temp;
      MGMT_CALL(updateEntityFromDatabase("/safplusAmf/ServiceGroup",sg.name(),"autoAdjustInterval",strAutoAdjInterval));
    }
    if (sg.has_preferrednumactiveserviceunits())
    {
      uint32_t pnasu = sg.preferrednumactiveserviceunits();
      char temp[10];
      snprintf(temp, 9, "%ul", pnasu);
      std::string strPnasu = temp;
      MGMT_CALL(updateEntityFromDatabase("/safplusAmf/ServiceGroup",sg.name(),"preferredNumActiveServiceUnits",strPnasu));
    }
    if (sg.has_preferrednumstandbyserviceunits())
    {
      uint32_t pnssu = sg.preferrednumstandbyserviceunits();
      char temp[10];
      snprintf(temp, 9, "%ul", pnssu);
      std::string strPnssu = temp;
      MGMT_CALL(updateEntityFromDatabase("/safplusAmf/ServiceGroup",sg.name(),"preferredNumStandbyServiceUnits",strPnssu));
    }
    if (sg.has_preferrednumidleserviceunits())
    {
      uint32_t pnisu = sg.preferrednumidleserviceunits();
      char temp[10];
      snprintf(temp, 9, "%ul", pnisu);
      std::string strPnisu = temp;
      MGMT_CALL(updateEntityFromDatabase("/safplusAmf/ServiceGroup",sg.name(),"preferredNumIdleServiceUnits",strPnisu));
    }
    if (sg.has_maxactiveworkassignments())
    {
      uint32_t mawa = sg.maxactiveworkassignments();
      char temp[10];
      snprintf(temp, 9, "%ul", mawa);
      std::string strMawa = temp;
      MGMT_CALL(updateEntityFromDatabase("/safplusAmf/ServiceGroup",sg.name(),"maxActiveWorkAssignments",strMawa));
    }
    if (sg.has_maxstandbyworkassignments())
    {
      uint32_t mswa = sg.maxstandbyworkassignments();
      char temp[10];
      snprintf(temp, 9, "%ul", mswa);
      std::string strMswa = temp;
      MGMT_CALL(updateEntityFromDatabase("/safplusAmf/ServiceGroup",sg.name(),"maxStandbyWorkAssignments",strMswa));
    }
    if (sg.has_componentrestart())
    {      
      if (sg.componentrestart().has_escalationpolicy())
      {
        const EscalationPolicy& ep = sg.componentrestart().escalationpolicy();
        if (ep.has_maximum() && ep.has_duration())
        {
          uint64_t max = ep.maximum();
          SaTimeT duration = ep.duration();
          char temp[20];
          snprintf(temp, 19, "%" PRId64, max);
          std::string strMax = temp;
          snprintf(temp, 19, "%" PRId64, duration.uint64());
          std::string strDuration = temp;
          std::vector<std::string> v;
          v.push_back(std::string("maximum"));
          v.push_back(std::string("duration"));
          std::string value;
          MGMT_CALL(updateEntityFromDatabase("/safplusAmf/ServiceGroup",sg.name(),"componentRestart",value,&v));          
          MGMT_CALL(updateEntityFromDatabase("/safplusAmf/ServiceGroup",sg.name(),"componentRestart/maximum",strMax));
          MGMT_CALL(updateEntityFromDatabase("/safplusAmf/ServiceGroup",sg.name(),"componentRestart/duration",strDuration));
        }
      }
    }
    if (sg.has_serviceunitrestart())
    {      
      if (sg.serviceunitrestart().has_escalationpolicy())
      {
        const EscalationPolicy& sur = sg.serviceunitrestart().escalationpolicy();
        if (sur.has_maximum() && sur.has_duration())
        {
          uint64_t max = sur.maximum();
          SaTimeT duration = sur.duration();
          char temp[20];
          snprintf(temp, 19, "%" PRId64, max);
          std::string strMax = temp;
          snprintf(temp, 19, "%" PRId64, duration.uint64());
          std::string strDuration = temp;
          std::vector<std::string> v;
          v.push_back(std::string("maximum"));
          v.push_back(std::string("duration"));
          std::string value;
          MGMT_CALL(updateEntityFromDatabase("/safplusAmf/ServiceGroup",sg.name(),"serviceUnitRestart",value,&v));
          MGMT_CALL(updateEntityFromDatabase("/safplusAmf/ServiceGroup",sg.name(),"serviceUnitRestart/maximum",strMax));
          MGMT_CALL(updateEntityFromDatabase("/safplusAmf/ServiceGroup",sg.name(),"serviceUnitRestart/duration",strDuration));
        }
      }
    }
    int susSize = 0;
    if ((susSize=sg.serviceunits_size())>0)
    {
      std::vector<std::string> sus;
      for (int i=0;i<susSize;i++)
      {
         sus.push_back(sg.serviceunits(i));
      }
      MGMT_CALL(updateEntityAsListTypeFromDatabase("/safplusAmf/ServiceGroup",sg.name(),"serviceUnits",sus));
    }
    int sisSize = 0;
    if ((sisSize=sg.serviceinstances_size())>0)
    {
      std::vector<std::string> sis;
      for (int i=0;i<sisSize;i++)
      {
         sis.push_back(sg.serviceinstances(i));
      }
      MGMT_CALL(updateEntityAsListTypeFromDatabase("/safplusAmf/ServiceGroup",sg.name(),"serviceInstances",sis));
    }    

    return rc;
  }

  ClRcT fillSgConfig(const SAFplusAmf::ServiceGroup* sg, ServiceGroupConfig* sgConfig)
  {
    ClRcT rc = CL_OK;
    logDebug("MGMT","RPC", "server is processing [%s] for entity [%s]", __FUNCTION__, sg->name.value.c_str());
    sgConfig->set_name(sg->name.value);
    sgConfig->set_adminstate(static_cast<SAFplus::Rpc::amfMgmtRpc::AdministrativeState>(sg->adminState.value));
    sgConfig->set_autorepair(sg->autoRepair.value);
    sgConfig->set_autoadjust(sg->autoAdjust.value);

    SaTimeT* adi = new SaTimeT();
    adi->set_uint64(sg->autoAdjustInterval.value);    
    sgConfig->set_allocated_autoadjustinterval(adi);

    sgConfig->set_preferrednumactiveserviceunits(sg->preferredNumActiveServiceUnits.value);
    sgConfig->set_preferrednumstandbyserviceunits(sg->preferredNumStandbyServiceUnits.value);
    sgConfig->set_preferrednumidleserviceunits(sg->preferredNumIdleServiceUnits.value);
    sgConfig->set_maxactiveworkassignments(sg->maxActiveWorkAssignments.value);
    sgConfig->set_maxstandbyworkassignments(sg->maxStandbyWorkAssignments.value);

    ComponentRestart* compRestart = new ComponentRestart();
    EscalationPolicy* ep = new EscalationPolicy();
    ep->set_maximum(sg->componentRestart.maximum.value);
    SaTimeT* duration = new SaTimeT();
    duration->set_uint64(sg->componentRestart.duration.value);
    ep->set_allocated_duration(duration);
    compRestart->set_allocated_escalationpolicy(ep);
    sgConfig->set_allocated_componentrestart(compRestart);
    
    ServiceUnitRestart* suRestart = new ServiceUnitRestart();
    EscalationPolicy* ep2 = new EscalationPolicy();
    ep2->set_maximum(sg->serviceUnitRestart.maximum.value);
    SaTimeT* dur = new SaTimeT();
    dur->set_uint64(sg->serviceUnitRestart.duration.value);
    ep2->set_allocated_duration(dur);
    suRestart->set_allocated_escalationpolicy(ep2);
    sgConfig->set_allocated_serviceunitrestart(suRestart);
            
    SAFplus::MgtIdentifierList<SAFplusAmf::ServiceUnit*>::iterator itsu = const_cast<SAFplusAmf::ServiceGroup*>(sg)->serviceUnits.listBegin();
    SAFplus::MgtIdentifierList<SAFplusAmf::ServiceUnit*>::iterator itend = const_cast<SAFplusAmf::ServiceGroup*>(sg)->serviceUnits.listEnd();
    for (; itsu != itend; itsu++)
    {      
      SAFplusAmf::ServiceUnit* su = dynamic_cast<SAFplusAmf::ServiceUnit*>(*itsu);
      sgConfig->add_serviceunits(su->name.value);
    }

    SAFplus::MgtIdentifierList<SAFplusAmf::ServiceInstance*>::iterator itsi = const_cast<SAFplusAmf::ServiceGroup*>(sg)->serviceInstances.listBegin();
    SAFplus::MgtIdentifierList<SAFplusAmf::ServiceInstance*>::iterator itend2 = const_cast<SAFplusAmf::ServiceGroup*>(sg)->serviceInstances.listEnd();
    for (; itsi != itend2; itsi++)
    {      
      SAFplusAmf::ServiceInstance* si = dynamic_cast<SAFplusAmf::ServiceInstance*>(*itsi);
      sgConfig->add_serviceinstances(si->name.value);
    }
   
    return rc;
  }

  ClRcT fillSgStatus(const SAFplusAmf::ServiceGroup* sg, ServiceGroupStatus* sgStatus)
  {
    ClRcT rc = CL_OK;
    logDebug("MGMT","RPC", "server is processing [%s] for entity [%s]", __FUNCTION__, sg->name.value.c_str());
    sgStatus->set_name(sg->name.value);

    NumAssignedServiceUnits* nasu = new NumAssignedServiceUnits();
    IntStatistic* intStat = new IntStatistic();   
    intStat->set_current(sg->numAssignedServiceUnits.current.value);
    nasu->set_allocated_intstatistic(intStat);
    sgStatus->set_allocated_numassignedserviceunits(nasu);

    NumIdleServiceUnits* nisu = new NumIdleServiceUnits();
    IntStatistic* intStat2 = new IntStatistic();   
    intStat2->set_current(sg->numIdleServiceUnits.current.value);
    nisu->set_allocated_intstatistic(intStat2);
    sgStatus->set_allocated_numidleserviceunits(nisu);

    NumSpareServiceUnits* nssu = new NumSpareServiceUnits();
    IntStatistic* intStat3 = new IntStatistic();   
    intStat3->set_current(sg->numSpareServiceUnits.current.value);
    nssu->set_allocated_intstatistic(intStat3);
    sgStatus->set_allocated_numspareserviceunits(nssu);

    return rc;
  }

  ClRcT fillSiStatus(const SAFplusAmf::ServiceInstance* si, ServiceInstanceStatus* siStatus)
  {
    ClRcT rc = CL_OK;
    logDebug("MGMT","RPC", "server is processing [%s] for entity [%s]", __FUNCTION__, si->name.value.c_str());
    siStatus->set_name(si->name.value);

    siStatus->set_assignmentstate(static_cast<::SAFplus::Rpc::amfMgmtRpc::AssignmentState>(si->assignmentState.value));

    SAFplus::MgtIdentifierList<SAFplusAmf::ServiceUnit*>::iterator itsu = const_cast<SAFplusAmf::ServiceInstance*>(si)->activeAssignments.listBegin();
    SAFplus::MgtIdentifierList<SAFplusAmf::ServiceUnit*>::iterator itend = const_cast<SAFplusAmf::ServiceInstance*>(si)->activeAssignments.listEnd();
    for (; itsu != itend; itsu++)
    {      
      SAFplusAmf::ServiceUnit* su = dynamic_cast<SAFplusAmf::ServiceUnit*>(*itsu);
      siStatus->add_activeassignments(su->name.value);
    }

    itsu = const_cast<SAFplusAmf::ServiceInstance*>(si)->standbyAssignments.listBegin();
    itend = const_cast<SAFplusAmf::ServiceInstance*>(si)->standbyAssignments.listEnd();
    for (; itsu != itend; itsu++)
    {      
      SAFplusAmf::ServiceUnit* su = dynamic_cast<SAFplusAmf::ServiceUnit*>(*itsu);
      siStatus->add_standbyassignments(su->name.value);
    }

    NumActiveAssignments* naa = new NumActiveAssignments();
    IntStatistic* intStat = new IntStatistic();   
    intStat->set_current(si->numActiveAssignments.current.value);
    naa->set_allocated_intstatistic(intStat);
    siStatus->set_allocated_numactiveassignments(naa);

    NumStandbyAssignments* nsa = new NumStandbyAssignments();
    IntStatistic* intStat2 = new IntStatistic();   
    intStat2->set_current(si->numStandbyAssignments.current.value);
    nsa->set_allocated_intstatistic(intStat2);
    siStatus->set_allocated_numstandbyassignments(nsa);    

    return rc;
  }

  ClRcT fillCsiStatus(const SAFplusAmf::ComponentServiceInstance* csi, ComponentServiceInstanceStatus* csiStatus)
  {
    ClRcT rc = CL_OK;
    logDebug("MGMT","RPC", "server is processing [%s] for entity [%s]", __FUNCTION__, csi->name.value.c_str());
    csiStatus->set_name(csi->name.value);

    SAFplus::MgtIdentifierList<SAFplusAmf::Component*>::iterator itcomp = const_cast<SAFplusAmf::ComponentServiceInstance*>(csi)->standbyComponents.listBegin();
    SAFplus::MgtIdentifierList<SAFplusAmf::Component*>::iterator itend = const_cast<SAFplusAmf::ComponentServiceInstance*>(csi)->standbyComponents.listEnd();
    for (; itcomp != itend; itcomp++)
    {
      SAFplusAmf::Component* c = dynamic_cast<SAFplusAmf::Component*>(*itcomp);
      csiStatus->add_standbycomponents(c->name.value);
    }

    itcomp = const_cast<SAFplusAmf::ComponentServiceInstance*>(csi)->activeComponents.listBegin();
    itend = const_cast<SAFplusAmf::ComponentServiceInstance*>(csi)->activeComponents.listEnd();
    for (; itcomp != itend; itcomp++)
    {
      SAFplusAmf::Component* c = dynamic_cast<SAFplusAmf::Component*>(*itcomp);
      csiStatus->add_activecomponents(c->name.value);
    }

    return rc;
  }

  ClRcT nodeCommit(const NodeConfig& node)
  {
    ClRcT rc = CL_OK;
    logDebug("MGMT","RPC", "server is processing [%s] for entity [%s]", __FUNCTION__, node.name().c_str());
    if (node.name().length()==0)
    {
      return CL_ERR_INVALID_PARAMETER;      
    }
    rc = addEntityToDatabase("/safplusAmf","Node", node.name());
    if (rc == CL_ERR_ALREADY_EXIST)
    {
       logNotice("MGMT","NODE.COMMIT","xpath for entity [%s] already exists", node.name().c_str());
    }
    if (rc != CL_OK) return rc;
    if (node.has_adminstate())
    {
      SAFplusAmf::AdministrativeState as = static_cast<SAFplusAmf::AdministrativeState>(node.adminstate());
      std::stringstream ssAs;
      ssAs<<as;
      std::string strAs;
      ssAs>>strAs;
      MGMT_CALL(updateEntityFromDatabase("/safplusAmf/Node",node.name(),"adminState",strAs));
    }    
    if (node.has_autorepair())
    {
      bool autoRepair = node.autorepair();
      MGMT_CALL(updateEntityFromDatabase("/safplusAmf/Node",node.name(),"autoRepair",B2S(autoRepair)));
    }  
    if (node.has_failfastoninstantiationfailure())
    {
      bool ffoif = node.failfastoninstantiationfailure();
      MGMT_CALL(updateEntityFromDatabase("/safplusAmf/Node",node.name(),"failFastOnInstantiationFailure",B2S(ffoif)));
    }
    if (node.has_failfastoncleanupfailure())
    {
      bool ffocf = node.failfastoncleanupfailure();
      MGMT_CALL(updateEntityFromDatabase("/safplusAmf/Node",node.name(),"failFastOnCleanupFailure",B2S(ffocf)));
    }    
    if (node.has_serviceunitfailureescalationpolicy())
    {
      if (node.serviceunitfailureescalationpolicy().has_escalationpolicy())
      {
        const EscalationPolicy& ep = node.serviceunitfailureescalationpolicy().escalationpolicy();
        if (ep.has_maximum() && ep.has_duration())
        {
          uint64_t max = ep.maximum();
          SaTimeT duration = ep.duration();
          char temp[20];
          snprintf(temp, 19, "%" PRId64, max);
          std::string strMax = temp;
          snprintf(temp, 19, "%" PRId64, duration.uint64());
          std::string strDuration = temp;
          std::vector<std::string> v;
          v.push_back(std::string("maximum"));
          v.push_back(std::string("duration"));
          std::string value;
          MGMT_CALL(updateEntityFromDatabase("/safplusAmf/Node",node.name(),"serviceUnitFailureEscalationPolicy",value,&v));          
          MGMT_CALL(updateEntityFromDatabase("/safplusAmf/Node",node.name(),"serviceUnitFailureEscalationPolicy/maximum",strMax));
          MGMT_CALL(updateEntityFromDatabase("/safplusAmf/Node",node.name(),"serviceUnitFailureEscalationPolicy/duration",strDuration));
        }
      }
    }
    int susSize = 0;
    if ((susSize=node.serviceunits_size())>0)
    {
      std::vector<std::string> sus;
      for (int i=0;i<susSize;i++)
      {
         sus.push_back(node.serviceunits(i));
      }
      MGMT_CALL(updateEntityAsListTypeFromDatabase("/safplusAmf/Node",node.name(),"serviceUnits",sus));
    }

    return rc;
  }

  ClRcT fillNodeConfig(const SAFplusAmf::Node* node, NodeConfig* nodeConfig)
  {
    ClRcT rc = CL_OK;
    logDebug("MGMT","RPC", "server is processing [%s] for entity [%s]", __FUNCTION__, node->name.value.c_str());
    nodeConfig->set_name(node->name.value);
    nodeConfig->set_adminstate(static_cast<SAFplus::Rpc::amfMgmtRpc::AdministrativeState>(node->adminState.value));
    
    nodeConfig->set_autorepair(node->autoRepair.value);
    
    nodeConfig->set_failfastoninstantiationfailure(node->failFastOnInstantiationFailure.value);

    nodeConfig->set_failfastoncleanupfailure(node->failFastOnCleanupFailure.value);
    
    ServiceUnitFailureEscalationPolicy* sufeop = new ServiceUnitFailureEscalationPolicy();
    EscalationPolicy* ep = new EscalationPolicy();
    ep->set_maximum(node->serviceUnitFailureEscalationPolicy.maximum.value);
    SaTimeT* duration = new SaTimeT();
    duration->set_uint64(node->serviceUnitFailureEscalationPolicy.duration.value);
    ep->set_allocated_duration(duration);
    sufeop->set_allocated_escalationpolicy(ep);
    nodeConfig->set_allocated_serviceunitfailureescalationpolicy(sufeop);
    
    SAFplus::MgtIdentifierList<SAFplusAmf::ServiceUnit*>::iterator itsu = const_cast<SAFplusAmf::Node*>(node)->serviceUnits.listBegin();
    SAFplus::MgtIdentifierList<SAFplusAmf::ServiceUnit*>::iterator itend = const_cast<SAFplusAmf::Node*>(node)->serviceUnits.listEnd();
    for (; itsu != itend; itsu++)
    {      
      SAFplusAmf::ServiceUnit* su = dynamic_cast<SAFplusAmf::ServiceUnit*>(*itsu);
      nodeConfig->add_serviceunits(su->name.value);
    }

    return rc;
  }

  ClRcT fillNodeStatus(const SAFplusAmf::Node* node, NodeStatus* nodeStatus)
  {
    ClRcT rc = CL_OK;
    logDebug("MGMT","RPC", "server is processing [%s] for entity [%s]", __FUNCTION__, node->name.value.c_str());

    nodeStatus->set_name(node->name.value);
    
    Stats* stats = new Stats();
    Load* load = new Load();
    User* user = new User();    
    DecStatistic* decStat = new DecStatistic();
    decStat->set_current(node->stats.load.user.current.value);
    user->set_allocated_decstatistic(decStat);
    load->set_allocated_user(user);

    LowPriorityUser* luser = new LowPriorityUser();    
    DecStatistic* decStat2 = new DecStatistic();
    decStat2->set_current(node->stats.load.lowPriorityUser.current.value);
    luser->set_allocated_decstatistic(decStat2);
    load->set_allocated_lowpriorityuser(luser);

    IoWait* ioWait = new IoWait();    
    DecStatistic* decStat3 = new DecStatistic();
    decStat3->set_current(node->stats.load.ioWait.current.value);
    ioWait->set_allocated_decstatistic(decStat3);
    load->set_allocated_iowait(ioWait);

    SysTime* sysTime = new SysTime();    
    DecStatistic* decStat4 = new DecStatistic();
    decStat4->set_current(node->stats.load.sysTime.current.value);
    sysTime->set_allocated_decstatistic(decStat4);
    load->set_allocated_systime(sysTime);

    IntTime* intTime = new IntTime();    
    DecStatistic* decStat5 = new DecStatistic();
    decStat5->set_current(node->stats.load.intTime.current.value);
    intTime->set_allocated_decstatistic(decStat5);
    load->set_allocated_inttime(intTime);

    SoftIrqs* softIrqs = new SoftIrqs();    
    DecStatistic* decStat6 = new DecStatistic();
    decStat6->set_current(node->stats.load.softIrqs.current.value);
    softIrqs->set_allocated_decstatistic(decStat6);
    load->set_allocated_softirqs(softIrqs);

    Idle* idle = new Idle();    
    DecStatistic* decStat7 = new DecStatistic();
    decStat7->set_current(node->stats.load.idle.current.value);
    idle->set_allocated_decstatistic(decStat7);
    load->set_allocated_idle(idle);

    ContextSwitches* contextSwitches = new ContextSwitches();
    IntStatistic* intStat = new IntStatistic();   
    intStat->set_current(node->stats.load.contextSwitches.current.value);
    contextSwitches->set_allocated_intstatistic(intStat);
    load->set_allocated_contextswitches(contextSwitches);

    ProcessCount* processCount = new ProcessCount();
    IntStatistic* intStat2 = new IntStatistic();   
    intStat2->set_current(node->stats.load.processCount.current.value);
    processCount->set_allocated_intstatistic(intStat2);
    load->set_allocated_processcount(processCount);

    ProcessStarts* processStarts = new ProcessStarts();
    IntStatistic* intStat3 = new IntStatistic();   
    intStat3->set_current(node->stats.load.processStarts.current.value);
    processStarts->set_allocated_intstatistic(intStat3);
    load->set_allocated_processstarts(processStarts);

    stats->set_allocated_load(load);
//    nodeStatus->set_allocated_stats(stats);

    stats->set_uptime(node->stats.upTime.value);
    stats->set_boottime(node->stats.bootTime.value);
    nodeStatus->set_allocated_stats(stats); 

    nodeStatus->set_presencestate(static_cast<::SAFplus::Rpc::amfMgmtRpc::PresenceState>(node->presenceState.value));
    nodeStatus->set_operstate(node->operState.value);

    return rc;
  }

  ClRcT suDeleteCommit(const std::string& suName)
  {
    ClRcT rc = CL_OK;
    logDebug("MGMT","RPC", "server is processing [%s] for entity [%s]", __FUNCTION__, suName.c_str());
    if (suName.length()==0)
    {
      return CL_ERR_INVALID_PARAMETER;
    }
    SAFplusAmf::ServiceUnit* su = dynamic_cast<SAFplusAmf::ServiceUnit*>(cfg.safplusAmf.serviceUnitList[suName]);
    SAFplusAmf::ServiceGroup* sg = NULL;
    SAFplusAmf::Node* node = NULL;
    if (su)
    {
       sg = su->serviceGroup.value;
       node = su->node.value;
    }
    else
    {
       logError("MGMT","RPC", "sg obj for su [%s] not found", suName.c_str());
    }
    ScopedLock<ProcSem> lock(amfOpsMgmt->mutex);
    rc = cfg.safplusAmf.serviceUnitList.deleteObj(suName);
    if (rc != CL_OK)
    {
       logError("MGMT","RPC", "delete su name [%s] failed, rc [0x%x]", suName.c_str(),rc);
       return rc;
    }
    rc = deleteEntityFromDatabase("/safplusAmf", "ServiceUnit",  suName, "serviceUnits");
    if (rc != CL_OK) {
       logError("MGMT","RPC", "delete su name [%s] failed, rc [0x%x]", suName.c_str(),rc);
       return rc;
    }
    if (sg) {
       logDebug("MGMT","RPC", "read for sg [%s]",sg->name.value.c_str());
       sg->read();
    }
    if (node) {
       logDebug("MGMT","RPC", "read for node [%s]", node->name.value.c_str());
       node->read();
    }
    return rc;
  }

  ClRcT sgDeleteCommit(const std::string& sgName)
  {
    ClRcT rc = CL_OK;
    logDebug("MGMT","RPC", "server is processing [%s] for entity [%s]", __FUNCTION__, sgName.c_str());
    if (sgName.length()==0)
    {
      return CL_ERR_INVALID_PARAMETER;      
    }
    ScopedLock<ProcSem> lock(amfOpsMgmt->mutex);
    rc = cfg.safplusAmf.serviceGroupList.deleteObj(sgName);
    if (rc != CL_OK)
    {
       logError("MGMT","RPC", "delete sg name [%s] failed, rc [0x%x]", sgName.c_str(),rc);
       return rc;
    }
    rc = deleteEntityFromDatabase("/safplusAmf", "ServiceGroup",  sgName, "serviceGroups");
    if (rc != CL_OK)
       logError("MGMT","RPC", "delete sg name [%s] failed, rc [0x%x]", sgName.c_str(),rc);
    return rc;
  }

  ClRcT nodeDeleteCommit(const std::string& nodeName)
  {
    ClRcT rc = CL_OK;
    logDebug("MGMT","RPC", "server is processing [%s] for entity [%s]", __FUNCTION__, nodeName.c_str());
    if (nodeName.length()==0)
    {
      return CL_ERR_INVALID_PARAMETER;      
    }
    ScopedLock<ProcSem> lock(amfOpsMgmt->mutex);
    rc = cfg.safplusAmf.nodeList.deleteObj(nodeName);
    if (rc != CL_OK)
    {
       logError("MGMT","RPC", "delete node name [%s] failed, rc [0x%x]", nodeName.c_str(),rc);
       return rc;
    }
    rc = deleteEntityFromDatabase("/safplusAmf", "Node",  nodeName, "nodes");
    if (rc != CL_OK)
       logError("MGMT","RPC", "delete node name [%s] failed, rc [0x%x]",nodeName.c_str(),rc);
    return rc;
  }

  ClRcT siCommit(const ServiceInstanceConfig& si)
  {
    ClRcT rc = CL_OK;
    logDebug("MGMT","RPC", "server is processing [%s] for entity [%s]", __FUNCTION__, si.name().c_str());
    if (si.name().length()==0)
    {
      return CL_ERR_INVALID_PARAMETER;      
    }
    rc = addEntityToDatabase("/safplusAmf","ServiceInstance", si.name());
    if (rc == CL_ERR_ALREADY_EXIST)
    {
       logNotice("MGMT","SG.COMMIT","xpath for entity [%s] already exists", si.name().c_str());
    }
    if (rc != CL_OK) return rc;
    if (si.has_adminstate())
    {
      SAFplusAmf::AdministrativeState as = static_cast<SAFplusAmf::AdministrativeState>(si.adminstate());
      std::stringstream ssAs;
      ssAs<<as;
      std::string strAs;
      ssAs>>strAs;
      MGMT_CALL(updateEntityFromDatabase("/safplusAmf/ServiceInstance",si.name(),"adminState",strAs));
    }    
    if (si.has_preferredactiveassignments())
    {
      uint32_t paa = si.preferredactiveassignments();
      char temp[10];
      snprintf(temp, 9, "%ul", paa);
      std::string strPaa = temp;
      MGMT_CALL(updateEntityFromDatabase("/safplusAmf/ServiceInstance",si.name(),"preferredActiveAssignments",strPaa));
    }
    if (si.has_preferredstandbyassignments())
    {
      uint32_t psa = si.preferredstandbyassignments();
      char temp[10];
      snprintf(temp, 9, "%ul", psa);
      std::string strPsa = temp;
      MGMT_CALL(updateEntityFromDatabase("/safplusAmf/ServiceInstance",si.name(),"preferredStandbyAssignments",strPsa));
    }
    if (si.has_rank())
    {
      uint32_t rank = si.rank();
      char temp[10];
      snprintf(temp, 9, "%ul", rank);
      std::string strRank = temp;
      MGMT_CALL(updateEntityFromDatabase("/safplusAmf/ServiceInstance",si.name(),"rank",strRank));
    }
    int csiSize = 0;
    if ((csiSize=si.componentserviceinstances_size())>0)
    {
      std::vector<std::string> csis;
      for (int i=0;i<csiSize;i++)
      {
         csis.push_back(si.componentserviceinstances(i));
      }
      MGMT_CALL(updateEntityAsListTypeFromDatabase("/safplusAmf/ServiceInstance",si.name(),"componentServiceInstances",csis));
    }        
    if (si.has_servicegroup())
    {
      const std::string& sg = si.servicegroup();
      MGMT_CALL(updateEntityFromDatabase("/safplusAmf/ServiceInstance",si.name(),"serviceGroup",sg));
    }

    return rc;
  }

  ClRcT fillSiConfig(const SAFplusAmf::ServiceInstance* si, ServiceInstanceConfig* siConfig)
  {
    ClRcT rc = CL_OK;
    logDebug("MGMT","RPC", "server is processing [%s] for entity [%s]", __FUNCTION__, si->name.value.c_str());
    siConfig->set_name(si->name.value);
    siConfig->set_adminstate(static_cast<SAFplus::Rpc::amfMgmtRpc::AdministrativeState>(si->adminState.value));
    siConfig->set_preferredactiveassignments(si->preferredActiveAssignments.value);
    siConfig->set_preferredstandbyassignments(si->preferredStandbyAssignments.value);
    siConfig->set_rank(si->rank.value);

    SAFplus::MgtIdentifierList<SAFplusAmf::ComponentServiceInstance*>::iterator itcsi = const_cast<SAFplusAmf::ServiceInstance*>(si)->componentServiceInstances.listBegin();
    SAFplus::MgtIdentifierList<SAFplusAmf::ComponentServiceInstance*>::iterator itend = const_cast<SAFplusAmf::ServiceInstance*>(si)->componentServiceInstances.listEnd();
    for (; itcsi != itend; itcsi++)
    {      
      SAFplusAmf::ComponentServiceInstance* csi = dynamic_cast<SAFplusAmf::ComponentServiceInstance*>(*itcsi);
      siConfig->add_componentserviceinstances(csi->name.value);
    }
        
    siConfig->set_servicegroup(si->serviceGroup.value->name.value);   

    return rc;
  }

  ClRcT siDeleteCommit(const std::string& siName)
  {
    ClRcT rc = CL_OK;
    logDebug("MGMT","RPC", "server is processing [%s] for entity [%s]", __FUNCTION__, siName.c_str());
    if (siName.length()==0)
    {
      return CL_ERR_INVALID_PARAMETER;      
    }
    SAFplusAmf::ServiceInstance* si = dynamic_cast<SAFplusAmf::ServiceInstance*>(cfg.safplusAmf.serviceInstanceList[siName]);
    SAFplusAmf::ServiceGroup* sg = NULL;
    if (si)
    {
       sg = si->serviceGroup.value;
    }
    else
    {
       logError("MGMT","RPC", "sg obj for si [%s] not found", siName.c_str());
    }
    ScopedLock<ProcSem> lock(amfOpsMgmt->mutex);
    rc = cfg.safplusAmf.serviceInstanceList.deleteObj(siName);
    if (rc != CL_OK)
    {
       logError("MGMT","RPC", "delete si name [%s] failed, rc [0x%x]", siName.c_str(),rc);
       return rc;
    }
    rc = deleteEntityFromDatabase("/safplusAmf", "ServiceInstance",  siName, "serviceInstances");
    if (rc != CL_OK) {
       logError("MGMT","RPC", "delete si name [%s] failed, rc [0x%x]",siName.c_str(),rc);
       return rc;
    }
    if (sg) {
       logDebug("MGMT","RPC", "read for sg [%s]",sg->name.value.c_str());
       sg->read();
    }

    return rc;
  }

  ClRcT csiCommit(const ComponentServiceInstanceConfig& csi)
  {
    ClRcT rc = CL_OK;
    logDebug("MGMT","RPC", "server is processing [%s] for entity [%s]", __FUNCTION__, csi.name().c_str());
    if (csi.name().length()==0)
    {
      return CL_ERR_INVALID_PARAMETER;      
    }
    rc = addEntityToDatabase("/safplusAmf","ComponentServiceInstance", csi.name());
    if (rc == CL_ERR_ALREADY_EXIST)
    {
       logNotice("MGMT","SG.COMMIT","xpath for entity [%s] already exists", csi.name().c_str());
    }
    if (rc != CL_OK) return rc;
    int depSize = 0;
    if ((depSize=csi.dependencies_size())>0)
    {
      std::vector<std::string> deps;
      for (int i=0;i<depSize;i++)
      {
         deps.push_back(csi.dependencies(i));
      }
      MGMT_CALL(updateEntityAsListTypeFromDatabase("/safplusAmf/ComponentServiceInstance",csi.name(),"dependencies",deps));
    }
    int dataSize = 0;
    if ((dataSize=csi.data_size())>0)
    {
      KeyValueHashMap data;
      for (int i=0;i<dataSize;i++)
      {
         KeyValueMapPair kvp(csi.data(i).name(),csi.data(i).val());
         data.insert(kvp);         
      }
      MGMT_CALL(updateEntityFromDatabase("/safplusAmf/ComponentServiceInstance", csi.name(), "data", "val", data));
    }           
    if (csi.has_serviceinstance())
    {
      const std::string& si = csi.serviceinstance();
      MGMT_CALL(updateEntityFromDatabase("/safplusAmf/ComponentServiceInstance",csi.name(),"serviceInstance",si));
    }
    if (csi.has_type())
    {
      const std::string& type = csi.type();
      MGMT_CALL(updateEntityFromDatabase("/safplusAmf/ComponentServiceInstance",csi.name(),"type",type));
    }

    return rc;
  }

  ClRcT fillCsiConfig(const SAFplusAmf::ComponentServiceInstance* csi, ComponentServiceInstanceConfig* csiConfig)
  {
    ClRcT rc = CL_OK;
    logDebug("MGMT","RPC", "server is processing [%s] for entity [%s]", __FUNCTION__, csi->name.value.c_str());
    csiConfig->set_name(csi->name.value);
    SAFplus::MgtIdentifierList<SAFplusAmf::ComponentServiceInstance*>::iterator itcsiDep = const_cast<SAFplusAmf::ComponentServiceInstance*>(csi)->dependencies.listBegin();
    SAFplus::MgtIdentifierList<SAFplusAmf::ComponentServiceInstance*>::iterator itend = const_cast<SAFplusAmf::ComponentServiceInstance*>(csi)->dependencies.listEnd();
    for (; itcsiDep != itend; itcsiDep++)
    {      
      SAFplusAmf::ComponentServiceInstance* csiDep = dynamic_cast<SAFplusAmf::ComponentServiceInstance*>(*itcsiDep);
      csiConfig->add_dependencies(csiDep->name.value);
    }
    
    SAFplus::MgtList<std::string>::Iterator it = const_cast<SAFplusAmf::ComponentServiceInstance*>(csi)->dataList.begin();
    SAFplus::MgtList<std::string>::Iterator itdataend = const_cast<SAFplusAmf::ComponentServiceInstance*>(csi)->dataList.end();
    int i=0;
    for (; it != itdataend; it++)
    {
      logDebug("MGMT","RPC","[%s] key [%s]", __FUNCTION__, it->first.c_str());
      SAFplus::MgtObject *obj = it->second;
      SAFplusAmf::Data* kv =  dynamic_cast<SAFplusAmf::Data*>(obj); 
      csiConfig->add_data();
      Data* data = csiConfig->mutable_data(i);
      data->set_name(kv->name);
      data->set_val(kv->val);
      i++;
    }
    
    csiConfig->set_serviceinstance(csi->serviceInstance.value->name.value);
    csiConfig->set_type(csi->type.value);

    return rc;
  }

  ClRcT csiDeleteCommit(const std::string& csiName)
  {
    ClRcT rc = CL_OK;
    logDebug("MGMT","RPC", "server is processing [%s] for entity [%s]", __FUNCTION__, csiName.c_str());
    if (csiName.length()==0)
    {
      return CL_ERR_INVALID_PARAMETER;      
    }
    SAFplusAmf::ComponentServiceInstance* csi = dynamic_cast<SAFplusAmf::ComponentServiceInstance*>(cfg.safplusAmf.componentServiceInstanceList[csiName]);
    SAFplusAmf::ServiceInstance* si = NULL;
    if (si)
    {
       si = csi->serviceInstance.value;
    }
    else
    {
       logError("MGMT","RPC", "si obj for csi [%s] not found", csiName.c_str());
    }
    ScopedLock<ProcSem> lock(amfOpsMgmt->mutex);
    rc = cfg.safplusAmf.componentServiceInstanceList.deleteObj(csiName);
    if (rc != CL_OK)
    {
       logError("MGMT","RPC", "delete csi name [%s] failed, rc [0x%x]", csiName.c_str(),rc);
       return rc;
    }
    rc = deleteEntityFromDatabase("/safplusAmf", "ComponentServiceInstance",  csiName, "componentServiceInstances");
    if (rc != CL_OK) {
       logError("MGMT","RPC", "delete csi name [%s] failed, rc [0x%x]",csiName.c_str(),rc);
       return rc;
    }
    if (si) {
       logDebug("MGMT","RPC", "read for si [%s]",si->name.value.c_str());
       si->read();
    }
    return rc;
  }

  ClRcT csiNVPDeleteCommit(const std::string& csiName)
  {
    ClRcT rc = CL_OK;
    logDebug("MGMT","RPC", "server is processing [%s] for entity [%s]", __FUNCTION__, csiName.c_str());
    if (csiName.length()==0)
    {
      return CL_ERR_INVALID_PARAMETER;      
    }
    ScopedLock<ProcSem> lock(amfOpsMgmt->mutex);
    SAFplusAmf::ComponentServiceInstance* csi = dynamic_cast<SAFplusAmf::ComponentServiceInstance*>(cfg.safplusAmf.componentServiceInstanceList[csiName]);
    if (csi == NULL)
    {
       logError("MGMT","RPC", "csi object with name [%s] doesn't exist", csiName.c_str());
       return CL_ERR_NOT_EXIST;
    }
    SAFplus::MgtList<std::string>::Iterator it;
    //Store keys of data KVP list
    std::vector<std::string> keys;
    for (it = csi->dataList.begin(); it != csi->dataList.end(); it++)
    {
      SAFplusAmf::Data* kv =  dynamic_cast<SAFplusAmf::Data*>(it->second); 
      logDebug("MGMT","RPC", "storing key [%s] to vector", kv->name.value.c_str());
      keys.push_back(kv->name.value);
    }
    std::vector<std::string>::iterator itKey;
    for (itKey = keys.begin(); itKey != keys.end(); itKey++)
    {
      rc = csi->dataList.deleteObj("/safplusAmf/ComponentServiceInstance",csiName,"data","val",*itKey);
      logDebug("MGMT","RPC", "deleting kvp key name [%s] of csi [%s] return [0x%x]", (*itKey).c_str(),csiName.c_str(),rc);
      if (rc != CL_OK)
      {
        break;
      }
    }
    if (keys.size() == 0)
    {
      logInfo("MGMT","RPC", "no KVP found for csi [%s]", csiName.c_str());
      rc = CL_ERR_NOT_EXIST;
    }
#if 0
    std::string data = "data";
    rc = csi->deleteObj(data);
    logDebug("MGMT","RPC", "deleting kvp key name [%s] of csi [%s] return [0x%x]", data.c_str(),csiName.c_str(),rc);
#endif
    return rc;
  }

  ClRcT nodeSUListDeleteCommit(const DeleteNodeSUListRequest& request)
  {
    const std::string& nodeName = request.nodename();
    logDebug("MGMT","RPC", "server is processing [%s] for entity [%s]", __FUNCTION__, nodeName.c_str());
    if (nodeName.length()==0)
    {
      return CL_ERR_INVALID_PARAMETER;      
    }
    ClRcT rc = CL_OK;
    int suSize = 0;
    if ((suSize=request.sulist_size())>0)
    {
      std::vector<std::string> sus;
      for (int i=0;i<suSize;i++)
      {
         sus.push_back(request.sulist(i));
      }
      MGMT_CALL(deleteEntityAsListTypeFromDatabase("/safplusAmf/Node",nodeName,"node","serviceUnits",sus));
    }
    else
    {
      logError("MGMT","RPC", "su list is empty for delete su list of node [%s]", nodeName.c_str());
      rc = CL_ERR_INVALID_PARAMETER;
    }
    return rc;
  }

  ClRcT sgSUListDeleteCommit(const DeleteSGSUListRequest& request)
  {
    const std::string& sgName = request.sgname();
    logDebug("MGMT","RPC", "server is processing [%s] for entity [%s]", __FUNCTION__, sgName.c_str());
    if (sgName.length()==0)
    {
      return CL_ERR_INVALID_PARAMETER;      
    }
    ClRcT rc = CL_OK;
    int suSize = 0;
    if ((suSize=request.sulist_size())>0)
    {
      std::vector<std::string> sus;
      for (int i=0;i<suSize;i++)
      {
         sus.push_back(request.sulist(i));
      }
      MGMT_CALL(deleteEntityAsListTypeFromDatabase("/safplusAmf/ServiceGroup",sgName,"serviceGroup","serviceUnits",sus));
    }
    else
    {
      logError("MGMT","RPC", "su list is empty for delete su list of node [%s]", sgName.c_str());
      rc = CL_ERR_INVALID_PARAMETER;
    }
    return rc;
  }

  ClRcT sgSIListDeleteCommit(const DeleteSGSIListRequest& request)
  {
    const std::string& sgName = request.sgname();
    logDebug("MGMT","RPC", "server is processing [%s] for entity [%s]", __FUNCTION__, sgName.c_str());
    if (sgName.length()==0)
    {
      return CL_ERR_INVALID_PARAMETER;      
    }
    ClRcT rc = CL_OK;
    int siSize = 0;
    if ((siSize=request.silist_size())>0)
    {
      std::vector<std::string> sis;
      for (int i=0;i<siSize;i++)
      {
         sis.push_back(request.silist(i));
      }
      MGMT_CALL(deleteEntityAsListTypeFromDatabase("/safplusAmf/ServiceGroup",sgName,"serviceGroup","serviceInstances",sis));
    }
    else
    {
      logError("MGMT","RPC", "su list is empty for delete su list of node [%s]", sgName.c_str());
      rc = CL_ERR_INVALID_PARAMETER;
    }
    return rc;
  }

  ClRcT suCompListDeleteCommit(const DeleteSUCompListRequest& request)
  {
    const std::string& suName = request.suname();
    logDebug("MGMT","RPC", "server is processing [%s] for entity [%s]", __FUNCTION__, suName.c_str());
    if (suName.length()==0)
    {
      return CL_ERR_INVALID_PARAMETER;      
    }
    ClRcT rc = CL_OK;
    int compSize = 0;
    if ((compSize=request.complist_size())>0)
    {
      std::vector<std::string> comps;
      for (int i=0;i<compSize;i++)
      {
         comps.push_back(request.complist(i));
      }
      MGMT_CALL(deleteEntityAsListTypeFromDatabase("/safplusAmf/ServiceUnit",suName,"serviceUnit","components",comps));
    }
    else
    {
      logError("MGMT","RPC", "su list is empty for delete su list of node [%s]", suName.c_str());
      rc = CL_ERR_INVALID_PARAMETER;
    }
    return rc;
  }

  ClRcT siCSIListDeleteCommit(const DeleteSICSIListRequest& request)
  {
    const std::string& siName = request.siname();
    logDebug("MGMT","RPC", "server is processing [%s] for entity [%s]", __FUNCTION__, siName.c_str());
    if (siName.length()==0)
    {
      return CL_ERR_INVALID_PARAMETER;      
    }
    ClRcT rc = CL_OK;
    int csiSize = 0;
    if ((csiSize=request.csilist_size())>0)
    {
      std::vector<std::string> csi;
      for (int i=0;i<csiSize;i++)
      {
         csi.push_back(request.csilist(i));
      }
      MGMT_CALL(deleteEntityAsListTypeFromDatabase("/safplusAmf/ServiceInstance",siName,"serviceInstance","componentServiceInstances",csi));
    }
    else
    {
      logError("MGMT","RPC", "su list is empty for delete su list of node [%s]", siName.c_str());
      rc = CL_ERR_INVALID_PARAMETER;
    }
    return rc;
  }

  ClRcT handleCommit(const ClDBKeyHandleT recKey, ClUint32T keySize, const ClDBRecordHandleT recData, ClUint32T dataSize)
  {
    int op = 0;
    ClRcT rc = CL_OK;
    memcpy(&op, recKey, keySize);
    switch (op>>16)
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
      {
        CreateSGRequest request;
        std::string strRequestData;
        strRequestData.assign((ClCharT*)recData, dataSize);
        request.ParseFromString(strRequestData);
        const ServiceGroupConfig& sg = request.servicegroupconfig();
        logDebug("MGMT","COMMIT","handleCommit op [%d], entity [%s]", op, sg.name().c_str());
        rc = sgCommit(sg);
        break;
      }
    case AMF_MGMT_OP_SG_UPDATE:
      {
        UpdateSGRequest request;
        std::string strRequestData;
        strRequestData.assign((ClCharT*)recData, dataSize);
        request.ParseFromString(strRequestData);
        const ServiceGroupConfig& sg = request.servicegroupconfig();
        logDebug("MGMT","COMMIT","handleCommit op [%d], entity [%s]", op, sg.name().c_str());
        rc = sgCommit(sg);
        break;
      }
    case AMF_MGMT_OP_SG_DELETE:
      {
        DeleteSGRequest request;
        std::string strRequestData;
        strRequestData.assign((ClCharT*)recData, dataSize);
        request.ParseFromString(strRequestData);
        rc = sgDeleteCommit(request.name());
        break;
      }
    case AMF_MGMT_OP_NODE_CREATE:
      {
        CreateNodeRequest request;
        std::string strRequestData;
        strRequestData.assign((ClCharT*)recData, dataSize);
        request.ParseFromString(strRequestData);
        const NodeConfig& node = request.nodeconfig();
        logDebug("MGMT","COMMIT","handleCommit op [%d], entity [%s]", op, node.name().c_str());
        rc = nodeCommit(node);
        break;
      }
    case AMF_MGMT_OP_NODE_UPDATE:
      {
        UpdateNodeRequest request;
        std::string strRequestData;
        strRequestData.assign((ClCharT*)recData, dataSize);
        request.ParseFromString(strRequestData);
        const NodeConfig& node = request.nodeconfig();
        logDebug("MGMT","COMMIT","handleCommit op [%d], entity [%s]", op, node.name().c_str());
        rc = nodeCommit(node);
        break;
      }
    case AMF_MGMT_OP_NODE_DELETE:
      {
        DeleteNodeRequest request;
        std::string strRequestData;
        strRequestData.assign((ClCharT*)recData, dataSize);
        request.ParseFromString(strRequestData);
        rc = nodeDeleteCommit(request.name());
        break;
      }
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
      {
        CreateSIRequest request;
        std::string strRequestData;
        strRequestData.assign((ClCharT*)recData, dataSize);
        request.ParseFromString(strRequestData);
        const ServiceInstanceConfig& si = request.serviceinstanceconfig();
        logDebug("MGMT","COMMIT","handleCommit op [%d], entity [%s]", op, si.name().c_str());
        rc = siCommit(si);
        break;
      }
    case AMF_MGMT_OP_SI_UPDATE:
      {
        UpdateSIRequest request;
        std::string strRequestData;
        strRequestData.assign((ClCharT*)recData, dataSize);
        request.ParseFromString(strRequestData);
        const ServiceInstanceConfig& si = request.serviceinstanceconfig();
        logDebug("MGMT","COMMIT","handleCommit op [%d], entity [%s]", op, si.name().c_str());
        rc = siCommit(si);
        break;
      }
    case AMF_MGMT_OP_SI_DELETE:
      {
        DeleteSIRequest request;
        std::string strRequestData;
        strRequestData.assign((ClCharT*)recData, dataSize);
        request.ParseFromString(strRequestData);
        logDebug("MGMT","COMMIT","handleCommit op [%d], entity [%s]", op, request.name().c_str());
        rc = siDeleteCommit(request.name());
        break;
      }
    case AMF_MGMT_OP_CSI_CREATE:
      {
        CreateCSIRequest request;
        std::string strRequestData;
        strRequestData.assign((ClCharT*)recData, dataSize);
        request.ParseFromString(strRequestData);
        const ComponentServiceInstanceConfig& csi = request.componentserviceinstanceconfig();
        logDebug("MGMT","COMMIT","handleCommit op [%d], entity [%s]", op, csi.name().c_str());
        rc = csiCommit(csi);
        break;
      }
    case AMF_MGMT_OP_CSI_UPDATE:
      {
        UpdateCSIRequest request;
        std::string strRequestData;
        strRequestData.assign((ClCharT*)recData, dataSize);
        request.ParseFromString(strRequestData);
        const ComponentServiceInstanceConfig& csi = request.componentserviceinstanceconfig();
        logDebug("MGMT","COMMIT","handleCommit op [%d], entity [%s]", op, csi.name().c_str());
        rc = csiCommit(csi);
        break;
      }      
    case AMF_MGMT_OP_CSI_DELETE:
      {
        DeleteCSIRequest request;
        std::string strRequestData;
        strRequestData.assign((ClCharT*)recData, dataSize);
        request.ParseFromString(strRequestData);        
        logDebug("MGMT","COMMIT","handleCommit op [%d], entity [%s]", op, request.name().c_str());
        rc = csiDeleteCommit(request.name());
        break;
      }
    case AMF_MGMT_OP_CSI_NVP_DELETE:
      {
        DeleteCSINVPRequest request;
        std::string strRequestData;
        strRequestData.assign((ClCharT*)recData, dataSize);
        request.ParseFromString(strRequestData);        
        logDebug("MGMT","COMMIT","handleCommit op [%d], entity [%s]", op, request.name().c_str());
        rc = csiNVPDeleteCommit(request.name());
        break;
      }
    case AMF_MGMT_OP_NODE_SU_LIST_DELETE:
      {
        DeleteNodeSUListRequest request;
        std::string strRequestData;
        strRequestData.assign((ClCharT*)recData, dataSize);
        request.ParseFromString(strRequestData);        
        logDebug("MGMT","COMMIT","handleCommit op [%d], entity [%s]", op, request.nodename().c_str());
        rc = nodeSUListDeleteCommit(request);
        break;
      }
    case AMF_MGMT_OP_SG_SU_LIST_DELETE:
      {
        DeleteSGSUListRequest request;
        std::string strRequestData;
        strRequestData.assign((ClCharT*)recData, dataSize);
        request.ParseFromString(strRequestData);        
        logDebug("MGMT","COMMIT","handleCommit op [%d], entity [%s]", op, request.sgname().c_str());
        rc = sgSUListDeleteCommit(request);
        break;
      }
    case AMF_MGMT_OP_SG_SI_LIST_DELETE:
      {
        DeleteSGSIListRequest request;
        std::string strRequestData;
        strRequestData.assign((ClCharT*)recData, dataSize);
        request.ParseFromString(strRequestData);        
        logDebug("MGMT","COMMIT","handleCommit op [%d], entity [%s]", op, request.sgname().c_str());
        rc = sgSIListDeleteCommit(request);
        break;
      }
    case AMF_MGMT_OP_SU_COMP_LIST_DELETE:
      {
        DeleteSUCompListRequest request;
        std::string strRequestData;
        strRequestData.assign((ClCharT*)recData, dataSize);
        request.ParseFromString(strRequestData);        
        logDebug("MGMT","COMMIT","handleCommit op [%d], entity [%s]", op, request.suname().c_str());
        rc = suCompListDeleteCommit(request);
        break;
      }
    case AMF_MGMT_OP_SI_CSI_LIST_DELETE:
      {
        DeleteSICSIListRequest request;
        std::string strRequestData;
        strRequestData.assign((ClCharT*)recData, dataSize);
        request.ParseFromString(strRequestData);        
        logDebug("MGMT","COMMIT","handleCommit op [%d], entity [%s]", op, request.siname().c_str());
        rc = siCSIListDeleteCommit(request);
        break;
      }
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
      logError("MGMT","RPC","invalid rpc operation [%d]",op>>16);
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
    char fname[CL_MAX_NAME_LENGTH];
    snprintf(fname,CL_MAX_NAME_LENGTH,"%s/%s",(ASP_RUNDIR[0] != 0) ? ASP_RUNDIR : ".", dbname);
    ClRcT rc = p->open(fname, fname, CL_DB_CREAT, 255, 5000);
    if (rc == CL_OK)
    {
      logDebug("MGMT","RPC", "adding handle [%" PRIx64 ":%" PRIx64 "] and Dbal object to map", hdl.id[0],hdl.id[1]);
      DbalMapPair kv(hdl,p);
      amfMgmtMap.insert(kv);
      inc = 1;
    }
    else
    {
       logError("MGMT","RPC", "openning database [%s] filename [%s] failed", dbname, fname);
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
    ClRcT rc2 = CL_ERR_NOT_EXIST;
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
        rc2 = handleCommit(recKey,keySize,recData,dataSize);
        logDebug("MGMT","COMMIT","handleCommit returns [0x%x]", rc2);
        ClDBKeyHandleT curKey = recKey;
        pd->freeRecord(recData);
        rc = pd->getNextRecord(curKey, keySize, &recKey, &nextKeySize, &recData, &dataSize);
        logDebug("MGMT","COMMIT","[%s] getNextRecord returns [0x%x]", __FUNCTION__,rc);
        pd->freeKey(curKey);
      }
      if (rc2 == CL_OK)
      {
        logInfo("MGMT","RPC","read the DB to reflect the changes");
        ScopedLock<ProcSem> lock(amfOpsMgmt->mutex);
        cfg.read(&amfDb);
        if (CL_GET_ERROR_CODE(rc)==CL_ERR_NOT_EXIST)
        {
          rc = CL_OK;
        }
      }      
    }
    else
    {
      logError("MGMT","COMMIT","no Dbal object found for the specified amfMgmtHandle");
    }
    if (rc2 != CL_OK)
    {
      response->set_err(rc2);      
    }
    else
    {
      response->set_err(rc);
    }
    if (rc == CL_OK && rc2 == CL_OK) inc = 1;
  }

  ClRcT validateAdminState(int opId, SAFplusAmf::EntityId* containingEntity, const ::google::protobuf::RepeatedPtrField< ::std::string>& list)
  {
    ClRcT rc = CL_OK;
    switch (opId>>16)
    {
    case AMF_MGMT_OP_NODE_SU_LIST_DELETE:
      {
        SAFplusAmf::Node* node = dynamic_cast<SAFplusAmf::Node*>(containingEntity);
        for (int i=0;i<list.size();i++)
        {
          const std::string& suName = list.Get(i);
          SAFplusAmf::ServiceUnit* su = dynamic_cast<SAFplusAmf::ServiceUnit*>(cfg.safplusAmf.serviceUnitList[suName]);
          if (su == NULL) return CL_ERR_NOT_EXIST;
          if (node->serviceUnits.contains(su) == false) return CL_ERR_INVALID_PARAMETER;
          if (SAFplus::effectiveAdminState(su) != SAFplusAmf::AdministrativeState::off) return CL_ERR_INVALID_STATE;
        }
        break;
      }
#if 1
    case AMF_MGMT_OP_SG_SU_LIST_DELETE:
      {
        SAFplusAmf::ServiceGroup* sg = dynamic_cast<SAFplusAmf::ServiceGroup*>(containingEntity);
        for (int i=0;i<list.size();i++)
        {
          const std::string& suName = list.Get(i);
          SAFplusAmf::ServiceUnit* su = dynamic_cast<SAFplusAmf::ServiceUnit*>(cfg.safplusAmf.serviceUnitList[suName]);
          if (su == NULL) return CL_ERR_NOT_EXIST;
          if (sg->serviceUnits.contains(su) == false) return CL_ERR_INVALID_PARAMETER;
          if (SAFplus::effectiveAdminState(su) != SAFplusAmf::AdministrativeState::off) return CL_ERR_INVALID_STATE;
        }
        break;
      }
    case AMF_MGMT_OP_SG_SI_LIST_DELETE:
      {
        SAFplusAmf::ServiceGroup* sg = dynamic_cast<SAFplusAmf::ServiceGroup*>(containingEntity);
        for (int i=0;i<list.size();i++)
        {
          const std::string& siName = list.Get(i);
          SAFplusAmf::ServiceInstance* si = dynamic_cast<SAFplusAmf::ServiceInstance*>(cfg.safplusAmf.serviceInstanceList[siName]);
          if (si == NULL) return CL_ERR_NOT_EXIST;
          if (sg->serviceInstances.contains(si) == false) return CL_ERR_INVALID_PARAMETER;
          if (SAFplus::effectiveAdminState(si) != SAFplusAmf::AdministrativeState::idle) return CL_ERR_INVALID_STATE;
        }
        break;
      }
    case AMF_MGMT_OP_SU_COMP_LIST_DELETE:
      {
        SAFplusAmf::ServiceUnit* su = dynamic_cast<SAFplusAmf::ServiceUnit*>(containingEntity);
        for (int i=0;i<list.size();i++)
        {
          const std::string& compName = list.Get(i);
          SAFplusAmf::Component* comp = dynamic_cast<SAFplusAmf::Component*>(cfg.safplusAmf.componentList[compName]);
          if (comp == NULL) return CL_ERR_NOT_EXIST;
          if (su->components.contains(comp) == false) return CL_ERR_INVALID_PARAMETER;
          if (SAFplus::effectiveAdminState(comp) != SAFplusAmf::AdministrativeState::off) return CL_ERR_INVALID_STATE;
        }
        break;
      }
    case AMF_MGMT_OP_SI_CSI_LIST_DELETE:
      {
        SAFplusAmf::ServiceInstance* si = dynamic_cast<SAFplusAmf::ServiceInstance*>(containingEntity);
        for (int i=0;i<list.size();i++)
        {
          const std::string& csiName = list.Get(i);
          SAFplusAmf::ComponentServiceInstance* csi = dynamic_cast<SAFplusAmf::ComponentServiceInstance*>(cfg.safplusAmf.componentServiceInstanceList[csiName]);
          if (csi == NULL) return CL_ERR_NOT_EXIST;
          if (si->componentServiceInstances.contains(csi) == false) return CL_ERR_INVALID_PARAMETER;
          if (SAFplus::effectiveAdminState(csi) != SAFplusAmf::AdministrativeState::idle) return CL_ERR_INVALID_STATE;
        }
        break;
      }
#endif
    }
    return rc;
  }

  ClRcT validateOperation(int opId, const ::google::protobuf::Message* msg)
  {
    ClRcT rc = CL_OK;
    switch (opId>>16)
    {
    case AMF_MGMT_OP_COMPONENT_CREATE:
      {
        const CreateComponentRequest* request = dynamic_cast<const CreateComponentRequest*>(msg);
        if (!request)
        {
          logError("MGMT","VALIDATE.OP","invalid protobuf message passed, expected [CreateComponentRequest], actual [%s]", typeid(*msg).name());
          rc = CL_ERR_INVALID_PARAMETER;
          break;
        }
        const ComponentConfig& comp = request->componentconfig();
        if (dynamic_cast<SAFplusAmf::Component*>(cfg.safplusAmf.componentList[comp.name()]) != NULL)
        {
          logError("MGMT","VALIDATE.OP","component with name [%s] already exists", comp.name().c_str());
          rc = CL_ERR_ALREADY_EXIST;
        }
        break;
      }

    case AMF_MGMT_OP_COMPONENT_UPDATE:
      {
        const UpdateComponentRequest* request = dynamic_cast<const UpdateComponentRequest*>(msg);
        if (!request)
        {
          logError("MGMT","VALIDATE.OP","invalid protobuf message passed, expected [UpdateComponentRequest], actual [%s]", typeid(*msg).name());
          rc = CL_ERR_INVALID_PARAMETER;
          break;
        }
        const ComponentConfig& comp = request->componentconfig();
        SAFplusAmf::Component* safComp = dynamic_cast<SAFplusAmf::Component*>(cfg.safplusAmf.componentList[comp.name()]);
        if (safComp == NULL)
        {
          logError("MGMT","VALIDATE.OP","component with name [%s] doesn't exist, cannot update it", comp.name().c_str());
          rc = CL_ERR_NOT_EXIST;
          break;
        }
        #if 0 // no check except instantiateLevel set!!!
        if (SAFplus::effectiveAdminState(safComp) != SAFplusAmf::AdministrativeState::off)
        {
          rc = CL_ERR_INVALID_STATE;
        }
        #endif
        break;
      }

    case AMF_MGMT_OP_COMPONENT_DELETE:
      {
        const DeleteComponentRequest* request = dynamic_cast<const DeleteComponentRequest*>(msg);
        if (!request)
        {
          logError("MGMT","VALIDATE.OP","invalid protobuf message passed, expected [DeleteComponentRequest], actual [%s]", typeid(*msg).name());
          rc = CL_ERR_INVALID_PARAMETER;
          break;
        }
        const std::string& compName = request->name();
        SAFplusAmf::Component* safComp = dynamic_cast<SAFplusAmf::Component*>(cfg.safplusAmf.componentList[compName]);
        if (safComp == NULL)
        {
          logError("MGMT","VALIDATE.OP","component with name [%s] doesn't exist, cannot delete it", compName.c_str());
          rc = CL_ERR_NOT_EXIST;
          break;
        }
        
        if (SAFplus::effectiveAdminState(safComp) != SAFplusAmf::AdministrativeState::off)
        {
          rc = CL_ERR_INVALID_STATE;
        }
        break;
      }

    case AMF_MGMT_OP_SG_CREATE:
      {
        const CreateSGRequest* request = dynamic_cast<const CreateSGRequest*>(msg);
        if (!request)
        {
          logError("MGMT","VALIDATE.OP","invalid protobuf message passed, expected [CreateSGRequest], actual [%s]", typeid(*msg).name());
          rc = CL_ERR_INVALID_PARAMETER;
          break;
        }
        const ServiceGroupConfig& sg = request->servicegroupconfig();
        if (dynamic_cast<SAFplusAmf::ServiceGroup*>(cfg.safplusAmf.serviceGroupList[sg.name()]) != NULL)
        {
          logError("MGMT","VALIDATE.OP","sg with name [%s] already exists", sg.name().c_str());
          rc = CL_ERR_ALREADY_EXIST;
        }
        break;
      }

    case AMF_MGMT_OP_SG_UPDATE:
      {
        const UpdateSGRequest* request = dynamic_cast<const UpdateSGRequest*>(msg);
        if (!request)
        {
          logError("MGMT","VALIDATE.OP","invalid protobuf message passed, expected [UpdateSGRequest], actual [%s]", typeid(*msg).name());
          rc = CL_ERR_INVALID_PARAMETER;
          break;
        }
        const ServiceGroupConfig& sg = request->servicegroupconfig();
        SAFplusAmf::ServiceGroup* safSg = dynamic_cast<SAFplusAmf::ServiceGroup*>(cfg.safplusAmf.serviceGroupList[sg.name()]);
        if (safSg == NULL)
        {
          logError("MGMT","VALIDATE.OP","sg with name [%s] doesn't exist, cannot update it", sg.name().c_str());
          rc = CL_ERR_NOT_EXIST;
          break;

        }
#if 0
        if (SAFplus::effectiveAdminState(safSg) != SAFplusAmf::AdministrativeState::off)
        {
          rc = CL_ERR_INVALID_STATE;
        }
#endif
        break;
      }
    case AMF_MGMT_OP_SG_DELETE:
      {
        const DeleteSGRequest* request = dynamic_cast<const DeleteSGRequest*>(msg);
        if (!request)
        {
          logError("MGMT","VALIDATE.OP","invalid protobuf message passed, expected [DeleteSGRequest], actual [%s]", typeid(*msg).name());
          rc = CL_ERR_INVALID_PARAMETER;
          break;
        }
        const std::string& sgName = request->name();
        SAFplusAmf::ServiceGroup* safSg = dynamic_cast<SAFplusAmf::ServiceGroup*>(cfg.safplusAmf.serviceGroupList[sgName]);
        if (safSg == NULL)
        {
          logError("MGMT","VALIDATE.OP","sg with name [%s] doesn't exist, cannot delete it", sgName.c_str());
          rc = CL_ERR_NOT_EXIST;
          break;
        }
        
        if (SAFplus::effectiveAdminState(safSg) != SAFplusAmf::AdministrativeState::off)
        {
          rc = CL_ERR_INVALID_STATE;
        }
        break;
      }

    case AMF_MGMT_OP_NODE_CREATE:
      {
        const CreateNodeRequest* request = dynamic_cast<const CreateNodeRequest*>(msg);
        if (!request)
        {
          logError("MGMT","VALIDATE.OP","invalid protobuf message passed, expected [CreateNodeRequest], actual [%s]", typeid(*msg).name());
          rc = CL_ERR_INVALID_PARAMETER;
          break;
        }
        const NodeConfig& node = request->nodeconfig();
        if (dynamic_cast<SAFplusAmf::Node*>(cfg.safplusAmf.nodeList[node.name()]) != NULL)
        {
          logError("MGMT","VALIDATE.OP","Node with name [%s] already exists", node.name().c_str());
          rc = CL_ERR_ALREADY_EXIST;
        }
        break;
      }
    case AMF_MGMT_OP_NODE_UPDATE:
      {
        const UpdateNodeRequest* request = dynamic_cast<const UpdateNodeRequest*>(msg);
        if (!request)
        {
          logError("MGMT","VALIDATE.OP","invalid protobuf message passed, expected [UpdateNodeRequest], actual [%s]", typeid(*msg).name());
          rc = CL_ERR_INVALID_PARAMETER;
          break;
        }
        const NodeConfig& node = request->nodeconfig();
        SAFplusAmf::Node* safNode = dynamic_cast<SAFplusAmf::Node*>(cfg.safplusAmf.nodeList[node.name()]);
        if (safNode == NULL)
        {
          logError("MGMT","VALIDATE.OP","node with name [%s] doesn't exist, cannot update it", node.name().c_str());
          rc = CL_ERR_NOT_EXIST;         
        }
        break;
      }
    case AMF_MGMT_OP_NODE_DELETE:
      {
        const DeleteNodeRequest* request = dynamic_cast<const DeleteNodeRequest*>(msg);
        if (!request)
        {
          logError("MGMT","VALIDATE.OP","invalid protobuf message passed, expected [DeleteNodeRequest], actual [%s]", typeid(*msg).name());
          rc = CL_ERR_INVALID_PARAMETER;
          break;
        }
        const std::string& nodeName = request->name();
        SAFplusAmf::Node* safNode = dynamic_cast<SAFplusAmf::Node*>(cfg.safplusAmf.nodeList[nodeName]);
        if (safNode == NULL)
        {
          logError("MGMT","VALIDATE.OP","node with name [%s] doesn't exist, cannot update it", nodeName.c_str());
          rc = CL_ERR_NOT_EXIST;
          break;
        }
        if (safNode->operState.value)//(safNode->adminState != SAFplusAmf::AdministrativeState::off)
        {
          rc = CL_ERR_INVALID_STATE;
        }
        break;
      }

    case AMF_MGMT_OP_SU_CREATE:
      {
        const CreateSURequest* request = dynamic_cast<const CreateSURequest*>(msg);
        if (!request)
        {
          logError("MGMT","VALIDATE.OP","invalid protobuf message passed, expected [CreateSURequest], actual [%s]", typeid(*msg).name());
          rc = CL_ERR_INVALID_PARAMETER;
          break;
        }
        const ServiceUnitConfig& su = request->serviceunitconfig();
        if (dynamic_cast<SAFplusAmf::ServiceUnit*>(cfg.safplusAmf.serviceUnitList[su.name()]) != NULL)
        {
          logError("MGMT","VALIDATE.OP","su with name [%s] already exists", su.name().c_str());
          rc = CL_ERR_ALREADY_EXIST;
        }
        break;
      }
    case AMF_MGMT_OP_SU_UPDATE:
      {
        const UpdateSURequest* request = dynamic_cast<const UpdateSURequest*>(msg);
        if (!request)
        {
          logError("MGMT","VALIDATE.OP","invalid protobuf message passed, expected [UpdateSURequest], actual [%s]", typeid(*msg).name());
          rc = CL_ERR_INVALID_PARAMETER;
          break;
        }
        const ServiceUnitConfig& su = request->serviceunitconfig();
        SAFplusAmf::ServiceUnit* safSu = dynamic_cast<SAFplusAmf::ServiceUnit*>(cfg.safplusAmf.serviceUnitList[su.name()]);
        if (safSu == NULL)
        {
          logError("MGMT","VALIDATE.OP","su with name [%s] doesn't exist, cannot update it", su.name().c_str());
          rc = CL_ERR_NOT_EXIST;         
        }
        if (su.has_rank())
        {
          if (SAFplus::effectiveAdminState(safSu) != SAFplusAmf::AdministrativeState::off)
          {
            rc = CL_ERR_INVALID_STATE;
          }
        }
        break;
      }
    case AMF_MGMT_OP_SU_DELETE:
      {
        const DeleteSURequest* request = dynamic_cast<const DeleteSURequest*>(msg);
        if (!request)
        {
          logError("MGMT","VALIDATE.OP","invalid protobuf message passed, expected [DeleteSURequest], actual [%s]", typeid(*msg).name());
          rc = CL_ERR_INVALID_PARAMETER;
          break;
        }
        const std::string& suName = request->name();
        SAFplusAmf::ServiceUnit* safSu = dynamic_cast<SAFplusAmf::ServiceUnit*>(cfg.safplusAmf.serviceUnitList[suName]);
        if (safSu == NULL)
        {
          logError("MGMT","VALIDATE.OP","su with name [%s] doesn't exist, cannot update it", suName.c_str());
          rc = CL_ERR_NOT_EXIST;
          break;
        }
        if (SAFplus::effectiveAdminState(safSu) != SAFplusAmf::AdministrativeState::off)
        {
          rc = CL_ERR_INVALID_STATE;
        }
        break;
      }

    case AMF_MGMT_OP_SI_CREATE:
      {
        const CreateSIRequest* request = dynamic_cast<const CreateSIRequest*>(msg);
        if (!request)
        {
          logError("MGMT","VALIDATE.OP","invalid protobuf message passed, expected [CreateSIRequest], actual [%s]", typeid(*msg).name());
          rc = CL_ERR_INVALID_PARAMETER;
          break;
        }
        const ServiceInstanceConfig& si = request->serviceinstanceconfig();
        if (dynamic_cast<SAFplusAmf::ServiceInstance*>(cfg.safplusAmf.serviceInstanceList[si.name()]) != NULL)
        {
          logError("MGMT","VALIDATE.OP","si with name [%s] already exists", si.name().c_str());
          rc = CL_ERR_ALREADY_EXIST;
        }
        break;
      }
    case AMF_MGMT_OP_SI_UPDATE:
      {
        const UpdateSIRequest* request = dynamic_cast<const UpdateSIRequest*>(msg);
        if (!request)
        {
          logError("MGMT","VALIDATE.OP","invalid protobuf message passed, expected [UpdateSIRequest], actual [%s]", typeid(*msg).name());
          rc = CL_ERR_INVALID_PARAMETER;
          break;
        }
        const ServiceInstanceConfig& si = request->serviceinstanceconfig();
        SAFplusAmf::ServiceInstance* safSi = dynamic_cast<SAFplusAmf::ServiceInstance*>(cfg.safplusAmf.serviceInstanceList[si.name()]);
        if (safSi == NULL)
        {
          logError("MGMT","VALIDATE.OP","si with name [%s] doesn't exist, cannot update it", si.name().c_str());
          rc = CL_ERR_NOT_EXIST;         
        }
        if (si.has_rank())
        {
          if (SAFplus::effectiveAdminState(safSi) != SAFplusAmf::AdministrativeState::off)
          {
            rc = CL_ERR_INVALID_STATE;
          }
        }
        break;
      }
    case AMF_MGMT_OP_SI_DELETE:
      {
        const DeleteSIRequest* request = dynamic_cast<const DeleteSIRequest*>(msg);
        if (!request)
        {
          logError("MGMT","VALIDATE.OP","invalid protobuf message passed, expected [DeleteSIRequest], actual [%s]", typeid(*msg).name());
          rc = CL_ERR_INVALID_PARAMETER;
          break;
        }
        const std::string& siName = request->name();
        SAFplusAmf::ServiceInstance* safSi = dynamic_cast<SAFplusAmf::ServiceInstance*>(cfg.safplusAmf.serviceInstanceList[siName]);
        if (safSi == NULL)
        {
          logError("MGMT","VALIDATE.OP","si with name [%s] doesn't exist, cannot update it", siName.c_str());
          rc = CL_ERR_NOT_EXIST;
          break;
        }
        if (safSi->adminState.value != SAFplusAmf::AdministrativeState::idle)
        {
          rc = CL_ERR_INVALID_STATE;
        }
        break;
      }
    case AMF_MGMT_OP_CSI_CREATE:
      {
        const CreateCSIRequest* request = dynamic_cast<const CreateCSIRequest*>(msg);
        if (!request)
        {
          logError("MGMT","VALIDATE.OP","invalid protobuf message passed, expected [CreateCSIRequest], actual [%s]", typeid(*msg).name());
          rc = CL_ERR_INVALID_PARAMETER;
          break;
        }
        const ComponentServiceInstanceConfig& csi = request->componentserviceinstanceconfig();
        if (dynamic_cast<SAFplusAmf::ComponentServiceInstance*>(cfg.safplusAmf.componentServiceInstanceList[csi.name()]) != NULL)
        {
          logError("MGMT","VALIDATE.OP","csi with name [%s] already exists", csi.name().c_str());
          rc = CL_ERR_ALREADY_EXIST;
        }
        break;
      }
    case AMF_MGMT_OP_CSI_UPDATE:
      {
        const UpdateCSIRequest* request = dynamic_cast<const UpdateCSIRequest*>(msg);
        if (!request)
        {
          logError("MGMT","VALIDATE.OP","invalid protobuf message passed, expected [UpdateCSIRequest], actual [%s]", typeid(*msg).name());
          rc = CL_ERR_INVALID_PARAMETER;
          break;
        }
        const ComponentServiceInstanceConfig& csi = request->componentserviceinstanceconfig();
        SAFplusAmf::ComponentServiceInstance* safCsi = dynamic_cast<SAFplusAmf::ComponentServiceInstance*>(cfg.safplusAmf.componentServiceInstanceList[csi.name()]);
        if (safCsi == NULL)
        {
          logError("MGMT","VALIDATE.OP","csi with name [%s] doesn't exist, cannot update it", csi.name().c_str());
          rc = CL_ERR_NOT_EXIST;
          break;         
        }
#if 0
        if (SAFplus::effectiveAdminState(safCsi) != SAFplusAmf::AdministrativeState::idle)
        {
          rc = CL_ERR_INVALID_STATE;
        }
#endif
        break;
      }      
    case AMF_MGMT_OP_CSI_DELETE:
      {
        const DeleteCSIRequest* request = dynamic_cast<const DeleteCSIRequest*>(msg);
        if (!request)
        {
          logError("MGMT","VALIDATE.OP","invalid protobuf message passed, expected [DeleteCSIRequest], actual [%s]", typeid(*msg).name());
          rc = CL_ERR_INVALID_PARAMETER;
          break;
        }
        const std::string& csiName = request->name();
        SAFplusAmf::ComponentServiceInstance* safCsi = dynamic_cast<SAFplusAmf::ComponentServiceInstance*>(cfg.safplusAmf.componentServiceInstanceList[csiName]);
        if (safCsi == NULL)
        {
          logError("MGMT","VALIDATE.OP","csi with name [%s] doesn't exist, cannot update it", csiName.c_str());
          rc = CL_ERR_NOT_EXIST;
          break;
        }
        SAFplusAmf::ServiceInstance* si = safCsi->serviceInstance.value;
        if (si && si->adminState.value != SAFplusAmf::AdministrativeState::idle)
        {
          rc = CL_ERR_INVALID_STATE;
        }
        break;
      }

    case AMF_MGMT_OP_CSI_NVP_DELETE:
      {
        const DeleteCSINVPRequest* request = dynamic_cast<const DeleteCSINVPRequest*>(msg);
        if (!request)
        {
          logError("MGMT","VALIDATE.OP","invalid protobuf message passed, expected [DeleteCSINVPRequest], actual [%s]", typeid(*msg).name());
          rc = CL_ERR_INVALID_PARAMETER;
          break;
        }
        const std::string& csiName = request->name();
        SAFplusAmf::ComponentServiceInstance* safCsi = dynamic_cast<SAFplusAmf::ComponentServiceInstance*>(cfg.safplusAmf.componentServiceInstanceList[csiName]);
        if (safCsi == NULL)
        {
          logError("MGMT","VALIDATE.OP","csi with name [%s] doesn't exist, cannot update it", csiName.c_str());
          rc = CL_ERR_NOT_EXIST;
        }
        break;
      }

    case AMF_MGMT_OP_NODE_SU_LIST_DELETE:
      {
        const DeleteNodeSUListRequest* request = dynamic_cast<const DeleteNodeSUListRequest*>(msg);
        if (!request)
        {
          logError("MGMT","VALIDATE.OP","invalid protobuf message passed, expected [DeleteNodeSUListRequest], actual [%s]", typeid(*msg).name());
          rc = CL_ERR_INVALID_PARAMETER;
          break;
        }
        const std::string& nodeName = request->nodename();
        SAFplusAmf::Node* safNode = dynamic_cast<SAFplusAmf::Node*>(cfg.safplusAmf.nodeList[nodeName]);
        if (safNode == NULL)
        {
          logError("MGMT","VALIDATE.OP","node with name [%s] doesn't exist, cannot update it", nodeName.c_str());
          rc = CL_ERR_NOT_EXIST;
          break;
        }
#if 0  // does it need to check the admin state of the containing entity? NO
        if (safNode->adminState != SAFplusAmf::AdministrativeState::off)
        {
          rc = CL_ERR_INVALID_STATE;
        }
#endif
        rc = validateAdminState(opId, safNode, request->sulist());
        break;
      }

    case AMF_MGMT_OP_SG_SU_LIST_DELETE:
      {
        const DeleteSGSUListRequest* request = dynamic_cast<const DeleteSGSUListRequest*>(msg);
        if (!request)
        {
          logError("MGMT","VALIDATE.OP","invalid protobuf message passed, expected [DeleteSGSUListRequest], actual [%s]", typeid(*msg).name());
          rc = CL_ERR_INVALID_PARAMETER;
          break;
        }
        const std::string& sgName = request->sgname();
        SAFplusAmf::ServiceGroup* safSg = dynamic_cast<SAFplusAmf::ServiceGroup*>(cfg.safplusAmf.serviceGroupList[sgName]);
        if (safSg == NULL)
        {
          logError("MGMT","VALIDATE.OP","sg with name [%s] doesn't exist, cannot update it", sgName.c_str());
          rc = CL_ERR_NOT_EXIST;
          break;
        }
#if 0  // does it need to check the admin state of the containing entity? NO
        if (SAFplus::effectiveAdminState(safSg) != SAFplusAmf::AdministrativeState::off)
        {
          rc = CL_ERR_INVALID_STATE;
        }
#endif
        rc = validateAdminState(opId, safSg, request->sulist());
        break;
      }
    case AMF_MGMT_OP_SG_SI_LIST_DELETE:
      {
        const DeleteSGSIListRequest* request = dynamic_cast<const DeleteSGSIListRequest*>(msg);
        if (!request)
        {
          logError("MGMT","VALIDATE.OP","invalid protobuf message passed, expected [DeleteSGSIListRequest], actual [%s]", typeid(*msg).name());
          rc = CL_ERR_INVALID_PARAMETER;
          break;
        }
        const std::string& sgName = request->sgname();
        SAFplusAmf::ServiceGroup* safSg = dynamic_cast<SAFplusAmf::ServiceGroup*>(cfg.safplusAmf.serviceGroupList[sgName]);
        if (safSg == NULL)
        {
          logError("MGMT","VALIDATE.OP","sg with name [%s] doesn't exist, cannot update it", sgName.c_str());
          rc = CL_ERR_NOT_EXIST;
          break;
        }
#if 0  // does it need to check the admin state of the containing entity? NO
        if (SAFplus::effectiveAdminState(safSg) != SAFplusAmf::AdministrativeState::off)
        {
          rc = CL_ERR_INVALID_STATE;
        }
#endif
        rc = validateAdminState(opId, safSg, request->silist());
        break;
      }
    case AMF_MGMT_OP_SU_COMP_LIST_DELETE:
      {
        const DeleteSUCompListRequest* request = dynamic_cast<const DeleteSUCompListRequest*>(msg);
        if (!request)
        {
          logError("MGMT","VALIDATE.OP","invalid protobuf message passed, expected [DeleteSUCompListRequest], actual [%s]", typeid(*msg).name());
          rc = CL_ERR_INVALID_PARAMETER;
          break;
        }
        const std::string& suName = request->suname();
        SAFplusAmf::ServiceUnit* safSu = dynamic_cast<SAFplusAmf::ServiceUnit*>(cfg.safplusAmf.serviceUnitList[suName]);
        if (safSu == NULL)
        {
          logError("MGMT","VALIDATE.OP","su with name [%s] doesn't exist, cannot update it", suName.c_str());
          rc = CL_ERR_NOT_EXIST;
          break;
        }
#if 0  // does it need to check the admin state of the containing entity? NO
        if (SAFplus::effectiveAdminState(safSu) != SAFplusAmf::AdministrativeState::off)
        {
          rc = CL_ERR_INVALID_STATE;
        }
#endif
        rc = validateAdminState(opId, safSu, request->complist());
        break;
      }
    case AMF_MGMT_OP_SI_CSI_LIST_DELETE:
      {
        const DeleteSICSIListRequest* request = dynamic_cast<const DeleteSICSIListRequest*>(msg);
        if (!request)
        {
          logError("MGMT","VALIDATE.OP","invalid protobuf message passed, expected [DeleteSICSIListRequest], actual [%s]", typeid(*msg).name());
          rc = CL_ERR_INVALID_PARAMETER;
          break;
        }
        const std::string& siName = request->siname();
        SAFplusAmf::ServiceInstance* safSi = dynamic_cast<SAFplusAmf::ServiceInstance*>(cfg.safplusAmf.serviceInstanceList[siName]);
        if (safSi == NULL)
        {
          logError("MGMT","VALIDATE.OP","si with name [%s] doesn't exist, cannot update it", siName.c_str());
          rc = CL_ERR_NOT_EXIST;
          break;
        }
#if 0  // does it need to check the admin state of the containing entity? NO
        if (SAFplus::effectiveAdminState(safSi) != SAFplusAmf::AdministrativeState::off)
        {
          rc = CL_ERR_INVALID_STATE;
        }
#endif
        rc = validateAdminState(opId, safSi, request->csilist());
        break;
      }
#if 0
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
#endif
    default:
      logError("MGMT","RPC","invalid rpc operation [%d]",opId>>16);
      break;
    }
    return rc;
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
    int opId = (AMF_MGMT_OP_COMPONENT_CREATE<<16)|inc++;
    ClRcT rc = validateOperation(opId, request);
    if (rc != CL_OK)
    {
      response->set_err(rc);
      return;
    }
    DbalPlugin* pd = NULL;
    rc = getDbalObj(request->amfmgmthandle().Get(0).c_str(), &pd);
    if (rc == CL_OK)
    {
      std::string strMsgReq;
      request->SerializeToString(&strMsgReq);      
      rc = pd->insertRecord(ClDBKeyT(&opId),
                           (ClUint32T)sizeof(opId),
                           (ClDBRecordT)strMsgReq.c_str(),
                           (ClUint32T)strMsgReq.length());
      logDebug("MGMT","RPC","[%s] insertRecord with key [%d], inc [%d] returns [0x%x]",__FUNCTION__, opId, inc-1, rc);
    } 
    response->set_err(rc);
  }


  void amfMgmtRpcImpl::updateComponent(const ::SAFplus::Rpc::amfMgmtRpc::UpdateComponentRequest* request,
                                ::SAFplus::Rpc::amfMgmtRpc::UpdateComponentResponse* response)
  {
    const ComponentConfig& comp = request->componentconfig();
    logTrace("MGMT","RPC","enter [%s] with param comp name [%s]",__FUNCTION__,comp.name().c_str());
    int opId = (AMF_MGMT_OP_COMPONENT_UPDATE<<16)|inc++;
    ClRcT rc = validateOperation(opId, request);
    if (rc != CL_OK)
    {
      response->set_err(rc);
      return;
    }
    DbalPlugin* pd = NULL;
    rc = getDbalObj(request->amfmgmthandle().Get(0).c_str(), &pd);
    if (rc == CL_OK)
    {
      std::string strMsgReq;      
      request->SerializeToString(&strMsgReq);      
      rc = pd->insertRecord(ClDBKeyT(&opId),
                           (ClUint32T)sizeof(opId),
                           (ClDBRecordT)strMsgReq.c_str(),
                           (ClUint32T)strMsgReq.length());
      logDebug("MGMT","RPC","[%s] insertRecord with key [%d], inc [%d] returns [0x%x]",__FUNCTION__, opId, inc-1, rc);
    } 
    response->set_err(rc);
  }



  void amfMgmtRpcImpl::deleteComponent(const ::SAFplus::Rpc::amfMgmtRpc::DeleteComponentRequest* request,
                                ::SAFplus::Rpc::amfMgmtRpc::DeleteComponentResponse* response)
  {
    const std::string& compName = request->name();
    logDebug("MGMT","RPC","enter [%s] with param comp name [%s]",__FUNCTION__,compName.c_str());
    int opId = (AMF_MGMT_OP_COMPONENT_DELETE<<16)|inc++;
    ClRcT rc = validateOperation(opId, request);
    if (rc != CL_OK)
    {
      response->set_err(rc);
      return;
    }
    DbalPlugin* pd = NULL;
    rc = getDbalObj(request->amfmgmthandle().Get(0).c_str(), &pd);
    if (rc == CL_OK)
    {
      std::string strMsgReq;      
      request->SerializeToString(&strMsgReq);      
      rc = pd->insertRecord(ClDBKeyT(&opId),
                           (ClUint32T)sizeof(opId),
                           (ClDBRecordT)strMsgReq.c_str(),
                           (ClUint32T)strMsgReq.length());
      logDebug("MGMT","RPC","[%s] insertRecord with key [%d], inc [%d] returns [0x%x]",__FUNCTION__, opId, inc-1, rc);      
    } 
    response->set_err(rc);
  }

  void amfMgmtRpcImpl::createSG(const ::SAFplus::Rpc::amfMgmtRpc::CreateSGRequest* request,
                                ::SAFplus::Rpc::amfMgmtRpc::CreateSGResponse* response)
  {
    const ServiceGroupConfig& sg = request->servicegroupconfig();
    logDebug("MGMT","RPC","enter [%s] with param sg name [%s]",__FUNCTION__,sg.name().c_str());
    int opId = (AMF_MGMT_OP_SG_CREATE<<16)|inc++;
    ClRcT rc = validateOperation(opId, request);
    if (rc != CL_OK)
    {
      response->set_err(rc);
      return;
    }
    DbalPlugin* pd = NULL;
    rc = getDbalObj(request->amfmgmthandle().Get(0).c_str(), &pd);
    if (rc == CL_OK)
    {
      std::string strMsgReq;      
      request->SerializeToString(&strMsgReq);      
      rc = pd->insertRecord(ClDBKeyT(&opId),
                           (ClUint32T)sizeof(opId),
                           (ClDBRecordT)strMsgReq.c_str(),
                           (ClUint32T)strMsgReq.length());      
      logDebug("MGMT","RPC","[%s] insertRecord with key [%d], inc [%d] returns [0x%x]",__FUNCTION__, opId, inc-1, rc);
    }
  }

  void amfMgmtRpcImpl::updateSG(const ::SAFplus::Rpc::amfMgmtRpc::UpdateSGRequest* request,
                                ::SAFplus::Rpc::amfMgmtRpc::UpdateSGResponse* response)
  {
    const ServiceGroupConfig& sg = request->servicegroupconfig();
    logDebug("MGMT","RPC","enter [%s] with param sg name [%s]",__FUNCTION__,sg.name().c_str());
    int opId = (AMF_MGMT_OP_SG_UPDATE<<16)|inc++;
    ClRcT rc = validateOperation(opId, request);
    if (rc != CL_OK)
    {
      response->set_err(rc);
      return;
    }
    DbalPlugin* pd = NULL;
    rc = getDbalObj(request->amfmgmthandle().Get(0).c_str(), &pd);
    if (rc == CL_OK)
    {
      std::string strMsgReq;      
      request->SerializeToString(&strMsgReq);      
      rc = pd->insertRecord(ClDBKeyT(&opId),
                           (ClUint32T)sizeof(opId),
                           (ClDBRecordT)strMsgReq.c_str(),
                           (ClUint32T)strMsgReq.length());      
      logDebug("MGMT","RPC","[%s] insertRecord with key [%d], inc [%d] returns [0x%x]",__FUNCTION__, opId, inc-1, rc);
    }
  }

  void amfMgmtRpcImpl::deleteSG(const ::SAFplus::Rpc::amfMgmtRpc::DeleteSGRequest* request,
                                ::SAFplus::Rpc::amfMgmtRpc::DeleteSGResponse* response)
  {
    const std::string& sgName = request->name();
    logDebug("MGMT","RPC","enter [%s] with param sg name [%s]",__FUNCTION__,sgName.c_str());
    int opId = (AMF_MGMT_OP_SG_DELETE<<16)|inc++;
    ClRcT rc = validateOperation(opId, request);
    if (rc != CL_OK)
    {
      response->set_err(rc);
      return;
    }
    DbalPlugin* pd = NULL;
    rc = getDbalObj(request->amfmgmthandle().Get(0).c_str(), &pd);
    if (rc == CL_OK)
    {
      std::string strMsgReq;      
      request->SerializeToString(&strMsgReq);
      
      rc = pd->insertRecord(ClDBKeyT(&opId),
                           (ClUint32T)sizeof(opId),
                           (ClDBRecordT)strMsgReq.c_str(),
                           (ClUint32T)strMsgReq.length());      
      logDebug("MGMT","RPC","[%s] insertRecord with key [%d], inc [%d] returns [0x%x]",__FUNCTION__, opId, inc-1, rc);
    }
  }

  void amfMgmtRpcImpl::createNode(const ::SAFplus::Rpc::amfMgmtRpc::CreateNodeRequest* request,
                                ::SAFplus::Rpc::amfMgmtRpc::CreateNodeResponse* response)
  {
    const NodeConfig& node = request->nodeconfig();
    logDebug("MGMT","RPC","enter [%s] with param node name [%s]",__FUNCTION__,node.name().c_str());
    int opId = (AMF_MGMT_OP_NODE_CREATE<<16)|inc++;
    ClRcT rc = validateOperation(opId, request);
    if (rc != CL_OK)
    {
      response->set_err(rc);
      return;
    }
    DbalPlugin* pd = NULL;
    rc = getDbalObj(request->amfmgmthandle().Get(0).c_str(), &pd);
    if (rc == CL_OK)
    {
      std::string strMsgReq;      
      request->SerializeToString(&strMsgReq);
      
      rc = pd->insertRecord(ClDBKeyT(&opId),
                           (ClUint32T)sizeof(opId),
                           (ClDBRecordT)strMsgReq.c_str(),
                           (ClUint32T)strMsgReq.length());      
      logDebug("MGMT","RPC","[%s] insertRecord with key [%d], inc [%d] returns [0x%x]",__FUNCTION__, opId, inc-1, rc);
    }
  }

  void amfMgmtRpcImpl::updateNode(const ::SAFplus::Rpc::amfMgmtRpc::UpdateNodeRequest* request,
                                ::SAFplus::Rpc::amfMgmtRpc::UpdateNodeResponse* response)
  {
    const NodeConfig& node = request->nodeconfig();
    logDebug("MGMT","RPC","enter [%s] with param node name [%s]",__FUNCTION__,node.name().c_str());
    int opId = (AMF_MGMT_OP_NODE_UPDATE<<16)|inc++;
    ClRcT rc = validateOperation(opId, request);
    if (rc != CL_OK)
    {
      response->set_err(rc);
      return;
    }
    DbalPlugin* pd = NULL;
    rc = getDbalObj(request->amfmgmthandle().Get(0).c_str(), &pd);
    if (rc == CL_OK)
    {
      std::string strMsgReq;      
      request->SerializeToString(&strMsgReq);      
      rc = pd->insertRecord(ClDBKeyT(&opId),
                           (ClUint32T)sizeof(opId),
                           (ClDBRecordT)strMsgReq.c_str(),
                           (ClUint32T)strMsgReq.length());      
      logDebug("MGMT","RPC","[%s] insertRecord with key [%d], inc [%d] returns [0x%x]",__FUNCTION__, opId, inc-1, rc);
    }
  }

  void amfMgmtRpcImpl::deleteNode(const ::SAFplus::Rpc::amfMgmtRpc::DeleteNodeRequest* request,
                                ::SAFplus::Rpc::amfMgmtRpc::DeleteNodeResponse* response)
  {
    const std::string& nodeName = request->name();
    logDebug("MGMT","RPC","enter [%s] with param node name [%s]",__FUNCTION__,nodeName.c_str());
    int opId = (AMF_MGMT_OP_NODE_DELETE<<16)|inc++;
    ClRcT rc = validateOperation(opId, request);
    if (rc != CL_OK)
    {
      response->set_err(rc);
      return;
    }
    DbalPlugin* pd = NULL;
    rc = getDbalObj(request->amfmgmthandle().Get(0).c_str(), &pd);
    if (rc == CL_OK)
    {
      std::string strMsgReq;      
      request->SerializeToString(&strMsgReq);      
      rc = pd->insertRecord(ClDBKeyT(&opId),
                           (ClUint32T)sizeof(opId),
                           (ClDBRecordT)strMsgReq.c_str(),
                           (ClUint32T)strMsgReq.length());      
      logDebug("MGMT","RPC","[%s] insertRecord with key [%d], inc [%d] returns [0x%x]",__FUNCTION__, opId, inc-1, rc);
    }
  }

  void amfMgmtRpcImpl::createSU(const ::SAFplus::Rpc::amfMgmtRpc::CreateSURequest* request,
                                ::SAFplus::Rpc::amfMgmtRpc::CreateSUResponse* response)
  {
    const ServiceUnitConfig& su = request->serviceunitconfig();
    logDebug("MGMT","RPC","enter [%s] with param su name [%s]",__FUNCTION__,su.name().c_str());
    int opId = (AMF_MGMT_OP_SU_CREATE<<16)|inc++;
    ClRcT rc = validateOperation(opId, request);
    if (rc != CL_OK)
    {
      response->set_err(rc);
      return;
    }
    DbalPlugin* pd = NULL;
    rc = getDbalObj(request->amfmgmthandle().Get(0).c_str(), &pd);
    if (rc == CL_OK)
    {
      std::string strMsgReq;      
      request->SerializeToString(&strMsgReq);      
      rc = pd->insertRecord(ClDBKeyT(&opId),
                           (ClUint32T)sizeof(opId),
                           (ClDBRecordT)strMsgReq.c_str(),
                           (ClUint32T)strMsgReq.length());      
      logDebug("MGMT","RPC","[%s] insertRecord with key [%d], inc [%d] returns [0x%x]",__FUNCTION__, opId, inc-1, rc);
    } 
    response->set_err(rc);
  }

  void amfMgmtRpcImpl::updateSU(const ::SAFplus::Rpc::amfMgmtRpc::UpdateSURequest* request,
                                ::SAFplus::Rpc::amfMgmtRpc::UpdateSUResponse* response)
  {
    const ServiceUnitConfig& su = request->serviceunitconfig();
    logDebug("MGMT","RPC","enter [%s] with param su name [%s]",__FUNCTION__,su.name().c_str());
    int opId = (AMF_MGMT_OP_SU_UPDATE<<16)|inc++;
    ClRcT rc = validateOperation(opId, request);
    if (rc != CL_OK)
    {
      response->set_err(rc);
      return;
    }
    DbalPlugin* pd = NULL;
    rc = getDbalObj(request->amfmgmthandle().Get(0).c_str(), &pd);
    if (rc == CL_OK)
    {
      std::string strMsgReq;      
      request->SerializeToString(&strMsgReq);      
      rc = pd->insertRecord(ClDBKeyT(&opId),
                           (ClUint32T)sizeof(opId),
                           (ClDBRecordT)strMsgReq.c_str(),
                           (ClUint32T)strMsgReq.length());      
      logDebug("MGMT","RPC","[%s] insertRecord with key [%d], inc [%d] returns [0x%x]",__FUNCTION__, opId, inc-1, rc);
    } 
    response->set_err(rc);  
  }

  void amfMgmtRpcImpl::deleteSU(const ::SAFplus::Rpc::amfMgmtRpc::DeleteSURequest* request,
                                ::SAFplus::Rpc::amfMgmtRpc::DeleteSUResponse* response)
  {
    const std::string& suName = request->name();
    logDebug("MGMT","RPC","enter [%s] with param su name [%s]",__FUNCTION__,suName.c_str());
    int opId = (AMF_MGMT_OP_SU_DELETE<<16)|inc++;
    ClRcT rc = validateOperation(opId, request);
    if (rc != CL_OK)
    {
      response->set_err(rc);
      return;
    }
    DbalPlugin* pd = NULL;
    rc = getDbalObj(request->amfmgmthandle().Get(0).c_str(), &pd);
    if (rc == CL_OK)
    {
      std::string strMsgReq;      
      request->SerializeToString(&strMsgReq);      
      rc = pd->insertRecord(ClDBKeyT(&opId),
                           (ClUint32T)sizeof(opId),
                           (ClDBRecordT)strMsgReq.c_str(),
                           (ClUint32T)strMsgReq.length());      
      logDebug("MGMT","RPC","[%s] insertRecord with key [%d], inc [%d] returns [0x%x]",__FUNCTION__, opId, inc-1, rc);
    } 
    response->set_err(rc);
  }

  void amfMgmtRpcImpl::createSI(const ::SAFplus::Rpc::amfMgmtRpc::CreateSIRequest* request,
                                ::SAFplus::Rpc::amfMgmtRpc::CreateSIResponse* response)
  {
    const ServiceInstanceConfig& si = request->serviceinstanceconfig();
    logDebug("MGMT","RPC","enter [%s] with param si name [%s]",__FUNCTION__,si.name().c_str());
    int opId = (AMF_MGMT_OP_SI_CREATE<<16)|inc++;
    ClRcT rc = validateOperation(opId, request);
    if (rc != CL_OK)
    {
      response->set_err(rc);
      return;
    }
    DbalPlugin* pd = NULL;
    rc = getDbalObj(request->amfmgmthandle().Get(0).c_str(), &pd);
    if (rc == CL_OK)
    {
      std::string strMsgReq;      
      request->SerializeToString(&strMsgReq);      
      rc = pd->insertRecord(ClDBKeyT(&opId),
                           (ClUint32T)sizeof(opId),
                           (ClDBRecordT)strMsgReq.c_str(),
                           (ClUint32T)strMsgReq.length());      
      logDebug("MGMT","RPC","[%s] insertRecord with key [%d], inc [%d] returns [0x%x]",__FUNCTION__, opId, inc-1, rc);
    } 
    response->set_err(rc);
  }

  void amfMgmtRpcImpl::updateSI(const ::SAFplus::Rpc::amfMgmtRpc::UpdateSIRequest* request,
                                ::SAFplus::Rpc::amfMgmtRpc::UpdateSIResponse* response)
  {
    const ServiceInstanceConfig& si = request->serviceinstanceconfig();
    logDebug("MGMT","RPC","enter [%s] with param si name [%s]",__FUNCTION__,si.name().c_str());
    int opId = (AMF_MGMT_OP_SI_UPDATE<<16)|inc++;
    ClRcT rc = validateOperation(opId, request);
    if (rc != CL_OK)
    {
      response->set_err(rc);
      return;
    }
    DbalPlugin* pd = NULL;
    rc = getDbalObj(request->amfmgmthandle().Get(0).c_str(), &pd);
    if (rc == CL_OK)
    {
      std::string strMsgReq;      
      request->SerializeToString(&strMsgReq);      
      rc = pd->insertRecord(ClDBKeyT(&opId),
                           (ClUint32T)sizeof(opId),
                           (ClDBRecordT)strMsgReq.c_str(),
                           (ClUint32T)strMsgReq.length());      
      logDebug("MGMT","RPC","[%s] insertRecord with key [%d], inc [%d] returns [0x%x]",__FUNCTION__, opId, inc-1, rc);
    } 
    response->set_err(rc);
  }

  void amfMgmtRpcImpl::deleteSI(const ::SAFplus::Rpc::amfMgmtRpc::DeleteSIRequest* request,
                                ::SAFplus::Rpc::amfMgmtRpc::DeleteSIResponse* response)
  {
    const std::string& siName = request->name();
    logDebug("MGMT","RPC","enter [%s] with param si name [%s]",__FUNCTION__,siName.c_str());
    int opId = (AMF_MGMT_OP_SI_DELETE<<16)|inc++;
    ClRcT rc = validateOperation(opId, request);
    if (rc != CL_OK)
    {
      response->set_err(rc);
      return;
    }
    DbalPlugin* pd = NULL;
    rc = getDbalObj(request->amfmgmthandle().Get(0).c_str(), &pd);
    if (rc == CL_OK)
    {
      std::string strMsgReq;      
      request->SerializeToString(&strMsgReq);      
      rc = pd->insertRecord(ClDBKeyT(&opId),
                           (ClUint32T)sizeof(opId),
                           (ClDBRecordT)strMsgReq.c_str(),
                           (ClUint32T)strMsgReq.length());      
      logDebug("MGMT","RPC","[%s] insertRecord with key [%d], inc [%d] returns [0x%x]",__FUNCTION__, opId, inc-1, rc);
    } 
    response->set_err(rc);
  }

  void amfMgmtRpcImpl::createCSI(const ::SAFplus::Rpc::amfMgmtRpc::CreateCSIRequest* request,
                                ::SAFplus::Rpc::amfMgmtRpc::CreateCSIResponse* response)
  {
    const ComponentServiceInstanceConfig& csi = request->componentserviceinstanceconfig();
    logDebug("MGMT","RPC","enter [%s] with param csi name [%s]",__FUNCTION__,csi.name().c_str());
    int opId = (AMF_MGMT_OP_CSI_CREATE<<16)|inc++;
    ClRcT rc = validateOperation(opId, request);
    if (rc != CL_OK)
    {
      response->set_err(rc);
      return;
    }
    DbalPlugin* pd = NULL;
    rc = getDbalObj(request->amfmgmthandle().Get(0).c_str(), &pd);
    if (rc == CL_OK)
    {
      std::string strMsgReq;      
      request->SerializeToString(&strMsgReq);      
      rc = pd->insertRecord(ClDBKeyT(&opId),
                           (ClUint32T)sizeof(opId),
                           (ClDBRecordT)strMsgReq.c_str(),
                           (ClUint32T)strMsgReq.length());      
      logDebug("MGMT","RPC","[%s] insertRecord with key [%d], inc [%d] returns [0x%x]",__FUNCTION__, opId, inc-1, rc);
    } 
    response->set_err(rc);
  }

  void amfMgmtRpcImpl::updateCSI(const ::SAFplus::Rpc::amfMgmtRpc::UpdateCSIRequest* request,
                                ::SAFplus::Rpc::amfMgmtRpc::UpdateCSIResponse* response)
  {
    const ComponentServiceInstanceConfig& csi = request->componentserviceinstanceconfig();
    logDebug("MGMT","RPC","enter [%s] with param csi name [%s]",__FUNCTION__,csi.name().c_str());
    int opId = (AMF_MGMT_OP_CSI_UPDATE<<16)|inc++;
    ClRcT rc = validateOperation(opId, request);
    if (rc != CL_OK)
    {
      response->set_err(rc);
      return;
    }
    DbalPlugin* pd = NULL;
    rc = getDbalObj(request->amfmgmthandle().Get(0).c_str(), &pd);
    if (rc == CL_OK)
    {
      std::string strMsgReq;      
      request->SerializeToString(&strMsgReq);      
      rc = pd->insertRecord(ClDBKeyT(&opId),
                           (ClUint32T)sizeof(opId),
                           (ClDBRecordT)strMsgReq.c_str(),
                           (ClUint32T)strMsgReq.length());      
      logDebug("MGMT","RPC","[%s] insertRecord with key [%d], inc [%d] returns [0x%x]",__FUNCTION__, opId, inc-1, rc);
    } 
    response->set_err(rc);
  }

  void amfMgmtRpcImpl::deleteCSI(const ::SAFplus::Rpc::amfMgmtRpc::DeleteCSIRequest* request,
                                ::SAFplus::Rpc::amfMgmtRpc::DeleteCSIResponse* response)
  {
    const std::string& csiName = request->name();
    logDebug("MGMT","RPC","enter [%s] with param csi name [%s]",__FUNCTION__,csiName.c_str());
    int opId = (AMF_MGMT_OP_CSI_DELETE<<16)|inc++;
    ClRcT rc = validateOperation(opId, request);
    if (rc != CL_OK)
    {
      response->set_err(rc);
      return;
    }
    DbalPlugin* pd = NULL;
    rc = getDbalObj(request->amfmgmthandle().Get(0).c_str(), &pd);
    if (rc == CL_OK)
    {
      std::string strMsgReq;      
      request->SerializeToString(&strMsgReq);      
      rc = pd->insertRecord(ClDBKeyT(&opId),
                           (ClUint32T)sizeof(opId),
                           (ClDBRecordT)strMsgReq.c_str(),
                           (ClUint32T)strMsgReq.length());      
      logDebug("MGMT","RPC","[%s] insertRecord with key [%d], inc [%d] returns [0x%x]",__FUNCTION__, opId, inc-1, rc);
    } 
    response->set_err(rc);
  }

  void amfMgmtRpcImpl::deleteCSINVP(const ::SAFplus::Rpc::amfMgmtRpc::DeleteCSINVPRequest* request,
                                ::SAFplus::Rpc::amfMgmtRpc::DeleteCSINVPResponse* response)
  {
    const std::string& csiName = request->name();
    logDebug("MGMT","RPC","enter [%s] with param csi name [%s]",__FUNCTION__,csiName.c_str());
    int opId = (AMF_MGMT_OP_CSI_NVP_DELETE<<16)|inc++;
    ClRcT rc = validateOperation(opId, request);
    if (rc != CL_OK)
    {
      response->set_err(rc);
      return;
    }
    DbalPlugin* pd = NULL;
    rc = getDbalObj(request->amfmgmthandle().Get(0).c_str(), &pd);
    if (rc == CL_OK)
    {
      std::string strMsgReq;
      request->SerializeToString(&strMsgReq);      
      rc = pd->insertRecord(ClDBKeyT(&opId),
                           (ClUint32T)sizeof(opId),
                           (ClDBRecordT)strMsgReq.c_str(),
                           (ClUint32T)strMsgReq.length());
      logDebug("MGMT","RPC","[%s] insertRecord with key [%d], inc [%d] returns [0x%x]",__FUNCTION__, opId, inc-1, rc);
    }
    response->set_err(rc);
  }

  void amfMgmtRpcImpl::deleteNodeSUList(const ::SAFplus::Rpc::amfMgmtRpc::DeleteNodeSUListRequest* request,
                                ::SAFplus::Rpc::amfMgmtRpc::DeleteNodeSUListResponse* response)
  {
    const std::string& nodeName = request->nodename();
    logDebug("MGMT","RPC","enter [%s] with param node name [%s]",__FUNCTION__,nodeName.c_str());
    int opId = (AMF_MGMT_OP_NODE_SU_LIST_DELETE<<16)|inc++; 
    ClRcT rc = validateOperation(opId, request);
    if (rc != CL_OK)
    {
      response->set_err(rc);
      return;
    }
    DbalPlugin* pd = NULL;
    rc = getDbalObj(request->amfmgmthandle().Get(0).c_str(), &pd);
    if (rc == CL_OK)
    {
      std::string strMsgReq;
      request->SerializeToString(&strMsgReq);           
      rc = pd->insertRecord(ClDBKeyT(&opId),
                           (ClUint32T)sizeof(opId),
                           (ClDBRecordT)strMsgReq.c_str(),
                           (ClUint32T)strMsgReq.length());
      logDebug("MGMT","RPC","[%s] insertRecord with key [%d], inc [%d] returns [0x%x]",__FUNCTION__, opId, inc-1, rc);
    }
    response->set_err(rc);
  }

  void amfMgmtRpcImpl::deleteSGSUList(const ::SAFplus::Rpc::amfMgmtRpc::DeleteSGSUListRequest* request,
                                ::SAFplus::Rpc::amfMgmtRpc::DeleteSGSUListResponse* response)
  {
    const std::string& sgName = request->sgname();
    logDebug("MGMT","RPC","enter [%s] with param sg name [%s]",__FUNCTION__,sgName.c_str());
    int opId = (AMF_MGMT_OP_SG_SU_LIST_DELETE<<16)|inc++;
    ClRcT rc = validateOperation(opId, request);
    if (rc != CL_OK)
    {
      response->set_err(rc);
      return;
    }
    DbalPlugin* pd = NULL;
    rc = getDbalObj(request->amfmgmthandle().Get(0).c_str(), &pd);
    if (rc == CL_OK)
    {
      std::string strMsgReq;
      request->SerializeToString(&strMsgReq);      
      rc = pd->insertRecord(ClDBKeyT(&opId),
                           (ClUint32T)sizeof(opId),
                           (ClDBRecordT)strMsgReq.c_str(),
                           (ClUint32T)strMsgReq.length());
      logDebug("MGMT","RPC","[%s] insertRecord with key [%d], inc [%d] returns [0x%x]",__FUNCTION__, opId, inc-1, rc);
    }
    response->set_err(rc);
  }

  void amfMgmtRpcImpl::deleteSGSIList(const ::SAFplus::Rpc::amfMgmtRpc::DeleteSGSIListRequest* request,
                                ::SAFplus::Rpc::amfMgmtRpc::DeleteSGSIListResponse* response)
  {
    const std::string& sgName = request->sgname();
    logDebug("MGMT","RPC","enter [%s] with param sg name [%s]",__FUNCTION__,sgName.c_str());
    int opId = (AMF_MGMT_OP_SG_SI_LIST_DELETE<<16)|inc++;
    ClRcT rc = validateOperation(opId, request);
    if (rc != CL_OK)
    {
      response->set_err(rc);
      return;
    }
    DbalPlugin* pd = NULL;
    rc = getDbalObj(request->amfmgmthandle().Get(0).c_str(), &pd);
    if (rc == CL_OK)
    {
      std::string strMsgReq;
      request->SerializeToString(&strMsgReq);      
      rc = pd->insertRecord(ClDBKeyT(&opId),
                           (ClUint32T)sizeof(opId),
                           (ClDBRecordT)strMsgReq.c_str(),
                           (ClUint32T)strMsgReq.length());
      logDebug("MGMT","RPC","[%s] insertRecord with key [%d], inc [%d] returns [0x%x]",__FUNCTION__, opId, inc-1, rc);
    }
    response->set_err(rc);
  }

  void amfMgmtRpcImpl::deleteSUCompList(const ::SAFplus::Rpc::amfMgmtRpc::DeleteSUCompListRequest* request,
                                ::SAFplus::Rpc::amfMgmtRpc::DeleteSUCompListResponse* response)
  {
    const std::string& suName = request->suname();
    logDebug("MGMT","RPC","enter [%s] with param su name [%s]",__FUNCTION__,suName.c_str());
    int opId = (AMF_MGMT_OP_SU_COMP_LIST_DELETE<<16)|inc++;
    ClRcT rc = validateOperation(opId, request);
    if (rc != CL_OK)
    {
      response->set_err(rc);
      return;
    }
    DbalPlugin* pd = NULL;
    rc = getDbalObj(request->amfmgmthandle().Get(0).c_str(), &pd);
    if (rc == CL_OK)
    {
      std::string strMsgReq;
      request->SerializeToString(&strMsgReq);      
      rc = pd->insertRecord(ClDBKeyT(&opId),
                           (ClUint32T)sizeof(opId),
                           (ClDBRecordT)strMsgReq.c_str(),
                           (ClUint32T)strMsgReq.length());
      logDebug("MGMT","RPC","[%s] insertRecord with key [%d], inc [%d] returns [0x%x]",__FUNCTION__, opId, inc-1, rc);
    }
    response->set_err(rc);
  }

  void amfMgmtRpcImpl::deleteSICSIList(const ::SAFplus::Rpc::amfMgmtRpc::DeleteSICSIListRequest* request,
                                ::SAFplus::Rpc::amfMgmtRpc::DeleteSICSIListResponse* response)
  {
    const std::string& siName = request->siname();
    logDebug("MGMT","RPC","enter [%s] with param si name [%s]",__FUNCTION__,siName.c_str());
    int opId = (AMF_MGMT_OP_SI_CSI_LIST_DELETE<<16)|inc++;
    ClRcT rc = validateOperation(opId, request);
    if (rc != CL_OK)
    {
      response->set_err(rc);
      return;
    }
    DbalPlugin* pd = NULL;
    rc = getDbalObj(request->amfmgmthandle().Get(0).c_str(), &pd);
    if (rc == CL_OK)
    {
      std::string strMsgReq;
      request->SerializeToString(&strMsgReq);      
      rc = pd->insertRecord(ClDBKeyT(&opId),
                           (ClUint32T)sizeof(opId),
                           (ClDBRecordT)strMsgReq.c_str(),
                           (ClUint32T)strMsgReq.length());
      logDebug("MGMT","RPC","[%s] insertRecord with key [%d], inc [%d] returns [0x%x]",__FUNCTION__, opId, inc-1, rc);
    }
    response->set_err(rc);
  }

  void amfMgmtRpcImpl::lockNodeAssignment(const ::SAFplus::Rpc::amfMgmtRpc::LockNodeAssignmentRequest* request,
                                ::SAFplus::Rpc::amfMgmtRpc::LockNodeAssignmentResponse* response)
  {
    const std::string& nodeName = request->nodename();
    logDebug("MGMT","RPC","enter [%s] with param node name [%s]",__FUNCTION__,nodeName.c_str());
    ClRcT rc = CL_OK;
#ifdef HANDLE_VALIDATE
    DbalPlugin* pd = NULL;
    rc = getDbalObj(request->amfmgmthandle().Get(0).c_str(), &pd);
    if (rc != CL_OK)
    {
      logDebug("MGMT","RPC","invalid handle, rc [0x%x", rc);
      response->set_err(rc);
      return;
    }
#endif
    SAFplusAmf::Node* node = dynamic_cast<SAFplusAmf::Node*>(cfg.safplusAmf.nodeList[nodeName]);
    if (node == NULL)
    {
      logDebug("MGMT","RPC","node object is null for its name [%s]", nodeName.c_str());
      rc = CL_ERR_INVALID_PARAMETER;
    }
    else
    {
      //rc = SAFplus::setAdminState(node, SAFplusAmf::AdministrativeState::idle,true);
      if(SAFplus::operationsPendingForNode(node))
      {
        logNotice("MGMT","RPC","Node [%s] has pending operation. Deferring lock assignment!", nodeName.c_str());
        rc = CL_ERR_TRY_AGAIN;
      }
      else
        rc = SAFplus::setAdminState(node,SAFplusAmf::AdministrativeState::idle,true);
    }
    response->set_err(rc);
  }

  void amfMgmtRpcImpl::lockSGAssignment(const ::SAFplus::Rpc::amfMgmtRpc::LockSGAssignmentRequest* request,
                                ::SAFplus::Rpc::amfMgmtRpc::LockSGAssignmentResponse* response)
  {
    const std::string& sgName = request->sgname();
    logDebug("MGMT","RPC","enter [%s] with param sg name [%s]",__FUNCTION__,sgName.c_str());
    ClRcT rc = CL_OK;
#ifdef HANDLE_VALIDATE
    DbalPlugin* pd = NULL;
    rc = getDbalObj(request->amfmgmthandle().Get(0).c_str(), &pd);
    if (rc != CL_OK)
    {
      logDebug("MGMT","RPC","invalid handle, rc [0x%x", rc);
      response->set_err(rc);
      return;
    }
#endif
    SAFplusAmf::ServiceGroup* sg = dynamic_cast<SAFplusAmf::ServiceGroup*>(cfg.safplusAmf.serviceGroupList[sgName]);
    if (sg == NULL)
    {
      logDebug("MGMT","RPC","sg object is null for its name [%s]", sgName.c_str());
      rc = CL_ERR_INVALID_PARAMETER;
    }
    else
    {
      //rc = SAFplus::setAdminState(sg, SAFplusAmf::AdministrativeState::idle,true);
      if(SAFplus::operationsPendingForSG(sg))
      {
        logNotice("MGMT","RPC","SG [%s] has pending operation. Deferring lock assignment!", sgName.c_str());
        rc = CL_ERR_TRY_AGAIN;
      }
      else
        rc = SAFplus::setAdminState(sg,SAFplusAmf::AdministrativeState::idle,true);
    }
    response->set_err(rc);
  }

  void amfMgmtRpcImpl::lockSUAssignment(const ::SAFplus::Rpc::amfMgmtRpc::LockSUAssignmentRequest* request,
                                ::SAFplus::Rpc::amfMgmtRpc::LockSUAssignmentResponse* response)
  {
    const std::string& suName = request->suname();
    logDebug("MGMT","RPC","enter [%s] with param su name [%s]",__FUNCTION__,suName.c_str());
    ClRcT rc = CL_OK;
#ifdef HANDLE_VALIDATE
    DbalPlugin* pd = NULL;
    rc = getDbalObj(request->amfmgmthandle().Get(0).c_str(), &pd);
    if (rc != CL_OK)
    {
      logDebug("MGMT","RPC","invalid handle, rc [0x%x", rc);
      response->set_err(rc);
      return;
    }
#endif
    SAFplusAmf::ServiceUnit* su = dynamic_cast<SAFplusAmf::ServiceUnit*>(cfg.safplusAmf.serviceUnitList[suName]);
    if (su == NULL)
    {
      logDebug("MGMT","RPC","su object is null for its name [%s]", suName.c_str());
      rc = CL_ERR_INVALID_PARAMETER;
    }
    else
    {
      //rc = SAFplus::setAdminState(su,SAFplusAmf::AdministrativeState::idle,true);
      if(SAFplus::operationsPendingForSU(su))
      {
        logNotice("MGMT","RPC","SU [%s] has pending operation. Deferring lock assignment!", suName.c_str());
        rc = CL_ERR_TRY_AGAIN;
      }
      else
        rc = SAFplus::setAdminState(su,SAFplusAmf::AdministrativeState::idle,true);
    }
    
    response->set_err(rc);
  }

  void amfMgmtRpcImpl::lockSIAssignment(const ::SAFplus::Rpc::amfMgmtRpc::LockSIAssignmentRequest* request,
                                ::SAFplus::Rpc::amfMgmtRpc::LockSIAssignmentResponse* response)
  {
    const std::string& siName = request->siname();
    logDebug("MGMT","RPC","enter [%s] with param si name [%s]",__FUNCTION__,siName.c_str());
    ClRcT rc = CL_OK;
#ifdef HANDLE_VALIDATE
    DbalPlugin* pd = NULL;
    rc = getDbalObj(request->amfmgmthandle().Get(0).c_str(), &pd);
    if (rc != CL_OK)
    {
      logDebug("MGMT","RPC","invalid handle, rc [0x%x", rc);
      response->set_err(rc);
      return;
    }
#endif
    SAFplusAmf::ServiceInstance* si = dynamic_cast<SAFplusAmf::ServiceInstance*>(cfg.safplusAmf.serviceInstanceList[siName]);
    if (si == NULL)
    {
      logDebug("MGMT","RPC","si object is null for its name [%s]", siName.c_str());
      rc = CL_ERR_INVALID_PARAMETER;
    }
    else
    {
      //rc = SAFplus::setAdminState(si,SAFplusAmf::AdministrativeState::idle,true);
      if(SAFplus::operationsPendingForSI(si))
      {
        logNotice("MGMT","RPC","SI [%s] has pending operation. Deferring lock assignment!", siName.c_str());
        rc = CL_ERR_TRY_AGAIN;
      }
      else
        rc = SAFplus::setAdminState(si,SAFplusAmf::AdministrativeState::idle,true);
    }

    response->set_err(rc);
  }

  void amfMgmtRpcImpl::lockNodeInstantiation(const ::SAFplus::Rpc::amfMgmtRpc::LockNodeInstantiationRequest* request,
                                ::SAFplus::Rpc::amfMgmtRpc::LockNodeInstantiationResponse* response)
  {
    const std::string& nodeName = request->nodename();
    logDebug("MGMT","RPC","enter [%s] with param node name [%s]",__FUNCTION__,nodeName.c_str());
    ClRcT rc = CL_OK;
#ifdef HANDLE_VALIDATE
    DbalPlugin* pd = NULL;
    rc = getDbalObj(request->amfmgmthandle().Get(0).c_str(), &pd);
    if (rc != CL_OK)
    {
      logDebug("MGMT","RPC","invalid handle, rc [0x%x", rc);
      response->set_err(rc);
      return;
    }
#endif
    SAFplusAmf::Node* node = dynamic_cast<SAFplusAmf::Node*>(cfg.safplusAmf.nodeList[nodeName]);
    if (node == NULL)
    {
      logDebug("MGMT","RPC","node object is null for its name [%s]", nodeName.c_str());
      rc = CL_ERR_INVALID_PARAMETER;
    }
    else
    {
      if (node->adminState.value != SAFplusAmf::AdministrativeState::idle)
      {
        rc = CL_ERR_INVALID_STATE;
      }
      else
      {
        //rc = SAFplus::setAdminState(node, SAFplusAmf::AdministrativeState::off,true);
        if(SAFplus::operationsPendingForNode(node))
        {
          logNotice("MGMT","RPC","Node [%s] has pending operation. Deferring lock instantiation!", nodeName.c_str());
          rc = CL_ERR_TRY_AGAIN;
        }
        else
          rc = SAFplus::setAdminState(node,SAFplusAmf::AdministrativeState::off,true);
      }
    }

    response->set_err(rc);
  }

  void amfMgmtRpcImpl::lockSGInstantiation(const ::SAFplus::Rpc::amfMgmtRpc::LockSGInstantiationRequest* request,
                                ::SAFplus::Rpc::amfMgmtRpc::LockSGInstantiationResponse* response)
  {
    const std::string& sgName = request->sgname();
    logDebug("MGMT","RPC","enter [%s] with param sg name [%s]",__FUNCTION__,sgName.c_str());
    ClRcT rc = CL_OK;
#ifdef HANDLE_VALIDATE
    DbalPlugin* pd = NULL;
    rc = getDbalObj(request->amfmgmthandle().Get(0).c_str(), &pd);
    if (rc != CL_OK)
    {
      logDebug("MGMT","RPC","invalid handle, rc [0x%x", rc);
      response->set_err(rc);
      return;
    }
#endif
    SAFplusAmf::ServiceGroup* sg = dynamic_cast<SAFplusAmf::ServiceGroup*>(cfg.safplusAmf.serviceGroupList[sgName]);
    if (sg == NULL)
    {
      logDebug("MGMT","RPC","sg object is null for its name [%s]", sgName.c_str());
      rc = CL_ERR_INVALID_PARAMETER;
    }
    else
    {
      if (sg->adminState.value != SAFplusAmf::AdministrativeState::idle)
      {
        rc = CL_ERR_INVALID_STATE;
      }
      else
      {
        //rc = SAFplus::setAdminState(sg, SAFplusAmf::AdministrativeState::off,true);
        if(SAFplus::operationsPendingForSG(sg))
        {
          logNotice("MGMT","RPC","SG [%s] has pending operation. Deferring lock instantiation!", sgName.c_str());
          rc = CL_ERR_TRY_AGAIN;
        }
        else
          rc = SAFplus::setAdminState(sg,SAFplusAmf::AdministrativeState::off,true);
      }
    }

    response->set_err(rc);
  }

  void amfMgmtRpcImpl::lockSUInstantiation(const ::SAFplus::Rpc::amfMgmtRpc::LockSUInstantiationRequest* request,
                                ::SAFplus::Rpc::amfMgmtRpc::LockSUInstantiationResponse* response)
  {
    const std::string& suName = request->suname();
    logDebug("MGMT","RPC","enter [%s] with param su name [%s]",__FUNCTION__,suName.c_str());
    ClRcT rc = CL_OK;
#ifdef HANDLE_VALIDATE
    DbalPlugin* pd = NULL;
    rc = getDbalObj(request->amfmgmthandle().Get(0).c_str(), &pd);
    if (rc != CL_OK)
    {
      logDebug("MGMT","RPC","invalid handle, rc [0x%x", rc);
      response->set_err(rc);
      return;
    }
#endif
    SAFplusAmf::ServiceUnit* su = dynamic_cast<SAFplusAmf::ServiceUnit*>(cfg.safplusAmf.serviceUnitList[suName]);
    if (su == NULL)
    {
      logDebug("MGMT","RPC","su object is null for its name [%s]", suName.c_str());
      rc = CL_ERR_INVALID_PARAMETER;
    }
    else
    {
      if (su->adminState.value != SAFplusAmf::AdministrativeState::idle)
      {
        rc = CL_ERR_INVALID_STATE;
      }
      else
      {
        //rc = SAFplus::setAdminState(su,SAFplusAmf::AdministrativeState::off,true);
        if(SAFplus::operationsPendingForSU(su))
        {
          logNotice("MGMT","RPC","SU [%s] has pending operation. Deferring lock instantiation!", suName.c_str());
          rc = CL_ERR_TRY_AGAIN;
        }
        else
          rc = SAFplus::setAdminState(su,SAFplusAmf::AdministrativeState::off,true);
      }
      //logInfo("MGMT","RPC","setting service unit [%s] to admin state [%s] ==> writting changes to DB",suName.c_str(),c_str(SAFplusAmf::AdministrativeState::off));
      //su->write(); // write immedidately rather than waiting for AMF to write, which is too late to reflect the changes
    }
    response->set_err(rc);
  }

  void amfMgmtRpcImpl::unlockNode(const ::SAFplus::Rpc::amfMgmtRpc::UnlockNodeRequest* request,
                                ::SAFplus::Rpc::amfMgmtRpc::UnlockNodeResponse* response)
  {
    const std::string& nodeName = request->nodename();
    logDebug("MGMT","RPC","enter [%s] with param node name [%s]",__FUNCTION__,nodeName.c_str());
    ClRcT rc = CL_OK;
#ifdef HANDLE_VALIDATE
    DbalPlugin* pd = NULL;
    rc = getDbalObj(request->amfmgmthandle().Get(0).c_str(), &pd);
    if (rc != CL_OK)
    {
      logDebug("MGMT","RPC","invalid handle, rc [0x%x", rc);
      response->set_err(rc);
      return;
    }
#endif
    SAFplusAmf::Node* node = dynamic_cast<SAFplusAmf::Node*>(cfg.safplusAmf.nodeList[nodeName]);
    if (node == NULL)
    {
      logDebug("MGMT","RPC","node object is null for its name [%s]", nodeName.c_str());
      rc = CL_ERR_INVALID_PARAMETER;
    }
    else
    {
      if (node->adminState.value != SAFplusAmf::AdministrativeState::idle)
      {
        rc = CL_ERR_INVALID_STATE;
      }
      else
      {
        //rc = SAFplus::setAdminState(node, SAFplusAmf::AdministrativeState::on,true);
        if(SAFplus::operationsPendingForNode(node))
        {
          logNotice("MGMT","RPC","Node [%s] has pending operation. Deferring unlock!", nodeName.c_str());
          rc = CL_ERR_TRY_AGAIN;
        }
        else
          rc = SAFplus::setAdminState(node,SAFplusAmf::AdministrativeState::on,true);
      }
    }
    response->set_err(rc);
  }

  void amfMgmtRpcImpl::unlockSG(const ::SAFplus::Rpc::amfMgmtRpc::UnlockSGRequest* request,
                                ::SAFplus::Rpc::amfMgmtRpc::UnlockSGResponse* response)
  {
    const std::string& sgName = request->sgname();
    logDebug("MGMT","RPC","enter [%s] with param sg name [%s]",__FUNCTION__,sgName.c_str());
    ClRcT rc = CL_OK;
#ifdef HANDLE_VALIDATE
    DbalPlugin* pd = NULL;
    rc = getDbalObj(request->amfmgmthandle().Get(0).c_str(), &pd);
    if (rc != CL_OK)
    {
      logDebug("MGMT","RPC","invalid handle, rc [0x%x", rc);
      response->set_err(rc);
      return;
    }
#endif
    SAFplusAmf::ServiceGroup* sg = dynamic_cast<SAFplusAmf::ServiceGroup*>(cfg.safplusAmf.serviceGroupList[sgName]);
    if (sg == NULL)
    {
      logDebug("MGMT","RPC","sg object is null for its name [%s]", sgName.c_str());
      rc = CL_ERR_INVALID_PARAMETER;
    }
    else
    {
      if (sg->adminState.value != SAFplusAmf::AdministrativeState::idle)
      {
        rc = CL_ERR_INVALID_STATE;
      }
      else
      {
        //rc = SAFplus::setAdminState(sg, SAFplusAmf::AdministrativeState::on,true);
        if(SAFplus::operationsPendingForSG(sg))
        {
          logNotice("MGMT","RPC","SG [%s] has pending operation. Deferring unlock!", sgName.c_str());
          rc = CL_ERR_TRY_AGAIN;
        }
        else
          rc = SAFplus::setAdminState(sg,SAFplusAmf::AdministrativeState::on,true);
      }
    }
    response->set_err(rc);
  }

  void amfMgmtRpcImpl::unlockSU(const ::SAFplus::Rpc::amfMgmtRpc::UnlockSURequest* request,
                                ::SAFplus::Rpc::amfMgmtRpc::UnlockSUResponse* response)
  {
    const std::string& suName = request->suname();
    logDebug("MGMT","RPC","enter [%s] with param su name [%s]",__FUNCTION__,suName.c_str());
    ClRcT rc = CL_OK;
#ifdef HANDLE_VALIDATE
    DbalPlugin* pd = NULL;
    rc = getDbalObj(request->amfmgmthandle().Get(0).c_str(), &pd);
    if (rc != CL_OK)
    {
      logDebug("MGMT","RPC","invalid handle, rc [0x%x", rc);
      response->set_err(rc);
      return;
    }
#endif
    SAFplusAmf::ServiceUnit* su = dynamic_cast<SAFplusAmf::ServiceUnit*>(cfg.safplusAmf.serviceUnitList[suName]);
    if (su == NULL)
    {
      logDebug("MGMT","RPC","su object is null for its name [%s]", suName.c_str());
      rc = CL_ERR_INVALID_PARAMETER;
    }
    else
    {
      if (su->adminState.value != SAFplusAmf::AdministrativeState::idle)
      {
        rc = CL_ERR_INVALID_STATE;
      }
      else
      {
        //rc = SAFplus::setAdminState(su,SAFplusAmf::AdministrativeState::on,true);
        if(SAFplus::operationsPendingForSU(su))
        {
          logNotice("MGMT","RPC","SU [%s] has pending operation. Deferring unlock!", suName.c_str());
          rc = CL_ERR_TRY_AGAIN;
        }
        else
          rc = SAFplus::setAdminState(su,SAFplusAmf::AdministrativeState::on,true);
      }
    }
    response->set_err(rc);
  }

  void amfMgmtRpcImpl::unlockSI(const ::SAFplus::Rpc::amfMgmtRpc::UnlockSIRequest* request,
                                ::SAFplus::Rpc::amfMgmtRpc::UnlockSIResponse* response)
  {
    const std::string& siName = request->siname();
    logDebug("MGMT","RPC","enter [%s] with param si name [%s]",__FUNCTION__,siName.c_str());
    ClRcT rc = CL_OK;
#ifdef HANDLE_VALIDATE
    DbalPlugin* pd = NULL;
    rc = getDbalObj(request->amfmgmthandle().Get(0).c_str(), &pd);
    if (rc != CL_OK)
    {
      logDebug("MGMT","RPC","invalid handle, rc [0x%x", rc);
      response->set_err(rc);
      return;
    }
#endif
    SAFplusAmf::ServiceInstance* si = dynamic_cast<SAFplusAmf::ServiceInstance*>(cfg.safplusAmf.serviceInstanceList[siName]);
    if (si == NULL)
    {
      logDebug("MGMT","RPC","si object is null for its name [%s]", siName.c_str());
      rc = CL_ERR_INVALID_PARAMETER;
    }
    else
    {
      if (si->adminState.value != SAFplusAmf::AdministrativeState::idle)
      {
        rc = CL_ERR_INVALID_STATE;
      }
      else
      {
        //rc = SAFplus::setAdminState(si,SAFplusAmf::AdministrativeState::on,true);
        if(SAFplus::operationsPendingForSI(si))
        {
          logNotice("MGMT","RPC","SI [%s] has pending operation. Deferring unlock!", siName.c_str());
          rc = CL_ERR_TRY_AGAIN;
        }
        else
          rc = SAFplus::setAdminState(si,SAFplusAmf::AdministrativeState::on,true);
      }
    }
    response->set_err(rc);
  }

  void amfMgmtRpcImpl::repairNode(const ::SAFplus::Rpc::amfMgmtRpc::RepairNodeRequest* request,
                                ::SAFplus::Rpc::amfMgmtRpc::RepairNodeResponse* response)
  {
    const std::string& nodeName = request->nodename();
    logDebug("MGMT","RPC","enter [%s] with param node name [%s]",__FUNCTION__,nodeName.c_str());
    ClRcT rc = CL_OK;
#ifdef HANDLE_VALIDATE
    DbalPlugin* pd = NULL;
    rc = getDbalObj(request->amfmgmthandle().Get(0).c_str(), &pd);
    if (rc != CL_OK)
    {
      logDebug("MGMT","RPC","invalid handle, rc [0x%x", rc);
      response->set_err(rc);
      return;
    }
#endif
    SAFplusAmf::Node* node = dynamic_cast<SAFplusAmf::Node*>(cfg.safplusAmf.nodeList[nodeName]);
    if (node == NULL)
    {
      logDebug("MGMT","RPC","node object is null for its name [%s]", nodeName.c_str());
      rc = CL_ERR_INVALID_PARAMETER;
    }
    else
    {
      if (node->operState.value == false)
      {       
        node->operState.value = true;
        node->operState.write(); // write to DB to prevent amf reload it from DB with the old value
        logInfo("MGMT","RPC","node [%s] is repaired", nodeName.c_str());
      }
      else
      {
        rc = CL_ERR_INVALID_STATE;
        logDebug("MGMT","RPC","node [%s] does not need repairing", nodeName.c_str());
      }
    }
    response->set_err(rc);
  }

  void amfMgmtRpcImpl::repairComponent(const ::SAFplus::Rpc::amfMgmtRpc::RepairComponentRequest* request,
                                ::SAFplus::Rpc::amfMgmtRpc::RepairComponentResponse* response)
  {
    const std::string& compName = request->compname();
    logDebug("MGMT","RPC","enter [%s] with param comp name [%s]",__FUNCTION__,compName.c_str());
    ClRcT rc = CL_OK;
#ifdef HANDLE_VALIDATE
    DbalPlugin* pd = NULL;
    rc = getDbalObj(request->amfmgmthandle().Get(0).c_str(), &pd);
    if (rc != CL_OK)
    {
      logDebug("MGMT","RPC","invalid handle, rc [0x%x", rc);
      response->set_err(rc);
      return;
    }
#endif
    SAFplusAmf::Component* comp = dynamic_cast<SAFplusAmf::Component*>(cfg.safplusAmf.componentList[compName]);
    if (comp == NULL)
    {
      logDebug("MGMT","RPC","comp object is null for its name [%s]", compName.c_str());
      rc = CL_ERR_INVALID_PARAMETER;
    }
    else
    {
      if (comp->operState.value == false)
      {       
        comp->operState.value = true;
        comp->operState.write(); // write to DB to prevent amf reload it from DB with the old value
        logInfo("MGMT","RPC","comp [%s] is repaired", compName.c_str());      
      }
      else
      {
        rc = CL_ERR_INVALID_STATE;
        logDebug("MGMT","RPC","comp [%s] does not need repairing", compName.c_str());
      }
    }
    response->set_err(rc);
  }

  void amfMgmtRpcImpl::repairSU(const ::SAFplus::Rpc::amfMgmtRpc::RepairSURequest* request,
                                ::SAFplus::Rpc::amfMgmtRpc::RepairSUResponse* response)
  {
    const std::string& suName = request->suname();
    logDebug("MGMT","RPC","enter [%s] with param su name [%s]",__FUNCTION__,suName.c_str());
    ClRcT rc = CL_OK;
#ifdef HANDLE_VALIDATE
    DbalPlugin* pd = NULL;
    rc = getDbalObj(request->amfmgmthandle().Get(0).c_str(), &pd);
    if (rc != CL_OK)
    {
      logDebug("MGMT","RPC","invalid handle, rc [0x%x", rc);
      response->set_err(rc);
      return;
    }
#endif
    SAFplusAmf::ServiceUnit* su = dynamic_cast<SAFplusAmf::ServiceUnit*>(cfg.safplusAmf.serviceUnitList[suName]);
    if (su == NULL)
    {
      logDebug("MGMT","RPC","su object is null for its name [%s]", suName.c_str());
      rc = CL_ERR_INVALID_PARAMETER;
    }
    else
    {
      if (su->operState.value == false)
      {       
        su->operState.value = true;
        su->operState.write(); // write to DB to prevent amf reload it from DB with the old value
        logInfo("MGMT","RPC","su [%s] is repaired", suName.c_str());
      }
      else
      {
        rc = CL_ERR_INVALID_STATE;
        logDebug("MGMT","RPC","su [%s] does not need repairing", suName.c_str());
      }
    }
    response->set_err(rc);
  }

  void amfMgmtRpcImpl::getComponentConfig(const ::SAFplus::Rpc::amfMgmtRpc::GetComponentConfigRequest* request,
                                ::SAFplus::Rpc::amfMgmtRpc::GetComponentConfigResponse* response)
  {
    const std::string& compName = request->compname();
    logDebug("MGMT","RPC","enter [%s] with param comp name [%s]",__FUNCTION__,compName.c_str());
    ClRcT rc = CL_OK;
    SAFplusAmf::Component* comp = dynamic_cast<SAFplusAmf::Component*>(cfg.safplusAmf.componentList[compName]);
    if (comp == NULL)
    {
      logDebug("MGMT","RPC","comp object is null for its name [%s]", compName.c_str());
      rc = CL_ERR_INVALID_PARAMETER;
    }
    else
    {
      ComponentConfig* compConfig = new ComponentConfig();
      if (compConfig == NULL)
      {
        logError("MGMT","RPC","No memory");
        rc = CL_ERR_NO_MEMORY;
      }
      else
      {
        rc = fillCompConfig(comp,compConfig);        
        response->set_allocated_componentconfig(compConfig);
      }
    }
    response->set_err(rc);
  }

  void amfMgmtRpcImpl::getNodeConfig(const ::SAFplus::Rpc::amfMgmtRpc::GetNodeConfigRequest* request,
                                ::SAFplus::Rpc::amfMgmtRpc::GetNodeConfigResponse* response)
  {
    const std::string& nodeName = request->nodename();
    logDebug("MGMT","RPC","enter [%s] with param node name [%s]",__FUNCTION__,nodeName.c_str());
    ClRcT rc = CL_OK;
    SAFplusAmf::Node* node = dynamic_cast<SAFplusAmf::Node*>(cfg.safplusAmf.nodeList[nodeName]);
    if (node == NULL)
    {
      logDebug("MGMT","RPC","node object is null for its name [%s]", nodeName.c_str());
      rc = CL_ERR_INVALID_PARAMETER;
    }
    else
    {
      NodeConfig* nodeConfig = new NodeConfig();
      if (nodeConfig == NULL)
      {
        logError("MGMT","RPC","No memory");
        rc = CL_ERR_NO_MEMORY;
      }
      else
      {
        rc = fillNodeConfig(node,nodeConfig);
        response->set_allocated_nodeconfig(nodeConfig);        
      }
    }
    response->set_err(rc);
  }

  void amfMgmtRpcImpl::getSGConfig(const ::SAFplus::Rpc::amfMgmtRpc::GetSGConfigRequest* request,
                                ::SAFplus::Rpc::amfMgmtRpc::GetSGConfigResponse* response)
  {
    const std::string& sgName = request->sgname();
    logDebug("MGMT","RPC","enter [%s] with param name [%s]",__FUNCTION__,sgName.c_str());
    ClRcT rc = CL_OK;
    SAFplusAmf::ServiceGroup* sg = dynamic_cast<SAFplusAmf::ServiceGroup*>(cfg.safplusAmf.serviceGroupList[sgName]);
    if (sg == NULL)
    {
      logDebug("MGMT","RPC","object is null for its name [%s]", sgName.c_str());
      rc = CL_ERR_INVALID_PARAMETER;
    }
    else
    {
      ServiceGroupConfig* sgConfig = new ServiceGroupConfig();
      if (sgConfig == NULL)
      {
        logError("MGMT","RPC","No memory");
        rc = CL_ERR_NO_MEMORY;
      }
      else
      {
        rc = fillSgConfig(sg,sgConfig);
        response->set_allocated_servicegroupconfig(sgConfig);        
      }
    }
    response->set_err(rc);
  }

  void amfMgmtRpcImpl::getSUConfig(const ::SAFplus::Rpc::amfMgmtRpc::GetSUConfigRequest* request,
                                ::SAFplus::Rpc::amfMgmtRpc::GetSUConfigResponse* response)
  {
    const std::string& suName = request->suname();
    logDebug("MGMT","RPC","enter [%s] with param name [%s]",__FUNCTION__,suName.c_str());
    ClRcT rc = CL_OK;
    SAFplusAmf::ServiceUnit* su = dynamic_cast<SAFplusAmf::ServiceUnit*>(cfg.safplusAmf.serviceUnitList[suName]);
    if (su == NULL)
    {
      logDebug("MGMT","RPC","object is null for its name [%s]", suName.c_str());
      rc = CL_ERR_INVALID_PARAMETER;
    }
    else
    {
      ServiceUnitConfig* suConfig = new ServiceUnitConfig();
      if (suConfig == NULL)
      {
        logError("MGMT","RPC","No memory");
        rc = CL_ERR_NO_MEMORY;
      }
      else
      {
        rc = fillSuConfig(su,suConfig);
        response->set_allocated_serviceunitconfig(suConfig);        
      }
    }
    response->set_err(rc);
  }
 
  void amfMgmtRpcImpl::getSIConfig(const ::SAFplus::Rpc::amfMgmtRpc::GetSIConfigRequest* request,
                                ::SAFplus::Rpc::amfMgmtRpc::GetSIConfigResponse* response)
  {
    const std::string& siName = request->siname();
    logDebug("MGMT","RPC","enter [%s] with param name [%s]",__FUNCTION__,siName.c_str());
    ClRcT rc = CL_OK;
    SAFplusAmf::ServiceInstance* si = dynamic_cast<SAFplusAmf::ServiceInstance*>(cfg.safplusAmf.serviceInstanceList[siName]);
    if (si == NULL)
    {
      logDebug("MGMT","RPC","object is null for its name [%s]", siName.c_str());
      rc = CL_ERR_INVALID_PARAMETER;
    }
    else
    {
      ServiceInstanceConfig* siConfig = new ServiceInstanceConfig();
      if (siConfig == NULL)
      {
        logError("MGMT","RPC","No memory");
        rc = CL_ERR_NO_MEMORY;
      }
      else
      {
        rc = fillSiConfig(si,siConfig);
        response->set_allocated_serviceinstanceconfig(siConfig);        
      }
    }
    response->set_err(rc);
  }

  void amfMgmtRpcImpl::getCSIConfig(const ::SAFplus::Rpc::amfMgmtRpc::GetCSIConfigRequest* request,
                                ::SAFplus::Rpc::amfMgmtRpc::GetCSIConfigResponse* response)
  {
    const std::string& csiName = request->csiname();
    logDebug("MGMT","RPC","enter [%s] with param name [%s]",__FUNCTION__,csiName.c_str());
    ClRcT rc = CL_OK;
    SAFplusAmf::ComponentServiceInstance* csi = dynamic_cast<SAFplusAmf::ComponentServiceInstance*>(cfg.safplusAmf.componentServiceInstanceList[csiName]);
    if (csi == NULL)
    {
      logDebug("MGMT","RPC","object is null for its name [%s]", csiName.c_str());
      rc = CL_ERR_INVALID_PARAMETER;
    }
    else
    {
      ComponentServiceInstanceConfig* csiConfig = new ComponentServiceInstanceConfig();
      if (csiConfig == NULL)
      {
        logError("MGMT","RPC","No memory");
        rc = CL_ERR_NO_MEMORY;
      }
      else
      {
        rc = fillCsiConfig(csi,csiConfig);
        response->set_allocated_componentserviceinstanceconfig(csiConfig);        
      }
    }
    response->set_err(rc);
  }
  
  void amfMgmtRpcImpl::getComponentStatus(const ::SAFplus::Rpc::amfMgmtRpc::GetComponentStatusRequest* request,
                                ::SAFplus::Rpc::amfMgmtRpc::GetComponentStatusResponse* response)
  {
    const std::string& compName = request->compname();
    logDebug("MGMT","RPC","enter [%s] with param comp name [%s]",__FUNCTION__,compName.c_str());
    ClRcT rc = CL_OK;
    SAFplusAmf::Component* comp = dynamic_cast<SAFplusAmf::Component*>(cfg.safplusAmf.componentList[compName]);
    if (comp == NULL)
    {
      logDebug("MGMT","RPC","comp object is null for its name [%s]", compName.c_str());
      rc = CL_ERR_INVALID_PARAMETER;
    }
    else
    {
      ComponentStatus* compStatus = new ComponentStatus();
      if (compStatus == NULL)
      {
        logError("MGMT","RPC","No memory");
        rc = CL_ERR_NO_MEMORY;
      }
      else
      {
        rc = fillCompStatus(comp,compStatus);
        response->set_allocated_componentstatus(compStatus);
      }
    }
    response->set_err(rc);
  }

  void amfMgmtRpcImpl::getNodeStatus(const ::SAFplus::Rpc::amfMgmtRpc::GetNodeStatusRequest* request,
                                ::SAFplus::Rpc::amfMgmtRpc::GetNodeStatusResponse* response)
  {
    const std::string& nodeName = request->nodename();
    logDebug("MGMT","RPC","enter [%s] with param node name [%s]",__FUNCTION__,nodeName.c_str());
    ClRcT rc = CL_OK;
    SAFplusAmf::Node* node = dynamic_cast<SAFplusAmf::Node*>(cfg.safplusAmf.nodeList[nodeName]);
    if (node == NULL)
    {
      logDebug("MGMT","RPC","node object is null for its name [%s]", nodeName.c_str());
      rc = CL_ERR_INVALID_PARAMETER;
    }
    else
    {
      NodeStatus* nodeStatus = new NodeStatus();
      if (nodeStatus == NULL)
      {
        logError("MGMT","RPC","No memory");
        rc = CL_ERR_NO_MEMORY;
      }
      else
      {
        rc = fillNodeStatus(node,nodeStatus);
        response->set_allocated_nodestatus(nodeStatus);
      }
    }
    response->set_err(rc);
  }

  void amfMgmtRpcImpl::getSUStatus(const ::SAFplus::Rpc::amfMgmtRpc::GetSUStatusRequest* request,
                                ::SAFplus::Rpc::amfMgmtRpc::GetSUStatusResponse* response)
  {
    const std::string& suName = request->suname();
    logDebug("MGMT","RPC","enter [%s] with param name [%s]",__FUNCTION__,suName.c_str());
    ClRcT rc = CL_OK;
    SAFplusAmf::ServiceUnit* su = dynamic_cast<SAFplusAmf::ServiceUnit*>(cfg.safplusAmf.serviceUnitList[suName]);
    if (su == NULL)
    {
      logDebug("MGMT","RPC","object is null for its name [%s]", suName.c_str());
      rc = CL_ERR_INVALID_PARAMETER;
    }
    else
    {
      ServiceUnitStatus* suStatus = new ServiceUnitStatus();
      if (suStatus == NULL)
      {
        logError("MGMT","RPC","No memory");
        rc = CL_ERR_NO_MEMORY;
      }
      else
      {
        rc = fillSuStatus(su,suStatus);
        response->set_allocated_serviceunitstatus(suStatus);        
      }
    }
    response->set_err(rc);
  }

  void amfMgmtRpcImpl::getSGStatus(const ::SAFplus::Rpc::amfMgmtRpc::GetSGStatusRequest* request,
                                ::SAFplus::Rpc::amfMgmtRpc::GetSGStatusResponse* response)
  {
    const std::string& sgName = request->sgname();
    logDebug("MGMT","RPC","enter [%s] with param name [%s]",__FUNCTION__,sgName.c_str());
    ClRcT rc = CL_OK;
    SAFplusAmf::ServiceGroup* sg = dynamic_cast<SAFplusAmf::ServiceGroup*>(cfg.safplusAmf.serviceGroupList[sgName]);
    if (sg == NULL)
    {
      logDebug("MGMT","RPC","object is null for its name [%s]", sgName.c_str());
      rc = CL_ERR_INVALID_PARAMETER;
    }
    else
    {
      ServiceGroupStatus* sgStatus = new ServiceGroupStatus();
      if (sgStatus == NULL)
      {
        logError("MGMT","RPC","No memory");
        rc = CL_ERR_NO_MEMORY;
      }
      else
      {
        rc = fillSgStatus(sg,sgStatus);
        response->set_allocated_servicegroupstatus(sgStatus);        
      }
    }
    response->set_err(rc);
  }

  void amfMgmtRpcImpl::getSIStatus(const ::SAFplus::Rpc::amfMgmtRpc::GetSIStatusRequest* request,
                                ::SAFplus::Rpc::amfMgmtRpc::GetSIStatusResponse* response)
  {
    const std::string& siName = request->siname();
    logDebug("MGMT","RPC","enter [%s] with param name [%s]",__FUNCTION__,siName.c_str());
    ClRcT rc = CL_OK;
    SAFplusAmf::ServiceInstance* si = dynamic_cast<SAFplusAmf::ServiceInstance*>(cfg.safplusAmf.serviceInstanceList[siName]);
    if (si == NULL)
    {
      logDebug("MGMT","RPC","object is null for its name [%s]", siName.c_str());
      rc = CL_ERR_INVALID_PARAMETER;
    }
    else
    {
      ServiceInstanceStatus* siStatus = new ServiceInstanceStatus();
      if (siStatus == NULL)
      {
        logError("MGMT","RPC","No memory");
        rc = CL_ERR_NO_MEMORY;
      }
      else
      {
        rc = fillSiStatus(si,siStatus);
        response->set_allocated_serviceinstancestatus(siStatus);        
      }
    }
    response->set_err(rc);
  }

  void amfMgmtRpcImpl::getCSIStatus(const ::SAFplus::Rpc::amfMgmtRpc::GetCSIStatusRequest* request,
                                ::SAFplus::Rpc::amfMgmtRpc::GetCSIStatusResponse* response)
  {
    const std::string& csiName = request->csiname();
    logDebug("MGMT","RPC","enter [%s] with param name [%s]",__FUNCTION__,csiName.c_str());
    ClRcT rc = CL_OK;
    SAFplusAmf::ComponentServiceInstance* csi = dynamic_cast<SAFplusAmf::ComponentServiceInstance*>(cfg.safplusAmf.componentServiceInstanceList[csiName]);
    if (csi == NULL)
    {
      logDebug("MGMT","RPC","object is null for its name [%s]", csiName.c_str());
      rc = CL_ERR_INVALID_PARAMETER;
    }
    else
    {
      ComponentServiceInstanceStatus* csiStatus = new ComponentServiceInstanceStatus();
      if (csiStatus == NULL)
      {
        logError("MGMT","RPC","No memory");
        rc = CL_ERR_NO_MEMORY;
      }
      else
      {
        rc = fillCsiStatus(csi,csiStatus);
        response->set_allocated_componentserviceinstancestatus(csiStatus);
      }
    }
    response->set_err(rc);
  }

  void amfMgmtRpcImpl::nodeRestart(const ::SAFplus::Rpc::amfMgmtRpc::NodeRestartRequest* request,
                                   ::SAFplus::Rpc::amfMgmtRpc::NodeRestartResponse* response)
  {
      const std::string& nodeName = request->nodename();
      logDebug("MGMT","RPC","enter [%s] with param node name [%s]",__FUNCTION__,nodeName.c_str());
      ClRcT rc = CL_OK;
#ifdef HANDLE_VALIDATE
      DbalPlugin* pd = NULL;
      rc = getDbalObj(request->amfmgmthandle().Get(0).c_str(), &pd);
      if (rc != CL_OK)
      {
          logDebug("MGMT","RPC","invalid handle, rc [0x%x", rc);
          response->set_err(rc);
          return;
      }
#endif
      SAFplusAmf::Node* node = dynamic_cast<SAFplusAmf::Node*>(cfg.safplusAmf.nodeList[nodeName]);
      if (node == NULL)
      {
          logDebug("MGMT","RPC","node object is null for its name [%s]", nodeName.c_str());
          rc = CL_ERR_INVALID_PARAMETER;
      }
      else
      {
          if (node->restartable.value == true)
          {
              amfOpsMgmt->nodeRestart(node);
          }
          else
          {
              logDebug("MGMT","RPC","Configure node  cannot restart");
              rc = CL_ERR_NO_OP;
          }
      }

      response->set_err(rc);
  }

  void amfMgmtRpcImpl::serviceUnitRestart(const ::SAFplus::Rpc::amfMgmtRpc::ServiceUnitRestartRequest* request,
                                          ::SAFplus::Rpc::amfMgmtRpc::ServiceUnitRestartResponse* response)
  {
      const std::string& suName = request->suname();
      logDebug("MGMT","RPC","enter [%s] with param su name [%s]",__FUNCTION__,suName.c_str());
      ClRcT rc = CL_OK;
#ifdef HANDLE_VALIDATE
      DbalPlugin* pd = NULL;
      rc = getDbalObj(request->amfmgmthandle().Get(0).c_str(), &pd);
      if (rc != CL_OK)
      {
          logDebug("MGMT","RPC","invalid handle, rc [0x%x", rc);
          response->set_err(rc);
          return;
      }
#endif
      SAFplusAmf::ServiceUnit* su = dynamic_cast<SAFplusAmf::ServiceUnit*>(cfg.safplusAmf.serviceUnitList[suName]);
      if (su == NULL)
      {
          logDebug("MGMT","RPC","su object is null for its name [%s]", suName.c_str());
          rc = CL_ERR_INVALID_PARAMETER;
      }
      else
      {
          if (su->restartable.value == true)
          {
              amfOpsMgmt->serviceUnitRestart(su);
          }
          else
          {
              logDebug("MGMT","RPC","Configure serviceUnit  cannot restart");
              rc = CL_ERR_NO_OP;
          }
      }

      response->set_err(rc);
  }

  void amfMgmtRpcImpl::componentRestart(const ::SAFplus::Rpc::amfMgmtRpc::ComponentRestartRequest* request,
                                        ::SAFplus::Rpc::amfMgmtRpc::ComponentRestartResponse* response)
  {
      const std::string& compName = request->compname();
      logDebug("MGMT","RPC","enter [%s] with param comp name [%s]",__FUNCTION__,compName.c_str());
      ClRcT rc = CL_OK;
#ifdef HANDLE_VALIDATE
      DbalPlugin* pd = NULL;
      rc = getDbalObj(request->amfmgmthandle().Get(0).c_str(), &pd);
      if (rc != CL_OK)
      {
          logDebug("MGMT","RPC","invalid handle, rc [0x%x", rc);
          response->set_err(rc);
          return;
      }
#endif
      SAFplusAmf::Component* comp = dynamic_cast<SAFplusAmf::Component*>(cfg.safplusAmf.componentList[compName]);
      if (comp == NULL)
      {
          logDebug("MGMT","RPC","comp object is null for its name [%s]", compName.c_str());
          rc = CL_ERR_INVALID_PARAMETER;
      }
      else
      {
          if (comp->restartable.value == true)
          {
              amfOpsMgmt->componentRestart(comp);
          }
          else
          {
              logDebug("MGMT","RPC","Configure component  cannot restart");
              rc = CL_ERR_NO_OP;
          }
      }
  }

  void amfMgmtRpcImpl::adjustSG(const ::SAFplus::Rpc::amfMgmtRpc::AdjustSGRequest* request,
                                ::SAFplus::Rpc::amfMgmtRpc::AdjustSGResponse* response)
  {
      const std::string& sgName = request->sgname();
      bool enabled = request->enabled();
      logDebug("MGMT","RPC","enter [%s] with param sg name [%s], enabled [%d]",__FUNCTION__,sgName.c_str(), enabled);
      ClRcT rc = CL_OK;
#ifdef HANDLE_VALIDATE
      DbalPlugin* pd = NULL;
      rc = getDbalObj(request->amfmgmthandle().Get(0).c_str(), &pd);
      if (rc != CL_OK)
      {
         logDebug("MGMT","RPC","invalid handle, rc [0x%x", rc);
         response->set_err(rc);
         return;
      }
#endif
      SAFplusAmf::ServiceGroup* sg = dynamic_cast<SAFplusAmf::ServiceGroup*>(cfg.safplusAmf.serviceGroupList[sgName]);
      if (sg == NULL)
      {
         logWarning("MGMT","RPC","sg object is null for its name [%s]", sgName.c_str());
         rc = CL_ERR_NOT_EXIST;
      }
      else
      {
          if (!enabled)
          {
            sg->autoAdjust.value = false;
          }
          else
          {
              if (sg->autoAdjust.value)
              {
                  logDebug("MGMT","RPC","nothing to do for sg [%s] adjustment due to its autoAdjust flag being set", sgName.c_str());
              }
              /*if(clAmsInvocationsPendingForSG(sg))
              {
                  clLogInfo("SG", "ADJUST",
                            "SG [%s] has pending invocations. Deferring adjust",
                            sg->config.entity.name.value);
                  return CL_AMS_RC(CL_ERR_TRY_AGAIN);
              }*/
              else
              {
                 sg->autoAdjust.value = true;
                /*
                * Start the auto adjust probation timer to reset adjustments.
                * */
                 sg->startAdjustTimer();
                 //rc = sgAdjust(sg);
                 rc = amfOpsMgmt->sgAdjust(sg);
              }
          }
      }
      response->set_err(rc);
  }

  void amfMgmtRpcImpl::swapSI(const ::SAFplus::Rpc::amfMgmtRpc::SwapSIRequest* request,
                                ::SAFplus::Rpc::amfMgmtRpc::SwapSIResponse* response)
  {
      const std::string& siName = request->siname();
      logDebug("MGMT","RPC","enter [%s] with param si name [%s]",__FUNCTION__,siName.c_str());
      ClRcT rc = CL_OK;
#ifdef HANDLE_VALIDATE
      DbalPlugin* pd = NULL;
      rc = getDbalObj(request->amfmgmthandle().Get(0).c_str(), &pd);
      if (rc != CL_OK)
      {
          logDebug("MGMT","RPC","invalid handle, rc [0x%x", rc);
          response->set_err(rc);
          return;
      }
#endif
      rc = amfOpsMgmt->swapSI(siName);

      response->set_err(rc);
  }


  void amfMgmtRpcImpl::compErrorReport(const ::SAFplus::Rpc::amfMgmtRpc::CompErrorReportRequest* request,
                                ::SAFplus::Rpc::amfMgmtRpc::CompErrorReportResponse* response)
  {
      const std::string& compName = request->compname();
      Recovery recommendedRecovery = request->recommendedrecovery();
      logDebug("MGMT","RPC","enter [%s] with param comp name [%s], recommendedRecovery [%d]",__FUNCTION__,compName.c_str(), recommendedRecovery);
      ClRcT rc = CL_OK;
#ifdef HANDLE_VALIDATE
      DbalPlugin* pd = NULL;
      rc = getDbalObj(request->amfmgmthandle().Get(0).c_str(), &pd);
      if (rc != CL_OK)
      {
          logDebug("MGMT","RPC","invalid handle, rc [0x%x", rc);
          response->set_err(rc);
          return;
      }
#endif
      SAFplusAmf::Component* comp = dynamic_cast<SAFplusAmf::Component*>(cfg.safplusAmf.componentList[compName]);
      if (comp == NULL)
      {
          logWarning("MGMT","RPC","comp object is null for its name [%s]", compName.c_str());
          rc = CL_ERR_NOT_EXIST;
      }
      else
      {
          //processFaultyComp(amfOpsMgmt, comp, (SAFplusAmf::Recovery) recommendedRecovery);
          for (auto it = redPolicies.begin(); it != redPolicies.end();it++)
              {
              ClAmfPolicyPlugin_1* pp = dynamic_cast<ClAmfPolicyPlugin_1*>(it->second->pluginApi);
              //gAmfPolicy = pp;
              pp->compFaultReport(comp, (SAFplusAmf::Recovery) recommendedRecovery);
              }
      }
      response->set_err(rc);
  }

  void amfMgmtRpcImpl::nodeErrorReport(const ::SAFplus::Rpc::amfMgmtRpc::NodeErrorReportRequest* request,
                                  ::SAFplus::Rpc::amfMgmtRpc::NodeErrorReportResponse* response)
  {
      const std::string& nodeName = request->nodename();
      bool shutdownAmf = request->shutdownamf();
      bool rebootNode = request->rebootnode();
      bool graceful = request->gracefulswitchover();
      bool restartAmf = request->restartamf();
      logDebug("MGMT","RPC","enter [%s] with param node name [%s], shutdownAmf [%d], rebootNode [%d], graceful [%d], restartAmf [%d]",__FUNCTION__,nodeName.c_str(), shutdownAmf, rebootNode, graceful, restartAmf);
      ClRcT rc = CL_OK;
#ifdef HANDLE_VALIDATE
      DbalPlugin* pd = NULL;
      rc = getDbalObj(request->amfmgmthandle().Get(0).c_str(), &pd);
      if (rc != CL_OK)
      {
          logDebug("MGMT","RPC","invalid handle, rc [0x%x", rc);
          response->set_err(rc);
          return;
      }
#endif
      SAFplusAmf::Node* node = dynamic_cast<SAFplusAmf::Node*>(cfg.safplusAmf.nodeList[nodeName]);
      if (node == NULL)
      {
          logWarning("MGMT","RPC","node object is null for its name [%s], it might be deleted or invalid name", nodeName.c_str());
          rc = CL_ERR_NOT_EXIST;
      }
      else
      {
          rc = amfOpsMgmt->nodeErrorReport(node, graceful, shutdownAmf, restartAmf, rebootNode);
      }
      response->set_err(rc);
  }

  void amfMgmtRpcImpl::nodeErrorClear(const ::SAFplus::Rpc::amfMgmtRpc::NodeErrorClearRequest* request,
                                  ::SAFplus::Rpc::amfMgmtRpc::NodeErrorClearResponse* response)
  {
      const std::string& nodeName = request->nodename();
      logDebug("MGMT","RPC","enter [%s] with param node name [%s]",__FUNCTION__,nodeName.c_str());
      ClRcT rc = CL_OK;
#ifdef HANDLE_VALIDATE
      DbalPlugin* pd = NULL;
      rc = getDbalObj(request->amfmgmthandle().Get(0).c_str(), &pd);
      if (rc != CL_OK)
      {
          logDebug("MGMT","RPC","invalid handle, rc [0x%x", rc);
          response->set_err(rc);
          return;
      }
#endif
      SAFplusAmf::Node* node = dynamic_cast<SAFplusAmf::Node*>(cfg.safplusAmf.nodeList[nodeName]);
      if (node == NULL)
      {
          logWarning("MGMT","RPC","node object is null for its name [%s]", nodeName.c_str());
          rc = CL_ERR_NOT_EXIST;
      }
      else
      {
          amfOpsMgmt->nodeErrorClear(node);
      }
      response->set_err(rc);
  }

  void amfMgmtRpcImpl::assignSUtoSI(const ::SAFplus::Rpc::amfMgmtRpc::AssignSUtoSIRequest* request,
                                ::SAFplus::Rpc::amfMgmtRpc::AssignSUtoSIResponse* response)
  {
      const std::string& siName = request->siname();
      const std::string& activeSUName = request->activesuname();
      const std::string& standbySUName = request->standbysuname();
      logDebug("MGMT","RPC","enter [%s] with param si name [%s], active su name [%s], standby su name [%s]",__FUNCTION__,siName.c_str(),activeSUName.c_str(),standbySUName.c_str());
      ClRcT rc = CL_OK;
#ifdef HANDLE_VALIDATE
      DbalPlugin* pd = NULL;
      rc = getDbalObj(request->amfmgmthandle().Get(0).c_str(), &pd);
      if (rc != CL_OK)
      {
          logDebug("MGMT","RPC","invalid handle, rc [0x%x", rc);
          response->set_err(rc);
          return;
      }
#endif
       rc = amfOpsMgmt->assignSUtoSI(siName, activeSUName, standbySUName);

      response->set_err(rc);
  }

  void amfMgmtRpcImpl::forceLockInstantiation(const ::SAFplus::Rpc::amfMgmtRpc::ForceLockInstantiationRequest* request,
                                ::SAFplus::Rpc::amfMgmtRpc::ForceLockInstantiationResponse* response)
  {
    const std::string& suName = request->suname();
    logDebug("MGMT","RPC","enter [%s] with param su name [%s]",__FUNCTION__,suName.c_str());
    ClRcT rc = CL_OK;
#ifdef HANDLE_VALIDATE
    DbalPlugin* pd = NULL;
    rc = getDbalObj(request->amfmgmthandle().Get(0).c_str(), &pd);
    if (rc != CL_OK)
    {
      logDebug("MGMT","RPC","invalid handle, rc [0x%x", rc);
      response->set_err(rc);
      return;
    }
#endif
    SAFplusAmf::ServiceUnit* su = dynamic_cast<SAFplusAmf::ServiceUnit*>(cfg.safplusAmf.serviceUnitList[suName]);
    if (su == NULL)
    {
      logDebug("MGMT","RPC","su object is null for its name [%s]", suName.c_str());
      rc = CL_ERR_INVALID_PARAMETER;
    }
    else
    {
        amfOpsMgmt->removeWorkWithoutAppRemove(su);
        rc = SAFplus::setAdminState(su,SAFplusAmf::AdministrativeState::off,true);
        SAFplus::MgtIdentifierList<SAFplusAmf::Component*>::iterator beginComp = su->components.listBegin();
        SAFplus::MgtIdentifierList<SAFplusAmf::Component*>::iterator endComp = su->components.listEnd();
        for (auto itComp{beginComp}; itComp != endComp; itComp++)
        {
            SAFplusAmf::Component* comp = dynamic_cast<SAFplusAmf::Component*>(*itComp);
            amfOpsMgmt->abort(comp, true);
        }
    }

    response->set_err(rc);
  }

  void amfMgmtRpcImpl::getSafplusInstallInfo(const ::SAFplus::Rpc::amfMgmtRpc::GetSafplusInstallInfoRequest* request,
                                ::SAFplus::Rpc::amfMgmtRpc::GetSafplusInstallInfoResponse* response)
  {
     const std::string& nodeName = request->nodename();    
     logDebug("MGMT","RPC","enter [%s] with param node name [%s]",__FUNCTION__,nodeName.c_str());
     std::string installInfo;
     ClRcT rc = getInstallInfo(nodeName, installInfo);
     response->set_safplusinstallinfo(installInfo);
     response->set_err(rc);
  }

  void amfMgmtRpcImpl::setSafplusInstallInfo(const ::SAFplus::Rpc::amfMgmtRpc::SetSafplusInstallInfoRequest* request,
                                ::SAFplus::Rpc::amfMgmtRpc::SetSafplusInstallInfoResponse* response)
  {
     const std::string& nodeName = request->nodename();
     const std::string& safplusInstallInfo = request->safplusinstallinfo();
     logDebug("MGMT","RPC","enter [%s] with param node name [%s], safplusInstallInfo [%s]",__FUNCTION__,nodeName.c_str(), safplusInstallInfo.c_str());
     ClRcT rc = setInstallInfo(nodeName, safplusInstallInfo);
     response->set_err(rc);
  }

}  // namespace amfMgmtRpc
}  // namespace Rpc
}  // namespace SAFplus
