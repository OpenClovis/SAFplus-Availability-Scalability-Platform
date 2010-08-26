# Python includes
import shutil
import sys
import types
import os
import os.path
from string import Template

  # Add include paths
myDir = os.path.dirname(__file__)
if not myDir: myDir = os.getcwd()
sys.path.append("%s/../../common" % myDir)
sys.path.append("%s/../.." % myDir)
#sys.path.append("%s/../templates" % myDir)

TemplatePath = myDir + "/../templates/" 

# Library includes
import kid

# Local includes

from common import *



class Output:
        """This abstract class defines how the generated output is written to the file system
           or tarball.

           In these routines if you tell it to write a file or template to a nonexistant path
           then that path is created.  So you no longer have to worry about creating directories.
           If you call writes("/foo/bar/yarh/myfile.txt","contents") then the directory hierarchy /foo/bar/yarh is automatically created.  The file "myfile.txt" is created and written with "contents".
        """
	def write(self, filename, data):
		if type(data) in types.StringTypes:
			self.writes(filename, data)
		else: self.writet(filename,data)
				
	def writet(self, filename, template):
		pass
	def writes(self, filename, template):
		pass


class FilesystemOutput(Output):
        """Write to a normal file system"""
        
	def writet(self, filename, template):
		if not os.path.exists(os.path.dirname(filename)):
			os.makedirs(os.path.dirname(filename))			
		template.write(filename, "utf-8",False,"xml")
		
	def writes(self, filename, string):
		if not os.path.exists(os.path.dirname(filename)):
			os.makedirs(os.path.dirname(filename))			
		f = open(filename,"w")
		f.write(string)
		f.close()

class DictionaryOutput(Output):
        """Write to a dictionary, where the key is the path+filename and the value the file contents"""
	def __init__(self):
		self.dict = {}
		
	def writet(self, filename, template):
		self.dict[filename] = template.serialize("utf-8",False,"xml")
		
	def writes(self, filename, string):
		self.dict[filename] = string		



class AmfClassDef:
	def __init__(self, clstype, typeattrs, instattrs={}, dn=None):
		self.clstype = clstype
		self.name = clstype.name
		self.typeattrs = typeattrs
		self.instattrs = instattrs
		self.dn = dn
		
class AmfObjectDef:
	def __init__(self, objtype, clstype, dn):
		self.objtype = objtype
		self.name = objtype.name
		self.clstype = clstype
		self.instattrs = self.getinstattrs(clstype.instattrs)
		self.dn = dn
	def getinstattrs(self, attrs):
		iattrs = {}
		for name,value in attrs.items():
			iattrs[name] = value
		return iattrs
					
class AmfAssociation:
	def __init__(self, clsname, dn, attrs={}):
		self.clsname = clsname
		self.dn = dn
		self.attrs = attrs
		
class AttrDef:
	def __init__(self, attrdef):
		self.attrdef = attrdef
		self.name = attrdef.name
		self.type = attrdef.dataType
		self.category = "SA_" + attrdef.type
		self.defaultvalue = attrdef.defaultValue
		self.flags = self.getflags(attrdef)
			
	def getflags(self, attrdef):
		flags = []
		if attrdef.persistency == 'true':
			flags.append("SA_PERSISTENT")
		if attrdef.caching == 'true':
			flags.append("SA_CACHED")
		if attrdef.writable == 'true':
			flags.append("SA_WRITABLE")
		if attrdef.initialized == 'true':
			flags.append("SA_INITIALIZED")									
		return flags
			
class ClassDef:
	def __init__(self,classdef):
		self.classdef = classdef
		self.name = classdef.name
		self.prov = classdef.getElementsByTagName('provisioning')[0]
		self.attrdefs = self.getattrdefs(self.prov)
		self.category = self.getcategory()
		
	def getcategory(self):
		if len(self.attrdefs) == 0:
			return 'SA_CONFIG'
		category = 'SA_RUNTIME'
		for attrdef in self.attrdefs:
			if attrdef.category == 'SA_CONFIG':
				return 'SA_CONFIG'
		return category
						
	def getattrdefs(self, prov=None):
		attrdefs = []
		attributes = []
		if prov==None:
			attributes = self.classdef.getElementsByTagName('attribute')
		else:
			attributes = self.prov.getElementsByTagName('attribute')
		for attribute in attributes:
			attrdefs.append(AttrDef(attribute))
		return attrdefs

