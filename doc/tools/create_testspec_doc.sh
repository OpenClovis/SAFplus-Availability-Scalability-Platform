#!/bin/bash

#------------------------------------------------------------------------------
# Usage:
#    %s <docname> <format_in> <format_out> <username> <password> 
#
# Example:
#    %s installguide singlepage singlepage clovis clovis
#   
#    <docname>:   evalguide
#                 installguide
#                 tutorial
#                 relnotes
#                 safcompliance
#                 ideguide
#                 sdkguide
#                 logtoolguide
#
#   <format_in>:  singlepage
#                 multipage
#
#   <format_out>: singlepage
#                 multipage
#
#   <username>:   username using openclovis wiki pages
#
#   <password>:   password to openclovis wiki pages
#
#------------------------------------------------------------------------------
source config.sh

#------------------------------------------------------------------------------
# clean directory 
#------------------------------------------------------------------------------
rm "$1".wiki
rm "$1".txt


#------------------------------------------------------------------------------
# load down the doc from wiki to file <doc-name>.wiki
#------------------------------------------------------------------------------
if [ "$2" = "singlepage" ]; then 
   $WIKI_GRAB -rg http://$WIKI_IP/wiki $4 $5 Doc:$1 > $1.wiki     
elif [ "$2" = "multipage" ]; then
   $WIKI_GRAB -rg http://$WIKI_IP/wiki $4 $5 Doc:$1 > $1.wikisum   
   awk '/\#/ {next;} 
        /\[\[.*\]\]/ {
        match($0, /^\*+/);
        sub(/\** */,"");
        sub(/\[\[/,"");
        sub(/\]\]/,"");
        split($0, a, "|");
        gsub(/ /,"",a[1]);
        printf("%s:%d\n", a[1], RLENGTH-1);
   }' $1.wikisum > file.txt
   for line in `cat file.txt`; do
     file=`echo $line | awk -F: '{print $1}'`
     indent=`echo $line | awk -F: '{print $2}'` 
     echo "$file"
     
     pad=`echo | awk '{print substr("==============", 0, n)}' n=$indent`         
     $WIKI_GRAB -rg http://$WIKI_IP/wiki $4 $5 $file | \
         sed -r "s/(=(=)+)/\1$pad/g" >> $1.wiki
   done 
#   rm $1.wikisum
#   rm file.txt
else 
  echo "Wrong <format_in> value!"
fi

#------------------------------------------------------------------------------
# convert doc from wiki to doxygen format: <doc-name>.wiki---><doc-name>.txt 
#------------------------------------------------------------------------------
echo "$TOOLS_DIR/convert_wiki_to_doxygen.awk $1 $3 $1.wiki > $1.txt"
$TOOLS_DIR/convert_wiki_to_doxygen.awk $1 $3 $1.wiki > $1.txt

#------------------------------------------------------------------------------
# resize figures if needed in the latex output 
#------------------------------------------------------------------------------
$TOOLS_DIR/resize_figures.awk $1.txt > tmp.txt && mv -f tmp.txt $1.txt

#------------------------------------------------------------------------------
# create table of contents saved in toc.txt 
#------------------------------------------------------------------------------
$TOOLS_DIR/create_table_of_contents.awk $1 $3 $1.txt > $1toc.txt

#------------------------------------------------------------------------------
# reorganize pages to <format-out>
#------------------------------------------------------------------------------
if [ "$3" = "singlepage" ]; then 
   echo "$3"
   cat $1.txt>>$1toc.txt
   rm $1.txt
   mv $1toc.txt $1.txt
fi

