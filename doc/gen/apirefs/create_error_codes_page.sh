#!/bin/bash

################################################################################
#
# This script creates Error Codes page in API Reference Guide 
# using the source header files
#
################################################################################

SOURCE_DIR=../../src/SAFplus/components

printf "/**\n\n"
printf "\\\page apirefs_errorcodes OpenClovis Error and Return Codes\n"
printf "The majority of ASP API functions return a <b>Return Code</b> as their "
printf "return value. A zero return code (CL_OK) always indicates success. "
printf "Non-zero return codes always carry two types of information, "
printf "masked into one single value: a <b>Component Identifier</b> and "
printf "an <b>Error Code</b>. The component identifier identifies the component "
printf "(or software layer) in which the problem occurred, while the error "
printf "code describes the nature of the problem. Some error codes are common "
printf "across all components, while others are component specific.\n\n"
printf "The format of the return codes returned by OpenClovis APIs is 0xCCNNNN (given in hex), "
printf "where 0xCC is the ASP Component Identifier and 0xNNNN is the Error Code.\n"
printf " - NNNN <= 00ff : common Error Codes shared among all ASP components\n"
printf " - NNNN >  00ff : component specific Error Codes\n\n"
printf "The following macros, defined in clCommonErrors.h, help decoding the return codes:\n"
printf " - CL_GET_ERROR_CODE(RC) extracts the error code from the return code\n"
printf " - CL_GET_CID(RC) extracts the component identifier from the return code\n\n"
printf "\\\note <b>Only the Error Codes are listed as Return values " 
printf "in the Function Documentation of the API Reference pages. The real " 
printf "return value also contains the Component Identifier!</b>\n\n" 
printf "The following tables are provided to help software engineers to quickly decode return codes found in ASP log files:\n"
printf " - \\\ref apirefs_errorcodes_compid\n"
printf " - \\\ref <b>Error Codes:</b>\n"
printf "      - \\\ref apirefs_errorcodes_common\n"
printf "      - \\\ref apirefs_errorcodes_alarm\n"
printf "      - \\\ref apirefs_errorcodes_ams\n"
printf "      - \\\ref apirefs_errorcodes_cor\n"
printf "      - \\\ref apirefs_errorcodes_cpm\n"
printf "      - \\\ref apirefs_errorcodes_dbal\n"
printf "      - \\\ref apirefs_errorcodes_debug\n"
printf "      - \\\ref apirefs_errorcodes_event\n"
printf "      - \\\ref apirefs_errorcodes_eo\n"
printf "      - \\\ref apirefs_errorcodes_fault\n"
printf "      - \\\ref apirefs_errorcodes_gms\n"
printf "      - \\\ref apirefs_errorcodes_ioc\n"
printf "      - \\\ref apirefs_errorcodes_log\n"
printf "      - \\\ref apirefs_errorcodes_med\n"
printf "      - \\\ref apirefs_errorcodes_name\n"
printf "      - \\\ref apirefs_errorcodes_prov\n"
printf "      - \\\ref apirefs_errorcodes_osal\n"
printf "      - \\\ref apirefs_errorcodes_timer\n"
printf "      - \\\ref apirefs_errorcodes_txn\n"
printf "      - \\\ref apirefs_errorcodes_pm\n"

printf "\\section apirefs_errorcodes_compid ASP Component Identifiers\n"
./get_error_codes.awk "CL_CID_" $SOURCE_DIR/include/clCommon.h 
     
printf "\\section apirefs_errorcodes_common Common Error Codes\n"
./get_error_codes.awk " CL_ERR_" $SOURCE_DIR/include/clCommonErrors.h 

printf "\\section apirefs_errorcodes_alarm Alarm Error Codes\n"
./get_error_codes.awk " CL_ALARM_ERR_" $SOURCE_DIR/alarm/include/clAlarmErrors.h 

printf "\\section apirefs_errorcodes_ams Availability Management Service Error Codes\n"
./get_error_codes.awk " CL_AMS_ERR_" $SOURCE_DIR/amf/include/clAmsErrors.h 

