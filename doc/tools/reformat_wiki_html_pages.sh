#!/bin/bash

################################################################################
#
#            REFORMAT HTML PAGES DOWNLOADED FROM WIKI
#
# It replaces the links prev, up, next with tags on the top of the page 
# and icons.
# It replaces the links top with icons
################################################################################
       
for f in `find -name "*.html"`; do

    echo "Changing file $f"
 
    awk 'BEGIN { multipage = 0;}
         /<a href=\"pages.html\"><span>Related\&nbsp;Pages<\/span><\/a>/ {
            multipage = 1;
         }
         END {print multipage;}' $f > format.txt

    if [ `cat format.txt` = 1 ]; then    
        echo "Mutipage format"
        
        awk 'BEGIN { 
                printing = 1;
             }
             {if (printing) {print;}}
             /<a href=\"index.html\"><span>Main\&nbsp;Page<\/span><\/a>/ {
                printing = 0;
             }' $f > tmp.html

        awk '/^ *Go to/ {
               if (match($0, /href=[^>]*>Previous</)) {
                  string=substr($0, RSTART, RLENGTH); 
                  sub(/href=/,"", string);
                  sub(/>Previous</,"", string);
                  printf("<li><a href=%s><span>Prev<\/span><\/a><\/li>\n", string);
               } 
            }' $f >> tmp.html

        awk '/^ *Go to/ {
               if (match($0, /href=[^>]*>Up</)) {
                  string=substr($0, RSTART, RLENGTH); 
                  sub(/href=/,"", string);
                  sub(/>Up</,"", string);
                  printf("<li><a href=%s><span>Up<\/span><\/a><\/li>\n", string);
               } 
            }' $f >> tmp.html

        awk '/^ *Go to/ {
               if (match($0, /href=[^>]*>Next</)) {
                  string=substr($0, RSTART, RLENGTH); 
                  sub(/href=/,"", string);
                  sub(/>Next</,"", string);
                  printf("<li><a href=%s><span>Next<\/span><\/a><\/li>\n", string);
               } 
            }' $f >> tmp.html

        awk 'BEGIN { 
                printing = 0;
             }
             /<a href=\"pages.html\"><span>Related\&nbsp;Pages<\/span><\/a>/ {
                printing = 1;
             }
             {if (printing) {print;}}' $f >> tmp.html

        mv -f tmp.html $f  
    
        awk '
#             /^ *Go to/ {
#                if (match($0, />Home</)) {
#                   sub(/Home/, "<img src=\"OpenClovis_GoHome.png\" alt=\"OpenClovis_GoHome.png\" border=\"0\">");
#                }
#
#                if (match($0, />Up</)) {
#                   sub(/Up/, "<img src=\"OpenClovis_UpArrow.png\" alt=\"OpenClovis_UpArrow.png\" border=\"0\">");
#                }
#
#                if (match($0, />Next</)) {
#                   sub(/Next/, "<img src=\"OpenClovis_RightArrow.png\" alt=\"OpenClovis_RightArrow.png\" border=\"0\">");
#                }
#
#                if (match($0, />Previous</)) {
#                   sub(/Previous/, "<img src=\"OpenClovis_LeftArrow.png\" alt=\"OpenClovis_LeftArrow.png\" border=\"0\">");
#                }
#
#                gsub(/,/,"");
#                sub(/^ *Go to/, "");
#             } 

             /^ *Go to/ {
                if (match($0, />Home</)) {next;}
             }      

            {print;}' $f > tmp.html && mv -f tmp.html $f  
        
    else 
        echo "Singlepage format"

#        awk '/^ *Back to/ {
#                if (match($0, />Top</)) {
#                    sub(/>Top</, "><img src=\"OpenClovis_UpArrow.png\" alt=\"OpenClovis_UpArrow.png\" border=\"0\"><");
#                }
#
#                gsub(/,/,"");
#                sub(/^ *Back to/, "");
#            }
#        
#            {print;}' $f > tmp.html && mv -f tmp.html $f 
                       
    fi
    
    rm format.txt
    
done
