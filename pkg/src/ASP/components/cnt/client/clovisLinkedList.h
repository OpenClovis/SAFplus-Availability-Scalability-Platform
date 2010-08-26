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
 * ModuleName  : cnt
 * File        : clovisLinkedList.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This file contains essential definitions for the Linked List Container    
 * library implementation.                                                   
 *                                                                           
 *****************************************************************************/

#ifndef _CLOVIS_LINKED_LIST_H_
# define _CLOVIS_LINKED_LIST_H_

# ifdef __cplusplus
extern "C"
{
# endif

typedef struct LinkedListNode_t {
  ClCntKeyHandleT            userKey;
  ClCntHandleT               containerHandle;
  ClCntHandleT               hListHead;
  struct LinkedListNode_t*   pPreviousNode;
  struct LinkedListNode_t*   pNextNode;
} LinkedListNode_t;
/*******************************************************/
typedef struct LinkedListHead_t {
  CclContainer_t      container;
  LinkedListNode_t*   pFirstNode;
  LinkedListNode_t*   pLastNode;
  ClOsalMutexIdT      listMutex;
} LinkedListHead_t;
/*******************************************************/

# ifdef __cplusplus
}
# endif

#endif                          /* _CLOVIS_LINKED_LIST_H_ */
