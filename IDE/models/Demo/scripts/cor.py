################################################################################
# ModuleName  : Demo
# $File$
# $Author$
# $Date$
################################################################################
# Description :
# usage: python cor.py <args>
# create corMetaStruct.c
######################################################################################
import sys
import string
import xml.dom.minidom
from string import Template

   
headerTemplate = Template("""\
#include <clCorApi.h>
#include <clCorServiceId.h>
#include <clCorUtilityApi.h>
#include <clOmCommonClassTypes.h>
#include <limits.h>
#include "clOmClassId.h"        
#include "clCorMetaStruct.h"


extern ClCorAttribPropsT alarmMsoAttrList[]; 
extern ClCorArrAttribDefT alarmMsoArrAttrList[] ;
extern ClCorContAttribDefT alarmMsoContAttrList[];
extern ClCorMoClassDefT alarmInfo[];
extern ClCorMoClassDefT clAlarmBaseClass[];

extern ClCorAttribPropsT   cpmCompMoAttrList[];
extern ClCorContAttribDefT   cpmCompMoContAttrList[];
extern ClCorArrAttribDefT   cpmCompMoArrAttrList[];
extern ClCorAttribPropsT   cpmSUMoAttrList[];
extern ClCorArrAttribDefT   cpmSUMoArrAttrList[];
extern ClCorMoClassDefT compArgvContClass[];
extern ClCorMoClassDefT compEnvContClass[];

""")

corCfgTemplate = Template("""\

ClCorMOIdT corCfgAppParentMOId={
	{{CLASS_${chassisName}_MO, CL_COR_INSTANCE_WILD_CARD }},
	CL_COR_INVALID_SVC_ID,
	1,
	CL_COR_MO_PATH_ABSOLUTE
};
""")

AttributeClassTemplate = Template("""\

ClCorAttribPropsT ${className}ProvAttrList[]=
{
${ProvattributeEntries}	
	{"UNKNOWN_ATTRIB", 0,0,0,0}	
};

ClCorAttribPropsT ${className}MOAttrList[]=
{
${MOattributeEntries}	
	{"UNKNOWN_ATTRIB", 0,0,0,0}	
};
""")

AttributeEntryTemplate = Template("""\
	{"${className}_${attrName}", ${classid}_${attrName}, CL_COR_${attrType}, ${defValue},${minValue}, ${maxValue}, 0},
""")

ArrayAttributeClassTemplate = Template("""\

ClCorArrAttribDefT ${className}ProvArrAttrList[] =
{
${ProvarrayAttributeEntries}	
	{"UNKNOWN_ATTRIB", 0,0,0,0}
};

ClCorArrAttribDefT ${className}MOArrAttrList[] =
{
${MOarrayAttributeEntries}	
	{"UNKNOWN_ATTRIB", 0,0,0,0}
};
""")

ArrayAttributeEntryTemplate = Template("""\
	{"${className}_${attrName}", ${classid}_${attrName}, CL_COR_${attrType}, ${arraySize}, 0},
""")

AssocAttributeClassTemplate = Template("""\

ClCorAssocAttribDefT ${className}AssocAttrList[] =
{
${assocAttributeEntries}	
	{"UNKNOWN_ATTRIB", 0,0,0,0}
};
""")

AssocAttributeEntryTemplate = Template("""\
	{"${attrName}", ${classid}_${attrName}, CLASS_${attrName}_MO, ${multiplicity}, 0},
""")

ContAttributeClassTemplate = Template("""\
ClCorContAttribDefT ${className}ContAttrList[] =
{
${contAttributeEntries}	
	{"UNKNOWN_ATTRIB", 0,0,0,0}
};
""")

ContAttributeEntryTemplate = Template("""\
	{"${attrName}", ${classid}_${attrName}, CLASS_${attrName}_MO, ${multiplicity}, 0},
""")

MOclassDefinitionTemplate = Template("""\

ClCorMoClassDefT ${className}MO[]=
{
	{"${className}",
	  CLASS_${classid}_MO,
	  ${superClass},
	  ${maxInstances},
	  ${attrList},
	  ${arrayAttrList},
	  ${associatedAttrList},
	  ${contAttrList} },	
	{"CL_COR_UTILS_UNKNOWN_CLASS", 0,0,0,0}	
};
""")

