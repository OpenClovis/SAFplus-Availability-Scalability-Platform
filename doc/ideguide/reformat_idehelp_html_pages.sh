#!/bin/bash

################################################################################
#
#                  REFORMAT ONLINE IDE HELP HTML PAGES
#
################################################################################

HTML_DIR=../html/idehelp

#-------------------------------------------------------------------------------
# Remove the unnecessary references to other documents
#-------------------------------------------------------------------------------
        
for f in `find $HTML_DIR -name "*.html"`; do

    echo "Changing file $f"
    
    # Remove references to other docs
    sed -e 's/<a href=\"\.\.\/relnotes\/index\.html\">OpenClovis Release Notes<\/a>/OpenClovis Release Notes/g' \
        -e 's/<a href=\"\.\.\/safcompliance\/index\.html\">OpenClovis SA Forum Compliance<\/a>/OpenClovis SA Forum Compliance/g' \
        -e 's/<a href=\"\.\.\/installguide\/index\.html\">OpenClovis Installation Guide<\/a>/OpenClovis Installation Guide/g' \
        -e 's/<a href=\"\.\.\/tutorial\/index\.html\">OpenClovis Modeling Tutorial<\/a>/OpenClovis Modeling Tutorial/g' \
        -e 's/<a href=\"\.\.\/sdkguide\/index\.html\">OpenClovis SDK User Guide<\/a>/OpenClovis SDK User Guide/g' \
        -e 's/<a href=\"\.\.\/evalguide\/index\.html\">OpenClovis Evaluation System User Guide<\/a>/OpenClovis Evaluation System User Guide/g' \
        -e 's/<a href=\"\.\.\/logtoolguide\/index\.html\">OpenClovis Log Tool User Guide<\/a>/OpenClovis Log Tool User Guide/g' \
        -e 's/<a href=\"\.\.\/apirefs\/index\.html\">OpenClovis API Reference Guide<\/a>/OpenClovis API Reference Guide/g' \
        -e 's/<a href=\"\.\.\/debugcli\/index\.html\">OpenClovis Debug CLI Reference Guide<\/a>/OpenClovis Debug CLI Reference Guide/g' \
        -e 's/Back to <a href=\"\.\.\/index\.html\">OpenClovis SDK Documentation Main Page<\/a><p>//g' \
    $f > tmp.html && mv -f tmp.html $f

done
