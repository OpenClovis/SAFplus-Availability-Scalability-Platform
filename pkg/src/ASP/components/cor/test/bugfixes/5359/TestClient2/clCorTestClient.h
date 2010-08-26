/*
 * Copyright (C) 2002-2009 by OpenClovis Inc. All  Rights Reserved.
 * 
 * The source code for  this program is not published  or otherwise 
 * divested of  its trade secrets, irrespective  of  what  has been 
 * deposited with the U.S. Copyright office
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

/*******************************************************************************
 * ModuleName  : cor
 * $File: //depot/dev/RC2/ASP/components/cor/test/clCorTestClient.h $
 * $Author: bkpavan $
 * $Date: 2006/06/07 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * Cor Test Client - Unit Test 
 *
 *****************************************************************************/


#if 0
#ifndef _CL_COR_TEST_CLIENT_H_
#define _CL_COR_TEST_CLIENT_H_

#ifdef __cplusplus
extern "C"
{
#endif

/* Global definitions */
ClEventInitHandleT corTestEventHandle;
ClEventChannelHandleT corTestEventChannelHandle;
ClCpmHandleT cpmHandle;

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


#define COR_EVT_TIMEOUT                        100
#define CL_COR_TEST_CLIENT_EO_THREAD_PRIORITY  1
#define CL_COR_TEST_CLIENT_EO_THREADS          4
#define CL_COR_TEST_CLIENT_IOC_PORT            0x8400
#define CL_COR_TEST_IOC_PORT_1                 0x8256
#define CL_COR_TEST_IOC_ADDRESS_1              2
#define CL_COR_TEST_IOC_ADDRESS_2              3



/* Class Id and AttrId #defines */

#define TEST_CLASS_A                            0x100
#define TEST_CLASS_A_ATTR_1                     0x101
#define TEST_CLASS_A_ATTR_2                     0x102
#define TEST_CLASS_A_ATTR_3                     0x103
#define TEST_CLASS_A_ATTR_4                     0x104
#define TEST_CLASS_A_ATTR_5                     0x105
#define TEST_CLASS_A_ATTR_6                     0x106
#define TEST_CLASS_A_ATTR_7                     0x107
#define TEST_CLASS_A_ATTR_8                     0x108
#define TEST_CLASS_A_ATTR_9                     0x109

#define TEST_CLASS_B                            0x200 
#define TEST_CLASS_B_ATTR_1                     0x201
#define TEST_CLASS_B_ATTR_2                     0x202
#define TEST_CLASS_B_ATTR_3                     0x203

#define TEST_CLASS_C                            0x300
#define TEST_CLASS_C_ATTR_1                     0x301
#define TEST_CLASS_C_ATTR_2                     0x302
#define TEST_CLASS_C_ATTR_3                     0x303


#define TEST_CLASS_D                            0x400
#define TEST_CLASS_D_ATTR_1                     0x401

#define TEST_CLASS_E                            0x500
#define TEST_CLASS_E_ATTR_1                     0x501


#define TEST_CLASS_F                            0x8957

#define TEST_CLASS_G                            0x9999
#define TEST_CLASS_G_ATTR_1                     0x1
#define TEST_CLASS_G_ATTR_2                     0x2
#define TEST_CLASS_G_ATTR_3                     0x3

#define TEST_CLASS_H                            0x1007
#define TEST_CLASS_H_ATTR_1                     0x10071

#define TEST_CLASS_I                            0x1006
#define TEST_CLASS_I_ATTR_1                     0x10061

#define TEST_CLASS_J                            0x1005
#define TEST_CLASS_J_ATTR_1                     0x10051
#define TEST_CLASS_J_ATTR_2                     0x10056
#define TEST_CLASS_J_ATTR_3                     0x10057

#define TEST_CLASS_K                            0x1004
#define TEST_CLASS_K_ATTR_1                     0x10041
#define TEST_CLASS_K_ATTR_2                     0x10045
#define TEST_CLASS_K_ATTR_3                     0x10046

#define TEST_CLASS_L                            0x1003
#define TEST_CLASS_L_ATTR_1                     0x10031
#define TEST_CLASS_L_ATTR_2                     0x10034
#define TEST_CLASS_L_ATTR_3                     0x10037

#define TEST_CLASS_M                            0x1002
#define TEST_CLASS_M_ATTR_1                     0x10021
#define TEST_CLASS_M_ATTR_2                     0x10023
#define TEST_CLASS_M_ATTR_3                     0x10025

#define TEST_CLASS_N                            0x1001
#define TEST_CLASS_N_ATTR_1                     0x10011
#define TEST_CLASS_N_ATTR_2                     0x10012
#define TEST_CLASS_N_ATTR_3                     0x10014

#define TEST_CLASS_O                            0x6700
#define TEST_CLASS_O_ATTR_1                     700
#define TEST_CLASS_O_ATTR_2                     6701

#define TEST_CLASS_P                            0x6701
#define TEST_CLASS_P_ATTR_1                     701
#define TEST_CLASS_P_ATTR_2                     6702

#define TEST_CLASS_Q                            0x6702
#define TEST_CLASS_Q_ATTR_1                     702
#define TEST_CLASS_Q_ATTR_2                     6700

/*Forward declarations for Event related functions*/
void corTestEvtChannelOpenCallBack( ClInvocationT invocation, 
        ClEventChannelHandleT channelHandle, ClRcT error );

void corTestEventDeliverCallBack( ClEventSubscriptionIdT subscriptionId,
        ClEventHandleT eventHandle, ClSizeT eventDataSize );

ClEventSubscriptionIdT generateSubscriptionId();

/* callbacks for Events */
ClEventCallbacksT corTestEvtCallbacks =
{
	corTestEvtChannelOpenCallBack,
	corTestEventDeliverCallBack
};


#ifdef __cplusplus
}
#endif

