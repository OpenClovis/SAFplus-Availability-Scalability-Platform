################################################################################
# ModuleName  : com
# $File$
# $Author$
# $Date$
################################################################################
# Description :
# usage: python mibexport.py <args>
# Exports resource information in SNMP MIB File
################################################################################
import sys
import string
import xml.dom.minidom
from string import Template

mibFileTemplate = Template("""\
${modname} DEFINITIONS ::= BEGIN

IMPORTS
	MODULE-IDENTITY,
	OBJECT-TYPE,
	OBJECT-IDENTITY,
	Unsigned32,
	Integer32,
	Counter32,
	Gauge32,
	IpAddress,
	TimeTicks,
	Counter64,
	Opaque,
	enterprises,
	OBJECT-TYPE,	
	NOTIFICATION-TYPE				   FROM SNMPv2-SMI
	MODULE-COMPLIANCE,
	OBJECT-GROUP,
	NOTIFICATION-GROUP				  FROM SNMPv2-CONF
	TEXTUAL-CONVENTION				  FROM SNMPv2-TC;

${prefix}MIB MODULE-IDENTITY
	LAST-UPDATED "${updateId}" -- ${systemDate}
	ORGANIZATION "${org}"
	CONTACT-INFO
		"${contactinfo}"
	DESCRIPTION
		"${desc}"

	--Revision History

	REVISION	"${revId}" -- ${revDate}
	DESCRIPTION "${revDesc}"
	::= { enterprises ${enterpriseOID}}
""")

moTemplate = Template("""
${prefix}${moName}Table OBJECT-TYPE
	SYNTAX	   SEQUENCE OF ${prefix}${moName}EntryColumns
	MAX-ACCESS   not-accessible
	STATUS	   current
	DESCRIPTION  \"$moDesc\"
	::= { ${prefix}MIB $moId }

${prefix}${moName}Entry OBJECT-TYPE
	SYNTAX	   ${prefix}${moName}EntryColumns
	MAX-ACCESS   not-accessible
	STATUS	   current
	DESCRIPTION  \"$moDesc\"
	INDEX {${indexAttr}}
	::= { ${prefix}${moName}Table 1 }
""")

attrTemplate = Template("""
${prefix}${moName}${attrName} OBJECT-TYPE
	SYNTAX	   ${attrType}
	MAX-ACCESS   read-write
	STATUS	   current
	DESCRIPTION  \"${attrName}\"
	::= { ${prefix}${moName}Entry ${attrId} }
""")

attrColumnTemplete = Template("""\
		${attrName}
				${attrType},
""")

attrColumnSequenceTemplate = Template("""
${prefix}${moName}EntryColumns ::= SEQUENCE {
${attrColumn}
}
""")

#------------------------------------------------------------------------------
#Creates ID-OID Map
def createIdOidMap(mapXml):
	result = dict();
	doc = xml.dom.minidom.parse(mapXml)
	moList = doc.getElementsByTagName("ResourceInfo")
	for mo in moList:
		cwKey = mo.attributes["rdn"].value
		oid   = mo.attributes["ID"].value
		result[cwKey] = oid
		attributeList = mo.getElementsByTagName("Attributes")
		for attribute in attributeList:
			cwKey = attribute.attributes["rdn"].value
			oid   = attribute.attributes["ID"].value
			result[cwKey] = oid
	return result
#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
#Creates SNMP Type - Clovis Type Map
def createDataTypeMap(mapXml):
	result = dict();
	doc = xml.dom.minidom.parse(mapXml)
	dataTypeList = doc.getElementsByTagName("datatypesmap:Map")
	for dataType in dataTypeList:
		snmpType = dataType.attributes["SnmpType"].value
		clvsTypeObj = dataType.getElementsByTagName("ClovisType")
		if len(clvsTypeObj) >0:
			clvsType = clvsTypeObj[0].attributes["AttributeType"].value
#		if result.get(clvsType) == None:
		result[clvsType] = snmpType
	return result
#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
#Creates rdn - Resource Obj Map
def createIdResourceMap(resourceList):
	result = dict();
	for res in resourceList:
		key = res.attributes["rdn"].value
		result[key] = res
	return result
#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
def processResourceList(resList, oidOverride):
	result = ""
	for res  in resList:
		rdn = res.attributes["rdn"].value
		symbolMap["moId"]   = idOidMap.get(rdn, -1)
		symbolMap["moName"] = res.attributes["name"].value
		symbolMap["moDesc"] = res.attributes["documentation"].value
		moAttributeList = res.getElementsByTagName("attribute")
		if len(moAttributeList) > 0:
			symbolMap["indexAttr"] = moAttributeList[0].attributes["name"].value
			result += moTemplate.safe_substitute(symbolMap)
			result += processAttributeList(moAttributeList, oidOverride)
		else:
			continue
	return result
#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
def processAttributeList(attrList, oidOverride):
	result = ""
	sequenceStr = ""
	for attr  in attrList:
		if oidOverride == "true":
			rdn	= attr.attributes["rdn"].value
			symbolMap['attrId']   = idOidMap.get(rdn, -1)
			symbolMap['attrName'] = attr.attributes["name"].value
			clAttrType			= attr.attributes["dataType"].value
			symbolMap['attrType'] = typeMap.get(clAttrType, clAttrType)
			result  += attrTemplate.safe_substitute(symbolMap)
			sequenceStr += attrColumnTemplete.safe_substitute(symbolMap)
		else:
			isImported = attr.attributes["isImported"].value
			if isImported == "false":
				rdn	= attr.attributes["rdn"].value
				symbolMap['attrId']   = idOidMap.get(rdn, -1)
				symbolMap['attrName'] = attr.attributes["name"].value
				clAttrType			= attr.attributes["dataType"].value
				symbolMap['attrType'] = typeMap.get(clAttrType, "Integer32")
				result  += attrTemplate.safe_substitute(symbolMap)
				sequenceStr += attrColumnTemplete.safe_substitute(symbolMap)
	symbolMap["attrColumn"] = sequenceStr
	return attrColumnSequenceTemplate.safe_substitute(symbolMap) + result
#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
#Script Execution Starts Here.
dataTypeXml = sys.argv[1]
idOidMapXml = sys.argv[2]
resourceXml = sys.argv[3]

symbolMap = dict()
symbolMap["prefix"]		= sys.argv[4]
symbolMap["org"]		   = sys.argv[5]
symbolMap["contactinfo"]   = sys.argv[6]
symbolMap["enterpriseOID"] = sys.argv[7]
symbolMap["modname"]	   = sys.argv[8]

idOidMap = createIdOidMap(idOidMapXml)
typeMap  = createDataTypeMap(dataTypeXml)

resourceDoc = xml.dom.minidom.parse(resourceXml)
resourceInfoObj = resourceDoc.getElementsByTagName("resource:resourceInformation")
resourceList = resourceInfoObj[0].getElementsByTagName("hardwareResource") + resourceInfoObj[0].getElementsByTagName("softwareResource") + resourceInfoObj[0].getElementsByTagName("systemController") + resourceInfoObj[0].getElementsByTagName("nodeHardwareResource")
idResourceMap = createIdResourceMap(resourceList);
selResourceList = []
for i in range(11, len(sys.argv)):
	rdn = sys.argv[i]
	selObj = idResourceMap.get(rdn, -1)
	selResourceList.append(selObj)
oidOverride = sys.argv[10]
result  = mibFileTemplate.safe_substitute(symbolMap)
result += processResourceList(selResourceList, oidOverride)
result += "\nEND"
out_file = open(sys.argv[9], "w")
out_file.write(result)
out_file.close()

