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
 * ModuleName  : cor                                                           
 * File        : clCorNiLocal.h
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 *
 * Name interface related declarations.
 *
 *
 *****************************************************************************/

#ifndef _COR_NI_LOCAL_H_
#define _COR_NI_LOCAL_H_

#include <clCommon.h>
#include <clCorMetaData.h>
#include <clCntApi.h>


#ifdef __cplusplus
	extern "C" {
#endif

#define COR_NI_PKT_SIGNATURE  0x12345

typedef struct corNIInfo
{
    ClCntHandleT    niList;       /* NI List*/
    ClUint32T       niAttrCount;  /* Attr count */
}corNIInfo_t;

typedef struct corClassName
{
    char                     name[CL_COR_MAX_NAME_SZ];   /* Class Name */
    ClCntHandleT     attrList;                /* List of Attributes */
}corClassName_t;

/* This structure is used to pack and unpack the tables */
typedef struct 
{
    ClUint32T    signature;   /* Safety from wrong packets */
    ClUint32T    len;         /* Safety from wrong packets */
    ClUint32T    clsCount;    /* Number of Classes */
    ClUint8T     padding[4];    /* Number of Classes */
    char         data[8];       /* Actual Pkt   */
}corNiTblPkt_t;

/* Packet contains entries of the following type */
typedef struct corNiClsEntry
{
	ClCorClassTypeT	classId;               /* Class Id*/
	char            name[CL_COR_MAX_NAME_SZ]; /* Class Name */
} corNiClsEntry_t;

/* Packet contains entries of the following type */
typedef struct corNiPktCls
{
	ClCorClassTypeT	classId;               /* Class Id*/
    ClUint32T       attrCount;
	char            name[CL_COR_MAX_NAME_SZ]; /* Class Name */
    char            attrData[8];
} corNiPktCls_t;

/* Packet contains entries of the following type */
typedef struct corNiPktAttr
{
	ClCorAttrIdT     attrId;               /*  Attr Id*/
    ClUint8T        padding[4];
	char            name[CL_COR_MAX_NAME_SZ]; /* Attr Name */
} corNiPktAttr_t;

typedef corNiTblPkt_t* corNiTblPkt_h;

/* PROTOTYPES */
ClRcT corNiInit();
void corNiFinalize(void);
ClRcT corNiEntryAdd(char *name, ClCorClassTypeT key);
ClRcT corNiEntryDelete(ClCorClassTypeT key);
ClRcT corNiNameToKeyGet(char *name, ClCorClassTypeT *key);
ClRcT corNiKeyToNameGet(ClCorClassTypeT key, char *name );
void corNiTableShow(ClBufferHandleT *pMsgHdl);

ClRcT corXlateMOPath(char *path, ClCorMOIdPtrT cAddr);
ClRcT corXlatePath(char *path, ClCorMOClassPathPtrT cAddr);

#ifdef __cplusplus
}
#endif
                                                                                                                   
#endif  /* _COR_NI_LOCAL_H_ */

