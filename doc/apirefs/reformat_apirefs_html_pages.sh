#!/bin/bash

################################################################################
#
#         SCRIPT REFORMATS APIREFS HTML PAGES CREATED BY DOXYGEN
#
################################################################################

HTML_DIR=../html/apirefs

#-------------------------------------------------------------------------------
# Original module.html lists all modules and sub(sub)modules.  
# Remove all sub(sub)modules in listing
#-------------------------------------------------------------------------------

# Remove the unnecessary sub-items in the modules page
echo "Changing file $HTML_DIR/modules.html"
./reformat_modules.awk $HTML_DIR/modules.html > tmp.html && mv -f tmp.html $HTML_DIR/modules.html
        
#-------------------------------------------------------------------------------
# General changes in all HTML pages:
# * Rename: class ---> structure
# * Add new items in main tab line and remove second tab line
# * Reformat callback functions
#-------------------------------------------------------------------------------

for f in `find $HTML_DIR -name "*.html" -o -name "search.php"`; do

    echo "Changing file $f"
 
    # Replacing tab name "Classes" with "Structures" and
    # change all H1 titles "Class" to "Structure" and
    # change all H2 titles "Classes" to "Structures" and
    # change all "Class&nbspMembers" to "Structure&nbspFields" and
    # change all "Class&nbspList" to "Structure&nbspList"
    sed -e 's/<span>Classes<\/span>/<span>Structures<\/span>/g' \
        -e 's/<h1>OpenClovis SDK API Class/<h1>OpenClovis SDK API Structure/g' \
        -e 's/<h2>Classes<\/h2>/<h2>Structures<\/h2>/g' \
        -e 's/<span>Class&nbsp;Members<\/span>/<span>Structure\&nbsp;Fields<\/span>/g' \
        -e 's/<span>Class&nbsp;List<\/span>/<span>Structure\&nbsp;List<\/span>/g' \
        -e 's/<h2>Detailed Description<\/h2>/<p>/g' \
        $f > tmp.html && mv -f tmp.html $f

    # Insert a few new tabs in main tab line between Structures and Files
    awk '{print}/<span>Structures/{exit}' $f > tmp.html
    cat << EOF >> tmp.html
    <li><a href="globals_func.html"><span>Functions</span></a></li>
    <li><a href="globals_vars.html"><span>Variables</span></a></li>
    <li><a href="globals_type.html"><span>Typedefs</span></a></li>
    <li><a href="globals_enum.html"><span>Enumerations</span></a></li>
    <li><a href="globals_eval.html"><span>Enumerator</span></a></li>
    <li><a href="globals_defs.html"><span>Defines</span></a></li>
EOF

# If no global variables exist
#    cat << EOF >> tmp.html
#    <li><a href="globals_func.html"><span>Functions</span></a></li>
#    <li><a href="globals_type.html"><span>Typedefs</span></a></li>
#    <li><a href="globals_enum.html"><span>Enumerations</span></a></li>
#    <li><a href="globals_eval.html"><span>Enumerator</span></a></li>
#    <li><a href="globals_defs.html"><span>Defines</span></a></li>
#EOF
    awk '/<span>Files/{needed=1}needed{print}' $f >> tmp.html
    mv -f tmp.html $f
    
    # For all pages where the File tab is active, remove second row of tabs
    if [ `grep '<li id="current"><a href="files.html">' $f | wc -l` -ne 0 ]; then
        # We look for 2nd <div>...</div> skip and remove it
        awk '/<div class="tabs">/{n++}n==2{skip=1}!skip{print}/<\/div>/{if(skip){skip=0;n++}}' $f > tmp.html
        mv -f tmp.html $f
    fi
    
    # Reformat Callback functions typedefs format --> function format
    ./reformat_callback_functions.awk $f > tmp.html && mv -f tmp.html $f
    
done

#-------------------------------------------------------------------------------
# Remove HTML pages where all types are listed together and then remove 
# third tabs line, all will come up in first tab line. 
#-------------------------------------------------------------------------------

# Get rid of the "All" (globals.html page and its b, c, d, etc., index pages
rm -f $HTML_DIR/globals.html $HTML_DIR/globals_0x??.html

