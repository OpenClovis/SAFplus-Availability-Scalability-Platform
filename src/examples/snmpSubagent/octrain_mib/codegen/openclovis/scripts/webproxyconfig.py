################################################################################
# ModuleName  : com
# $File$
# $Author$
# $Date$
################################################################################
# Description :
################################################################################
#!/bin/env python
import sys
import string
import os
import xml.dom.minidom
import xml.dom.minidom as dom
import re
from xml.dom.minidom import Node

result = ""
alarmMap = {}
#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
# These mapping are based on the clovisworks mapping in datatypesmap.xmi
# Each value has (attrType, multiplicity) pair
attrBasicTypeMap = \
{\
	'Int8': ('INT8', 1),\
	'Int16': ('INT16', 1),\
	'Int32': ('INT32',1),\
	'Int64': ('INT64',1),\
	'Uint8': ('UINT8',1),\
	'Uint16': ('UINT16',1),\
	'Uint32': ('UINT32',1),\
	'Uint64': ('UINT64',1),\
	'INTEGER': ('INT32',1),\
	'Integer32': ('INT32',1),\
	'Unsigned32': ('UINT32',1),\
	'Gauge32': ('UINT32',1),\
	'Counter32': ('UINT32', 1),\
	'Counter64': ('UINT64', 1),\
	'TimerTicks': ('UINT32', 1),\
	'BITS': ('UINT32', 1),\
	'TruthValue': ('UINT8', 1),\
	'OCTET STRING': ('UINT8',255),\
	'IpAddress': ('UINT8',4),\
	'NetworkAddress': ('UINT8',4),\
	'OPAQUE': ('UINT8',255),\
	'UTF8String': ('UINT8',255),\
	'IA5String': ('UINT8',255),\
	'DisplayString': ('UINT8', 255),\
	'OBJECT IDENTIFIER': ('UINT32', 128),\
}

def getType(type):
#	type = type.upper()
	corType = 'CL_COR_'
	multiplicity = 1

	if attrBasicTypeMap.has_key(type):
		type, multiplicity = attrBasicTypeMap[type]
		corType += type
	else:	
		print "Warning: No matching attribute type for", type
		corType += 'UNDEFINED'
		
	return (corType, multiplicity)

#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
def processAlarmdataList(alarmdata):
	alarm2Dlist = [];
	for alarm in alarmdata:	
		probCause = alarm.attributes["ProbableCause"].value	
		category = alarm.attributes["Category"].value					
		severity = alarm.attributes["Severity"].value
		cwkey = alarm.attributes["rdn"].value
		alarmId = alarm.attributes["alarmID"].value

		alarmDetails = [probCause, category, severity, cwkey, alarmId]		
		alarm2Dlist.append(alarmDetails)				
	return alarm2Dlist
#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
def processResourcedataList(resourcedata):
	result = ""	
	result += '<CLASS_CONFIG>\n'
	resourcelist = []
	id = 65536
	attid = 1
	for resource in resourcedata:
		result += '<CLASS '
		name = resource.attributes["name"].value
		id += 1
		result += 'NAME="%s"' %name
		result += ' ID="%d"'%id
		result += '>\n'
		childnodes = resource.childNodes
		if len(childnodes) > 0:
			attrList = []
			alarmsList = []
			for node in childnodes:				
				if node.nodeName == "attribute":
					attrList.append(node)					
				
				if node.nodeName == "alarmManagement":
					if node.attributes["isEnabled"].value == "true":
						id += 1
					alarmchildnodes = node.childNodes					
					if len(alarmchildnodes) > 0:
						for alarm in alarmchildnodes:
							if alarm.nodeType == Node.ELEMENT_NODE and alarm.nodeName == "alarmIDs":								
								alarmsList.append(alarm.firstChild.data)
				if node.nodeName == "provisioning":
					if node.attributes["isEnabled"].value == "true":
						id += 1
					pronodes = node.childNodes
					if len(pronodes) > 0:
						for pronode in pronodes:
							if pronode.nodeName == "attribute":
								attrList.append(pronode)
						
					
					
			result += '<ATTRIBUTES>\n'

			attrSimple = []
			attrArray = []	
			for attr in attrList:
				multiplicity = attr.attributes["multiplicity"].value					
				if string.atoi(multiplicity) > 1:
					attrArray.append(attr)
				else:
					dummy, multiplicity = getType(attr.attributes["type"].value)
					if multiplicity == 1:
						attrSimple.append(attr)		
					else:
						attrArray.append(attr)

			for sattr in attrSimple:
				result += '<ATTRIBUTE '
				attname = sattr.attributes["name"].value
				attid += 1
				type, multiplicity = getType(sattr.attributes["type"].value)
				if multiplicity == 1:				
					attrType = 'CL_COR_SIMPLE_ATTR'					
				else:
					attrType = 'CL_COR_ARRAY_ATTR'
				result += 'NAME="%s"' %attname
				result += ' ID="%d"' %attid
				result += ' ATTRTYPE="%s"' %attrType
				result += ' TYPE="%s"' %type
				result += '></ATTRIBUTE>\n'

			for aattr in attrArray:
				result += '<ATTRIBUTE '
				attname = aattr.attributes["name"].value
				attid += 1
				type, multiplicity = getType(aattr.attributes["type"].value)				
				attrType = 'CL_COR_ARRAY_ATTR'					
				result += 'NAME="%s"' %attname
				result += ' ID="%d"' %attid
				result += ' ATTRTYPE="%s"' %attrType
				result += ' TYPE="%s"' %type
				result += '></ATTRIBUTE>\n'

			result += '</ATTRIBUTES>\n'

			result += '<ALARMS>\n'
			
			for alarm in alarmsList:						
				if alarmMap.has_key(alarm):
					result += '<ALARM>%s</ALARM>\n' %alarmMap[alarm]
			result += '</ALARMS>\n'
		result += '</CLASS>\n'
	result += '</CLASS_CONFIG>\n'
	return result