MSOClassDefinitionTemplate = Template("""\

ClCorMsoClassDefT ${className}${svc}Mso[]=
{
	{
       "CLASS_${className}_${capsvc}_MSO" ,
       CLASS_${classid}_${capsvc}_MSO, 
       CL_COR_UTILS_UNKNOWN_CLASS , 
       ${maxMsoCount}, 
       ${serviceID}, 
       CL_OM_${capsvc}_${classid}_CLASS_TYPE,
       ${msoAttrList}, 
       ${msoArrAttrList}, 
       ${msoAssociatedAttrList}, 
       ${msoContAttrList},
    },
    {
        "CL_COR_UTILS_UNKNOWN_CLASS",
        0, 0, 0, 0, 0, 0, 0, 0, 0
    }
};
""")

attributeEnumEntry = Template("""\
	${attributeEnumName},
""")

attributesEnumsTemplate = Template("""\
enum ENUM_attributes
{
	ATTRIBUTE_RESERVED = 1,
${attributesEnums}
	COR_ATTRIBUTES_MAX
};
""")

classEnumsTemplate = Template("""\
enum ENUM_classes
{
	CLASS_RESERVED=65536,
	${classenums}
	COR_CLASS_MAX
};
""")

classEnumEntryTemplate = Template("""\
	CLASS_${classname}_${suffix} ,
""")


resourceClassTableTemplate = Template("""\

ClCorClassTblT pCorResourceClassTable[]=
{
	/* 
    	Relative depth in the COR Tree , 
 	    Indicates whether it is a base class or an MO class,
    	Pointer  to MO class def, 
    	Maximum no of objects to be allowed , 
    	Pointer  to MSO class definition, 
    	Maximum no of MSO objects to be allowed .        
    */
${dsTableEntries}	
	{ 0, CL_COR_BASE_CLASS, clAlarmBaseClass, 10, NULL, 0},
    	{ 0, CL_COR_BASE_CLASS, alarmInfo, 10, NULL, 0},		
${tableEntries}
	{0, 0, NULL, 0, NULL, 0}
};
""")

componentClassTableTemplate = Template("""\
ClCorClassTblT pCorSAFClassTable[]=
{
    /* 
 	    Relative depth in the COR Tree , 
 	    Indicates whether it is a base class or an MO class,
        Pointer  to MO class def, 
    	Maximum no of objects to be allowed,
    	Pointer  to MSO class definition, 
    	Maximum no of MSO objects to be allowed .        
    */        
   	{0, CL_COR_BASE_CLASS, compArgvContClass, 0,  NULL, 0},
   	{0, CL_COR_BASE_CLASS, compEnvContClass,  0,  NULL, 0},
${tableEntries}
	{0, 0, NULL, 0, NULL, 0}
};
""")


classTableEntryTemplate = Template("""\
	{ ${depth}, ${classType}, ${moClassPointer}, ${maxMOCount}, ${msoClassPointer}, ${maxMSOCount}}, 	
""")

#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
resClassTableEntries = ""
compClassTableEntries = ""

#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
def createClassEnums(resList):
	enumEntries=""	
	for res in resList:
		resName = string.upper(res.attributes["Name"].value)
		enumEntries += classEnumEntryTemplate.safe_substitute(classname=resName, suffix="MO")
		
		if isProvEnabled(res):				
			enumEntries += classEnumEntryTemplate.safe_substitute(classname=resName, suffix="PROV_MSO");					
		
		if isAlarmEnabled(res):
			enumEntries += classEnumEntryTemplate.safe_substitute(classname=resName, suffix="ALARM_MSO");					
	
	return classEnumsTemplate.safe_substitute(classenums = enumEntries)
	

#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
def writeIntoClasstable(res, depth):
	global resClassTableEntries
	resName = res.attributes["Name"].value
#  try block is to catch the error in case of DS which does not have max instances
	try:
		resMaxCount = res.attributes["MaxInstances"].value
	except:
		resMaxCount = 0
	entryMade = False
	symbolMap['depth']=depth
	symbolMap['classType']=getClassType(res)
	symbolMap['moClassPointer']=resName+"MO"
	symbolMap['maxMOCount']=resMaxCount
	symbolMap['msoClassPointer']="NULL"
	symbolMap['maxMSOCount']=resMaxCount
	
	if isProvEnabled(res):			
		symbolMap['msoClassPointer']=resName+"ProvMso"
		symbolMap['maxMSOCount']=resMaxCount
		resClassTableEntries += classTableEntryTemplate.safe_substitute(symbolMap)
		entryMade=True
		
	if isAlarmEnabled(res):			
		symbolMap['msoClassPointer']=resName+"AlarmMso"
		symbolMap['maxMSOCount']=resMaxCount
		resClassTableEntries += classTableEntryTemplate.safe_substitute(symbolMap)
		entryMade=True;			
		
	if entryMade == False:
		symbolMap['msoClassPointer']="NULL"
		symbolMap['maxMSOCount']=0
		resClassTableEntries += classTableEntryTemplate.safe_substitute(symbolMap)

