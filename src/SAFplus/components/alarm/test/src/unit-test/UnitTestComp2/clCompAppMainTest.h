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
/******************************************************************************
 *
 * clCompAppMain.h
 *
 ***************************** Legal Notice ***********************************
 *
 * This file is autogenerated by OpenClovis IDE, Copyright (C) 2002-2006 by 
 * OpenClovis. All rights reserved.
 *
 ***************************** Description ************************************
 *
 * This file provides a skeleton for writing a SAF aware component. Application
 * specific code should be added between the ---BEGIN_APPLICATION_CODE--- and
 * ---END_APPLICATION_CODE--- separators.
 *
 * Template Version: 1.0
 *
 ***************************** Editor Commands ********************************
 * For vi/vim
 * :set shiftwidth=4
 * :set softtabstop=4
 * :set expandtab
 *****************************************************************************/

#ifndef CL_COMP_APP_MAIN
#define CL_COMP_APP_MAIN

/******************************************************************************
 * Include files needed to compile this file
 *****************************************************************************/

#include "./clCompCfgTest.h"

#ifndef COMP_NAME
#error "COMP_NAME is not defined. Bad or missing ./clCompCfg.h"
#endif

/******************************************************************************
 * Utility macros
 *****************************************************************************/

#define STRING_HA_STATE(S)                                                  \
(   ((S) == CL_AMS_HA_STATE_ACTIVE)             ? "Active" :                \
    ((S) == CL_AMS_HA_STATE_STANDBY)            ? "Standby" :               \
    ((S) == CL_AMS_HA_STATE_QUIESCED)           ? "Quiesced" :              \
    ((S) == CL_AMS_HA_STATE_QUIESCING)          ? "Quiescing" :             \
    ((S) == CL_AMS_HA_STATE_NONE)               ? "None" :                  \
                                                  "Unknown" )

/******************************************************************************
 * Application Life Cycle Management Functions
 *****************************************************************************/

ClRcT 
clCompAppInitialize(
        ClUint32T           argc,
        ClCharT             *argv[]);

ClRcT
clCompAppFinalize();

ClRcT
clCompAppStateChange(
        ClEoStateT          eoState);

ClRcT
clCompAppHealthCheck(
        ClEoSchedFeedBackT* schFeedback);

ClRcT
clCompAppTerminate(
        ClInvocationT       invocation,
        const SaNameT       *compName);

/******************************************************************************
 * Application Work Assignment Functions
 *****************************************************************************/

ClRcT
clCompAppAMFCSISet(
        ClInvocationT       invocation,
        const SaNameT       *compName,
        ClAmsHAStateT       haState,
        ClAmsCSIDescriptorT csiDescriptor);

ClRcT
clCompAppAMFCSIRemove(
        ClInvocationT       invocation,
        const SaNameT       *compName,
        const SaNameT       *csiName,
        ClAmsCSIFlagsT      csiFlags);

/******************************************************************************
 * Utility functions 
 *****************************************************************************/

ClRcT
clCompAppAMFPrintCSI(
    ClAmsCSIDescriptorT csiDescriptor,
    ClAmsHAStateT haState);

#endif // CL_COMP_APP_MAIN