# There is more to do with the remaining globals*.html
for f in `find $HTML_DIR -name "globals*.html"`; do
    # First, remove the 2nd raw of tabs (what used to be the 3rd before the above loop
    # We look for 2nd <div>...</div> skip and remove it
    awk '/<div class="tabs">/{n++}n==2{skip=1}!skip{print}/<\/div>/{if(skip){skip=0;n++}}' $f > tmp.html
    mv -f tmp.html $f
    
    # Insert the "id=current" for the appropriate tab, i..e., the tab that points to the
    # current file
    sed -r "s/(><a href=\"`basename $f`)/ id=\"current\"\1/g" $f > tmp.html && mv -f tmp.html $f
    
    # Remove "id=current" from the Files tab line
    sed -r "s/ id=\"current\"(><a href=\"files.html)/\1/g" $f > tmp.html && mv -f tmp.html $f
    
done

#-------------------------------------------------------------------------------
# Create same HTML index formatting style for functions, structures, defines, 
# enumerations, enumerators, variables, typedefs
#-------------------------------------------------------------------------------

## Fix tabs and indexing for the globals_func.html file
cat $HTML_DIR/globals_func*.html | \
    ./resort_index_in_globals.py | \
    awk '{print}/<\/div>/{if (!n){print "<h1>OpenClovis SDK API Function Index</h1>"}n=1}' | \
    sed 's/<title>.*<\/title>/<title>OpenClovis SDK API Function Index<\/title>/g' > tmp.html && \
    mv -f tmp.html $HTML_DIR/globals_func.html


# Same for globals_type
cat $HTML_DIR/globals_type*.html | \
    ./resort_index_in_globals.py | \
    awk '{print}/<\/div>/{if (!n){print "<h1>OpenClovis SDK API Typedef Index</h1>"}n=1}' | \
    sed 's/<title>.*<\/title>/<title>OpenClovis SDK API Typedef Index<\/title>/g' > tmp.html && \
    mv -f tmp.html $HTML_DIR/globals_type.html


# Same for globals_eval
cat $HTML_DIR/globals_eval*.html | \
    ./resort_index_in_globals.py | \
    awk '{print}/<\/div>/{if (!n){print "<h1>OpenClovis SDK API Enumerator Index</h1>"}n=1}' | \
    sed 's/<title>.*<\/title>/<title>OpenClovis SDK API Enumerator Index<\/title>/g' > tmp.html && \
    mv -f tmp.html $HTML_DIR/globals_eval.html

# Same for globals_enum
cat $HTML_DIR/globals_enum*.html | \
    ./resort_index_in_globals.py | \
    awk '{print}/<\/div>/{if (!n){print "<h1>OpenClovis SDK API Enumeration Index</h1>"}n=1}' | \
    sed 's/<title>.*<\/title>/<title>OpenClovis SDK API Enumeration Index<\/title>/g' > tmp.html && \
    mv -f tmp.html $HTML_DIR/globals_enum.html

# Same for globals_defs
cat $HTML_DIR/globals_defs*.html | \
    ./resort_index_in_globals.py  | \
    awk '{print}/<\/div>/{if (!n){print "<h1>OpenClovis SDK API Define Index</h1>"}n=1}' | \
    sed 's/<title>.*<\/title>/<title>OpenClovis SDK API Define Index<\/title>/g' > tmp.html && \
    mv -f tmp.html $HTML_DIR/globals_defs.html

# Variables are not indexed yet, but we insert the title nevertheless
cat < $HTML_DIR/globals_vars.html | \
    awk '{print}/<\/div>/{if (!n){print "<h1>OpenClovis SDK API Variable Index</h1>"}n=1}' | \
    sed 's/<title>.*<\/title>/<title>OpenClovis SDK API Variable Index<\/title>/g' > tmp.html && \
    mv -f tmp.html $HTML_DIR/globals_vars.html

# Reformat classes.html to have the same format as all  globals_*.html
echo "Changing file $HTML_DIR/classes.html"
./reformat_classes.sh $HTML_DIR/classes.html

#-------------------------------------------------------------------------------
