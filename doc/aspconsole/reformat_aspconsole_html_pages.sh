#!/bin/bash

# Replacing all Variables title to Commands

HTML_DIR=../html/aspconsole

for f in `find $HTML_DIR -name "*.html"`; do

    echo "Changing file $f"
    
    # change all H2 titles "Variables" to "Commands" 
    # change all H2 titles "Variable Documentation" to "Command Documentation" 
    sed -e 's/<h2>Variables<\/h2>/<h2>Commands<\/h2>/g' \
        -e 's/<h2>Variable Documentation<\/h2>/<h2>Command Documentation<\/h2>/g' \
        $f > tmp.html && mv -f tmp.html $f
    
done
