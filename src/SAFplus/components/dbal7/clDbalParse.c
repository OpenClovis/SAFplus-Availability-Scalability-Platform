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
#include <clCommon6.h>
#include <clCommonErrors6.h>
#include <clLogApi.hxx>

#define CL_ENGINE_DEFAULT_TYPE "SQLite"
#define CL_ENGINE_DEFAULT_PLUGIN "libclSQLiteDB.so"

#ifdef __cplusplus
extern "C" {
#endif

ClRcT dbalGetLibName(ClCharT  *pLibName)
{
    char*       envPlugin = NULL;

    envPlugin = getenv("SAFPLUS_DB_PLUGIN");
    if (envPlugin)
      {
      strcpy(pLibName, envPlugin);
      }
    else
      {
	logWarning("DBA", "INIT", "SAFPLUS_DB_PLUGIN environment variable not defined.  Will load default database [%s] with plugin [%s]", CL_ENGINE_DEFAULT_TYPE, CL_ENGINE_DEFAULT_PLUGIN);
        strcpy(pLibName, (char *)CL_ENGINE_DEFAULT_PLUGIN);
      }

    return CL_OK;
}

#ifdef __cplusplus
 }
#endif
