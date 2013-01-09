/*******************************************************************************
*
* This file is auto-generated by OpenClovis IDE version 3.1
*
* /home/hoangle/git/SAFplus-Availability-Scalability-Platform/src/examples/eval/.ngc/config/clCorMetaStruct.h
* 
*
********************************************************************************/
#ifndef _CL_COR_META_STRUCT_H_
#define _CL_COR_META_STRUCT_H_

#ifdef __cplusplus
extern "C" {
#endif

enum ENUM_classes
{
    CLASS_RESERVED=2,
        CLASS_CHASSIS_MO ,
    CLASS_CSA104RES_MO ,
    CLASS_CSA104RES_PROV_MSO ,
    CLASS_CSA105RES_MO ,
    CLASS_CSA105RES_PROV_MSO ,
    CLASS_CSA105RES_ALARM_MSO ,
    CLASS_SCNODERES_MO ,
    CLASS_PAYLOADNODERES_MO ,

    COR_CLASS_MAX
};
enum ENUM_attributes
{
    ATTRIBUTE_RESERVED = 1,
    CSA104RES_COUNTER_RESET,
    CSA104RES_DELTA_T,
    CSA104RES_COUNTER,
    CSA105RES_COUNTER_RESET,
    CSA105RES_COUNTER_THRESH,
    CSA105RES_DELTA_T,

    COR_ATTRIBUTES_MAX
};

#ifdef __cplusplus
}
#endif

#endif
