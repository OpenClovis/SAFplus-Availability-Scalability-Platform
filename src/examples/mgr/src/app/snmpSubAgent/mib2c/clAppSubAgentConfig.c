#include <clAppSubAgentConfig.h>
#include <clSnmpsafAmfInstXlation.h>
#include <clSnmpsafAmfUtil.h>
#include <clSnmpsafAmfNotifications.h>
                                                                                                                  
                                                                                                                             
/*
 * This structure contains information regarding oid of the table and the 
 * type of index. The index  can  either be  a string value or a non-string value.
 * Any SNMP type apart from string is treated as non-string.
 */

ClSnmpOidInfoT appOidInfo[] =
{
	{    CL_SAFAMF_SCALARS, "1.3.6.1.4.1.18568.2.2.2.1.1.1",
		1 /* no. of indexes */,
        {
	        { 0, 0 },
            
        },
        NULL, clSnmpsafAmfDefaultInstXlator, clSnmpsafAmfDefaultInstCompare
    },
	{    CL_SAFAMF_SCALARS, "1.3.6.1.4.1.18568.2.2.2.1.1.2",
		1 /* no. of indexes */,
        {
	        { 0, 0 },
            
        },
        NULL, clSnmpsafAmfDefaultInstXlator, clSnmpsafAmfDefaultInstCompare
    },
	{    CL_SAFAMF_SCALARS, "1.3.6.1.4.1.18568.2.2.2.1.1.3",
		1 /* no. of indexes */,
        {
	        { 0, 0 },
            
        },
        NULL, clSnmpsafAmfDefaultInstXlator, clSnmpsafAmfDefaultInstCompare
    },
	{    CL_SAFAMF_SCALARS, "1.3.6.1.4.1.18568.2.2.2.1.1.4",
		1 /* no. of indexes */,
        {
	        { 0, 0 },
            
        },
        NULL, clSnmpsafAmfDefaultInstXlator, clSnmpsafAmfDefaultInstCompare
    },
	{    CL_SAFAMF_SCALARS, "1.3.6.1.4.1.18568.2.2.2.1.1.5",
		1 /* no. of indexes */,
        {
	        { 0, 0 },
            
        },
        NULL, clSnmpsafAmfDefaultInstXlator, clSnmpsafAmfDefaultInstCompare
    },
	{    CL_SAFAMF_SCALARS, "1.3.6.1.4.1.18568.2.2.2.1.1.6",
		1 /* no. of indexes */,
        {
	        { 0, 0 },
            
        },
        NULL, clSnmpsafAmfDefaultInstXlator, clSnmpsafAmfDefaultInstCompare
    },
	{    CL_SAFAMF_SCALARS, "1.3.6.1.4.1.18568.2.2.2.1.1.7",
		1 /* no. of indexes */,
        {
	        { 0, 0 },
            
        },
        NULL, clSnmpsafAmfDefaultInstXlator, clSnmpsafAmfDefaultInstCompare
    },
	{    CL_SAFAMF_SCALARS, "1.3.6.1.4.1.18568.2.2.2.1.1.8",
		1 /* no. of indexes */,
        {
	        { 0, 0 },
            
        },
        NULL, clSnmpsafAmfDefaultInstXlator, clSnmpsafAmfDefaultInstCompare
    },
	{    CL_SAFAMF_SCALARS, "1.3.6.1.4.1.18568.2.2.2.1.3.1.1",
		1 /* no. of indexes */,
        {
	        { 0, 0 },
            
        },
        NULL, clSnmpsafAmfDefaultInstXlator, clSnmpsafAmfDefaultInstCompare
    },
	{    CL_SAFAMF_SCALARS, "1.3.6.1.4.1.18568.2.2.2.1.3.1.2",
		1 /* no. of indexes */,
        {
	        { 0, 0 },
            
        },
        NULL, clSnmpsafAmfDefaultInstXlator, clSnmpsafAmfDefaultInstCompare
    },
	{    CL_SAAMFAPPLICATIONTABLE, "1.3.6.1.4.1.18568.2.2.2.1.2.1",
		1 /* no. of indexes */,
        {
	        { CL_SNMP_NON_STRING_ATTR, 4 },
            
        },
        clSnmpsaAmfApplicationTableIndexCorAttrGet, clSnmpsaAmfApplicationTableInstXlator, clSnmpsaAmfApplicationTableInstCompare
    },
	{    CL_SAAMFNODETABLE, "1.3.6.1.4.1.18568.2.2.2.1.2.2",
		1 /* no. of indexes */,
        {
	        { CL_SNMP_NON_STRING_ATTR, 4 },
            
        },
        clSnmpsaAmfNodeTableIndexCorAttrGet, clSnmpsaAmfNodeTableInstXlator, clSnmpsaAmfNodeTableInstCompare
    },
	{    CL_SAAMFSGTABLE, "1.3.6.1.4.1.18568.2.2.2.1.2.3",
		1 /* no. of indexes */,
        {
	        { CL_SNMP_NON_STRING_ATTR, 4 },
            
        },
        clSnmpsaAmfSGTableIndexCorAttrGet, clSnmpsaAmfSGTableInstXlator, clSnmpsaAmfSGTableInstCompare
    },
	{    CL_SAAMFSUTABLE, "1.3.6.1.4.1.18568.2.2.2.1.2.4",
		1 /* no. of indexes */,
        {
	        { CL_SNMP_NON_STRING_ATTR, 4 },
            
        },
        clSnmpsaAmfSUTableIndexCorAttrGet, clSnmpsaAmfSUTableInstXlator, clSnmpsaAmfSUTableInstCompare
    },
	{    CL_SAAMFSITABLE, "1.3.6.1.4.1.18568.2.2.2.1.2.5",
		1 /* no. of indexes */,
        {
	        { CL_SNMP_NON_STRING_ATTR, 4 },
            
        },
        clSnmpsaAmfSITableIndexCorAttrGet, clSnmpsaAmfSITableInstXlator, clSnmpsaAmfSITableInstCompare
    },
	{    CL_SAAMFSUSPERSIRANKTABLE, "1.3.6.1.4.1.18568.2.2.2.1.2.6",
		2 /* no. of indexes */,
        {
	        { CL_SNMP_NON_STRING_ATTR, 4 },
	        { CL_SNMP_NON_STRING_ATTR, 4 },
            
        },
        clSnmpsaAmfSUsperSIRankTableIndexCorAttrGet, clSnmpsaAmfSUsperSIRankTableInstXlator, clSnmpsaAmfSUsperSIRankTableInstCompare
    },
	{    CL_SAAMFSGSIRANKTABLE, "1.3.6.1.4.1.18568.2.2.2.1.2.7",
		2 /* no. of indexes */,
        {
	        { CL_SNMP_NON_STRING_ATTR, 4 },
	        { CL_SNMP_NON_STRING_ATTR, 4 },
            
        },
        NULL, clSnmpsaAmfSGSIRankTableInstXlator, clSnmpsaAmfSGSIRankTableInstCompare
    },
	{    CL_SAAMFSGSURANKTABLE, "1.3.6.1.4.1.18568.2.2.2.1.2.8",
		2 /* no. of indexes */,
        {
	        { CL_SNMP_NON_STRING_ATTR, 4 },
	        { CL_SNMP_NON_STRING_ATTR, 4 },
            
        },
        NULL, clSnmpsaAmfSGSURankTableInstXlator, clSnmpsaAmfSGSURankTableInstCompare
    },
	{    CL_SAAMFSISIDEPTABLE, "1.3.6.1.4.1.18568.2.2.2.1.2.9",
		2 /* no. of indexes */,
        {
	        { CL_SNMP_NON_STRING_ATTR, 4 },
	        { CL_SNMP_NON_STRING_ATTR, 4 },
            
        },
        clSnmpsaAmfSISIDepTableIndexCorAttrGet, clSnmpsaAmfSISIDepTableInstXlator, clSnmpsaAmfSISIDepTableInstCompare
    },
	{    CL_SAAMFCOMPTABLE, "1.3.6.1.4.1.18568.2.2.2.1.2.10",
		1 /* no. of indexes */,
        {
	        { CL_SNMP_NON_STRING_ATTR, 4 },
            
        },
        clSnmpsaAmfCompTableIndexCorAttrGet, clSnmpsaAmfCompTableInstXlator, clSnmpsaAmfCompTableInstCompare
    },
	{    CL_SAAMFCOMPCSTYPESUPPORTEDTABLE, "1.3.6.1.4.1.18568.2.2.2.1.2.11",
		2 /* no. of indexes */,
        {
	        { CL_SNMP_NON_STRING_ATTR, 4 },
	        { CL_SNMP_NON_STRING_ATTR, 4 },
            
        },
        clSnmpsaAmfCompCSTypeSupportedTableIndexCorAttrGet, clSnmpsaAmfCompCSTypeSupportedTableInstXlator, clSnmpsaAmfCompCSTypeSupportedTableInstCompare
    },
	{    CL_SAAMFCSITABLE, "1.3.6.1.4.1.18568.2.2.2.1.2.12",
		1 /* no. of indexes */,
        {
	        { CL_SNMP_NON_STRING_ATTR, 4 },
            
        },
        clSnmpsaAmfCSITableIndexCorAttrGet, clSnmpsaAmfCSITableInstXlator, clSnmpsaAmfCSITableInstCompare
    },
	{    CL_SAAMFCSICSIDEPTABLE, "1.3.6.1.4.1.18568.2.2.2.1.2.13",
		2 /* no. of indexes */,
        {
	        { CL_SNMP_NON_STRING_ATTR, 4 },
	        { CL_SNMP_NON_STRING_ATTR, 4 },
            
        },
        clSnmpsaAmfCSICSIDepTableIndexCorAttrGet, clSnmpsaAmfCSICSIDepTableInstXlator, clSnmpsaAmfCSICSIDepTableInstCompare
    },
	{    CL_SAAMFCSINAMEVALUETABLE, "1.3.6.1.4.1.18568.2.2.2.1.2.14",
		2 /* no. of indexes */,
        {
	        { CL_SNMP_NON_STRING_ATTR, 4 },
	        { CL_SNMP_NON_STRING_ATTR, 4 },
            
        },
        clSnmpsaAmfCSINameValueTableIndexCorAttrGet, clSnmpsaAmfCSINameValueTableInstXlator, clSnmpsaAmfCSINameValueTableInstCompare
    },
	{    CL_SAAMFCSTYPEATTRNAMETABLE, "1.3.6.1.4.1.18568.2.2.2.1.2.15",
		2 /* no. of indexes */,
        {
	        { CL_SNMP_NON_STRING_ATTR, 4 },
	        { CL_SNMP_NON_STRING_ATTR, 4 },
            
        },
        clSnmpsaAmfCSTypeAttrNameTableIndexCorAttrGet, clSnmpsaAmfCSTypeAttrNameTableInstXlator, clSnmpsaAmfCSTypeAttrNameTableInstCompare
    },
	{    CL_SAAMFSUSITABLE, "1.3.6.1.4.1.18568.2.2.2.1.2.16",
		2 /* no. of indexes */,
        {
	        { CL_SNMP_NON_STRING_ATTR, 4 },
	        { CL_SNMP_NON_STRING_ATTR, 4 },
            
        },
        NULL, clSnmpsaAmfSUSITableInstXlator, clSnmpsaAmfSUSITableInstCompare
    },
	{    CL_SAAMFHEALTHCHECKTABLE, "1.3.6.1.4.1.18568.2.2.2.1.2.17",
		2 /* no. of indexes */,
        {
	        { CL_SNMP_NON_STRING_ATTR, 4 },
	        { CL_SNMP_STRING_ATTR, 32 },
            
        },
        clSnmpsaAmfHealthCheckTableIndexCorAttrGet, clSnmpsaAmfHealthCheckTableInstXlator, clSnmpsaAmfHealthCheckTableInstCompare
    },
	{    CL_SAAMFSCOMPCSITABLE, "1.3.6.1.4.1.18568.2.2.2.1.2.18",
		2 /* no. of indexes */,
        {
	        { CL_SNMP_NON_STRING_ATTR, 4 },
	        { CL_SNMP_NON_STRING_ATTR, 4 },
            
        },
        NULL, clSnmpsaAmfSCompCsiTableInstXlator, clSnmpsaAmfSCompCsiTableInstCompare
    },
	{    CL_SAAMFPROXYPROXIEDTABLE, "1.3.6.1.4.1.18568.2.2.2.1.2.19",
		2 /* no. of indexes */,
        {
	        { CL_SNMP_NON_STRING_ATTR, 4 },
	        { CL_SNMP_NON_STRING_ATTR, 4 },
            
        },
        NULL, clSnmpsaAmfProxyProxiedTableInstXlator, clSnmpsaAmfProxyProxiedTableInstCompare
    },
    
    {0, NULL, 0, {{0,0}}, NULL, NULL}
};

