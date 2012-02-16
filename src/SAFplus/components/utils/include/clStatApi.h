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

#ifndef _CL_STAT_H_
#define _CL_STAT_H_

#ifdef _cplusplus
extern "C" {
#endif

#define CL_BIN_FMT_LEN  50
#define CL_BIN_UNIT_LEN 50
#define CL_BIN_NAME_LEN 100
typedef struct _ClStatT
{
    double      min;
    double      max;
    ClUint32T   count;
    long double total;
    long double squareT;                    
    ClCharT     testName[CL_BIN_NAME_LEN];
    ClCharT     unit[CL_BIN_UNIT_LEN];
    ClCharT     format[CL_BIN_FMT_LEN];     /* Not used now */
}ClStatT;

#define ClStatHandleT   ClStatT

/* Documentation to be added after review */
extern ClRcT 
clStatInit(ClCharT       *pTestName, 
           ClCharT       *pUnit, 
           ClStatHandleT **pHandle);

extern ClRcT 
clStatAdd(ClStatHandleT *pHdl, 
          double ts);

extern ClRcT 
clStatReport(ClStatHandleT *pHdl);

extern ClRcT 
clStatFini(ClStatHandleT *pHdl);

#ifdef _cplusplus

}
#endif
#endif /* _CL_STAT_H_ */