def getNodeInstAttrList(node):
	attrlist = {}
	attrlist['saAmfNodeSuFailoverMax'] = node.suFailoverCountMax.data_
	attrlist['saAmfNodeSuFailOverProb'] = node.suFailoverDuration.data_
	return attrlist
	
def getSGTypeAttrList(sg):
	attrlist = {}
	attrlist['saAmfSgtRedundancyModel'] = redundancy[sg.redundancyModel.data_]
	attrlist['saAmfSgtValidSuTypes'] = 'safVersion=' + safVersion + ',safSuType='
	attrlist['saAmfSgtDefAutoAdjustProb'] = sg.autoAdjustProbation.data_
	attrlist['saAmfSgtDefCompRestartProb'] = sg.compRestartDuration.data_
	attrlist['saAmfSgtDefCompRestartMax'] = sg.compRestartCountMax.data_
	attrlist['saAmfSgtDefSuRestartProb'] = sg.suRestartDuration.data_
	attrlist['saAmfSgtDefSuRestartMax'] = sg.suRestartCountMax.data_
	return attrlist
	
def getSGInstaceAttrList(sg):
	attrlist = {}
	attrlist['saAmfSGType'] = 'safVersion=' + safVersion + ',safSgType=' + sg.name
	attrlist['saAmfSGSuHostNodeGroup'] = 'safAmfNodeGroup=AllNodes,safAmfCluster=' + amfcluster
	attrlist['saAmfSGAutoRepair'] = flag[str(sg.autoRepair.data_)]
	attrlist['saAmfSGAutoAdjust'] = flag[str(sg.autoAdjust.data_)]
	attrlist['saAmfSGNumPrefInserviceSUs'] = sg.numPrefInserviceSUs.data_
	attrlist['saAmfSGNumPrefAssignedSUs'] = sg.numPrefAssignedSUs.data_
	attrlist['saAmfSGMaxActiveSIsperSUs'] = sg.maxActiveSIsPerSU.data_
	attrlist['saAmfSGMaxStandbySIsperSUs'] = sg.maxStandbySIsPerSU.data_
	return attrlist

def getAppTypeAttrList(app):
	attrlist = {}
	attrlist['saAmfApptSGTypes'] = 'safVersion=' + safVersion + ',safSgType=' + app.name
	return attrlist

def getAppInstAttrList(app):
	attrlist = {}
	attrlist['saAmfAppType'] = 'safVersion=' + safVersion + ',safAppType=' + app.name
	return attrlist
		
def getSUTypeAttrList(su):
	attrlist = {}
	attrlist['saAmfSutIsExternal'] = ['0']
	attrlist['saAmfSutDefSUFailover'] = ['1']
	attrlist['saAmfSutProvidesSvcTypes'] = []
	return attrlist

def getSUInstAttrList(su):
	attrlist = {}
	attrlist['saAmfSUType'] = 'safVersion=' + safVersion + ',safSuType=' + su.name
	attrlist['saAmfSUHostNodeOrNodeGroup'] = 'safAmfNode=,safAmfCluster=' + amfcluster
	attrlist['saAmfSUFailover'] = '1'
	attrlist['saAmfSUAdminState'] = adminstate[su.adminState.data_]
	return attrlist
		
def getCompTypeAttrList(comp):
	attrlist = {}
	attrlist['saAmfCtCompCategory'] = ['1']
	attrlist['saAmfCtSwBundle'] = ['safBundle=' + modelname]
	attrlist['saAmfCtDefClcCliTimeout'] = ['10000000000']
	attrlist['saAmfCtDefCallbackTimeout'] = ['10000000000']
	attrlist['saAmfCtRelPathInstantiateCmd'] = [comp.name + '_script']
	attrlist['saAmfCtDefInstantiateCmdArgv'] = ['instantiate']
	attrlist['saAmfCtRelPathCleanupCmd'] = [comp.cleanupCommand.data_]
	attrlist['saAmfCtDefCleanupCmdArgv'] = ['cleanup']
	attrlist['saAmfCtDefRecoveryOnError'] = [recovery[comp.recoveryOnTimeout.data_]]
	attrlist['saAmfCtDefDisableRestart'] = [flag[str(comp.isRestartable.data_)]]
	return attrlist