#------------------------------------------------------------------------------
# writes the inheritence base classes in to the table
#------------------------------------------------------------------------------
def writeBaseClassIntoClasstable():
	global parentObjectMap
	baseClassTableEntries = ""
	uniqueBaseList = []
	iterator = parentObjectMap.itervalues()
	for base in iterator:
		if base not in uniqueBaseList:
			uniqueBaseList.append(base)
	for baseObject in uniqueBaseList:
		resName = baseObject.attributes["Name"].value
		entryMade = False
		symbolMap['depth']=0
		symbolMap['classType']="CL_COR_BASE_CLASS"
		symbolMap['moClassPointer']=resName+"MO"
		symbolMap['maxMOCount']=0
		symbolMap['msoClassPointer']="NULL"
		symbolMap['maxMSOCount']=0
		baseClassTableEntries += classTableEntryTemplate.safe_substitute(symbolMap)
	return baseClassTableEntries
#------------------------------------------------------------------------------
#------------------------------------------------------------------------------

def expand(res, depth):
	if res != None:
		writeIntoClasstable(res, depth)
		children = getChildren(res)
		for child in children:			
			expand(child, depth+1);		
			
#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
def getParents(resource):	
	resKey = resource.attributes["CWKEY"].value;		
	parents = []
	for composition in compositionList:
		targetKey = composition.attributes["target"].value		
		if targetKey == resKey:			
			parents.append(getResourceFromKey(composition.attributes["source"].value))			
		
	return parents
	
#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
def getResourceFromKey(key):
    	#list = resourceList + datastructureList	
    	for res in resourceList:				
		resKey = res.attributes["CWKEY"].value;		
		if resKey == key:
			return res
			
	return None

#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
def getChildren(resource):	
	children=[]
	for res in resourceList:
		parents = getParents(res)
		for parent in parents:
			if parent == resource:
				children.append(res);	
		
	return children
	

#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
def getClassType(resource):
	return "CL_COR_MO_CLASS"

#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
def isAlarmEnabled(resource):
	alarmElements=resource.getElementsByTagName("AlarmManagement")		
	if len(alarmElements) > 0:
		if alarmElements[0].attributes["isEnabled"].value == "true":				
			return True
	
	return False

	
#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
def isProvEnabled(resource):
	provElements=resource.getElementsByTagName("Provisioning")	
	if len(provElements) >0:
		if provElements[0].attributes["isEnabled"].value == "true":				
			return True
	
	return False


#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
def createMOClassDefinitions(resList):
	moClassDefinitionEntries=""	
	for res in resList:
		resName = res.attributes["Name"].value
		symbolMap['classid']=string.upper(resName)
		symbolMap['className']=resName
		symbolMap['superClass']=getSuperClass(res.attributes["Name"].value)
		symbolMap['maxInstances']=getCummulativeMaxInstances(res)	
		symbolMap['attrList'] = resName +"MOAttrList"
		symbolMap['arrayAttrList'] = resName +"MOArrAttrList"
		symbolMap['associatedAttrList'] = resName +"AssocAttrList"
		symbolMap['contAttrList'] = resName +"ContAttrList"
		moClassDefinitionEntries += MOclassDefinitionTemplate.safe_substitute(symbolMap)
	
	return moClassDefinitionEntries
	
#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
def getCummulativeMaxInstances(res):
	try:
		maxInstances = res.attributes["MaxInstances"].value
		parents = getParents(res)
		noOfParents = len(parents)
		if noOfParents == 0:		
			return maxInstances
		else:
			return (noOfParents * int(maxInstances))
	except:
		return 0
	return 0
