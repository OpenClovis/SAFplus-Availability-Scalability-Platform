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

#define CL_ENGINE_DEFAULT_TYPE "SQLite"
#define CL_ENGINE_DEFAULT_PLUGIN "libclSQLiteDB.so"

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
    if(!configPath) configPath = (ClCharT *)".";
    parent = clParserOpenFile(configPath, "clDbalConfig.xml");
    if (parent == NULL)
    {
      logWarning("DBA", "INIT", "Xml file for config is not proper. Would be loading default engine [%s] with plugin [%s]",
          CL_ENGINE_DEFAULT_TYPE, CL_ENGINE_DEFAULT_PLUGIN);
      strncat(pLibName, (char *)CL_ENGINE_DEFAULT_PLUGIN, strlen(CL_ENGINE_DEFAULT_PLUGIN));
      return rc;
    }
    engineType = clParserChild(parent,"Dbal");
    libName    = clParserChild(engineType,"Engine");
    fileName   = clParserChild(libName,"FileName");

    if(fileName != NULL && pLibName != NULL)
      strcpy(pLibName, fileName->txt);

    if (fileName == NULL && pLibName != NULL )
      {
        logWarning("DBA", "INIT", "Xml file for config is not proper. Would be loading default engine [%s] with plugin [%s]",
            CL_ENGINE_DEFAULT_TYPE, CL_ENGINE_DEFAULT_PLUGIN);
        strncat(pLibName, (char *)CL_ENGINE_DEFAULT_PLUGIN, strlen(CL_ENGINE_DEFAULT_PLUGIN));
      }
    if (parent != NULL) clParserFree(parent);
    return rc;
}

#ifdef __cplusplus
 }
#endif
