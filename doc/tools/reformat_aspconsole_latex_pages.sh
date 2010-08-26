#!/bin/bash

################################################################################
#
#                      Reformat aspconsole based latex pages 
#
################################################################################

#-------------------------------------------------------------------------------
# new style format to head of refman.tex is added
#-------------------------------------------------------------------------------
mv refman.tex refman_tail.tex
awk '/\\label\{/{n++} (n==0){print}'  refman_head.tex > refman.tex 
awk '/\\chapter\{/{n++} (n>0){print}' refman_tail.tex >> refman.tex 

#-------------------------------------------------------------------------------
# Move ASP Console Page Documentation to the front from the back
#-------------------------------------------------------------------------------
awk '/\\input\{index\}/ \
    {gsub( /\\input\{index\}/, \
    "\\input\{index\}\\include\{aspconsole_start\}\\include\{aspconsole_components\}\\include\{aspinfo_start\}\\include\{aspconsole_doc\}");}\
    {print}' refman.tex > tmp.tex 
mv tmp.tex refman.tex

awk 'BEGIN {skip=0} \
     /\\chapter\{Open(\\-)?Clovis SDK ASP Console Page Documentation\}/{skip=1} \
     /\\printindex/ {skip=0}(!skip){print}' refman.tex > tmp.tex 
mv tmp.tex refman.tex

#-------------------------------------------------------------------------------
# Rename Chapter Title
#-------------------------------------------------------------------------------
awk '/\\chapter\{Open(\\-)?Clovis SDK ASP Console Reference Guide \}/  \
     {gsub( /\\chapter\{Open(\\-)?Clovis SDK ASP Console Reference Guide \}/, \
     "\\chapter\{ASP Console Reference Guide Overview\}");}\
     {print}' refman.tex > tmp.tex 
mv tmp.tex refman.tex

awk '/\\chapter\{Open(\\-)?Clovis SDK ASP Console Module Documentation\}/  \
     {gsub( /\\chapter\{Open(\\-)?Clovis SDK ASP Console Module Documentation\}/, \
     "\\chapter\{ASP Console and ASP Info Module Documentation\}");}\
     {print}' refman.tex > tmp.tex 
mv tmp.tex refman.tex
#-------------------------------------------------------------------------------
# Remove reference to Main Doc, Main Doc exists only in HTML version
#-------------------------------------------------------------------------------
sed -r -e 's/Back to \\href\{\.\.\/index\.html\}\{\\tt Open(\\-)?Clovis SDK Documentation Main Page\}//g' \
    -e 's/\\section\{Key Topics\}/\\subsection\{Key Topics\}/g' \
index.tex > tmp.tex && mv -f tmp.tex index.tex

for f in `find -name "*.tex" `; do

   #-------------------------------------------------------------------------------
   # Some changes to improve document style
   #-------------------------------------------------------------------------------
   sed -e 's/\\subsection\*{Variables}/\\subsection\{Commands}/g' \
       -e 's/\\subsection{Variable Documentation}/\\subsection{Command Documentation}/g' \
       -e 's/\\item\[Parameters:\]/\\item[Parameters:]\\begin{Code}\\begin{verbatim}\\end{verbatim}\\end{Code}\\vspace{-3ex}/g' \
       -e 's/\\item\[Description:\]/\\item[Description:]\\begin{Code}\\begin{verbatim}\\end{verbatim}\\end{Code}\\vspace{-3ex}/g' \
       -e 's/\\item\[Note:\]/\\item[Note:]\\begin{Code}\\begin{verbatim}\\end{verbatim}\\end{Code}\\vspace{-3ex}/g' \
   $f > tmp.tex && mv -f tmp.tex $f

done