#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
def createMSOClassDefinitions(resList):
	msoClassDefinitionEntries=""	
	for res in resList:
		resName = res.attributes["Name"].value
		symbolMap['className']=resName
		symbolMap['classid']=string.upper(resName)
		symbolMap['maxMsoCount'] = getCummulativeMaxInstances(res)
		
		if isAlarmEnabled(res):
			symbolMap['svc']="Alarm"
			symbolMap['capsvc']="ALARM"
			symbolMap['serviceID']="CL_COR_SVC_ID_ALARM_MANAGEMENT";	
			symbolMap['msoAttrList'] = "alarmMsoAttrList"
			symbolMap['msoArrAttrList'] = "alarmMsoArrAttrList"
			symbolMap['msoAssociatedAttrList'] = "NULL"
			symbolMap['msoContAttrList'] = "alarmMsoContAttrList"			
			msoClassDefinitionEntries += MSOClassDefinitionTemplate.safe_substitute(symbolMap)
			
		if isProvEnabled(res):
			symbolMap['svc']="Prov"
			symbolMap['capsvc']="PROV"
			symbolMap['serviceID']="CL_COR_SVC_ID_PROVISIONING_MANAGEMENT";			
			symbolMap['msoAttrList'] = resName + "ProvAttrList"
			symbolMap['msoArrAttrList'] = resName +"ProvArrAttrList"
			symbolMap['msoAssociatedAttrList'] = "NULL"
			symbolMap['msoContAttrList'] = "NULL"			
			msoClassDefinitionEntries += MSOClassDefinitionTemplate.safe_substitute(symbolMap)
	
	return msoClassDefinitionEntries

#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
def getSuperClass(resource):
	global parentMap
	parent = parentMap.get(resource) 
	if parent != None:
		return "CLASS_" + string.upper(parent) + "_MO"
	else: 
		return "CL_COR_UTILS_UNKNOWN_CLASS"
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
			result[snmpType] = clvsTypeObj[0]
	return result	
#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
def createAttributeClassDefinition(resource):
	global attributesEnums
	ProvattributeEntries=""	
	MOattributeEntries=""	
	resName = resource.attributes["Name"].value
	allattributeList = resource.getElementsByTagName("Attributes")
	provAttributeList = []
	prov = resource.getElementsByTagName("Provisioning")
	if len(prov) >0:
		provAttributeList = prov[0].getElementsByTagName("Attributes")
	for attrib in allattributeList:
		attrName = attrib.attributes["Name"].value
		multiplicity = attrib.attributes["Multiplicity"].value
		symbolMap['className']=resName
		symbolMap['classid']=string.upper(resName)
		symbolMap['attrName']=string.upper(attrib.attributes["Name"].value)
		typeObj = typeMap.get(attrib.attributes["Type"].value)
		if typeObj != None:
			symbolMap['attrType']=string.upper(typeObj.attributes["AttributeType"].value)
			multiplicity = typeObj.attributes["ArraySize"].value
		else:
			symbolMap['attrType']=string.upper(attrib.attributes["Type"].value)
			
		symbolMap['defValue']=attrib.attributes["DefaultValue"].value
		symbolMap['minValue']=attrib.attributes["MinValue"].value
		symbolMap['maxValue']=attrib.attributes["MaxValue"].value
		if multiplicity == "0" or multiplicity == "1":
			if attrib not in provAttributeList:
				MOattributeEntries += AttributeEntryTemplate.safe_substitute(symbolMap)
				EnumName = string.upper(resName) + "_" + string.upper(attrName)
				attributesEnums  += attributeEnumEntry.safe_substitute(attributeEnumName = EnumName)
			else:
				if isProvEnabled(res):
					ProvattributeEntries += AttributeEntryTemplate.safe_substitute(symbolMap)
					EnumName = string.upper(resName) + "_" + string.upper(attrName)
					attributesEnums  += attributeEnumEntry.safe_substitute(attributeEnumName = EnumName)
	
	return AttributeClassTemplate.safe_substitute(className = resName, ProvattributeEntries = ProvattributeEntries, MOattributeEntries = MOattributeEntries)
