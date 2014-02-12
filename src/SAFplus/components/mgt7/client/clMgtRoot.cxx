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

#include "clMgtRoot.hxx"
#ifdef MGT_ACCESS
#include "clMgtIoc.hxx"
#endif

#ifdef __cplusplus
extern "C"
{
#endif
#include <clCommonErrors.h>
#include <clDebugApi.h>

#ifdef __cplusplus
} /* end extern 'C' */
#endif

using namespace std;

#define clLog(...)

#ifdef MGT_ACCESS
#define CL_IOC_MGT_NETCONF_PORT (CL_IOC_USER_APP_WELLKNOWN_PORTS_START + 1)
#define CL_IOC_MGT_SNMP_PORT (CL_IOC_USER_APP_WELLKNOWN_PORTS_START + 2)
#endif

ClMgtRoot *ClMgtRoot::singletonInstance = 0;

ClMgtRoot *ClMgtRoot::getInstance()
{
    return (singletonInstance ? singletonInstance : (singletonInstance =
                                        new ClMgtRoot()));
}

ClMgtRoot::~ClMgtRoot()
{
#ifdef MGT_ACCESS
  
    /*
     * Do the application specific finalization here.
     */
    ClMgtIoc* mgtIocInstance = ClMgtIoc::getInstance();
    delete mgtIocInstance;
#endif    
}

ClMgtRoot::ClMgtRoot()
{
#ifdef MGT_ACCESS
    /*
     * Register ioc protocol
     */
    ClMgtIoc* mgtIocInstance = ClMgtIoc::getInstance();
    mgtIocInstance->initializeIoc(CL_IOC_USER_PROTO_START + 2,
            CL_IOC_USER_PROTO_START + 1);
    mgtIocInstance->registerMsgHandler(CL_MGT_MSG_EDIT, clMgtMsgEditHandle);
    mgtIocInstance->registerMsgHandler(CL_MGT_MSG_GET, clMgtMsgGetHandle);
    mgtIocInstance->registerMsgHandler(CL_MGT_MSG_RPC, clMgtMsgRpcHandle);

    mgtIocInstance->registerMsgHandler(CL_MGT_MSG_OID_SET,
            clMgtMsgOidSetHandle);
    mgtIocInstance->registerMsgHandler(CL_MGT_MSG_OID_GET,
            clMgtMsgOidGetHandle);
#endif    
}

ClRcT ClMgtRoot::loadMgtModule(ClMgtModule *module, std::string moduleName)
{
    if (module == NULL)
    {
        return CL_ERR_NULL_POINTER;
    }

    /* Check if MGT module already exists in the database */
    if (mMgtModules.find(moduleName) != mMgtModules.end())
    {
        clLogDebug("MGT", "LOAD", "Module [%s] already exists!",
                moduleName.c_str());
        return CL_ERR_ALREADY_EXIST;
    }

    /* Insert MGT module into the database */
    mMgtModules.insert(pair<string, ClMgtModule *>(moduleName.c_str(), module));
    clLogDebug("MGT", "LOAD", "Module [%s] added successfully!",
            moduleName.c_str());

    return CL_OK;
}

ClRcT ClMgtRoot::unloadMgtModule(std::string moduleName)
{
    /* Check if MGT module already exists in the database */
    if (mMgtModules.find(moduleName) == mMgtModules.end())
    {
        clLogDebug("MGT", "LOAD", "Module [%s] is not existing!",
                moduleName.c_str());
        return CL_ERR_NOT_EXIST;
    }

    /* Remove MGT module out off the database */
    mMgtModules.erase(moduleName);
    clLogDebug("MGT", "LOAD", "Module [%s] removed successful!",
            moduleName.c_str());

    return CL_OK;
}

ClMgtModule *ClMgtRoot::getMgtModule(const std::string moduleName)
{
    map<string, ClMgtModule*>::iterator mapIndex = mMgtModules.find(moduleName);
    if (mapIndex != mMgtModules.end())
    {
        return static_cast<ClMgtModule *>((*mapIndex).second);
    }
    return NULL;
}

