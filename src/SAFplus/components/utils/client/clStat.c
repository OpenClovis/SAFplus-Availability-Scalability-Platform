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
#include <stdio.h>
#include <string.h>
#include <math.h>

#include <clCommon.h>
#include <clCommonErrors.h>
#include <clHeapApi.h>
#include <clDebugApi.h>
#include <clEoApi.h>
#include <clTestApi.h>

#include "clStatApi.h"

ClRcT clStatFini(ClStatHandleT *pHdl)
{
    ClStatT *pStat = NULL;

    pStat = pHdl;
    if(pStat)
        clHeapFree(pStat);

    return CL_OK;
}

ClRcT clStatInit(
        ClCharT         *pTestName,
        ClCharT         *pUnit,
        ClStatHandleT   **pHandle)
{
    ClRcT   rc = CL_OK;
    ClStatT *pStat = NULL;

    if(!pTestName || !pUnit || !pHandle)
        return CL_ERR_NULL_POINTER;

    pStat = (ClStatT*) clHeapCalloc(1,sizeof(*pStat));
    if(!pStat)
        return CL_ERR_NO_MEMORY;

    strncpy(pStat->testName, pTestName, sizeof(pStat->testName)-1);
    strncpy(pStat->unit, pUnit, sizeof(pStat->unit)-1);
    //strncpy(pStat->format, pFormat, sizeof(pStat->format));

    *pHandle = pStat;
    return rc;

}

ClRcT clStatAdd(
        ClStatHandleT *pHdl,
        double ts)
{
    ClRcT rc = CL_OK;
    ClStatT *pStat = NULL;

    
    pStat = pHdl;
    if(!pStat)
        return CL_ERR_NULL_POINTER;

    if(!pStat->count)
        pStat->min = pStat->max = ts;

    if(pStat->min > ts)
        pStat->min = ts;

    if(pStat->max < ts)
        pStat->max = ts;

    pStat->count++;
    pStat->total += ts;
    pStat->squareT += (ts*ts);

    return rc;
}

ClRcT clStatReport(ClStatHandleT *pHdl)
{
    ClRcT   rc = CL_OK;
    double  mean = 0, sd = 0;
    ClStatT *pStat = NULL;
    

    pStat = pHdl;
    if(!pStat)
        return CL_ERR_NULL_POINTER;

    mean = pStat->total/pStat->count;
    sd = sqrt( pStat->squareT/pStat->count - mean*mean ); /* Standard deviation */

    if(pStat->count == 1)
    {
        clTestPrint(("REPORT:%s: result = [%.5LF] %s",
                    pStat->testName,
                    pStat->total,
                    pStat->unit));
    }
    else if (pStat->count > 1)
    {
        clTestPrint(("REPORT:%s: min,max,mean,sd = [%.5F,%.5F,%.5F,%.5F] %s",
                    pStat->testName,
                    pStat->min,
                    pStat->max,
                    mean,
                    sd,
                    pStat->unit));
    } 
    return rc;
}

        
        
