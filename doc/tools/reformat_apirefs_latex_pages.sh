#!/bin/bash

################################################################################
#
#                      Reformat apirefs based latex pages 
#
################################################################################

#-------------------------------------------------------------------------------
# new style format to head of refman.tex is added
#-------------------------------------------------------------------------------
mv refman.tex refman_tail.tex
awk '/\\label\{/{n++} (n==0){print}'  refman_head.tex > refman.tex 
awk '/\\chapter\{/{n++} (n>0){print}' refman_tail.tex >> refman.tex 

#-------------------------------------------------------------------------------
# Move API Page Documentation to the front from the back
#-------------------------------------------------------------------------------
awk '/\\input\{index\}/ \
    {gsub( /\\input\{index\}/, \
    "\\input\{index\}\\include\{apirefs_components\}\\include\{apirefs_errorcodes\}\\include\{apirefs_doc\}");}\
    {print}' refman.tex > tmp.tex 
mv tmp.tex refman.tex

awk 'BEGIN {skip=0} \
     /\\chapter\{Open(\\-)?Clovis SDK API Page Documentation\}/{skip=1} \
     /\\printindex/ {skip=0}(!skip){print}' refman.tex > tmp.tex 
mv tmp.tex refman.tex

#-------------------------------------------------------------------------------
# Rename Chapter Titles
#-------------------------------------------------------------------------------
sed -r -e 's/Open(\\-)?Clovis SDK API Reference Guide/API Reference Guide Overview/g' \
    -e 's/\\chapter\{Open(\\-)*Clovis SDK API Module Documentation\}//g' \
    -e 's/Open(\\-)?Clovis SDK API Class Documentation/Appendix A: Structure Documentation/g' \
    -e 's/Open(\\-)?Clovis SDK API File Documentation/Appendix B: Header File Documentation/g' \
refman.tex > tmp.tex && mv -f tmp.tex refman.tex

#-------------------------------------------------------------------------------
# Remove reference to Main Doc, Main Doc exists only in HTML version
#-------------------------------------------------------------------------------
sed -r -e 's/Back to \\href\{\.\.\/index\.html\}\{\\tt Open(\\-)?Clovis SDK Documentation Main Page\}//g' \
    -e 's/\\section\{Key Topics\}/\\subsection\{Key Topics\}/g' \
index.tex > tmp.tex && mv -f tmp.tex index.tex

for f in `find -name "*.tex" `; do

   #-------------------------------------------------------------------------------
   # Remove Detailed Description title, extra title with no information
   #-------------------------------------------------------------------------------
   sed -e 's/\\subsection{Detailed Description}//g' \
      $f > tmp.tex && mv -f tmp.tex $f

   #-------------------------------------------------------------------------------
   # Use Structures instead of Classes for Type Definition
   #-------------------------------------------------------------------------------
   sed -e 's/\\subsection\*{Classes}/\\subsection\*{Structures}/g' \
      $f > tmp.tex && mv -f tmp.tex $f

   #-------------------------------------------------------------------------------
   # Move Components' title to one title up
   #-------------------------------------------------------------------------------
   sed -r -e 's/\\section\{Alarm Manager\}/\\chapter\{Alarm Manager\}/g' \
       -e 's/\\section\{Availability Management Service\}/\\chapter\{Availability Management Service\}/g' \
       -e 's/\\section\{Buffer Management\}/\\chapter\{Buffer Management\}/g' \
       -e 's/\\section\{Bitmap Management\}/\\chapter\{Bitmap Management\}/g' \
       -e 's/\\section\{Checkpointing Service\}/\\chapter\{Checkpointing Service\}/g' \
       -e 's/\\section\{Circular List Management\}/\\chapter\{Circular List Management\}/g' \
       -e 's/\\section\{Chassis Management\}/\\chapter\{Chassis Management\}/g' \
       -e 's/\\section\{Container Service}/\\chapter{Container Service}/g' \
       -e 's/\\section\{Common defines, structures, and functions\}/\\chapter\{Common defines, structures, and functions\}/g' \
       -e 's/\\section\{Clovis Object Repository \(COR\)\}/\\chapter\{Clovis Object Repository \(COR\)\}/g' \
       -e 's/\\section\{Component Manager\}/\\chapter\{Component Manager\}/g' \
       -e 's/\\section\{Database Abstraction Layer \(DBAL\)\}/\\chapter\{Database Abstraction Layer \(DBAL\)\}/g' \
       -e 's/\\section\{Debug Service\}/\\chapter\{Debug Service\}/g' \
       -e 's/\\section\{Execution Object \(EO\) Service\}/\\chapter\{Execution Object \(EO\) Service\}/g' \
       -e 's/\\section\{Event Service\}/\\chapter\{Event Service\}/g' \
       -e 's/\\section\{Fault Manager\}/\\chapter\{Fault Manager\}/g' \
       -e 's/\\section\{Group Membership Service\}/\\chapter\{Group Membership Service\}/g' \
       -e 's/\\section\{Handle Management\}/\\chapter\{Handle Management\}/g' \
       -e 's/\\section\{Heap Management\}/\\chapter\{Heap Management\}/g' \
       -e 's/\\section\{Intelligent Object Communication\}/\\chapter\{Intelligent Object Communication\}/g' \
       -e 's/\\section\{Log Service\}/\\chapter\{Log Service\}/g' \
       -e 's/\\section\{Message Service\}/\\chapter\{Message Service\}/g' \
       -e 's/\\section\{Name Service\}/\\chapter\{Name Service\}/g' \
       -e 's/\\section\{Operating System Abstraction Layer \(OSAL\)\}/\\chapter\{Operating System Abstraction Layer \(OSAL\)\}/g' \
       -e 's/\\section\{Performance Management Library\}/\\chapter\{Performance Management Library\}/g' \
       -e 's/\\section\{Provisioning Library\}/\\chapter\{Provisioning Library\}/g' \
       -e 's/\\section\{Queue Management Service\}/\\chapter\{Queue Management Service\}/g' \
       -e 's/\\section\{Remote Method Dispatch\}/\\chapter\{Remote Method Dispatch\}/g' \
       -e 's/\\section\{Rule Based Engine \(RBE\)\}/\\chapter\{Rule Based Engine \(RBE\)\}/g' \
       -e 's/\\section\{Open(\\-)?Clovis Test API\}/\\chapter\{OpenClovis Test API\}/g' \
       -e 's/\\section\{Timer\}/\\chapter\{Timer\}/g' \
       -e 's/\\section\{External Data Representation \(XDR\)\}/\\chapter\{External Data Representation \(XDR\)\}/g' \
   $f > tmp.tex && mv -f tmp.tex $f

done




