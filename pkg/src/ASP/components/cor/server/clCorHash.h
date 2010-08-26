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
 * File        : clCorHash.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * Contains macros for Hash Table usages.
 *
 *
 *****************************************************************************/
#ifndef _COR_HASH_H
#define _COR_HASH_H

#ifdef __cplusplus
	extern "C" {
#endif


/* Hash table macros */
#define NUMBER_OF_BUCKETS 7

/* Hash table prototypes */
extern int corKeyCompare(ClCntKeyHandleT key1, ClCntKeyHandleT key2);
extern unsigned int corHashFunction(ClCntKeyHandleT key);

/* Hash table types and macro functions */
typedef ClCntHandleT CORHashTable_h;
typedef ClCntKeyHandleT   CORHashKey_h;
typedef ClCntDataHandleT  CORHashValue_h;


#define HASH_CREATE(ht)\
 clCntHashtblCreate(NUMBER_OF_BUCKETS,corKeyCompare,corHashFunction,0,0, CL_CNT_UNIQUE_KEY,ht)

#define HASH_GET(ht, k, v)\
do  \
{ \
  ClRcT rc = CL_OK; \
  v=0; \
  if(ht)\
  rc = clCntDataForKeyGet(ht,(ClCntKeyHandleT)(ClWordT) (k), (ClCntDataHandleT*)&(v)); \
  if ((rc != CL_OK) && (CL_GET_ERROR_CODE(rc) != CL_ERR_NOT_EXIST)) \
    clLogError("HSH", "GET", "Failed while getting the data given the key from Hash table. rc[0x%x]", rc); \
} \
while(0)

#define HASH_PUT(ht, k, v)\
 clCntNodeAdd(ht,(ClCntKeyHandleT)(ClWordT) (k),(ClCntDataHandleT)(v), 0)

#define HASH_REMOVE(ht, k) \
do \
{ \
  ClCntNodeHandleT  node; \
  ClRcT   ret; \
  ret  = clCntNodeFind(ht, (ClCntKeyHandleT)(ClWordT) (k), &node); \
  if(ret == CL_OK) clCntNodeDelete(ht, node); \
} \
while(0)

#define HASH_FREE(ht)      clCntDelete(ht)
#define HASH_ITR(ht, fn)   clCntWalk(ht, fn, 0, 0)
#define HASH_ITR_COOKIE(ht, fn, cookie)   clCntWalk(ht, fn, cookie, sizeof(cookie))
#define HASH_ITR_ARG(ht, fn,userData,dataLength)   clCntWalk(ht, fn, (userData), (dataLength))

#define HASH_GET_FIRST_KEY(ht, k, rc)\
do  \
{ \
  ClCntNodeHandleT pNode = 0; \
  rc = clCntFirstNodeGet(ht,&pNode); \
  if (CL_OK == rc)  rc = clCntNodeUserKeyGet(ht, pNode, (ClCntKeyHandleT *)&(k)); \
} \
while(0)

#define HASH_GET_NEXT_KEY(ht, k, rc)\
do  \
{ \
  ClCntNodeHandleT pNode = 0; \
  rc = clCntNodeFind(ht,(ClCntKeyHandleT)(ClWordT) (k), &pNode); \
  if (CL_OK == rc)  rc = clCntNextNodeGet(ht,pNode , &pNode); \
  if (CL_OK == rc)  rc = clCntNodeUserKeyGet(ht, pNode, (ClCntKeyHandleT *)&(k)); \
} \
while(0)

#define HASH_FINALIZE(ht) \
do \
{ \
	 ClCntNodeHandleT  nodeH = 0; \
	ClCntNodeHandleT  nextNodeH = 0; \
   	 clCntFirstNodeGet(ht, &nodeH);\
	while(nodeH)	\
		{ \
		clCntNextNodeGet(ht, nodeH, &nextNodeH); \
		clCntNodeDelete(ht, nodeH); \
		nodeH = nextNodeH; \
		} \
	clCntDelete(ht);\
}while(0)

#ifdef __cplusplus
}
#endif

#endif  /* _CL_COR_META_DATA_H_ */
