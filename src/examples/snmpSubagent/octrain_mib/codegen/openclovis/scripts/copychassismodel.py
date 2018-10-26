################################################################################
# ModuleName  : com
# $File$
# $Author$
# $Date$
################################################################################
# Description :
################################################################################
import sys
import os
import os.path
import shutil
import xml.dom.minidom as dom


def main(argv):
	if len(argv) != 3:
		print 'Error: Invalid arguments'
		print 'syntax: copychassismodel.py <CWsrcpath> <DeployPath>'
		sys.exit(1)
	
	configFile = sys.argv[2] + '/config/clusterlayout.xml'
	configDoc = dom.parse(configFile)
	numSlotsTag = configDoc.getElementsByTagName('CHASSISSTYLE')
	if len(numSlotsTag) == 0:
		print 'Error: CHASSISSTYLE not found'
		sys.exit(1)
	
	chassisStyle = numSlotsTag[0].firstChild.data
	destDir = sys.argv[2] + "/" + chassisStyle
	if os.path.exists(destDir):
		shutil.rmtree(destDir)
	srcFile = sys.argv[1] + "/" + chassisStyle
	shutil.copytree(srcFile, destDir)
	
	configDoc.unlink()

if __name__ == '__main__':
	main(sys.argv)
