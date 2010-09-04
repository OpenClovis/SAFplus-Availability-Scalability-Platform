##############################################################################
#
#      CONFIG FILE FOR GENERATING HTML AND PDF PAGES WITH DOXYGEN
#                     USING DOWNLOADED WIKI PAGES
#
##############################################################################

##############################################################################
#                    Document specific parameters
##############################################################################

#-----------------------------------------------------------------------------
# Documentation unique identifier name
# e.g.: existing ones: installguide, tutorial, relnotes, safcompliance, 
#       ideguide, evalguide, sdkguide, logtoolguide, aspconsole, testspec
# * all links, unique header identifiers in doxygen file will contain this name,
#   not visible for user
# * It will also appear in the pdf file name
export DOCID=

#-----------------------------------------------------------------------------
# Documentation full name appears in Documentation HTML Main Page and 
# Title PDF Page 
#
export DOCTITLE=

#-----------------------------------------------------------------------------
# Wiki page URL
# e.g.:
# http://192.168.0.94:8080/wiki/index.php/Doc:HaDemoModelGuide ---> WIKI_URL=Doc:HaDemoModelGuide
# http://192.168.0.94:8080/wiki/index.php/Documentation ---> WIKI_URL=Documentation  
export WIKI_URL=

#-----------------------------------------------------------------------------
# Input wiki format
# Possible values:
# "singlepage" - Entire document is in one wiki page
# "multipage"  - Document is chopped into many wiki pages:
#                One summary wiki page exists, where all pages are listed
#                Bulleted list identifies the toc.
# NOTE: CONVERTER DOES NOT PICK UP WIKI PAGES REFERENCED ON ANY WIKI PAGE !!!!
#
export INPUT_WIKI_FORMAT=

#-----------------------------------------------------------------------------
# Output html format
# Entire document can be one single HTML page or chopped into many HTML pages
# Possible values:
#     "singlepage" - Entire document will be in one HTML page
#     "multipage1" - Document is chopped into many HTML pages, 
#                    each heading 1 (==) starts new HTML page 
#     "multipage2" - Document is chopped into many HTML pages, 
#                    heading 1 and heading 2 ( == & ===) start new HTML page 
#     "multipage3" - Document is chopped into many HTML pages, 
#                    heading 1, 2 and 3( == & === & ====) start new HTML page 
# NOTE: IF YOU USE multipage1,2,3 - PDF FORMAT WILL LOSE HIERARCHY, IN PDF ALL
#       HTML PAGES ARE ON THE SAME LEVEL. YOU HAVE TO REGENERATE PDF WITH 
#       OUTPUT_HTML_FORMAT=singlepage TO GET CORRECT TABLE OF CONTENTS.
export OUTPUT_HTML_FORMAT=

#-----------------------------------------------------------------------------
# html directory
#
export HTMLDIR=

#-----------------------------------------------------------------------------
# Output pdf format
# If you have a larger document you can use 'book' format. In this case the doc
# is diveded into several chapters, each heading (==) starting in a new page. 
# If you have a shorter doc, you can use the 'article' format, no page breaks.
# Possible values:
#     "book"    - Each main heading (==) in wiki will appear as a chapter
#                 starting on new page
#     "article" - Each main heading (==) in wiki will not start new page, 
#                 it will create one continuous single doc.
#
export OUTPUT_PDF_FORMAT=book

#-----------------------------------------------------------------------------
# pdf directory
#
export PDFDIR=../pdf

#-----------------------------------------------------------------------------
# Documentation Part Number appears in Documentation Main Page
# Numbers between 000101-000112 are already used for OpenClovis SDK Docs.
# Numbers pm-00x is used by PM
#
export PARTNUMBER=

#-----------------------------------------------------------------------------
# Documentation Revision Number appears in Documentation Main Page
#
export REVISION=01

##############################################################################
#                            General parameters
##############################################################################

#-----------------------------------------------------------------------------
# Documentation prefix appears in documentation main page and filenames
#
export COMMON_PREFIX=openclovis

#-----------------------------------------------------------------------------
# Product name appears in documentation main page before title
# You can leave it empty if it is not required.
# e.g.:
# PRODUCT_NAME="OpenClovis"
# PRODUCT_NAME=
export PRODUCT_NAME="OpenClovis SDK"

#-----------------------------------------------------------------------------
# Documentation release appears in documentation main page and filenames
#
export RELEASE=5.0

#-----------------------------------------------------------------------------
# Doxygen Config File 
# No change is required!
# 
export DOXYFILE=Doxyfile.mod

#-----------------------------------------------------------------------------
# Clovis Wiki IP address (and optional port)
# Examples:
#WIKI_IP=localhost:18080
#WIKI_IP=192.168.0.94
export WIKI_IP=192.168.0.94

#-----------------------------------------------------------------------------
# Wiki username
#
export WIKI_USER=clovis

#-----------------------------------------------------------------------------
# Wiki password
#
export WIKI_PW=clovis