/*
 * This provides mapping between trap oid and the notification call back function 
 * generated by mib2c. When an alarm is raised this mapping is used to call the
 * corresponding notification function.
 */
ClSnmpNtfyCallbackTableT clSnmpAppCallback[]= {
    {".1.3.6.1.4.1.18568.2.2.2.1.3.2.1", clSnmpsaAmfAlarmServiceImpairedIndexGet, clSnmpSendsaAmfAlarmServiceImpairedTrap},
    {".1.3.6.1.4.1.18568.2.2.2.1.3.2.2", clSnmpsaAmfAlarmCompInstantiationFailedIndexGet, clSnmpSendsaAmfAlarmCompInstantiationFailedTrap},
    {".1.3.6.1.4.1.18568.2.2.2.1.3.2.3", clSnmpsaAmfAlarmCompCleanupFailedIndexGet, clSnmpSendsaAmfAlarmCompCleanupFailedTrap},
    {".1.3.6.1.4.1.18568.2.2.2.1.3.2.4", clSnmpsaAmfAlarmClusterResetIndexGet, clSnmpSendsaAmfAlarmClusterResetTrap},
    {".1.3.6.1.4.1.18568.2.2.2.1.3.2.5", clSnmpsaAmfAlarmSIUnassignedIndexGet, clSnmpSendsaAmfAlarmSIUnassignedTrap},
    {".1.3.6.1.4.1.18568.2.2.2.1.3.2.6", clSnmpsaAmfAlarmProxiedCompUnproxiedIndexGet, clSnmpSendsaAmfAlarmProxiedCompUnproxiedTrap},
    {".1.3.6.1.4.1.18568.2.2.2.1.3.2.7", clSnmpsaAmfStateChgClusterAdminIndexGet, clSnmpSendsaAmfStateChgClusterAdminTrap},
    {".1.3.6.1.4.1.18568.2.2.2.1.3.2.8", clSnmpsaAmfStateChgApplicationAdminIndexGet, clSnmpSendsaAmfStateChgApplicationAdminTrap},
    {".1.3.6.1.4.1.18568.2.2.2.1.3.2.9", clSnmpsaAmfStateChgNodeAdminIndexGet, clSnmpSendsaAmfStateChgNodeAdminTrap},
    {".1.3.6.1.4.1.18568.2.2.2.1.3.2.10", clSnmpsaAmfStateChgSGAdminIndexGet, clSnmpSendsaAmfStateChgSGAdminTrap},
    {".1.3.6.1.4.1.18568.2.2.2.1.3.2.11", clSnmpsaAmfStateChgSUAdminIndexGet, clSnmpSendsaAmfStateChgSUAdminTrap},
    {".1.3.6.1.4.1.18568.2.2.2.1.3.2.12", clSnmpsaAmfStateChgSIAdminIndexGet, clSnmpSendsaAmfStateChgSIAdminTrap},
    {".1.3.6.1.4.1.18568.2.2.2.1.3.2.13", clSnmpsaAmfStateChgNodeOperIndexGet, clSnmpSendsaAmfStateChgNodeOperTrap},
    {".1.3.6.1.4.1.18568.2.2.2.1.3.2.14", clSnmpsaAmfStateChgSUOperIndexGet, clSnmpSendsaAmfStateChgSUOperTrap},
    {".1.3.6.1.4.1.18568.2.2.2.1.3.2.15", clSnmpsaAmfStateChgSUPresenceIndexGet, clSnmpSendsaAmfStateChgSUPresenceTrap},
    {".1.3.6.1.4.1.18568.2.2.2.1.3.2.16", clSnmpsaAmfStateChgSUHaStateIndexGet, clSnmpSendsaAmfStateChgSUHaStateTrap},
    {".1.3.6.1.4.1.18568.2.2.2.1.3.2.17", clSnmpsaAmfStateChgSIAssignmentIndexGet, clSnmpSendsaAmfStateChgSIAssignmentTrap},
    {".1.3.6.1.4.1.18568.2.2.2.1.3.2.18", clSnmpsaAmfStateChgUnproxiedCompProxiedIndexGet, clSnmpSendsaAmfStateChgUnproxiedCompProxiedTrap},

    {NULL, NULL, NULL}
};

void init_appMIB()
{
    init_safAmfMIB();

}
