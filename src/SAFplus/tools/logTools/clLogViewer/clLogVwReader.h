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
#ifndef _CL_LOGVWREADER_H
#define _CL_LOGVWREADER_H

#include <clCommon.h>
#include <clCommonErrors.h>

#include "clLogFileOwner.h"
#include "clLogVwType.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef struct 
{
    ClLogVwFlagT endianess;
    ClCharT *logicalFileName;
    ClLogVwMaxFileRotatedT maxFilesRotated;
    ClLogVwMaxRecSizeT maxRecSize;
    ClLogVwRecNumT maxRecNum;
    ClLogVwRecNumT numRecInPhyFile;
    ClLogVwRecNumT nextRecNum;
    ClCharT *phyFileName;
    ClUint16T headerSize;
    ClInt32T mFd;
    ClInt32T pFd;
    FILE *txtFp;
    ClLogVwByteT *buf;
    ClLogVwByteT *recBuf;
    ClLogVwRecNumT curFileNum;
    ClBoolT dispPrevRecs;
    ClUint32T numPrevRecs;
    ClLogFileHeaderT *mappedMtd;
    
}ClLogVwMetaDataInfoT;

#define CL_LOGVW_TXT_FILE_EXT ".txt"

extern ClRcT clLogVwSetPrevRecVal(ClCharT *numRec, ClBoolT setAllFlag);

extern ClRcT clLogVwReadRecords(void);

extern ClRcT clLogVwReadMetaData(ClCharT *logicalFileName);

extern ClRcT clLogVwGetRecSize(ClLogVwMaxRecSizeT *recSize);

extern ClRcT clLogVwGetHeaderSize(ClUint16T *headerSize);

extern ClRcT clLogVwCleanUpMetaData(void);

extern ClRcT clLogVwConvertBinFileToTxt(const char *fileName);

extern ClRcT clCheckCfgFile(ClCharT *cfgFileName);

#ifdef __cplusplus
}
#endif


#endif
