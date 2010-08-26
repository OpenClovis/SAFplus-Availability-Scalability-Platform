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

/**
 * This header file contains the definitions of the tags used by the
 * XML configuration file parser.
 */

#ifndef _CL_CPM_PARSER_H_
#define _CL_CPM_PARSER_H_

#ifdef __cplusplus
extern "C"
{
#endif


/*
 * Node related tags.
 */
#define CL_CPM_PARSER_TAG_NODE_INSTS                "nodeInstances"
#define CL_CPM_PARSER_TAG_NODE_INST                 "nodeInstance"
#define CL_CPM_PARSER_ATTR_NODE_INST_NAME           "name"
#define CL_CPM_PARSER_ATTR_NODE_INST_TYPE           "type"
#define CL_CPM_PARSER_ATTR_NODE_INST_NODE_MOID      "nodeMoId"
#define CL_CPM_PARSER_ATTR_NODE_INST_CHASSIS_ID     "chassisId"
#define CL_CPM_PARSER_ATTR_NODE_INST_SLOT_ID        "slotId"
#define CL_CPM_PARSER_TAG_NODE_CPM_CONFIGS          "cpmConfigs"
#define CL_CPM_PARSER_TAG_NODE_CPM_CONFIG           "cpmConfig"
#define CL_CPM_PARSER_TAG_NODE_CPM_CFG_BOOT_CFGS    "bootConfigs"
#define CL_CPM_PARSER_TAG_NODE_CPM_CFG_BOOT_CFG     "bootConfig"
#define CL_CPM_PARSER_ATTR_NODE_CPM_CFG_BOOT_PROF   "name"
#define CL_CPM_PARSER_ATTR_MAX_BOOT_LEVEL           "maxBootLevel"
#define CL_CPM_PARSER_ATTR_DEFAULT_BOOT_LEVEL       "defaultBootLevel"
#define CL_CPM_PARSER_TAG_NODE_CPM_CFG_ASP_SUS      "aspServiceUnits"
#define CL_CPM_PARSER_TAG_NODE_CPM_CFG_ASP_SU       "aspServiceUnit"
#define CL_CPM_PARSER_ATTR_NODE_CPM_CFG_ASP_SU_NAME  "name"
#define CL_CPM_PARSER_TAG_NODE_CPM_CFG_BOOT_LEVELS  "bootLevels"
#define CL_CPM_PARSER_TAG_NODE_CPM_CFG_BOOT_LEVEL   "bootLevel"
#define CL_CPM_PARSER_ATTR_NODE_CPM_CFG_LEVEL       "level"
#define CL_CPM_PARSER_TAG_NODE_CPM_CFG_SUS          "serviceUnits"
#define CL_CPM_PARSER_TAG_NODE_CPM_CFG_SU           "serviceUnit"
#define CL_CPM_PARSER_ATTR_NODE_TYPE                "nodeType"
#define CL_CPM_PARSER_ATTR_NODE_ID                  "identifier"
#define CL_CPM_PARSER_ATTR_NODE_CPM_TYPE            "cpmType"
#define CL_CPM_PARSER_ATTR_NODE_CPM_DFLT_TIME_OUT   "defaultTimeOut"
#define CL_CPM_PARSER_ATTR_NODE_CPM_MAX_TIME_OUT    "maxTimeOut"
#define CL_CPM_PARSER_ATTR_NODE_CPM_LOG_FILE_PATH   "logFilePath"
#define CL_CPM_PARSER_TAG_NODE_SU_INSTS             "serviceUnitInstances"
#define CL_CPM_PARSER_TAG_NODE_SU_INST              "serviceUnitInstance"
#define CL_CPM_PARSER_ATTR_NODE_SU_INST_TYPE        "type"
#define CL_CPM_PARSER_ATTR_NODE_SU_INST_NAME        "name"
#define CL_CPM_PARSER_ATTR_NODE_SU_INST_LEVEL       "level"
#define CL_CPM_PARSER_TAG_NODE_COMP_INSTS           "componentInstances"
#define CL_CPM_PARSER_TAG_NODE_COMP_INST            "componentInstance"
#define CL_CPM_PARSER_ATTR_NODE_COMP_INST_TYPE      "type"
#define CL_CPM_PARSER_ATTR_NODE_COMP_INST_NAME      "name"
#define CL_CPM_PARSER_TAG_NODE_TYPES                "nodeTypes"
#define CL_CPM_PARSER_TAG_NODE_TYPE                 "nodeType"
#define CL_CPM_PARSER_ATTR_NODE_TYPE_NAME           "name"
#define CL_CPM_PARSER_TAG_NODE_CLASS_TYPE           "classType"
#define CL_CPM_PARSER_TAG_NODE_CLASS_TYPES          "nodeClassTypes"
#define CL_CPM_PARSER_TAG_NODE_CLASS_TYPE1          "nodeClassType"
#define CL_CPM_PARSER_ATTR_NODE_CLASS_TYPE1_NAME    "name"

/*
 * Service unit related tags.
 */
#define CL_CPM_PARSER_TAG_SU_TYPES                  "suTypes"
#define CL_CPM_PARSER_TAG_SU_TYPE                   "suType"
#define CL_CPM_PARSER_ATTR_SU_TYPE_NAME             "name"
#define CL_CPM_PARSER_TAG_SU_PRE_INST               "isPreinstantiable"
#define CL_CPM_PARSER_TAG_SU_NUM_COMP               "numComponents"

/*
 * Component related tags and attributes.
 */
#define CL_CPM_PARSER_TAG_COMP_TYPES                "compTypes"
#define CL_CPM_PARSER_TAG_COMP_TYPE                 "compType"
#define CL_CPM_PARSER_ATTR_COMP_TYPE_NAME           "name"
#define CL_CPM_PARSER_TAG_COMP_TYPE_PROPERTY        "property"
#define CL_CPM_PARSER_TAG_COMP_TYPE_PROCREL         "processRel"
#define CL_CPM_PARSER_TAG_COMP_TYPE_INST_CMD        "instantiateCommand"
#define CL_CPM_PARSER_TAG_COMP_TYPE_ARGS            "args"
#define CL_CPM_PARSER_TAG_COMP_TYPE_ARG             "argument"
#define CL_CPM_PARSER_ATTR_COMP_TYPE_ARG_VALUE      "value"
#define CL_CPM_PARSER_TAG_COMP_TYPE_ENVS            "envs"
#define CL_CPM_PARSER_TAG_COMP_TYPE_NAME_VALUE      "nameValue"
#define CL_CPM_PARSER_ATTR_COMP_TYPE_NAMVAL_NAME    "name"
#define CL_CPM_PARSER_ATTR_COMP_TYPE_NAMVAL_VAL     "value"
#define CL_CPM_PARSER_TAG_COMP_TYPE_TERM_CMD        "terminateCommand"
#define CL_CPM_PARSER_TAG_COMP_TYPE_CLEANUP_CMD     "cleanupCommand"
#define CL_CPM_PARSER_TAG_COMP_TYPE_TIME_OUTS       "timeouts"
#define CL_CPM_PARSER_TAG_COMP_TYPE_TOUT_INST       "instantiateTimeout"
#define CL_CPM_PARSER_TAG_COMP_TYPE_TOUT_TERM       "terminateTimeout"
#define CL_CPM_PARSER_TAG_COMP_TYPE_TOUT_CLEANUP    "cleanupTimeout"
#define CL_CPM_PARSER_TAG_COMP_TYPE_TOUT_PROXY_INST "proxiedCompInstantiateTimeout"
#define CL_CPM_PARSER_TAG_COMP_TYPE_TOUT_PROXY_CLN  "proxiedCompCleanupTimeout"
#define CL_CPM_PARSER_TAG_COMP_TYPE_HEALTHCHECK     "healthCheck"
#define CL_CPM_PARSER_TAG_COMP_TYPE_HC_PERIOD       "period"
#define CL_CPM_PARSER_TAG_COMP_TYPE_HC_MAX_DURAION  "maxDuration"

/*
 * CM "slot.xml" tags.
 */
#define CL_CPM_PARSER_TAG_SLOTS                 "slots"
#define CL_CPM_PARSER_TAG_SLOT                  "slot"
#define CL_CPM_PARSER_ATTR_SLOT_NUM             "slotNumber"
#define CL_CPM_PARSER_TAG_CLASS_TYPES           "classTypes"
#define CL_CPM_PARSER_TAG_CLASS_TYPE            "classType"
#define CL_CPM_PARSER_ATTR_CLASS_TYPE_NAME      "name"
#define CL_CPM_PARSER_ATTR_CLASS_TYPE_ID        "identifier"

#ifdef __cplusplus
}
#endif

#endif /* _CL_CPM_PARSER_H_ */
