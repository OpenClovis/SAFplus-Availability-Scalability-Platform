################################################################################
# ModuleName  : SampleModel
# $File$
# $Author$
# $Date$
################################################################################
# Description :
################################################################################
#!/bin/env python

import sys
# Python2.3 or above is required for generating xml with encoding
reqVersion = (2,3,0)
if sys.version_info[:3] < reqVersion:
	print 'Python version ' + '.'.join(map(str,reqVersion)) + ' or above is required.'
	print 'Available version is ' + '.'.join(map(str,sys.version_info[:3]))
	print '\nPlease install the required version of python.'
	print 'If already installed please include the python bin directory into the PATH variable.'
	sys.exit(1) 

import xml.dom.minidom as dom
import os
import os.path

AMF_DEF_FILE  = 'amfDefinitions.xml'
AMF_CONF_FILE = 'amfConfig.xml'

sgTypeConfStr = """
    <sgType name="WebProxySG">
      <adminState>CL_AMS_ADMIN_STATE_UNLOCKED</adminState>
      <failbackOption>CL_FALSE</failbackOption>
      <restartDuration>5000</restartDuration>
      <numPrefActiveSUs>1</numPrefActiveSUs>
      <numPrefStandbySUs>1</numPrefStandbySUs>
      <numPrefInserviceSUs>2</numPrefInserviceSUs>
      <numPrefAssignedSUs>2</numPrefAssignedSUs>
      <numPrefActiveSUsPerSI>1</numPrefActiveSUsPerSI>
      <maxActiveSIsPerSU>1</maxActiveSIsPerSU>
      <maxStandbySIsPerSU>0</maxStandbySIsPerSU>
      <compRestartDuration>10000</compRestartDuration>
      <compRestartCountMax>3</compRestartCountMax>
      <suRestartDuration>20000</suRestartDuration>
      <suRestartCountMax>4</suRestartCountMax>
    </sgType>

"""

suTypeDefStr = """
	<suType name="WebProxySU">
      <adminState>CL_AMS_ADMIN_STATE_UNLOCKED</adminState>
      <rank>100</rank>
      <numComponents>1</numComponents>
      <isPreinstantiable>CL_TRUE</isPreinstantiable>
      <isRestartable>CL_TRUE</isRestartable>
      <isContainerSU>CL_FALSE</isContainerSU>
    </suType>

"""

siTypeConfStr = """
    <siType name="WebProxySI">
      <adminState>CL_AMS_ADMIN_STATE_UNLOCKED</adminState>
      <rank>4</rank>
      <numCSIs>1</numCSIs>
      <numStandbyAssignments>1</numStandbyAssignments>
    </siType>

"""	

compTypeDefStr = """
    <compType name="WebProxyComp">
      <property>CL_AMS_SA_AWARE</property>
      <processRel>CL_CPM_COMP_SINGLE_PROCESS</processRel>
      <instantiateCommand>asp_webProxy</instantiateCommand>
      <args/>
      <envs/>
      <terminateCommand></terminateCommand>
      <cleanupCommand></cleanupCommand>
      <timeouts>
        <instantiateTimeout>10000</instantiateTimeout>
        <terminateTimeout>10000</terminateTimeout>
        <cleanupTimeout>0</cleanupTimeout>
        <amStartTimeout>0</amStartTimeout>
        <amStopTimeout>0</amStopTimeout>
        <quiescingCompleteTimeout>10000</quiescingCompleteTimeout>
        <csiSetTimeout>10000</csiSetTimeout>
        <csiRemoveTimeout>10000</csiRemoveTimeout>
        <proxiedCompInstantiateTimeout>0</proxiedCompInstantiateTimeout>
        <proxiedCompCleanupTimeout>0</proxiedCompCleanupTimeout>
      </timeouts>
        <csiType>WebProxyCSIType</csiType>
    </compType>

"""

csiTypeConfStr = """
    <csiType name="WebProxyCSI">
      <type>WebProxyCSIType</type>
      <nameValueLists/>
      <rank>0</rank>
    </csiType>
"""

nodeConfStr = """
        <serviceUnitInstance name="WebProxySU" type="WebProxySU">
          <componentInstances>
            <componentInstance name="WebProxyComp" type="WebProxyComp"/>
          </componentInstances>
        </serviceUnitInstance>	

"""		
sgConfStr = """
    <serviceGroup name="WebProxySG" type="WebProxySG">
      <serviceInstances>
        <serviceInstance name="WebProxySI" type="WebProxySI">
          <componentServiceInstances>
            <componentServiceInstance name="WebProxyCSI" type="WebProxyCSI"/>
          </componentServiceInstances>
        </serviceInstance>
      </serviceInstances>
      <associatedServiceUnits>
        <associatedServiceUnit>WebProxySU</associatedServiceUnit>
      </associatedServiceUnits>
    </serviceGroup>

"""

sgTypeConf = dom.parseString(sgTypeConfStr)
suTypeDef = dom.parseString(suTypeDefStr)
siTypeConf = dom.parseString(siTypeConfStr)
compTypeDef = dom.parseString(compTypeDefStr)
csiTypeConf = dom.parseString(csiTypeConfStr)

nodeConf = dom.parseString(nodeConfStr)
sgConf = dom.parseString(sgConfStr)

def warn(str):
	sys.stderr.write('Warning: %s\n' % str);

def error(str):
	sys.stderr.write('Error: %s\n' % str);

