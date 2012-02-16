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
 * File        : cpmMetaStruct.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *
 *
 ****************************************************************************/


#include <clCpmApi.h>

#include <clCorApi.h>
#include <clCorServiceId.h>
#include <clCorUtilityApi.h>
#include <clOmCommonClassTypes.h>
#include <clAmsTypes.h>


/* MO definitions for CPM */

/* for definitions of UINT_MAX etc. */
/*#include <limits.h>*/
#define UINT_MAX    1000
#define MAX_NODES 100
#define MAX_SUS_PER_NODE 100
#define MAX_COMPONENTS_PER_SU 100

ClCorAttribPropsT   clCpmSUMoAttrList[] =
{
    /*{"SU_PRE_INST",             SU_PRE_INST,          CL_COR_UINT32,  1,                      0, 1,        0xffff},
    {"SU_NO_OF_COMP",           SU_NO_OF_COMP,          CL_COR_UINT32,  0,                      0, UINT_MAX, 0xffff},*/
    {"CL_CPM_SU_PRESENCE_STATE",        CL_CPM_SU_PRESENCE_STATE,       CL_COR_UINT32,  CL_AMS_PRESENCE_STATE_UNINSTANTIATED,      1,      7,        0xffff},
    {"CL_CPM_SU_OPERATIONAL_STATE",     CL_CPM_SU_OPERATIONAL_STATE,    CL_COR_UINT32,  CL_AMS_OPER_STATE_DISABLED,            1,      2,        0xffff},
    {"UNKNOWN_ATTRIBUTE",               0,                              0,              0,                          0,      0,        0}
};

ClCorArrAttribDefT   clCpmSUMoArrAttrList[] =
{
    {"CL_CPM_SU_NAME",          CL_CPM_SU_NAME,     CL_COR_INT8,    CL_MAX_NAME_LENGTH, 0},
    {"UNKNOWN_ATTRIBUTE",       0,                  0,              0,                  0}
};

ClCorArrAttribDefT   clCpmNodeMoArrAttrList[] =
{
    {"CL_CPM_NODE_NAME",           CL_CPM_NODE_NAME,   CL_COR_INT8,    CL_MAX_NAME_LENGTH, 0},
    {"CL_CPM_UNKNOWN_ATTRIBUTE",   0,                  0,              0,                  0}
};

ClCorAttribPropsT   clCpmCompMoAttrList[] =
{
/*    {"COMP_CAPABILITY",         COMP_CAPABILITY,        CL_COR_UINT32,  1,                  1, 7,        0xffff},*/
/*    {"COMP_PROPERTY",           COMP_PROPERTY,          CL_COR_UINT32,  1,                  1, 4,        0xffff},*/
/*    {"COMP_PROCESS_REL",        COMP_PROCESS_REL,       CL_COR_UINT32,  0,                  0, 3,        0xffff},*/
/*    {"COMP_TIME_OUT",           COMP_TIME_OUT,          CL_COR_UINT32,  0,                  0, UINT_MAX, 0xffff},*/
    {"CL_CPM_COMP_PRESENCE_STATE",      CL_CPM_COMP_PRESENCE_STATE,     CL_COR_UINT32,  CL_AMS_PRESENCE_STATE_UNINSTANTIATED,      1, 7,   0xffff},
    {"CL_CPM_COMP_OPERATIONAL_STATE",   CL_CPM_COMP_OPERATIONAL_STATE,  CL_COR_UINT32,  CL_AMS_OPER_STATE_DISABLED,            1, 2,   0xffff},
    {"UNKNOWN_ATTRIBUTE",               0,                              0,              0,                          0, 0,   0}
};

ClCorArrAttribDefT   clCpmCompMoArrAttrList[] =
{
/*    {"COMP_CSI_TYPE",      COMP_CSI_TYPE,      CL_COR_INT8,    CL_MAX_NAME_LENGTH, 0},
    {"COMP_INST_CMD",      COMP_INST_CMD,      CL_COR_INT8,    CL_MAX_NAME_LENGTH, 0},
    {"COMP_TERM_CMD",      COMP_TERM_CMD,      CL_COR_INT8,    CL_MAX_NAME_LENGTH, 0},
    {"COMP_CLEANUP_CMD",   COMP_CLEANUP_CMD,   CL_COR_INT8,    CL_MAX_NAME_LENGTH, 0},*/
    {"CL_CPM_COMP_NAME",    CL_CPM_COMP_NAME,   CL_COR_INT8,    CL_MAX_NAME_LENGTH, 0},
    {"UNKNOWN_ATTRIBUTE",   0,                  0,              0,                  0}
};