ClRcT ClMgtRoot::bindMgtObject(ClUint8T bindType, ClMgtObject *object,
        const std::string module, const std::string route)
{
    ClRcT rc = CL_OK;

    if (object == NULL)
    {
        return CL_ERR_NULL_POINTER;
    }

    /* Check if MGT module already exists in the database */
    ClMgtModule *mgtModule = getMgtModule(module);
    if (mgtModule == NULL)
    {
        clLogDebug("MGT", "BIND", "Module [%s] does not exist!",
                module.c_str());
        return CL_ERR_NOT_EXIST;
    }

    rc = mgtModule->addMgtObject(object, route);
    if ((rc != CL_OK) && (rc != CL_ERR_ALREADY_EXIST))
    {
        clLogDebug("MGT", "BIND",
                "Binding module [%s], route [%s], returning rc[0x%x].",
                module.c_str(), route.c_str(), rc);
        return rc;
    }
#ifdef MGT_ACCESS
    ClMgtIoc* mgtIocInstance = ClMgtIoc::getInstance();

    /* Send bind data to the server */
    ClIocAddressT allNodeReps;
    allNodeReps.iocPhyAddress.nodeAddress = CL_IOC_BROADCAST_ADDRESS;
    if (bindType == CL_NETCONF_BIND_TYPE)
    {
        ClMgtMessageBindTypeT bindData =
        {
        { 0 } };
        strncat(bindData.module, module.c_str(), CL_MAX_NAME_LENGTH - 1);
        strncat(bindData.route, route.c_str(), MGT_MAX_ATTR_STR_LEN - 1);
        allNodeReps.iocPhyAddress.portId = CL_IOC_MGT_NETCONF_PORT;
        rc = mgtIocInstance->sendIocMsgAsync(&allNodeReps, CL_MGT_MSG_BIND,
                &bindData, sizeof(bindData), NULL, NULL);
    }
    else
    {
        ClMgtMsgOidBindTypeT bindData =
        {
        { 0 } };
        strncat(bindData.module, module.c_str(), CL_MAX_NAME_LENGTH - 1);
        strncat(bindData.oid, route.c_str(), CL_MAX_NAME_LENGTH - 1);
        allNodeReps.iocPhyAddress.portId = CL_IOC_MGT_SNMP_PORT;
        rc = mgtIocInstance->sendIocMsgAsync(&allNodeReps, CL_MGT_MSG_OID_BIND,
                &bindData, sizeof(bindData), NULL, NULL);
    }
#endif    
    return rc;
}

ClRcT ClMgtRoot::registerRpc(const std::string module,
        const std::string rpcName)
{
    ClRcT rc = CL_OK;
#ifdef MGT_ACCESS

    ClMgtIoc* mgtIocInstance = ClMgtIoc::getInstance();
    ClMgtMessageBindTypeT bindData =
    {
    { 0 } };
    strncat(bindData.module, module.c_str(), CL_MAX_NAME_LENGTH - 1);
    strncat(bindData.route, rpcName.c_str(), MGT_MAX_ATTR_STR_LEN - 1);

    /* Send bind data to the server */
    ClIocAddressT allNodeReps;
    allNodeReps.iocPhyAddress.nodeAddress = CL_IOC_BROADCAST_ADDRESS;
    allNodeReps.iocPhyAddress.portId = CL_IOC_MGT_NETCONF_PORT;
    rc = mgtIocInstance->sendIocMsgAsync(&allNodeReps, CL_MGT_MSG_BIND_RPC,
            &bindData, sizeof(bindData), NULL, NULL);
#endif
    return rc;
}

