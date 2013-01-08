import sys
import xml.dom.minidom
import os

def mainMethod(srcDir, resourcedataDoc, dataTypeDoc, compileConfigsDoc, alarmDataDoc, alarmAssociationDoc):
	import cor
	import fm
	import omids

	cor.mainMethod(srcDir, resourcedataDoc, dataTypeDoc, compileConfigsDoc, alarmDataDoc, alarmAssociationDoc)
	fm.mainMethod(srcDir, compileConfigsDoc, resourcedataDoc)
	omids.mainMethod(srcDir, resourcedataDoc)
