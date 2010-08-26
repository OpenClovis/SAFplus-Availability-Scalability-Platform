#!/bin/bash

################################################################################
#
# This script create toc.xml using index.html for Online Help of IDE
#
################################################################################

#-------------------------------------------------------------------------------
# Reformat page by adding new line character in front of new commands 
#-------------------------------------------------------------------------------

sed -e 's/<\/ul>/\n<\/ul>/g' \
    -e 's/<li>/\n<li>/g' \
    -e 's/<\/li>/\n<\/li>/g' \
    -e 's/<p>/\n<p>/g' \
    -e 's/<a class="el"/\n<a class="el"/g' \
    -e 's/<hr size/\n<hr size/g' \
$1 > tmp.xml && mv -f tmp.xml $1

#-------------------------------------------------------------------------------
# Keep only table of contents, add new head and tail
#-------------------------------------------------------------------------------
awk 'BEGIN {
     n = 0;
     # add new head text
     printf("<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n");
     printf(" <?NLS TYPE=\"org.eclipse.help.toc\"?>\n");
     printf("<toc label=\"OpenClovis IDE User Guide Table of Contents\">\n");
     } \
     
     # print only the table of contents
     /<a class="el"/ {n++; print; next;} \
     /<ul>/          {if (n>0) print; next;} \
     /<\/ul>/        {if (n>0) print; next;} \
 
     END {
     # add new tail text
      printf("<\/toc>\n");
     }' \
$1 > tmp.xml && mv -f tmp.xml $1

#-------------------------------------------------------------------------------
# Find html file name and move to the end of line
#-------------------------------------------------------------------------------
awk '/<\/a>/ {
        sub(/<\/a>/,""); 
     } \
     /href=".*\.html">/ { 
        match($0,/href=".*\.html">/);  
        string = substr($0, RSTART, RLENGTH); 
        sub(/href=".*\.html">/,""); 
        printf("%s %s\n", $0, string); 
        next; 
     } \
     {print;}' \
$1 > tmp.xml && mv -f tmp.xml $1

#-------------------------------------------------------------------------------
# Add <topic> and </topic> xml markup and add path: directory contents
#-------------------------------------------------------------------------------
awk '/<a class="el"/ {
        # exchange class to topic markup
        gsub(/<a class="el" /, "<topic label=\"");
        # add path to html file name
        gsub(/ href="ideguide/, "\" href=\"contents/ideguide");
        # if line contains <ul>, it means it has subsections --> 
        # do not close yet with </topic>, otherwise close it
        if (match($0, /<ul>/)) {
            sub(/<ul>/,"");
            print;
            next;
        } else {
            printf("%s <\/topic>\n", $0);  
            next;
        }
     } \
     # line contains </ul> --> close it now with </topic>
     /<\/ul>/ {
        sub(/<\/ul>/, "<\/topic>");
        print;
        next;
     }      
     {print;}' \
$1 > tmp.xml && mv -f tmp.xml $1
         
#-------------------------------------------------------------------------------
# Sign # means 4th level heading --> remove it.
#-------------------------------------------------------------------------------
awk '/\#/ { next;} \
     {print;}' \
$1 > tmp.xml && mv -f tmp.xml $1
