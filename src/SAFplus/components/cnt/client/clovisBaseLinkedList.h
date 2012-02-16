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
 * ModuleName  : cnt
 * File        : clovisBaseLinkedList.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This file contains essential definitions for the Base Linked List Container
 * library implementation.                                                   
 *                                                                           
 *****************************************************************************/

#ifndef _CLOVIS_BASE_LINKED_LIST_H_
# define _CLOVIS_BASE_LINKED_LIST_H_

# ifdef __cplusplus
extern "C"
{
# endif

#include <stdio.h>
#include <stdlib.h>
#include <clCommon.h>
#include <clCntApi.h>
#include <clCommonErrors.h>
#include <clRuleApi.h>
#include "clovisContainer.h"
/*******************************************************/
typedef struct BaseLinkedListNode_t
{
  ClRuleExprT*                   pRbeExpression;
  ClCntDataHandleT               userData;
  ClCntKeyHandleT                userKey;
  ClUint32T                      validNode;
  struct BaseLinkedListNode_t*   pPreviousNode;
  struct BaseLinkedListNode_t*   pNextNode;
  ClCntNodeHandleT               parentNodeHandle;
} BaseLinkedListNode_t;
/*******************************************************/
typedef struct BaseLinkedListHead_t {
  CclContainer_t        container;
  BaseLinkedListNode_t* pFirstNode;
  BaseLinkedListNode_t* pLastNode;
} BaseLinkedListHead_t;
/*******************************************************/

# ifdef __cplusplus
}
# endif

#endif                          /* _CLOVIS_BASE_LINKED_LIST_H_ */