#ifdef MGT_ACCESS
void clMgtMsgEditHandle(ClIocPhysicalAddressT srcAddr, void *pInMsg,
        ClUint64T inMsgSize, void **ppOutMsg, ClUint64T *outMsgSize)
{
    ClRcT rc = CL_OK;

    clLogDebug("NETCONF", "COMP", "Receive Edit MSG");

    ClMgtMessageEditTypeT *editData = (ClMgtMessageEditTypeT *) pInMsg;
    ClCharT *data = editData->data;
    ClUint32T dataSize = strlen(data);

    ClMgtModule * module = ClMgtRoot::getInstance()->getMgtModule(
            editData->module);
    if (!module)
        return;

    ClMgtObject * object = module->getMgtObject(editData->route);
    if (!object)
        return;

    ClTransaction t;

    if (object->validate(data, dataSize, t))
    {
        object->set(t);
    }
    else
    {
        object->abort(t);
        rc = CL_ERR_INVALID_PARAMETER;
    }
    t.clean();

    *outMsgSize = sizeof(ClRcT);
    *ppOutMsg = malloc(*outMsgSize);
    memcpy(*ppOutMsg, &rc, *outMsgSize);
}

void clMgtMsgGetHandle(ClIocPhysicalAddressT srcAddr, void *pInMsg,
        ClUint64T inMsgSize, void **ppOutMsg, ClUint64T *outMsgSize)
{
    ClMgtMessageBindTypeT *bindData = (ClMgtMessageBindTypeT *) pInMsg;
    clLogDebug("NETCONF", "COMP",
            "Received Get request from [%d.%x] for module [%s] route [%s]",
            bindData->iocAddress.nodeAddress, bindData->iocAddress.portId,
            bindData->module, bindData->route);

    ClMgtModule * module = ClMgtRoot::getInstance()->getMgtModule(
            bindData->module);
    if (!module)
    {
        clLogError(
                "NETCONF",
                "COMP",
                "Received Get request from [%d.%x] for Non-existent module [%s] route [%s]",
                bindData->iocAddress.nodeAddress, bindData->iocAddress.portId,
                bindData->module, bindData->route);
        return;
    }

    ClMgtObject * object = module->getMgtObject(bindData->route);
    if (!object)
    {
        clLogError(
                "NETCONF",
                "COMP",
                "Received Get request from [%d.%x] for Non-existent route [%s] module [%s]",
                bindData->iocAddress.nodeAddress, bindData->iocAddress.portId,
                bindData->route, bindData->module);
        return;
    }

    object->get(ppOutMsg, outMsgSize);
}

