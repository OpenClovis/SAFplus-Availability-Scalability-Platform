/*
 * Copyright (C) 2002-2009 by OpenClovis Inc. All  Rights Reserved.
 * 
 * The source code for  this program is not published  or otherwise 
 * divested of  its trade secrets, irrespective  of  what  has been 
 * deposited with the U.S. Copyright office
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

/* This file defines all the internal function used. 
   This contains only clMedInstValidate()
 */
#ifndef  _CL_MED_IPI_H_
#define _CL_MED_IPI_H_

#ifdef __cplusplus
extern "C" {
#endif



/* INCLUDES */
#include "clCommon.h"
#include "clCntApi.h"
#include "clEoApi.h"
#include<clCorApi.h>
#include<clCorUtilityApi.h>
#include<clCorNotifyApi.h>
#include<clCorTxnApi.h>
#include<clCorErrors.h>
#include "clMedErrors.h"


/**
 ************************************
 *  \page pagemed113 clMedInstValidate
 *
 *  \par Description:
 *  This validates whether the instance present in pVarInfo is present in Med db, instXlationDb
 *
 *  \par Library File:
 *  libClMedClient.a,libClMedClient.so
 *
 *  \par Related Function(s):
 *  none.
 *
 */

 /* This function validates instance information */ 
ClRcT clMedInstValidate( ClMedHdlPtrT      pMm,
                        ClMedVarBindT   *pVarInfo);



#ifdef __cplusplus
}
#endif
#endif /* _CL_MED_API_H_*/

