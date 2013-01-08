import sys
import xml.dom.minidom
import os
import pdb
import microdom

def model2Microdom(modelsDir):

        resourcedataXmi      = os.path.join(modelsDir, "resourcedata.xml")
        componentdataXmi     = os.path.join(modelsDir, "componentdata.xml")
        alarmdataXmi         = os.path.join(modelsDir, "alarmdata.xml")
        modelchangesXmi      = os.path.join(modelsDir, "modelchanges.xmi")
        associatedResXml     = os.path.join(modelsDir, "component_resource_map.xml")
        associatedAlarmXml   = os.path.join(modelsDir, "resource_alarm_map.xml")
        compTemplateGroupXml = os.path.join(modelsDir, "component_templateGroup_map.xml")

        #compileConfigsXmi = os.path.join(projectLoc, ".temp_dir/configs/compileconfigs.xml")
        #idlXml = os.path.join(projectLoc, "idl/server_idl_interface.xml")
        #staticdir         = os.path.join(pkgLoc, "IDE/ASP/static/src")
        #templatesdir      = os.path.join(pkgLoc, "IDE/ASP/templates")


        alarmRuleXml = os.path.join(modelsDir, "alarmrule.xml")
        
        amfDefXml = os.path.join(modelsDir, "../configs/clAmfDefinitions.xml")
        amfConfigXml = os.path.join(modelsDir, "../configs/clAmfConfig.xml")

        cfgFiles = [resourcedataXmi,componentdataXmi,amfDefXml,associatedResXml,associatedAlarmXml,alarmdataXmi,modelchangesXmi,alarmRuleXml,amfConfigXml]
        cfgFiles = filter(os.path.exists,cfgFiles)

        cfg = microdom.Merge("top",microdom.LoadFile(cfgFiles))

        return cfg

    
