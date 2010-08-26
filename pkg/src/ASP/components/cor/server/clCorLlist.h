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
 * ModuleName  : cor
 * File        : clCorLlist.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This file contains macros for linked list usages.
 *
 *
 *****************************************************************************/
#ifndef _COR_LLIST_H
#define _COR_LLIST_H

#ifdef __cplusplus
    extern "C" {
#endif

/* Llist functions */
ClInt32T clCorLlistKeyCompare(ClCntKeyHandleT key1, ClCntKeyHandleT key2);

/* Linked list types and macro functions */
typedef ClCntHandleT CORLlist_h;
typedef ClCntKeyHandleT CORLlistKey_h;
typedef ClCntDataHandleT CORLlistValue_h;

#define COR_LLIST_CREATE(ht)\
    clCntLlistCreate(clCorLlistKeyCompare, 0, 0, CL_CNT_UNIQUE_KEY, ht)

#define COR_LLIST_PUT(ht, k, v)\
 clCntNodeAdd(ht,(ClCntKeyHandleT) (k),(ClCntDataHandleT)(v), 0)

#define COR_LLIST_FIND(ht, k, v) \
    clCntNodeFind(ht, (ClCntKeyHandleT) (k), &(v))

#define COR_LLIST_REMOVE(ht, k) \
do \
{ \
  ClCntNodeHandleT  node; \
  ClRcT   ret; \
  ret  = clCntNodeFind(ht, (ClCntKeyHandleT) (k), &node); \
  if(ret == CL_OK) ret = clCntNodeDelete(ht, node); \
} \
while(0)

#define COR_LLIST_FREE(ht)      clCntDelete(ht)

#define COR_LLIST_GET_FIRST_KEY(ht, k, rc)\
do  \
{ \
  ClCntNodeHandleT pNode = 0; \
  rc = clCntFirstNodeGet(ht,&pNode); \
  if (CL_OK == rc)  rc = clCntNodeUserKeyGet(ht, pNode, (ClCntKeyHandleT *)&(k)); \
} \
while(0)

#define COR_LLIST_GET_NEXT_KEY(ht, k, rc)\
do  \
{ \
  ClCntNodeHandleT pNode = 0; \
  rc = clCntNodeFind(ht,(ClCntKeyHandleT) (k), &pNode); \
  if (CL_OK == rc)  rc = clCntNextNodeGet(ht,pNode , &pNode); \
  if (CL_OK == rc)  rc = clCntNodeUserKeyGet(ht, pNode, (ClCntKeyHandleT *)&(k)); \
} \
while(0)

#define COR_LLIST_FINALIZE(ht, rc) \
do \
{ \
	 ClCntNodeHandleT  nodeH = 0; \
	 ClCntNodeHandleT  nextNodeH = 0; \
   	 rc = clCntFirstNodeGet(ht, &nodeH);\
     if (rc == CL_OK) \
     { \
	    while(nodeH)	\
		{ \
		    clCntNextNodeGet(ht, nodeH, &nextNodeH); \
		    clCntNodeDelete(ht, nodeH); \
		    nodeH = nextNodeH; \
		} \
     }\
	 clCntDelete(ht);\
}while(0)

#define COR_LLIST_REMOVE_ALL(ht, rc) \
do \
{ \
    ClCntNodeHandleT  nodeH = 0; \
    ClCntNodeHandleT  nextNodeH = 0; \
    rc = clCntFirstNodeGet(ht, &nodeH);\
    if (rc == CL_OK) \
    { \
        while(nodeH)    \
        { \
            clCntNextNodeGet(ht, nodeH, &nextNodeH); \
            clCntNodeDelete(ht, nodeH); \
            nodeH = nextNodeH; \
        } \
    } \
}while(0)

#ifdef __cplusplus
}
#endif

#endif  /* _COR_LLIST_H */
