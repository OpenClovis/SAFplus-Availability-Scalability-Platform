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
 * ModuleName  : dbal
 * File        : clDbalParse.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
*   This file contains Dbal configuration parsing routines
*****************************************************************************/
#include <stdio.h>
#include <string.h>
#include <clParserApi.h>
#include <clCommon.h>
#include <clCommonErrors.h>
#include <clLogApi.hxx>

#ifdef __cplusplus
extern "C" {
#endif

ClRcT dbalGetLibName(ClCharT  *pLibName)
{
    ClParserPtrT   parent     = NULL ;   /*parent*/
    ClParserPtrT   engineType = NULL ;   /*parser ptr to Component*/
    ClParserPtrT   libName    = NULL ;   /*parser ptr to ckptName*/
    ClParserPtrT   fileName    = NULL ;   /*parser ptr to ckptName*/
    ClRcT          rc         = CL_OK;
    ClCharT       *configPath = NULL;

    configPath = getenv("ASP_CONFIG");
    if (configPath != NULL)
    {
        parent    = clParserOpenFile(configPath, "clDbalConfig.xml");
        if (parent == NULL)
        {
          logError(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"Xml file for config is not proper");
          return rc;
        }
    }
    else
    {
        logWarning(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"ASP_CONFIG path is not set in the environment");
    }
    if(parent != NULL)
    {
       engineType = clParserChild(parent,"Dbal");
       libName    = clParserChild(engineType,"Engine");
       fileName   = clParserChild(libName,"FileName");
    }
    if(fileName != NULL && pLibName != NULL)
      strcpy(pLibName, fileName->txt);

    if (parent != NULL) clParserFree(parent);

    //Default plugin
    if (fileName == NULL && pLibName != NULL )
      {
        strncat(pLibName, "libclSQLiteDB.so", strlen("libclSQLiteDB.so"));
      }
    return rc;
}

#ifdef __cplusplus
 }
#endif