/*
ClCorArrAttribDefT compArgvContArrAttrList[] =
{
    { "COMP_ARGV",          COMP_ARGV,  CL_COR_INT8,    CL_MAX_NAME_LENGTH, 0},
    { "UNKNOWN_ATTRIBUTE",  0,          0,              0,                  0}
};
ClCorMoClassDefT compArgvContClass[] =
{
    {"COMP_ARGV_CONT_CLASS", COMP_ARGV_CONT_CLASS,  CPM_CLASS_UNKNOWN, 1, NULL, compArgvContArrAttrList, NULL, NULL},
    {"CPM_CLASS_UNKNOWN",        CPM_CLASS_UNKNOWN,         CPM_CLASS_UNKNOWN, 0, NULL, NULL,                    NULL, NULL}
};        

ClCorArrAttribDefT compEnvContArrAttrList[] =
{
    { "COMP_ENV_NAME",      COMP_ENV_NAME,  CL_COR_INT8,    CL_MAX_NAME_LENGTH, 0},
    { "COMP_ENV_VALUE",     COMP_ENV_VALUE, CL_COR_INT8,    CL_MAX_NAME_LENGTH, 0},
    { "UNKNOWN_ATTRIBUTE",  0,              0,              0,                  0}
};

       
ClCorMoClassDefT compEnvContClass[] =
{
    {"COMP_ENV_CONT_CLASS", COMP_ENV_CONT_CLASS, CPM_CLASS_UNKNOWN, 1, NULL, compEnvContArrAttrList, NULL, NULL},
    {"CPM_CLASS_UNKNOWN",       CPM_CLASS_UNKNOWN,       CPM_CLASS_UNKNOWN, 0, NULL, NULL,                   NULL, NULL}
};        

ClCorContAttribDefT cpmCompMoContAttrList[] = 
{
    { "COMP_ARGV_CONT_ATTR",    COMP_ARGV_CONT_ATTR,    COMP_ARGV_CONT_CLASS,   0, CL_MAX_ARGV, 0},
    { "COMP_ENV_CONT_ATTR",     COMP_ENV_CONT_ATTR,     COMP_ENV_CONT_CLASS,    0, CL_MAX_ENV,  0},
    { "UNKNOWN_ATTRIBUTE",      0,                      0,                      0, 0,           0}
};
*/

ClCorMoClassDefT clCpmComponentMO[] = 
{
    {"CL_CPM_CLASS_COMP",      CL_CPM_CLASS_COMP,     CL_CPM_CLASS_UNKNOWN,  UINT_MAX,   clCpmCompMoAttrList,   clCpmCompMoArrAttrList,   NULL,   NULL},
    {"CL_CPM_CLASS_UNKNOWN",   CL_CPM_CLASS_UNKNOWN,  CL_CPM_CLASS_UNKNOWN,  0,          NULL,                  NULL,                   NULL,   NULL}
};

ClCorMoClassDefT clCpmServiceUnitMO[] = 
{
    {"CL_CPM_CLASS_SU",        CL_CPM_CLASS_SU,       CL_CPM_CLASS_UNKNOWN,  UINT_MAX,     clCpmSUMoAttrList,  clCpmSUMoArrAttrList, NULL, NULL},
    {"CL_CPM_CLASS_UNKNOWN",   CL_CPM_CLASS_UNKNOWN,  CL_CPM_CLASS_UNKNOWN,  0,            NULL,               NULL,               NULL, NULL}
};

ClCorMoClassDefT clCpmNodeMO[] = 
{
    {"CL_CPM_CLASS_NODE",      CL_CPM_CLASS_NODE,         CL_CPM_CLASS_UNKNOWN, UINT_MAX,        NULL,  clCpmNodeMoArrAttrList, NULL, NULL},
    {"CL_CPM_CLASS_UNKNOWN",   CL_CPM_CLASS_UNKNOWN,      CL_CPM_CLASS_UNKNOWN, 0,               NULL,  NULL,                 NULL, NULL}
};


ClCorMoClassDefT clCpmClusterMO[] = 
{
    {"CL_CPM_CLASS_CLUSTER",   CL_CPM_CLASS_CLUSTER,  CL_CPM_CLASS_UNKNOWN,  UINT_MAX,       NULL,   NULL,   NULL,   NULL},
    {"CL_CPM_CLASS_UNKNOWN",   CL_CPM_CLASS_UNKNOWN,  CL_CPM_CLASS_UNKNOWN,  0,              NULL,   NULL,   NULL,   NULL}
};

ClCorClassTblT pCorSAFClassTable[] =
    {
        /* 
            Relative depth in the COR Tree , 
            Pointer  to MO class def, 
            Maximum no of objects to be allowed , 
            Pointer  to MSO class definition, 
            Maximum no of MSO objects to be allowed .        
        */        
        {0, CL_COR_MO_CLASS,        clCpmClusterMO,         2,             NULL,	0},
        {1, CL_COR_MO_CLASS,        clCpmNodeMO,            MAX_NODES,      NULL,   0}, 
        {2, CL_COR_MO_CLASS,        clCpmServiceUnitMO,     MAX_SUS_PER_NODE,      NULL,   0},        
        {3, CL_COR_MO_CLASS,        clCpmComponentMO,       MAX_COMPONENTS_PER_SU,      NULL,   0}, 
        {0, 0,                      NULL,                   0,             NULL,   0}
	};


