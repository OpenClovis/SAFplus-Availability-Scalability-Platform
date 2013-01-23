#!/usr/bin/awk -f

BEGIN {
   regexp = ARGV[1];
   input  = ARGV[2];
   delete ARGV[1];
   storing = 0;
   n_comments = 0;
   printf("<table border=\"0\" cellpadding=\"3\" cellspacing=\"3\">\n");
   if (match(input,/clCommon.h/)) {
      printf("<tr style=\"color:\#66b154; background:\#09477c\"><td><b>Value</b></td><td width="400"><b>Identifier</b></td><td width="350"><b>Component</b></td></tr>\n");
   }   
   if (match(input,/Error.*.h/)) {
      printf("<tr style=\"color:\#66b154; background:\#09477c\"><td><b>Value</b></td><td width="400"><b>Error Code</b></td><td width="350"><b>Comments</b></td></tr>\n");
   }   
}

/^ *\/\*\*[^*]/ || /^ *\/\*\*$/ {
   if (n_comments>0) {comment="";};
   n_comments++;
   storing = 1;
}

{if (storing) {
   newcomment = sprintf("%s%s",comment,$0);
   comment = newcomment;
   }
}

/\*\// {storing = 0;} 

/ *CL_RC/ {next;} # Macro definitions       

{if (match(input,/clCommon.h/)) {
   if (match($0, regexp)) {  
      match($3, /0x[0-9a-fA-F]*/);
      string=substr($3, RSTART, RLENGTH);
      sub(/^ *\/\*\*/,"",comment);
      sub(/\*\//,"",comment);
      sub(/ *\* */,"",comment);            
      printf("<tr style=\"color:\#ffffff; background:\#549cc6\"><td>%s</td><td>\\anchor apirefs_errorcodes_compid_%s %s</td><td>%s</td></tr>\n", string, $1, $1, comment); 
      n_comments=0; 
      comment="";   
   }
}} 
            
{if (match(input,/Error.*.h/)) {
   if (match($0, /^ *# *define/)) {
      if (match($0, regexp)) {
         if (match($3, /0x[0-9a-fA-F]*/)) {
            string=substr($3, RSTART, RLENGTH);
            sub(/^ *\/\*\*/,"",comment);
            sub(/\*\//,"",comment);
            gsub(/ *\*/,"",comment);            
            printf("<tr style=\"color:\#ffffff; background:\#549cc6\"><td>%s</td><td>%s</td><td>%s</td></tr>\n", string, $2, comment); 
            n_comments=0; 
            comment="";
         }
      }
   }
}}

END {
   printf("</table>\n");
}    