printf "\\section apirefs_errorcodes_cor Clovis Object Repository Error Codes\n"
./get_error_codes.awk " CL_COR_" $SOURCE_DIR/cor/include/clCorErrors.h 

printf "\\section apirefs_errorcodes_cpm Component Manager Error Codes\n"
./get_error_codes.awk " CL_CPM_ERR_" $SOURCE_DIR/amf/include/clCpmErrors.h 

printf "\\section apirefs_errorcodes_dbal Database Abstraction Layer Error Codes\n"
./get_error_codes.awk " CL_DBAL_ERR_" $SOURCE_DIR/dbal/include/clDbalErrors.h 

printf "\\section apirefs_errorcodes_debug Debug Service Error Codes\n"
./get_error_codes.awk " CL_" $SOURCE_DIR/debug/include/clDebugErrors.h 

printf "\\section apirefs_errorcodes_event Event Service Error Codes\n"
./get_error_codes.awk " CL_EVENT_" $SOURCE_DIR/event/include/clEventErrors.h 

printf "\\section apirefs_errorcodes_eo Execution Object Error Codes\n"
./get_error_codes.awk " CL_EO_" $SOURCE_DIR/eo/include/clEoErrors.h 

printf "\\section apirefs_errorcodes_fault Fault Manager Error Codes\n"
./get_error_codes.awk " CL_FAULT_ERR_" $SOURCE_DIR/fault/include/clFaultErrorId.h 

printf "\\section apirefs_errorcodes_gms Group Membership Service Error Codes\n"
./get_error_codes.awk " CL_GMS_ERR_" $SOURCE_DIR/gms/include/clGmsErrors.h 

printf "\\section apirefs_errorcodes_ioc Intelligent Object Communication Error Codes\n"
./get_error_codes.awk " CL_IOC_" $SOURCE_DIR/ioc/include/clIocErrors.h 

printf "\\section apirefs_errorcodes_log Log Service Error Codes\n"
./get_error_codes.awk " CL_LOG_ERR_" $SOURCE_DIR/log/include/clLogErrors.h 

printf "\\section apirefs_errorcodes_med Mediation Library Error Codes\n"
./get_error_codes.awk " CL_MED_ERR_" $SOURCE_DIR/med/include/clMedErrors.h 

printf "\\section apirefs_errorcodes_name Name Service Error Codes\n"
./get_error_codes.awk " CL_NS_ERR_" $SOURCE_DIR/name/include/clNameErrors.h 

printf "\\section apirefs_errorcodes_prov Provisioning Library Error Codes\n"
./get_error_codes.awk " CL_PROV_" $SOURCE_DIR/prov/include/clProvErrors.h 

printf "\\section apirefs_errorcodes_osal Operating System Abstraction Layer Error Codes\n"
./get_error_codes.awk " CL_OSAL_ERR_" $SOURCE_DIR/osal/include/clOsalErrors.h 

printf "\\section apirefs_errorcodes_timer Timer Error Codes\n"
./get_error_codes.awk " CL_TIMER_ERR_" $SOURCE_DIR/timer/include/clTimerErrors.h 

printf "\\section apirefs_errorcodes_txn Transaction Error Codes\n"
./get_error_codes.awk " CL_TXN_" $SOURCE_DIR/txn/include/clTxnErrors.h 

printf "\\section apirefs_errorcodes_pm Performance Management Error Codes\n"
./get_error_codes.awk " CL_PM_" $SOURCE_DIR/pm/include/clPMErrors.h 

printf "\n\n*/"


#for f in `find $SOURCE_DIR -name "*ErrorId.h" -o -name "*Errors.h"`; do
#    echo "Changing file $f"
#    awk 'BEGIN {printing=0}
#        /^ *\/\*\*[^*]/ || /^ *\/\*\*$/ {printing = 1;}
#        {if (printing) {print;}}
#        /\*\// {printing = 0;} 
#        /\# *define *CL_/ {print;}' $f
#done
