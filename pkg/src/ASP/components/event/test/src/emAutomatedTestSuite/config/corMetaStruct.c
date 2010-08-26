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
 * ModuleName  : event
 * $File: //depot/dev/main/Andromeda/ASP/components/event/test/unit-test/emAutomatedTestSuite/config/corMetaStruct.c $
 * $Author: bkpavan $
 * $Date: 2006/09/13 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

#include <clCorApi.h>
#include <clCorServiceId.h>
#include <clCorUtilityApi.h>
#include <clOmCommonClassTypes.h>
#include <limits.h>
#include "omClassId.h"
#include "corMetaStruct.h"

#if 0
extern ClCorMsoClassDefT fmMso[];
#endif

extern ClCorAttribPropsT alarmMsoAttrList[];
extern ClCorArrAttribDefT alarmMsoArrAttrList[];
extern ClCorContAttribDefT alarmMsoContAttrList[];
extern ClCorMoClassDefT alarmInfo[];
extern ClCorMoClassDefT clAlarmBaseClass[];
extern ClCorAttribPropsT fmMsoAttrList[];

extern ClCorAttribPropsT cpmCompMoAttrList[];
extern ClCorContAttribDefT cpmCompMoContAttrList[];
extern ClCorArrAttribDefT cpmCompMoArrAttrList[];
extern ClCorAttribPropsT cpmSUMoAttrList[];
extern ClCorArrAttribDefT cpmSUMoArrAttrList[];
extern ClCorMoClassDefT compArgvContClass[];
extern ClCorMoClassDefT compEnvContClass[];


ClCorMOIdT corCfgAppParentMOId = {
    {{CLASS_CHASSIS_MO, CL_COR_INSTANCE_WILD_CARD}},
    CL_COR_INVALID_SVC_ID,
    1,
    CL_COR_MO_PATH_ABSOLUTE
};

ClCorAttribPropsT ChassisAttrList[] = {

    {"UNKNOWN_ATTRIB", 0, 0, 0, 0}
};

ClCorArrAttribDefT ChassisArrAttrList[] = {

    {"UNKNOWN_ATTRIB", 0, 0, 0, 0}
};

ClCorAssocAttribDefT ChassisAssocAttrList[] = {

    {"UNKNOWN_ATTRIB", 0, 0, 0, 0}
};
ClCorContAttribDefT ChassisContAttrList[] = {

    {"UNKNOWN_ATTRIB", 0, 0, 0, 0}
};

ClCorAttribPropsT GigePortAttrList[] = {
    {"GigePort_MTU", GIGEPORT_MTU, CL_COR_UINT32, 1500, 1500, 25000, 0},
    {"GigePort_SPEED", GIGEPORT_SPEED, CL_COR_UINT32, 100, 10, 1000, 0},

    {"UNKNOWN_ATTRIB", 0, 0, 0, 0}
};

ClCorArrAttribDefT GigePortArrAttrList[] = {

    {"UNKNOWN_ATTRIB", 0, 0, 0, 0}
};

ClCorAssocAttribDefT GigePortAssocAttrList[] = {

    {"UNKNOWN_ATTRIB", 0, 0, 0, 0}
};
ClCorContAttribDefT GigePortContAttrList[] = {

    {"UNKNOWN_ATTRIB", 0, 0, 0, 0}
};

ClCorAttribPropsT T1PortAttrList[] = {
    {"T1Port_SPEED", T1PORT_SPEED, CL_COR_UINT32, 100, 10, 1000, 0},
    {"T1Port_MTU", T1PORT_MTU, CL_COR_UINT32, 1500, 1500, 25000, 0},

    {"UNKNOWN_ATTRIB", 0, 0, 0, 0}
};

ClCorArrAttribDefT T1PortArrAttrList[] = {

    {"UNKNOWN_ATTRIB", 0, 0, 0, 0}
};

ClCorAssocAttribDefT T1PortAssocAttrList[] = {

    {"UNKNOWN_ATTRIB", 0, 0, 0, 0}
};
ClCorContAttribDefT T1PortContAttrList[] = {

    {"UNKNOWN_ATTRIB", 0, 0, 0, 0}
};

ClCorAttribPropsT SystemControllerAttrList[] = {

    {"UNKNOWN_ATTRIB", 0, 0, 0, 0}
};

ClCorArrAttribDefT SystemControllerArrAttrList[] = {

    {"UNKNOWN_ATTRIB", 0, 0, 0, 0}
};

ClCorAssocAttribDefT SystemControllerAssocAttrList[] = {

    {"UNKNOWN_ATTRIB", 0, 0, 0, 0}
};
ClCorContAttribDefT SystemControllerContAttrList[] = {

    {"UNKNOWN_ATTRIB", 0, 0, 0, 0}
};

ClCorAttribPropsT GigeBladeAttrList[] = {
    {"GigeBlade_PROVINDEX", GIGEBLADE_PROVINDEX, CL_COR_INT8, 0, 0, 0, 0},
    {"GigeBlade_PROVTEMPTHRESHOLD", GIGEBLADE_PROVTEMPTHRESHOLD, CL_COR_UINT32,
     50, 0, 100, 0},

    {"UNKNOWN_ATTRIB", 0, 0, 0, 0}
};