void clMgtMsgRpcHandle(ClIocPhysicalAddressT srcAddr, void *pInMsg,
        ClUint64T inMsgSize, void **ppOutMsg, ClUint64T *outMsgSize)
{
    clLogDebug("NETCONF", "COMP", "Receive Rpc MSG");
    ClMgtMessageRpcTypeT *rpcData = (ClMgtMessageRpcTypeT *) pInMsg;
    ClCharT *data = rpcData->data;
    ClUint32T dataSize = strlen(data);

    if (*ppOutMsg == NULL)
    {
        *ppOutMsg = (void *) calloc(MGT_MAX_DATA_LEN, sizeof(char));
    }

    ClMgtModule * module = ClMgtRoot::getInstance()->getMgtModule(
            rpcData->module);
    if (!module)
    {
        snprintf((char*) *ppOutMsg, MGT_MAX_DATA_LEN,
                "<error>Invalid parameter</error>");
        *outMsgSize = strlen((char*) *ppOutMsg) + 1;
        return;
    }

    ClMgtRpc * rpc = module->getMgtRpc(rpcData->rpc);
    if (!rpc)
    {
        snprintf((char*) *ppOutMsg, MGT_MAX_DATA_LEN,
                "<error>Invalid parameter</error>");
        goto out;
    }

    if (rpc->setInParams(data, dataSize) == CL_FALSE)
    {
        snprintf((char*) *ppOutMsg, MGT_MAX_DATA_LEN,
                "<error>Invalid parameter</error>");
        goto out;
    }

    switch (rpcData->rpcType)
    {
    case CL_MGT_RPC_VALIDATE:
        if (rpc->validate())
        {
            strcpy((char*) *ppOutMsg, "<ok/>");
        }
        else
        {
            clLogDebug("MGT", "INI", "Validation error : %s",
                    rpc->ErrorMsg.c_str());
            if (rpc->ErrorMsg.length() == 0)
                snprintf((char*) *ppOutMsg, MGT_MAX_DATA_LEN,
                        "<error>Unknown error</error>");
            else
                snprintf((char*) *ppOutMsg, MGT_MAX_DATA_LEN,
                        "<error>%s</error>", rpc->ErrorMsg.c_str());
        }
        break;
    case CL_MGT_RPC_INVOKE:
        if (rpc->invoke())
        {
            rpc->getOutParams(ppOutMsg, outMsgSize);
        }
        else
        {
            if (rpc->ErrorMsg.length() == 0)
                snprintf((char*) *ppOutMsg, MGT_MAX_DATA_LEN,
                        "<error>Unknown error</error>");
            else
                snprintf((char*) *ppOutMsg, MGT_MAX_DATA_LEN,
                        "<error>%s</error>", rpc->ErrorMsg.c_str());
        }
        break;
    case CL_MGT_RPC_POSTREPLY:
        if (rpc->postReply())
        {
            strcpy((char*) *ppOutMsg, "<ok/>");
        }
        else
        {
            if (rpc->ErrorMsg.length() == 0)
                snprintf((char*) *ppOutMsg, MGT_MAX_DATA_LEN,
                        "<error>Unknown error</error>");
            else
                snprintf((char*) *ppOutMsg, MGT_MAX_DATA_LEN,
                        "<error>%s</error>", rpc->ErrorMsg.c_str());
        }
        break;
    default:
        snprintf((char*) *ppOutMsg, MGT_MAX_DATA_LEN,
                "<error>Invalid operation</error>");
        break;
    }

    out: *outMsgSize = strlen((char*) *ppOutMsg) + 1;
}

void clMgtMsgOidSetHandle(ClIocPhysicalAddressT srcAddr, void *pInMsg,
        ClUint64T inMsgSize, void **ppOutMsg, ClUint64T *outMsgSize)
{
    ClRcT rc = CL_OK;

    ClMgtMsgOidSetType *editData = (ClMgtMsgOidSetType *) pInMsg;

    clLogDebug("SNM", "COMP", "Receive 'edit' opcode, data [%s]",
            (ClCharT *)editData->data);

    ClMgtModule * module = ClMgtRoot::getInstance()->getMgtModule(
            editData->module);

    if (!module)
        return;

    ClMgtObject * object = module->getMgtObject(editData->oid);
    if (!object)
        return;

    ClTransaction t;

    string strInMsg((ClCharT *)editData->data);

    if (strInMsg.compare(1, object->Name.length(), object->Name))
    {
        strInMsg = "<" + object->Name + ">" + strInMsg + "</" + object->Name
                + ">";
    }

    if (object->validate((void *) strInMsg.c_str(), strInMsg.length(), t))
    {
        object->set(t);
    }
    else
    {
        object->abort(t);
        rc = CL_ERR_INVALID_PARAMETER;
    }
    t.clean();

    *outMsgSize = sizeof(ClRcT);
    *ppOutMsg = malloc(*outMsgSize);
    memcpy(*ppOutMsg, &rc, *outMsgSize);
}

void clMgtMsgOidGetHandle(ClIocPhysicalAddressT srcAddr, void *pInMsg,
        ClUint64T inMsgSize, void **ppOutMsg, ClUint64T *outMsgSize)
{
    clLogDebug("SNM", "COMP", "Receive 'get' opcode");

    ClMgtMsgOidBindTypeT *bindData = (ClMgtMsgOidBindTypeT *) pInMsg;

    ClMgtModule * module = ClMgtRoot::getInstance()->getMgtModule(
            bindData->module);
    if (!module)
        return;

    ClMgtObject * object = module->getMgtObject(bindData->oid);
    if (!object)
        return;

    object->get(ppOutMsg, outMsgSize);
}
#endif