#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
def createArrayAttributeClassDefinition(resource):
	global attributesEnums
	ProvarrayAttributeEntries=""	
	MOarrayAttributeEntries=""	
	resName = resource.attributes["Name"].value
	allattributeList = resource.getElementsByTagName("Attributes")
	provAttributeList = []
	prov = resource.getElementsByTagName("Provisioning")
	if len(prov) >0:
		provAttributeList = prov[0].getElementsByTagName("Attributes")
	for attrib in allattributeList:
		attrName = attrib.attributes["Name"].value
		multiplicity=attrib.attributes["Multiplicity"].value
		symbolMap['className']=resName
		symbolMap['classid']=string.upper(resName)
		symbolMap['attrName']=string.upper(attrib.attributes["Name"].value)
		typeObj = typeMap.get(attrib.attributes["Type"].value)
		if typeObj != None:
			symbolMap['attrType']=string.upper(typeObj.attributes["AttributeType"].value)
			multiplicity = typeObj.attributes["ArraySize"].value
		else:
			symbolMap['attrType']=string.upper(attrib.attributes["Type"].value)
				
		if multiplicity != "0" and multiplicity != "1":
			symbolMap['arraySize']=multiplicity
			if attrib not in provAttributeList:
				MOarrayAttributeEntries += ArrayAttributeEntryTemplate.safe_substitute(symbolMap)
				EnumName = string.upper(resName) + "_" + string.upper(attrName)
				attributesEnums  += attributeEnumEntry.safe_substitute(attributeEnumName = EnumName)
			else:
				if isProvEnabled(res):
					ProvarrayAttributeEntries += ArrayAttributeEntryTemplate.safe_substitute(symbolMap)
					EnumName = string.upper(resName) + "_" + string.upper(attrName)
					attributesEnums  += attributeEnumEntry.safe_substitute(attributeEnumName = EnumName)

	return ArrayAttributeClassTemplate.safe_substitute(className = resName, ProvarrayAttributeEntries = ProvarrayAttributeEntries, MOarrayAttributeEntries = MOarrayAttributeEntries)
#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
def createAssocAttributeClassDefinition(resource):
	global attributesEnums
	assocAttributeEntries=""
	resName = resource.attributes["Name"].value	
	for assoc in associationList:
		sourceName = getResourceFromKey(assoc.attributes["source"].value).attributes["Name"].value
		if resName == sourceName:
			target = getResourceFromKey(assoc.attributes["target"].value)
			symbolMap['classid']=string.upper(resName)
			symbolMap['attrName']=string.upper(target.attributes["Name"].value)
			symbolMap['multiplicity']=assoc.attributes["Target_Multiplicity"].value
			assocAttributeEntries += AssocAttributeEntryTemplate.safe_substitute(symbolMap)
			EnumName = string.upper(resName) + "_" + string.upper(target.attributes["Name"].value)
                        attributesEnums  += attributeEnumEntry.safe_substitute(attributeEnumName = EnumName)

	return AssocAttributeClassTemplate.safe_substitute(className = resName, assocAttributeEntries = assocAttributeEntries)
#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
def createContAttributeClassDefinition(resource):
	global attributesEnums
	contAttributeEntries=""	
	resName = resource.attributes["Name"].value	
	for cont in compositionList:
		sourceName = getResourceFromKey(cont.attributes["source"].value).attributes["Name"].value
		if resName == sourceName:
			target = getResourceFromKey(cont.attributes["target"].value)
			symbolMap['classid']=string.upper(resName)
			symbolMap['attrName']=string.upper(target.attributes["Name"].value)
			symbolMap['multiplicity']=cont.attributes["Target_Multiplicity"].value
			if isDS(target.attributes["Name"].value) == True:
				contAttributeEntries += ContAttributeEntryTemplate.safe_substitute(symbolMap)
				EnumName = string.upper(resName) + "_" + string.upper(target.attributes["Name"].value)
		      		attributesEnums  += attributeEnumEntry.safe_substitute(attributeEnumName = EnumName)

	return ContAttributeClassTemplate.safe_substitute(className = resName, contAttributeEntries = contAttributeEntries)
#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
def isDS(name):
	for ds in datastructureList:
		dsName = ds.attributes["Name"].value
		if dsName == name:
			return True
	return False		 
#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
def createAttrEnums(resList):
	enumResult = ""
	for res in resList:
		resName = string.upper(res.attributes["Name"].value)
		attrList = res.getElementsByTagName("Attributes")
		for attr in attrList:
			attrName = string.upper(resName) + "_" + string.upper(attr.attributes["Name"].value)
			enumResult  += attributeEnumEntry.safe_substitute(attributeEnumName = attrName)
		
		#getContainedAttributes(res)
		
	return attributesEnumsTemplate.safe_substitute(attributesEnums = enumResult)