maxInstRegExp = re.compile(r'^\s*\{\"(.*)\",\s*CLASS_(.*)_MO,\s*CL_COR_UTILS_UNKNOWN_CLASS,\s*(\d+)', re.MULTILINE)
classRegExp   = re.compile(r'^	\{ (\d+), CL_COR_MO_CLASS, (.*)MO')

def genTreeFromMetaStruct(aspPath, modelName):

	try:
		fName = '%s/models/%s/config/clCorMetaStruct.c' % (aspPath, modelName)
		metaStructFile = open(fName)
	except IOError:
		print 'Error: File %s not found' % fName
		print 'Exiting...'
		sys.exit(-1)
	
	maxInstMap = {}
	doc = dom.getDOMImplementation().createDocument(None, 'TREE_CONFIG', None)
	classes = {}

	
	str = metaStructFile.read()
	matches = maxInstRegExp.findall(str)
	for className, dummy, numInst in matches:
		maxInstMap[className] = numInst

	for line in str.splitlines():
		match = classRegExp.search(line)
		if not match:
			continue
		
		depth, className = match.groups()
		depth = int(depth)
		if className == classes.get(depth, [''])[0]:
			continue
		
		element = doc.createElement('CLASS')
		element.setAttribute('NAME', className)
		element.setAttribute('MAXINST', maxInstMap[className])
		classes[depth] = (className, element)

		if depth == 0:
			doc.documentElement.appendChild(element)
			continue
			
		parentName, parent = classes[depth - 1]
		if not parent.firstChild:
			parent.appendChild(doc.createElement('CHILDREN'))
		parent.firstChild.appendChild(element)
	
	str = '\n'.join(doc.toprettyxml('  ').splitlines()[1:])
	doc.unlink()		
	metaStructFile.close()
	return str
#------------------------------------------------------------------------------
#------------------------------------------------------------------------------

#Script Execution Starts Here.

if len(sys.argv) != 4:
	print 'Usage:WebProxyConfig.py <CW Workspace Path> <ASP Installation Path> <Model Name>'
	sys.exit(0)

#alarmdataXmi = sys.argv[1]+'/'+sys.argv[3]+'/models/alarmdata.xml'
#resourcedataXmi = sys.argv[1]+'/'+sys.argv[3]+'/models/resourcedata.xml'
alarmdataXmi = sys.argv[1]+'/models/alarmdata.xml'
resourcedataXmi = sys.argv[1] +'/models/resourcedata.xml'


output = open(sys.argv[2]+'/models/'+sys.argv[3]+'/config/WebProxyConfig.xml', 'w')

result += '<?xml version="1.0" encoding="UTF-8"?>\n'
result += '<CONFIG>\n'

if os.path.exists(alarmdataXmi) and os.path.exists(resourcedataXmi):	
	resourcedataDoc = xml.dom.minidom.parse(resourcedataXmi)
	alarmdataDoc = xml.dom.minidom.parse(alarmdataXmi)

	resourceInfoObj = resourcedataDoc.getElementsByTagName("resource:resourceInformation")
	resourcedataList = resourceInfoObj[0].getElementsByTagName("chassisResource") + resourceInfoObj[0].getElementsByTagName("hardwareResource") + resourceInfoObj[0].getElementsByTagName("softwareResource") + resourceInfoObj[0].getElementsByTagName("systemController") + resourceInfoObj[0].getElementsByTagName("nodeHardwareResource")
	alarmdataList = alarmdataDoc.getElementsByTagName("alarm:AlarmProfile")


	alarmdata2D = processAlarmdataList(alarmdataList)
	result += '<ALARM_CONFIG>\n'
	alarmid = 99
	for x in alarmdata2D:
		alarmid +=1
		result += '<ALARM_TYPE '
		result += 'NAME="%s"' % x[0].split('CL_ALARM_PROB_CAUSE_')[1]
		result += ' CATEGORY="%s"' % x[1][18:]
		result += ' SEVERITY="%s"'%x[2][18:]
		result += ' ID="%d"' %alarmid
		result += '></ALARM_TYPE>\n'
		alarmMap[x[3]] = alarmid
	result += '</ALARM_CONFIG>\n'



	result += processResourcedataList(resourcedataList)
	result += genTreeFromMetaStruct(sys.argv[2], sys.argv[3]) 
	result += '\n</CONFIG>'

	output.write(result)
	output.close()