def getHealthcheckTypeAttrList(healthcheck):
	attrlist = {}
	attrlist['saAmfHctDefPeriod'] = str(healthcheck.period.data_) + '000000'
	attrlist['saAmfHctDefMaxDuration'] = str(healthcheck.maxDuration.data_) + '000000'
	return attrlist
		
def getCompInstAttrList(comp):
	attrlist = {}
	attrlist['saAmfCompType'] = ['safVersion=' + safVersion + ',safCompType=' + comp.name]
	attrlist['saAmfCompInstantiateCmdArgv'] = ['instantiate']
	return attrlist

def getSIInstAttrList(si):
	attrlist = {}
	attrlist['saAmfSvcType'] = 'safVersion=' + safVersion + ',safSvcType=' + si.name
	attrlist['saAmfSIProtectedbySG'] = 'safSg=,safApp='
	return attrlist

def getCSIInstAttrList(csi):
	attrlist = {}
	attrlist['saAmfCSType'] = 'safVersion=' + safVersion + ',safCSType=' + csi.name
	return attrlist
														
def mainMethod(srcDir, codeGenModeDir, prjName, model, output=None):
	global safVersion
	global redundancy
	global flag
	global recovery
	global adminstate
	global scnodename
	global clmcluster
	global amfcluster
	global modelname

	if output==None: output = FilesystemOutput()
	
	modelname = prjName
	
	amfcluster = 'myAmfCluster'
	clmcluster = 'myClmCluster'
	
	capability = {'':'1','CL_AMS_COMP_CAP_X_ACTIVE_OR_Y_STANDBY':'2','CL_AMS_COMP_CAP_ONE_ACTIVE_OR_X_STANDBY':'3','CL_AMS_COMP_CAP_ONE_ACTIVE_OR_ONE_STANDBY':'4','CL_AMS_COMP_CAP_X_ACTIVE':'5','CL_AMS_COMP_CAP_ONE_ACTIVE':'6','CL_AMS_COMP_CAP_NON_PREINSTANTIABLE':'7'}
	redundancy = {'CL_AMS_SG_REDUNDANCY_MODEL_NO_REDUNDANCY':'5','CL_AMS_SG_REDUNDANCY_MODEL_TWO_N':'1','CL_AMS_SG_REDUNDANCY_MODEL_M_PLUS_N':'2','CL_AMS_SG_REDUNDANCY_MODEL_CUSTOM':'3'}
	flag = {'True':'1','False':'0','CL_TRUE':'1','CL_FALSE':'0'}
	recovery = {'CL_AMS_RECOVERY_NO_RECOMMENDATION':'1','CL_AMS_RECOVERY_COMP_RESTART':'2','CL_AMS_RECOVERY_COMP_FAILOVER':'3','CL_AMS_RECOVERY_NODE_SWITCHOVER':'4','CL_AMS_RECOVERY_NODE_FAILOVER':'5','CL_AMS_RECOVERY_NODE_FAILFAST':'6','CL_AMS_RECOVERY_CLUSTER_RESET':'7'}
	adminstate = {'CL_AMS_ADMIN_STATE_UNLOCKED':'1','CL_AMS_ADMIN_STATE_LOCKED_A':'2','CL_AMS_ADMIN_STATE_LOCKED_I':'3'}
	subst = {}
	safVersion = '4.0.0'	
	subst['safVersion'] = safVersion
	subst['modelname'] = modelname
	
	# class definitions
	classdefs = []
	#noderesources = model.filterByTag("nodeHardwareResource")
	#for resource in noderesources:
	#	classdefs.append(ClassDef(resource))
	#softresources = model.filterByTag("softwareResource")
	#for resource in softresources:
	#	classdefs.append(ClassDef(resource))		        	
	
	subst['classdefs'] = classdefs
		
	# amf definitions
	nodetypemap = {}
	sgtypemap = {}
	apptypemap = {}
	sutypemap  = {}
	comptypemap = {}
	sitypemap = {}
	csitypemap = {}
	
	nodetypes = []
	sgtypes = []
	apptypes = []
	sutypes = []
	comptypes = []
	healthchecktypes = []
	sitypes = []
	csitypes = []
	
	comp_capability_map = {}
	for nodetype in model.filterByTag("nodeType"):
		typeattrs = {}
		instattrs = getNodeInstAttrList(nodetype)
		if nodetype.classType.data_ == "CL_AMS_NODE_CLASS_B":
			scnodename = nodetype.name
		nodetypedef = AmfClassDef(nodetype, typeattrs, instattrs)
		nodetypemap[nodetypedef.name] = nodetypedef
		nodetypes.append(nodetypedef)
						
	for sgtype in model.filterByTag("sgType"):
		typeattrs = getSGTypeAttrList(sgtype)
		instattrs = getSGInstaceAttrList(sgtype)
		sgtypedef = AmfClassDef(sgtype, typeattrs, instattrs)
		sgtypemap[sgtypedef.name] = sgtypedef
		sgtypes.append(sgtypedef)
		apptypedef = AmfClassDef(sgtype, getAppTypeAttrList(sgtype), getAppInstAttrList(sgtype))
		apptypemap[apptypedef.name] = apptypedef
		apptypes.append(apptypedef)
		
	for sutype in model.filterByTag("suType"):
		typeattrs = getSUTypeAttrList(sutype)
		instattrs = getSUInstAttrList(sutype)
		sutypedef = AmfClassDef(sutype, typeattrs, instattrs)
		sutypemap[sutypedef.name] = sutypedef
		sutypes.append(sutypedef)
	
	comp_csi_type_map = {}
	for comptype in model.filterByTag("compType"):
		typeattrs = getCompTypeAttrList(comptype)
		instattrs = getCompInstAttrList(comptype)
		comp_capability_map[comptype.name] = capability[comptype.capabilityModel.data_]
		comptypedef = AmfClassDef(comptype, typeattrs, instattrs)
		comptypemap[comptypedef.name] = comptypedef
		comptypes.append(comptypedef)
		comp_csi_type_map[comptype.name] = []
		healthcheck = comptype.filterByTag('healthCheck')[0]
		healthchecktypeattrlist = getHealthcheckTypeAttrList(healthcheck)
		dn = 'safHealthcheckKey=' + comptypedef.name + ',safVersion=' + safVersion + ',safCompType=' + comptypedef.name
		healthcheckdef = AmfClassDef(comptype, healthchecktypeattrlist, {}, dn)
		healthchecktypes.append(healthcheckdef)
		
	for sitype in model.filterByTag("siType"):
		typeattrs = {}
		instattrs = getSIInstAttrList(sitype)
		sitypedef = AmfClassDef(sitype, typeattrs, instattrs)
		sitypemap[sitypedef.name] = sitypedef
		sitypes.append(sitypedef)
		
	for csitype in model.filterByTag('amfDefinitions:amfTypes')[0].get('csiTypes').children():
		typeattrs = {}
		instattrs = getCSIInstAttrList(csitype)
		csitypedef = AmfClassDef(csitype, typeattrs, instattrs)
		csitypemap[csitypedef.name] = csitypedef
		csitypes.append(csitypedef)

	subst['sgtypes'] = sgtypes
	subst['apptypes'] = apptypes
	subst['sutypes'] = sutypes
	subst['comptypes'] = comptypes
	subst['healthchecktypes'] = healthchecktypes
	subst['sitypes'] = sitypes
	subst['csitypes'] = csitypes
			
	# amf config
	nodeinstances = []
	sginstances = []
	appinstances = []
	siinstances = []
	csiinstances = []
	suinstances = []
	compinstances = []
	
	plnodes = []
	scnodes = []
	
	su_sg_map = {}
	
	if len(model.filterByTag("serviceGroups")) > 0:
		for sg in model.filterByTag("serviceGroups")[0].children():
			sgdn = 'safSg=' + sg.name + ',safApp=' + sg.name
			sginstances.append(AmfObjectDef(sg, sgtypemap[sg.type], sgdn))
			appinstances.append(AmfObjectDef(sg, apptypemap[sg.type], 'safApp=' + sg.name))
			for si in sg.children()[0].children():
				sidn = 'safSi=' + si.name + ',safApp=' + sg.name
				siobj = AmfObjectDef(si, sitypemap[si.type], sidn)
				siobj.instattrs['saAmfSIProtectedbySG'] = 'safSg=' + sg.name + ',safApp=' + sg.name
				siinstances.append(siobj)
				for csi in si.children()[0].children():
					csidn = 'safCsi=' + csi.name + ',safSi=' + si.name + ',safApp=' + sg.name
					csiinstances.append(AmfObjectDef(csi, csitypemap[csi.type], csidn))
			for child in sg.filterByTag('associatedServiceUnits')[0].children():
				su_sg_map[child.data_] = sg.name
				
	comp_dn_map = {}
	for node in model.filterByTag("nodeInstance"):
		nodedn = 'safAmfNode=' + node.name + ',safAmfCluster=' + amfcluster
		nodeinstances.append( AmfObjectDef(node, nodetypemap[node.type], nodedn))
		if node.type == scnodename:
			scnodes.append(node.name)
		else:
			plnodes.append(node.name)			 
		for su in node.children()[0].children():
			sudn = 'safSu=' + su.name + ',safSg=' + su_sg_map[su.name] + ',safApp=' + su_sg_map[su.name]
			suobj = AmfObjectDef(su, sutypemap[su.type], sudn)
			suobj.instattrs['saAmfSUHostNodeOrNodeGroup']='safAmfNode=' + node.name + ',safAmfCluster=' + amfcluster
			suinstances.append(suobj)
			for comp in su.children()[0].children():
				compdn = 'safComp=' + comp.name + ',safSu=' + su.name + ',safSg=' + su_sg_map[su.name] + ',safApp=' + su_sg_map[su.name]
				comp_dn_map[comp.name] = compdn
				compinstances.append(AmfObjectDef(comp, comptypemap[comp.type], compdn))
	
	subst['scnodes'] = scnodes
	subst['plnodes'] = plnodes
	subst['nodeinstances'] = nodeinstances					
	subst['sginstances'] = sginstances
	subst['appinstances'] = appinstances		
	subst['siinstances'] = siinstances
	subst['csiinstances'] = csiinstances
	subst['nodeinstances'] = nodeinstances
	subst['suinstances'] = suinstances
	subst['compinstances'] = compinstances
	
	associationobjects = []
	
	sg_si_type_map = {}
		
	compmodel = model.filterByTag("component:componentInformation")[0]
	for sg in compmodel.filterByTag('serviceGroup'):
		sg_si_type_map[sg.name] = []
		for con in sg.filterByTag('associatedTo'):
			if sitypemap.has_key(con.target):
				si = sitypemap[con.target]
				sg_si_type_map[sg.name].append(si.name)
		for con in sg.filterByTag('associatedTo'):
			if sutypemap.has_key(con.target):
				su = sutypemap[con.target]
				silist = sg_si_type_map[sg.name]
				for si in silist: 
					su.typeattrs['saAmfSutProvidesSvcTypes'].append('safVersion=' + safVersion + ',safSvcType=' + si)				
				sgtypemap[sg.name].typeattrs['saAmfSgtValidSuTypes']='safVersion=' + safVersion + ',safSuType=' + con.target

	for su in compmodel.filterByTag('serviceUnit'):
		for con in su.filterByTag('contains'):
			clsname = 'SaAmfSutCompType'
			dn = 'safMemberCompType=safVersion=' + safVersion + '\,safCompType=' + con.target + ',safVersion=' + safVersion + ',safSuType=' + su.name
			associationobjects.append(AmfAssociation(clsname,dn))
	
	for si in compmodel.filterByTag('serviceInstance'):
		for con in si.filterByTag('contains'):
			clsname = 'SaAmfSvcTypeCSTypes'
			dn = 'safMemberCSType=safVersion=' + safVersion + '\,safCSType=' + con.target + ',safVersion=' + safVersion + ',safSvcType=' + si.name
			associationobjects.append(AmfAssociation(clsname,dn))
	
	for csi in compmodel.filterByTag('componentServiceInstance'):
		for con in csi.filterByTag('associatedTo'):
			clsname = 'SaAmfCtCsType'
			dn = 'safSupportedCsType=safVersion=' + safVersion + '\,safCSType=' + csi.name + ',safVersion=' + safVersion + ',safCompType=' + con.target
			associationobjects.append(AmfAssociation(clsname,dn,{'saAmfCtCompCapability':comp_capability_map[con.target]}))
			comp_csi_type_map[con.target].append(csi.name)
				
	for comp in compinstances:
		clsname = 'SaAmfCompCsType'
		type = comp.clstype.name
		csis = comp_csi_type_map[type]
		for csi in csis:
			dn = 'safSupportedCsType=safVersion=4.0.0\,safCSType=' + csi + ',' + comp_dn_map[comp.name]
			associationobjects.append(AmfAssociation(clsname,dn))
									
	subst['associationobjects'] = associationobjects
								
	subst['clmcluster'] = clmcluster
	subst['amfcluster'] = amfcluster
	
	sc_node_objects = ""
	pl_node_objects = ""
	
	from openSAF.templates import scnode
	from openSAF.templates import plnode
	reload(scnode)
	reload(plnode)
	index = 0;	
	for sc in scnodes:
		index += 1
		sc_node_objects += scnode.scnodeTemplate.safe_substitute(nodename=sc,index=index)
	for pl in plnodes:
		index += 1
		pl_node_objects += plnode.plnodeTemplate.safe_substitute(nodename=pl,index=index)

	subst['sc_node_objects'] = sc_node_objects
	subst['pl_node_objects'] = pl_node_objects						
	immTemplate = kid.Template(file=TemplatePath + "immxml.kid",**subst)

	#immTemplate.write(os.path.join(srcDir, "config/imm.xml"),"utf-8",False,"xml")
	output.writet(os.path.join(srcDir, "openSAF/config/imm.xml"), immTemplate)
	
	appnames = ''
	for comp in comptypes:
		#if not os.path.exists(os.path.join(srcDir, "app/" + comp.name)):
		#	os.makedirs(os.path.join(srcDir, "app/" + comp.name))

		#appMakeFile = open(os.path.join(srcDir, "app/" + comp.name + "/Makefile"), "w")
		from openSAF.templates import appmakefile
		reload (appmakefile)
		#appMakeFile.write(appmakefile.appMakefileTemplate.safe_substitute(appname=comp.name))
		#appMakeFile.close()
		output.write(os.path.join(srcDir, "openSAF/app/" + comp.name + "/Makefile"), appmakefile.appMakefileTemplate.safe_substitute(appname=comp.name))
		
		appMainTemplateFile = open(TemplatePath + "main.c", "r")
		template = appMainTemplateFile.read(1000000)
		appMainTemplateFile.close()
		appMainTemplate = Template(template)

		#mainFile = open(os.path.join(srcDir, "app/" + comp.name + "/main.c"), "w")
		#mainFile.write(appMainTemplate.safe_substitute())
		#mainFile.close()
		output.write(os.path.join(srcDir, "openSAF/app/" + comp.name + "/main.c"),appMainTemplate.safe_substitute())
		
		from openSAF.templates import appscript
		reload (appscript)
		#appScriptFile = open(os.path.join(srcDir, "app/" + comp.name + "/" + comp.name + "_script"), "w")
		#appScriptFile.write(appscript.scriptTemplate.safe_substitute(modelname=modelname,appname=comp.name))
		#appScriptFile.close()
		output.write(os.path.join(srcDir, "openSAF/app/" + comp.name + "/" + comp.name + "_script"),appscript.scriptTemplate.safe_substitute(modelname=modelname,appname=comp.name))
		
		appnames += ' ' + comp.name
										
	from openSAF.templates import makefile
	reload(makefile)
	#makeFile = open(os.path.join(srcDir, "app/Makefile"), "w")
	#makeFile.write(makefile.makefileTemplate.safe_substitute(appnames=appnames,modelname=modelname))
	#makeFile.close()
	output.write(os.path.join(srcDir, "openSAF/app/Makefile"),makefile.makefileTemplate.safe_substitute(appnames=appnames,modelname=modelname))
	
			
		