ClCorArrAttribDefT GigeBladeArrAttrList[] = {

    {"UNKNOWN_ATTRIB", 0, 0, 0, 0}
};

ClCorAssocAttribDefT GigeBladeAssocAttrList[] = {

    {"UNKNOWN_ATTRIB", 0, 0, 0, 0}
};
ClCorContAttribDefT GigeBladeContAttrList[] = {

    {"UNKNOWN_ATTRIB", 0, 0, 0, 0}
};

ClCorAttribPropsT T1BladeAttrList[] = {
    {"T1Blade_PROVINDEX", T1BLADE_PROVINDEX, CL_COR_INT8, 0, 0, 0, 0},
    {"T1Blade_PROVTEMPTHRESHOLD", T1BLADE_PROVTEMPTHRESHOLD, CL_COR_UINT32, 50,
     0, 100, 0},

    {"UNKNOWN_ATTRIB", 0, 0, 0, 0}
};

ClCorArrAttribDefT T1BladeArrAttrList[] = {

    {"UNKNOWN_ATTRIB", 0, 0, 0, 0}
};

ClCorAssocAttribDefT T1BladeAssocAttrList[] = {

    {"UNKNOWN_ATTRIB", 0, 0, 0, 0}
};
ClCorContAttribDefT T1BladeContAttrList[] = {

    {"UNKNOWN_ATTRIB", 0, 0, 0, 0}
};

ClCorMoClassDefT ChassisMO[] = {
    {"Chassis", CLASS_CHASSIS_MO, CL_COR_UTILS_UNKNOWN_CLASS, 1,
     ChassisAttrList, ChassisArrAttrList, ChassisAssocAttrList,
     ChassisContAttrList},
    {"CL_COR_UTILS_UNKNOWN_CLASS", 0, 0, 0, 0}
};

ClCorMoClassDefT GigePortMO[] = {
    {"GigePort", CLASS_GIGEPORT_MO, CL_COR_UTILS_UNKNOWN_CLASS, 1,
     GigePortAttrList, GigePortArrAttrList, GigePortAssocAttrList,
     GigePortContAttrList},
    {"CL_COR_UTILS_UNKNOWN_CLASS", 0, 0, 0, 0}
};

ClCorMoClassDefT T1PortMO[] = {
    {"T1Port", CLASS_T1PORT_MO, CL_COR_UTILS_UNKNOWN_CLASS, 1, T1PortAttrList,
     T1PortArrAttrList, T1PortAssocAttrList, T1PortContAttrList},
    {"CL_COR_UTILS_UNKNOWN_CLASS", 0, 0, 0, 0}
};

ClCorMoClassDefT SystemControllerMO[] = {
    {"SystemController", CLASS_SYSTEMCONTROLLER_MO, CL_COR_UTILS_UNKNOWN_CLASS,
     1, SystemControllerAttrList, SystemControllerArrAttrList,
     SystemControllerAssocAttrList, SystemControllerContAttrList},
    {"CL_COR_UTILS_UNKNOWN_CLASS", 0, 0, 0, 0}
};

ClCorMoClassDefT GigeBladeMO[] = {
    {"GigeBlade", CLASS_GIGEBLADE_MO, CL_COR_UTILS_UNKNOWN_CLASS, 3,
     GigeBladeAttrList, GigeBladeArrAttrList, GigeBladeAssocAttrList,
     GigeBladeContAttrList},
    {"CL_COR_UTILS_UNKNOWN_CLASS", 0, 0, 0, 0}
};

ClCorMoClassDefT T1BladeMO[] = {
    {"T1Blade", CLASS_T1BLADE_MO, CL_COR_UTILS_UNKNOWN_CLASS, 1,
     T1BladeAttrList, T1BladeArrAttrList, T1BladeAssocAttrList,
     T1BladeContAttrList},
    {"CL_COR_UTILS_UNKNOWN_CLASS", 0, 0, 0, 0}
};

ClCorMsoClassDefT GigePortAlarmMso[] = {
    {
     "CLASS_GigePort_ALARM_MSO",
     CLASS_GIGEPORT_ALARM_MSO,
     CL_COR_UTILS_UNKNOWN_CLASS,
     1,
     CL_COR_SVC_ID_ALARM_MANAGEMENT,
     CL_OM_ALARM_GIGEPORT_CLASS_TYPE,
     alarmMsoAttrList,
     alarmMsoArrAttrList,
     NULL,
     alarmMsoContAttrList,
     },
    {
     "CL_COR_UTILS_UNKNOWN_CLASS",
     0, 0, 0, 0, 0, 0, 0, 0, 0}
};

