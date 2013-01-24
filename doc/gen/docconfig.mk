#-----------------------------------------------------------------------------
# Documentation unique identifier name
# e.g.: existing ones: installguide, tutorial, relnotes, safcompliance, 
#       ideguide, evalguide, sdkguide, logtoolguide, aspconsole, testspec
# * all links, unique header identifiers in doxygen file will contain this name,
#   not visible for user
# * It will also appear in the pdf file name
DOCID ?=

#-----------------------------------------------------------------------------
# Documentation full name appears in Documentation HTML Main Page and 
# Title PDF Page 
#
DOCTITLE ?=

#-----------------------------------------------------------------------------
# Documentation Part Number appears in Documentation Main Page
# Numbers between 000101-000112 are already used for OpenClovis SDK Docs.
# Numbers pm-00x is used by PM
#
PARTNUMBER ?=

#-----------------------------------------------------------------------------
# Documentation Revision Number appears in Documentation Main Page
#
REVISION ?=01

#-----------------------------------------------------------------------------
# Product name appears in documentation main page before title
# You can leave it empty if it is not required.
# e.g.:
# PRODUCT_NAME="OpenClovis"
# PRODUCT_NAME=
export PRODUCT_NAME ?="OpenClovis SAFplus SDK"

#-----------------------------------------------------------------------------
# Documentation release appears in documentation main page and filenames
#
RELEASE ?=6.0