#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
def createComponentMOClassDefinitions(compList):
	classDefinitionEntries=""	
	for comp in compList:
		name = comp.attributes["Name"].value				
		symbolMap['classid']=string.upper(name)
		symbolMap['className']=name
		symbolMap['superClass']="CL_COR_UTILS_UNKNOWN_CLASS"
		symbolMap['maxInstances']=10;	
		symbolMap['attrList'] = "NULL"
		symbolMap['arrayAttrList'] = "NULL"
		symbolMap['associatedAttrList'] = "NULL"
		symbolMap['contAttrList'] = "NULL"	
	
		if comp.tagName == "component:Component" or comp.tagName == "component:SAFComponent":
			symbolMap['attrList'] = "cpmCompMoAttrList"
			symbolMap['arrayAttrList'] = "cpmCompMoArrAttrList"
			symbolMap['contAttrList'] = "cpmCompMoContAttrList"
			
		classDefinitionEntries += MOclassDefinitionTemplate.safe_substitute(symbolMap)
	
	return classDefinitionEntries

	

#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
def processInheritanceList():
	global parentMap
	global parentObjectMap
	for conn in inheritanceList:
		sourceKey = conn.attributes["source"].value
		targetKey = conn.attributes["target"].value
		source = getResourceFromKey(sourceKey)
		target = getResourceFromKey(targetKey)
		parentObjectMap[source.attributes["Name"].value]= target
		parentMap[source.attributes["Name"].value] = target.attributes["Name"].value
#------------------------------------------------------------------------------
#------------------------------------------------------------------------------

#Script Execution Starts Here.
#Resource Tree information
attributesEnums = ""
parentMap = dict()
parentObjectMap = dict()
symbolMap = dict()
resourceXml = sys.argv[1]
componentsXml = sys.argv[2]
dataTypeXml = sys.argv[3]

resourceDoc = xml.dom.minidom.parse(resourceXml)
resourceList = resourceDoc.getElementsByTagName("resource:DataStructure") + resourceDoc.getElementsByTagName("resource:ChassisResource") + resourceDoc.getElementsByTagName("resource:HardwareResource") + resourceDoc.getElementsByTagName("resource:SoftwareResource") + resourceDoc.getElementsByTagName("resource:SystemController") + resourceDoc.getElementsByTagName("resource:NodeHardwareResource")
datastructureList = resourceDoc.getElementsByTagName("resource:DataStructure")
compositionList = resourceDoc.getElementsByTagName("resource:Composition")
associationList = resourceDoc.getElementsByTagName("resource:Association")
inheritanceList = resourceDoc.getElementsByTagName("resource:Inheritence")
processInheritanceList()

#chassisRes = getChassisRes(resourceList)
chassisResources = resourceDoc.getElementsByTagName("resource:ChassisResource")
chassisRes = chassisResources[0]
chassisName = chassisRes.attributes["Name"].value
 
srcFileContents = headerTemplate.safe_substitute()
srcFileContents += corCfgTemplate.safe_substitute(chassisName = string.upper(chassisName))    
typeMap  = createDataTypeMap(dataTypeXml)
for res in resourceList:
    srcFileContents += createAttributeClassDefinition(res)
    srcFileContents += createArrayAttributeClassDefinition(res)
    srcFileContents += createAssocAttributeClassDefinition(res)
    srcFileContents += createContAttributeClassDefinition(res)

srcFileContents += createMOClassDefinitions(resourceList)
srcFileContents += createMSOClassDefinitions(resourceList)
expand(chassisRes, 0)
dsTableEntries = writeBaseClassIntoClasstable()      
srcFileContents += resourceClassTableTemplate.safe_substitute(tableEntries = resClassTableEntries, dsTableEntries = dsTableEntries)

#------------------------------------------------------------------------------
#-------------------- Removing Component Tree code generation------------------

out_file = open("clCorMetaStruct.c","w")
out_file.write(srcFileContents)
out_file.close()

#-------------------- Removing Component List for class enum generation--------
headerFileContents = createClassEnums(resourceList)
#headerFileContents += createAttrEnums(resourceList)
headerFileContents += attributesEnumsTemplate.safe_substitute(attributesEnums = attributesEnums)
out_file = open("clCorMetaStruct.h","w")
out_file.write(headerFileContents)
out_file.close()