ClCorMsoClassDefT GigePortProvMso[] = {
    {
     "CLASS_GigePort_PROV_MSO",
     CLASS_GIGEPORT_PROV_MSO,
     CL_COR_UTILS_UNKNOWN_CLASS,
     1,
     CL_COR_SVC_ID_PROVISIONING_MANAGEMENT,
     CL_OM_PROV_GIGEPORT_CLASS_TYPE,
     GigePortAttrList,
     NULL,
     NULL,
     NULL,
     },
    {
     "CL_COR_UTILS_UNKNOWN_CLASS",
     0, 0, 0, 0, 0, 0, 0, 0, 0}
};

ClCorMsoClassDefT T1PortAlarmMso[] = {
    {
     "CLASS_T1Port_ALARM_MSO",
     CLASS_T1PORT_ALARM_MSO,
     CL_COR_UTILS_UNKNOWN_CLASS,
     1,
     CL_COR_SVC_ID_ALARM_MANAGEMENT,
     CL_OM_ALARM_T1PORT_CLASS_TYPE,
     alarmMsoAttrList,
     alarmMsoArrAttrList,
     NULL,
     alarmMsoContAttrList,
     },
    {
     "CL_COR_UTILS_UNKNOWN_CLASS",
     0, 0, 0, 0, 0, 0, 0, 0, 0}
};

ClCorMsoClassDefT T1PortProvMso[] = {
    {
     "CLASS_T1Port_PROV_MSO",
     CLASS_T1PORT_PROV_MSO,
     CL_COR_UTILS_UNKNOWN_CLASS,
     1,
     CL_COR_SVC_ID_PROVISIONING_MANAGEMENT,
     CL_OM_PROV_T1PORT_CLASS_TYPE,
     T1PortAttrList,
     NULL,
     NULL,
     NULL,
     },
    {
     "CL_COR_UTILS_UNKNOWN_CLASS",
     0, 0, 0, 0, 0, 0, 0, 0, 0}
};

ClCorMsoClassDefT GigeBladeProvMso[] = {
    {
     "CLASS_GigeBlade_PROV_MSO",
     CLASS_GIGEBLADE_PROV_MSO,
     CL_COR_UTILS_UNKNOWN_CLASS,
     1,
     CL_COR_SVC_ID_PROVISIONING_MANAGEMENT,
     CL_OM_PROV_GIGEBLADE_CLASS_TYPE,
     GigeBladeAttrList,
     NULL,
     NULL,
     NULL,
     },
    {
     "CL_COR_UTILS_UNKNOWN_CLASS",
     0, 0, 0, 0, 0, 0, 0, 0, 0}
};

ClCorMsoClassDefT T1BladeProvMso[] = {
    {
     "CLASS_T1Blade_PROV_MSO",
     CLASS_T1BLADE_PROV_MSO,
     CL_COR_UTILS_UNKNOWN_CLASS,
     1,
     CL_COR_SVC_ID_PROVISIONING_MANAGEMENT,
     CL_OM_PROV_T1BLADE_CLASS_TYPE,
     T1BladeAttrList,
     NULL,
     NULL,
     NULL,
     },
    {
     "CL_COR_UTILS_UNKNOWN_CLASS",
     0, 0, 0, 0, 0, 0, 0, 0, 0}
};

ClCorClassTblT pCorResourceClassTable[] = {
    /*
     * Relative depth in the COR Tree , Pointer to MO class def, Maximum no
     * of objects to be allowed , Pointer to MSO class definition, Maximum no 
     * of MSO objects to be allowed . 
     */

    {0, CL_COR_BASE_CLASS, clAlarmBaseClass, 10, NULL, 0},
    {0, CL_COR_BASE_CLASS, alarmInfo, 10, NULL, 0},
    /*
     * { 0, CL_COR_MO_CLASS, ChassisMO, 1, fmMso, 10}, 
     */
    {0, CL_COR_MO_CLASS, ChassisMO, 1, NULL, 0},
    /*
     * { 1, CL_COR_MO_CLASS, SystemControllerMO, 1, fmMso, 10}, 
     */
    {1, CL_COR_MO_CLASS, SystemControllerMO, 1, NULL, 0},
    /*
     * { 1, CL_COR_MO_CLASS, GigeBladeMO, 1, fmMso, 10}, 
     */
    {1, CL_COR_MO_CLASS, GigeBladeMO, 1, GigeBladeProvMso, 1},
    /*
     * { 2, CL_COR_MO_CLASS, GigePortMO, 1, fmMso, 10}, 
     */
    {2, CL_COR_MO_CLASS, GigePortMO, 1, GigePortProvMso, 1},
    {2, CL_COR_MO_CLASS, GigePortMO, 1, GigePortAlarmMso, 1},
    /*
     * { 1, CL_COR_MO_CLASS, T1BladeMO, 1, fmMso, 10}, 
     */
    {1, CL_COR_MO_CLASS, T1BladeMO, 1, T1BladeProvMso, 1},
    /*
     * { 2, CL_COR_MO_CLASS, T1PortMO, 1, fmMso, 10}, 
     */
    {2, CL_COR_MO_CLASS, T1PortMO, 1, T1PortProvMso, 1},
    {2, CL_COR_MO_CLASS, T1PortMO, 1, T1PortAlarmMso, 1},

    {0, 0, NULL, 0, NULL, 0}
};
