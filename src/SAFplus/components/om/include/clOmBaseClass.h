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
 * ModuleName  : om
 * File        : clOmBaseClass.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *
 * This module contains the definitions for the Base Class.
 *
 *
 *****************************************************************************/

#ifndef  _CL_OM_BASE_CLASS_H_
#define  _CL_OM_BASE_CLASS_H_ 

#ifdef __cplusplus
extern "C" {
#endif

#include <clCommon.h>
#include <clOmCommonClassTypes.h>
#include <clOmObjectManage.h>

/******************************************************************************
 *  Constant and Macro Definitions
 *****************************************************************************/
/* Definitions that are used to initialize the base class table */
#define CL_OM_BASE_CLASS_NAME	          "baseClass"
#define	CL_OM_BASE_CLASS_VERSION        0x0001000a
#define CL_OM_BASE_CLASS_METHOD_TABLE   (tFunc**)&CL_OM_BASE_CLASSMethodsMapping
#define CL_OM_BASE_CLASS_MAX_INSTANCES  10
#define CL_OM_BASE_CLASS_MAX_SLOTS      1

	/* Object States, may not be needed anymore */
    typedef enum {
        CL_OM_OBJ_STATE_INVALID,       /* Must be the first entry */
        CL_OM_OBJ_STATE_INIT,
        CL_OM_OBJ_STATE_ACTIVE,
        CL_OM_OBJ_STATE_INACTIVE,
        OBJ_STATE_INACTIVE,
        CL_OM_OBJ_STATE_DESTROYED,
        CL_OM_OBJ_STATE_UPGRADE_IN_PROGRESS,
        CL_OM_OBJ_STATE_MAX
    } ClOmObjectStateT;

    /* 
     * Object ID definition, which is a combination of the Domain ID and
     * Object's unique address within the node.
     *
     *          ------------------------------------------------
     *          |    DOMAIN ID    |   OBJECT'S LOCAL ID        |
     *          ------------------------------------------------
     */
    
    /* look like not need.
      @todo: Delete permanently, if not required. */
  
    /*
    typedef struct {
        union {
            struct objectIdFields {
                unsigned long domainId:16;
                unsigned long objLocalId:16;
            } b;
            unsigned long objectId;
        } u;
    } tObjHandle;
    */
    
/******************************************************************************
 *  Data Types 
 *****************************************************************************/
	/* baseClass definition */
    typedef struct CL_OM_BASE_CLASS CL_OM_BASE_CLASS;
    struct CL_OM_BASE_CLASS {
        ClOmClassTypeT __objType;      
	};
   
	
	CL_OM_BEGIN_METHOD(CL_OM_BASE_CLASS)
		ClRcT (*fGetObjectType)(CL_OM_BASE_CLASS* pThis, ClOmClassTypeT* eType);
	CL_OM_END

#ifdef __cplusplus
    }
#endif
#endif                       
