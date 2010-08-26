################################################################################
# ModuleName  : SampleModel
# $File$
# $Author$
# $Date$
################################################################################
# Description :
# usage: python aspenv.py <args>
# create clASPEnv.sh
################################################################################
import sys
import string
from string import Template

   

clASPenvTemplate = Template("""\
export ASP_MODEL_NAME=${modelName}
source clasp.env
""")

#------------------------------------------------------------------------------
#------------------------------------------------------------------------------

#Script Execution Starts Here.
#------------------------------------------------------------------------------
#-------------------- clAspEnv.sh file generation------------------
projectName = sys.argv[1]
print projectName
file = "clModelAspEnv.sh" 
fileContents = clASPenvTemplate.safe_substitute(modelName = projectName) 
out_file = open(file,"w")
out_file.write(fileContents)
out_file.close()
