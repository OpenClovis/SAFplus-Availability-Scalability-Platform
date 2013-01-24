#!/usr/bin/awk -f
#
# Short script to reformat callback function documentation
#

# Find typedefs in documentation
/<td class=\"memname\">typedef/ {

    # Find callback functions
    if (match($0, /T<\/a>\([^\*]/)) {
    
        string    = substr($0, RSTART, RLENGTH);
        html_line = $0;
        sub(/T<\/a>\(/,"T</a>,(,", string);
        sub(/T<\/a>\([^\*]/, string, html_line); 
               
        num_arg = split(html_line, a, ",");
        sub(/\) *<\/td>/, "", a[num_arg]);
        
        if (num_arg == 3) {
            
            if (match( a[3],/[a-zA-Z]+/)) {
            
               # print function name
               printf("%s</td>\n", a[1]);
               printf("<td>%s</td>", a[2]);
               
               n = split(a[3], b, " ");
               substring = a[3];            
               substring = substr(substring, 1, length(substring)-length(b[n]));
            
               # print parameter
               printf("<td class=\"paramtype\"> %s </td>\n", substring);
               printf("<td class=\"paramname\"> <em> %s </em> </td>\n", b[n]);
               printf("<td>)</td>\n<td width=\"100%\"></td>\n</tr>\n");
               next;
            } else {

               print $0;
               next;
            
            }
            
        } else if (num_arg > 3) {
       
            # print function name
            printf("%s</td>\n", a[1]);
            printf("<td>%s</td>", a[2]);
            
            n = split(a[3], b, " ");
            substring = a[3];            
            substring = substr(substring, 1, length(substring)-length(b[n]));
             
            printf("<td class=\"paramtype\"> %s </td>\n", substring);
            printf("<td class=\"paramname\"> <em> %s </em>, </td>\n</tr>\n", b[n]);             

            for ( x=4; x<=num_arg; x+=1) {

                n = split(a[x], b, " ");
                substring = a[x];          
                substring = substr(substring, 1, length(substring)-length(b[n]));
                                
                printf("<tr>\n<td class=\"paramkey\"></td>\n<td></td>\n");              
                printf("<td class=\"paramtype\"> %s </td>\n", substring);
                if (x == num_arg) {
                   printf("<td class=\"paramname\"> <em> %s </em> </td>\n</tr>\n", b[n]);
                   printf("<tr>\n<td></td>\n<td>)</td>\n<td></td><td></td><td width=\"100%\"></td>\n");
                } else {
                   printf("<td class=\"paramname\"> <em> %s </em>, </td>\n</tr>\n", b[n]);             
                }
            }
            next;
        }
    } 
}

{print;}
