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
 * File        : clCorTxnJobStream.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * Defines structures used to stream the jobs for a transaction.
 *
 *
 *****************************************************************************/

#ifndef _CL_COR_TXN_STREAM_IPI_H_
#define _CL_COR_TXN_STREAM_IPI_H_

#include <clCommon.h>
#include <clCorMetaData.h>

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 *  Constant and Macro Definitions
 *****************************************************************************/
#define CL_COR_TXN_JOB_STREAM_HEADER_TAG      1
#define CL_COR_TXN_JOB_HEADER_TAG             2
#define CL_COR_TXN_OBJ_JOB_TAG                3
#define CL_COR_TXN_ATTR_SET_JOB_TAG           4
#define CL_COR_TXN_ATTR_SET_JOB_SIZE_TAG      5

#define CL_COR_TXN_INFO_NUM_BUCKETS             1000

#define CL_COR_TXN_JOB_HDR_DELETED               1
#define CL_COR_TXN_JOB_HDR_OH_INVALID            1
/******************************************************************************
 *  Data Types 
 *****************************************************************************/


/* Attribute Set Job.*/
struct ClCorTxnAttrSetJob{
    ClInt32T                     attrType;        /* ClCorAttrTypeT */
    ClInt32T                     arrDataType;    /* ClCorTypeT. type qualifier, if attrType is ARRAY */
    ClCorAttrIdT                 attrId;
    ClInt32T                     index;
    ClUint32T                    size;
    ClUint8T                   *pValue; /* pointer to buffer, which stores value*/
    ClUint32T                   jobStatus; /* Status of the job */
    ClCorJobStatusT             *pJobStatus; /* Pointer to the status of the job */
};
typedef struct ClCorTxnAttrSetJob ClCorTxnAttrSetJobT;

/* This is needed to determine the size(additional )of Set job*/
typedef ClUint8T  ClCorTxnAttrSetJobSizeT;

/* Object Job  */
struct ClCorTxnObjJob {
    ClCorOpsT               op;
    ClCorAttrPathT          attrPath;
    ClUint32T               numJobs;   /* TODO: Add to xml */
    ClUint32T               jobStatus;   /* Status of the job */
};

typedef struct ClCorTxnObjJob    ClCorTxnObjJobT;

/*
   Header for COR's txn job.
   Here key is moId or oh, so one header per Object.
   There can be multiple jobs per object though.
   A Job Header of COR represents one Transaction Job. 
*/
struct ClCorTxnJobHeader {
    ClUint8T                  srcArch;          /** endianness of Txn - source BIG/LITTLE ENDIAN */
    ClUint8T                  regStatus;        /** txn data regularization status. 
                                                    CL_TRUE would mean data is formated in Network Order.  */
    ClUint8T                  reserved[2];
    ClCorMOIdT                moId;
    ClCorObjectHandleT        oh;            /*  object handle */ 
    ClUint16T                 ohSize;
    ClUint32T                 numJobs;
    ClUint8T                  isDeleted;     /* Flag to determine the header which is marked as deleted after regularization.*/
    ClCorTxnJobStatusT        jobStatus;     /* Status of the Job */
    ClCorClassTypeT           omClassId;     /* OM Class of the MSO job. For MO it will be zero. */
};
typedef struct ClCorTxnJobHeader ClCorTxnJobHeaderT;

typedef struct
{
    ClUint16T ohSize;
    ClCorObjectHandleT oh;
}VDECL_VER(ClCorObjectHandleIDLT, 4, 1, 0);

/******************************************************************************
 *  Functions
 *****************************************************************************/
 
#ifdef __cplusplus
}
#endif
#endif
