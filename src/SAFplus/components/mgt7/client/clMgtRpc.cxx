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
#include "clMgtRpc.hxx"

#ifdef __cplusplus
extern "C" {
#endif
#include <clCommonErrors.h>
#include <clDebugApi.h>
#ifdef __cplusplus
} /* end extern 'C' */
#endif

#ifdef SAFplus7
#define clLog(...)
#endif

ClMgtRpc::ClMgtRpc(const char* name) : mInParams(""), mOutParams("")
{
    Name.assign(name);
    Module.assign("");
    ErrorMsg.assign("");
    mInParams.Name.assign("input");
    mOutParams.Name.assign("output");
}

ClMgtRpc::~ClMgtRpc()
{}

/**
 * Function to add input parameter
 */
void ClMgtRpc::addInParam(std::string param, ClMgtObject *mgtObject)
{
	mInParams.addChildObject(mgtObject,param);
}

/**
 * Function to add output parameter
 */
void ClMgtRpc::addOutParam(std::string param, ClMgtObject *mgtObject)
{
	mOutParams.addChildObject(mgtObject,param);
}

ClBoolT ClMgtRpc::setInParams(void *pBuffer, ClUint64T buffLen)
{
    SAFplus::Transaction t;
    if(mInParams.set(pBuffer,buffLen, t) == CL_TRUE)
    {
        t.commit();
        return CL_TRUE;
    }
    else
    {
        t.abort();
        return CL_FALSE;
    }
}


void ClMgtRpc::getOutParams(void **ppBuffer, ClUint64T *pBuffLen)
{
	mOutParams.get(ppBuffer,pBuffLen);
}

ClRcT ClMgtRpc::registerRpc()
{
    if (!strcmp(Module.c_str(), ""))
    {
        clLogError("MGT","RPC", "Cannot register RPC [%s]", Name.c_str());
        return CL_ERR_NOT_EXIST;
    }

    return ClMgtRoot::getInstance()->registerRpc(Module, Name);
}

