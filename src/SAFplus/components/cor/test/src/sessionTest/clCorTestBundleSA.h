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
 * $File: //depot/dev/main/Andromeda/ASP/components/cor/test/sessionTest/clCorTestBundleSA.h $
 * $Author: bkpavan $
 * $Date: 2006/12/26 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * Cor Test Client - Unit Test 
 *
 *****************************************************************************/



#ifndef _CL_COR_TEST_SESSION_H_
#define _CL_COR_TEST_SESSION_H_

#ifdef __cplusplus
extern "C"
{
#endif


/* MACROS */
                                                                                                                            
#define   PASSTEST        1
#define   FAILTEST        0
#define   NOT_OK         11

#define CL_COR_TEST_PUT_IN_FILE(STRING) \
do \
{\
    FILE *fp; \
    fp = fopen("corTestClient.txt", "a");\
    fprintf(fp,"\nTest: %s - %s \n",__FUNCTION__,STRING);\
    fclose(fp); \
}\
while(0)

#define CL_COR_TEST_RETURN_ERROR(ERROR,STRING, RC)\
do \
{\
    {\
        FILE *fp = NULL; \
        fp = fopen("corTestClient.txt", "a");\
        fprintf(fp,"\ncorTestClient - %s . rc[0x%x], Result [%s]\n", STRING, RC, (RC)?"FAIL":"PASS");\
        CL_COR_TEST_PRINT_ERROR(STRING, RC);\
        fclose(fp); \
    }\
    if(ERROR <= CL_DEBUG_ERROR && (RC) != CL_OK)\
    {\
        CL_DEBUG_PRINT(ERROR,("\nCorTestClient - Error : %s, rc[0x%x], Result [%s] \n",(STRING), (RC), (RC)?"FAIL":"PASS"));\
    }\
    CL_FUNC_EXIT();\
    if((RC) != CL_OK) \
        return (RC); \
}\
while(0)

#define CL_COR_TEST_PRINT_ERROR(STRING, RC) \
do \
{ \
    if ((RC) != CL_OK) \
    { \
        clOsalPrintf("\n Error. %s. rc[0x%x] , Result [%s]\n", STRING, (RC), (RC)?"FAIL":"PASS"); \
        return (RC); \
    } \
}\
while(0)

/*STRUCTURE DEFINITION */
struct ClCorTestBundleData
{
    ClCorTypeT      attrType;
    ClCorAttrTypeT  arrType;
    ClPtrT          data;
    ClUint32T       size;
    ClCorJobStatusT *jobStatus;
    ClCorAttrIdT    attrId;
    ClCorMOIdT      moId;
    ClCorAttrPathT  attrPath;
};

typedef struct ClCorTestBundleData ClCorTestBundleDataT;
typedef ClCorTestBundleDataT * ClCorTestBundleDataPtrT;


struct ClCorTestBundleCookie
{
    ClCorTestBundleDataPtrT cookie;
    ClUint32T              noOfElem;
};

typedef struct ClCorTestBundleCookie ClCorTestBundleCookieT;
typedef ClCorTestBundleCookieT * ClCorTestBundleCookiePtrT;

#define CL_COR_TEST_SESSION_EO_THREAD_PRIORITY      1
#define CL_COR_TEST_SESSION_EO_THREADS              8
#define CL_COR_TEST_SESSION_IOC_PORT                0x25

/* Class Ids and attribute Ids for testing runtime and operational attribute funcitonality  */

#define TEST_CLASS_R                                0x2000

#define TEST_CLASS_S                                0x3000
#define TEST_CLASS_S_ATTR_1                         0x1
#define TEST_CLASS_S_ATTR_2                         0x2

#define TEST_CLASS_S_MSO                            0x3001
#define TEST_CLASS_S_MSO_ATTR_1                     0x9
#define TEST_CLASS_S_MSO_ATTR_2                     0xa
#define TEST_CLASS_S_MSO_ATTR_3                     0xb
#define TEST_CLASS_S_MSO_ATTR_4                     0xc
#define TEST_CLASS_S_MSO_ATTR_5                     0xd
#define TEST_CLASS_S_MSO_ATTR_6                     0xe
#define TEST_CLASS_S_MSO_ATTR_7                     0xf
#define TEST_CLASS_S_MSO_ATTR_8                     0x10
#define TEST_CLASS_S_MSO_ATTR_9                     0x11
#define TEST_CLASS_S_MSO_ATTR_10                    0x12
#define TEST_CLASS_S_MSO_ATTR_11                    0x13
#define TEST_CLASS_S_MSO_ATTR_12                    0x14
#define TEST_CLASS_S_MSO_ATTR_13                    0x15

#define TEST_CLASS_T                                0x5000

#define TEST_CLASS_T_MSO                            0x5001
#define TEST_CLASS_T_MSO_ATTR_1                     0xb
#define TEST_CLASS_T_MSO_ATTR_2                     0xc
#define TEST_CLASS_T_MSO_ATTR_3                     0xf
#define TEST_CLASS_T_MSO_ATTR_4                     0x10
#define TEST_CLASS_T_MSO_ATTR_5                     0x11
#define TEST_CLASS_T_MSO_ATTR_6                     0x12
#define TEST_CLASS_T_MSO_ATTR_7                     0x13
#define TEST_CLASS_T_MSO_ATTR_8                     0x14
#define TEST_CLASS_T_MSO_ATTR_9                     0x15
#define TEST_CLASS_T_MSO_ATTR_10                    0xd
#define TEST_CLASS_T_MSO_ATTR_11                    0xe
#define TEST_CLASS_T_MSO_ATTR_12                    0x16

#define TEST_CLASS_V                            0x6000

#ifdef __cplusplus
}
#endif

#endif  /* _CL_COR_TEST_SESSION_H_ */