#endif  /* _CL_COR_TEST_CLIENT_H_ */

#endif

/***************************************************************************************************************/

#ifndef _CL_COR_TEST_CLIENT_H_
#define _CL_COR_TEST_CLIENT_H_

#ifdef __cplusplus
extern "C"
{
#endif

ClCpmHandleT cpmHandle;

#define TEST_CLASS_A                0x100
#define TEST_CLASS_A_ATTR_1         0x101
#define TEST_CLASS_A_ATTR_2         0x102
#define TEST_CLASS_A_ATTR_3         0x103
#define TEST_CLASS_A_ATTR_4         0x104
#define TEST_CLASS_A_ATTR_5         0x105
#define TEST_CLASS_A_ATTR_6         0x106
#define TEST_CLASS_A_ATTR_7         0x107
#define TEST_CLASS_A_ATTR_8         0x108
#define TEST_CLASS_A_ATTR_9         0x109
#define TEST_CLASS_A_ATTR_10        0x110

#define TEST_CLASS_B                0x200
#define TEST_CLASS_B_ATTR_1         0x201
#define TEST_CLASS_B_ATTR_2         0x202
#define TEST_CLASS_B_ATTR_3         0x203
#define TEST_CLASS_B_ATTR_4         0x204
#define TEST_CLASS_B_ATTR_5         0x205

#define TEST_CLASS_C                0x300
#define TEST_CLASS_C_ATTR_1         0x301
#define TEST_CLASS_C_ATTR_2         0x302
#define TEST_CLASS_C_ATTR_3         0x303
#define TEST_CLASS_C_ATTR_4         0x304
#define TEST_CLASS_C_ATTR_5         0x305

#define TEST_CLASS_D                0x400
#define TEST_CLASS_D_ATTR_1         0x401
#define TEST_CLASS_D_ATTR_2         0x402
#define TEST_CLASS_D_ATTR_3         0x403
#define TEST_CLASS_D_ATTR_4         0x404
#define TEST_CLASS_D_ATTR_5         0x405

#define TEST_CLASS_E                0x500
#define TEST_CLASS_E_ATTR_1         0x501
#define TEST_CLASS_E_ATTR_2         0x502
#define TEST_CLASS_E_ATTR_3         0x503
#define TEST_CLASS_E_ATTR_4         0x504
#define TEST_CLASS_E_ATTR_5         0x505

#define TEST_CLASS_F                0x600
#define TEST_CLASS_F_ATTR_1         0x601
#define TEST_CLASS_F_ATTR_2         0x602
#define TEST_CLASS_F_ATTR_3         0x603
#define TEST_CLASS_F_ATTR_4         0x604
#define TEST_CLASS_F_ATTR_5         0x605

#ifdef __cplusplus
}
#endif

#endif