def removeElement(parentElem, **args):
	if not args.has_key('tag'):
		return
	tag = args['tag']
	del args['tag']
	elements = parentElem.getElementsByTagName(tag)

	delElem = False
	for key,value in args.iteritems():
		for element in elements:
			attrVal = element.getAttribute(key)
			if attrVal and attrVal == value:
				print 'Found', key, '=', value
				delElem = True
				parentElem.removeChild(element)
				break
		if delElem: break		

def insertIntoAmfDefinitions(path):
	defFileName = path + '/' + AMF_DEF_FILE
	if not os.path.exists(defFileName):
		error('amfDefinitions.xml not present in '+ path)
		return

	doc = dom.parse(defFileName)
	
	sgTypes = doc.getElementsByTagName('sgTypes')
	if sgTypes.length == 0:
		error('sgTypes not defined. Invalid definitions file.')
		return
	removeElement(sgTypes[0], tag = 'sgType', name = 'WebProxySG')
	sgTypes[0].appendChild(sgTypeConf.documentElement.cloneNode(1))
	
	suTypes = doc.getElementsByTagName('suTypes')
	if suTypes.length == 0:
		error('suTypes not defined. Invalid definitions file.')
		return 
	removeElement(suTypes[0], tag = 'suType', name = 'WebProxySU')
	suTypes[0].appendChild(suTypeDef.documentElement.cloneNode(1))	

	siTypes = doc.getElementsByTagName('siTypes')
	if siTypes.length == 0:
		error('siTypes not defined. Invalid definitions file.')
		return
	removeElement(siTypes[0], tag = 'siType', name = 'WebProxySI')
	siTypes[0].appendChild(siTypeConf.documentElement.cloneNode(1))
	
	compTypes = doc.getElementsByTagName('compTypes')
	if compTypes.length == 0:
		error('compTypes not defined. Invalid definitions file.')
		return
	removeElement(compTypes[0], tag = 'compType', name = 'WebProxyComp')
	compTypes[0].appendChild(compTypeDef.documentElement.cloneNode(1))

	csiTypes = doc.getElementsByTagName('csiTypes')
	if csiTypes.length == 0:
		error('csiTypes not defined. Invalid definitions file.')
		return
	removeElement(csiTypes[0], tag = 'csiType', name = 'WebProxyCSI')
	csiTypes[0].appendChild(csiTypeConf.documentElement.cloneNode(1))
		
	#print doc.toxml()
	print 'Taking bakup for %s' % defFileName
	os.system('mv -f %s %s.bak' % (defFileName, defFileName)) 

	print 'Writing new %s' % defFileName
	defFile = open(defFileName, 'w')
	defFile.write(doc.toxml('UTF-8'))
	defFile.close()
	doc.unlink()

def insertIntoAmfConfig(path):
	confFileName = path + '/' + AMF_CONF_FILE
	if not os.path.exists(confFileName):
		error('amfConfi.xml not present in '+path)
		return

	doc = dom.parse(confFileName)

	cpmConfs = doc.getElementsByTagName('cpmConfig')
	nodeType = ''
	# Get the first global node type, where webProxy is to be inserted
	for cpmConf in cpmConfs:
		if cpmConf.getAttribute('cpmType') == 'GLOBAL':
			nodeType = cpmConf.getAttribute('nodeType')
			break
			#bootLevels = cpmConf.getElementsByTagName('bootLevels')
			#if bootLevels.length == 0:
			#	warn('No \'bootLevels\' tag found under cpmConfig. Config file may have problems.')
			#bootLevels[0].appendChild(bootConf.documentElement.cloneNode(1))
				
	if nodeType == '':
		error('No CPM with type GLOBAL. Webproxy boot level not added')
		return

	nodeInstances = doc.getElementsByTagName('nodeInstance')
	done = False
	for nodeInstance in nodeInstances:
		if nodeInstance.getAttribute('type') == nodeType:
			suInstances = nodeInstance.getElementsByTagName('serviceUnitInstances')
			if suInstances.length == 0:
				error('serviceUnitInstances tag not present under GLOBAL nodeInstance.Invalid config file.')
				return
	
			removeElement(suInstances[0], tag = 'serviceUnitInstance', name = 'WebProxySU')
			suInstances[0].appendChild(nodeConf.documentElement.cloneNode(1))
			done = True
			break
	
	if not done:
		error(nodeType + ' not found under nodeInstances tag. Invalid config file.')
		return

	# Add Webproxy SG to the service groups tag	
	sgs = doc.getElementsByTagName('serviceGroups')
	if sgs.length == 0:
		error('\'serviceGroups\' tag not defined. Invalid config file.')
		return
	removeElement(sgs[0], tag = 'serviceGroup', name = 'WebProxySG')
	sgs[0].appendChild(sgConf.documentElement.cloneNode(1))
		
	print 'Taking bakup for %s' % confFileName
	os.system('mv -f %s %s.bak' % (confFileName, confFileName)) 

	print 'Writing new %s' % confFileName
	confFile = open(confFileName, 'w')
	confFile.write(doc.toxml('UTF-8'))
	confFile.close()	

	doc.unlink()

if __name__ == '__main__':
	if len(sys.argv) != 3:
		print 'Syntax: amfconfig_webproxy.py <ASP Installation Path> <Model Name>\n'
		sys.exit(0);
	
	modelPath = sys.argv[1]+'/models/'+sys.argv[2]+'/config'
	insertIntoAmfDefinitions(modelPath)
	insertIntoAmfConfig(modelPath)
	
