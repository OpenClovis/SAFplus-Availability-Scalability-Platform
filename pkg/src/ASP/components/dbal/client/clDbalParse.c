/*
 * Copyright (C) 2002-2009 by OpenClovis Inc. All  Rights Reserved.
 * 
 * The source code for  this program is not published  or otherwise 
 * divested of  its trade secrets, irrespective  of  what  has been 
 * deposited with the U.S. Copyright office.
 * 
 * This program is  free software; you can redistribute it and / or
 * modify  it under  the  terms  of  the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 * 
 * This program is distributed in the  hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied  warranty  of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 * 
 * You  should  have  received  a  copy of  the  GNU General Public
 * License along  with  this program. If  not,  write  to  the 
 * Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/*
 * Build: 4.2.0
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
#include <clDebugApi.h>

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
          CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Xml file for config is not proper rc [0x %x]\n",rc));
          return CL_ERR_NULL_POINTER;
        }
    }
    else
    {
        rc = CL_ERR_NULL_POINTER;
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("ASP_CONFIG path is not set in the environment irc[0x %x] \n",rc));
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
    return rc;
}
