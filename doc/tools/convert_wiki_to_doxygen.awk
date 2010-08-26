#!/usr/bin/awk -f

################################################################################
#
#                      WIKI-DOXYGEN CONVERTER
#
################################################################################
#
# Usage:
#    %s <docid> <output_format> <input_file> 
#
# Example:
#    %s installguide singlepage installguide.wiki
#   
#    <docid>:     unique identifier for doxygen, e.g.:
#                 evalguide
#                 installguide
#                 tutorial
#                 relnotes
#                 safcompliance
#                 ideguide
#                 sdkguide
#                 logtoolguide
#
#   <output_format>: format of html doc 
#                 singlepage
#                 multipage
#
#   <input_file>: input file in wiki format
#
################################################################################

BEGIN {
     ch = 0;
     h1 = 0;
     h2 = 0;
     h3 = 0;
     h4 = 0;
     h5 = 0;
     h6 = 0;
  
     internal = 0;
     incode_wiki = 0;
     incode_html = 0;
     intable = 0;
     inlist_wiki = 0;
     inlist_html = 0;

     firstpage = 1;
     newlineintable = 1;
     firstlineintable = 1;
     firsttitleintable = 1;
     firsteleminlineintable = 1;
     
     
     docid = ARGV[1];     
     format = ARGV[2];
     delete ARGV[1];
     delete ARGV[2];

     refid_prev   = sprintf("index");
     refid_act    = sprintf("index");
     refid_act_up = sprintf("index");    

}

#-------------------------------------------------------------------------------
# Intenal Doc part begins with {{internal-begin}} 
# Intenal Doc part ends with {{internal-end}} 
# Internal sections are not added to doc
#-------------------------------------------------------------------------------
# Intenal Doc part begins with {{internal-begin}} 
/\{\{internal-begin\}\}/ {
  internal = 1;
}

#-------------------------------------------------------------------------------
# Intenal Doc part ends with {{internal-end}} 
/\{\{internal-end\}\}/ {
  internal = 0;
  next;
}

#-------------------------------------------------------------------------------
# Internal sections should not be added to doc
{ if (internal) {next;}}

#-------------------------------------------------------------------------------
# Special Characters in the wiki pages has to be escaped in some cases  
# because doxygen uses it to detect special commands
#-------------------------------------------------------------------------------
#/\\/ {
#   gsub(/\\/, "\\\\");
#}

#-------------------------------------------------------------------------------
# Convert heading level 1 to page (multipage1,2,3), section (singlepage) with
# unique identifier and tag
#-------------------------------------------------------------------------------

/^==[^=]/ {
     sub(/^==/,""); 
     sub(/== *$/,"");
     gsub(/'''/,"");
     title = $0;
     gsub(/[ \/(),.:;-?]/,"");
     refname = sprintf("%s_%s", docid, $0);
     
     h1 = 0; 
     h2 = 0;
     h3 = 0;
     h4 = 0;
     h5 = 0;
     h6 = 0;
     
     if (firstpage) {
        if (match(title,/Preface/)) {
           ch=0;
        } else {
           ch=1;
        }
     } else {
        ch++;
     }
     
     refid_next = sprintf("%s_sec_%s", docid, ch);
     
     if ((format == "multipage1") || (format == "multipage2") || (format == "multipage3")) {
        if (!firstpage) {
           printf("\nGo to \\ref index \"Home\", \\ref %s \"Previous\", \\ref %s \"Up\", \\ref %s \"Next\" \n\n *\/ \n", refid_prev, refid_act_up, refid_next); 
           printf("\/** \n");
           printf("\\anchor %s\n", refname);     
           printf("\\page %s_sec_%s %s\n", docid, ch, title);                     
        } else {
           firstpage=0;       
           printf("\/** \n");
           printf("\\anchor %s\n", refname);     
           printf("\\page %s_sec_%s %s\n", docid, ch, title);                     
        }
        
        refid_prev = refid_act;               
        refid_act = sprintf("%s_sec_%s", docid, ch);
        refid_act_up = sprintf("index");   
         
     } else {
        if (!firstpage) {
           printf("\nBack to <a href=\"#top\">Top</a>\n");
           printf("\\anchor %s\n", refname);     
           printf("\\section %s_sec_%s %s\n", docid, ch, title);
        } else {
           firstpage=0;
           printf("\\anchor %s\n", refname);     
           printf("\\section %s_sec_%s %s\n", docid, ch, title);
        }     
     }        
     
     next;
}

#-------------------------------------------------------------------------------
# Convert heading level 2 to page (multipage2, multipage3), section (multipage1),
# subsection (singlepage) with unique identifier and tag 
#-------------------------------------------------------------------------------

/^===[^=]/ {
     sub(/^===/,""); 
     sub(/=== *$/,""); 
     title = $0;
     gsub(/[ \/(),.:;-?]/, "");
     refname = sprintf("%s_%s", docid, $0);
     h2 = 0;
     h3 = 0;
     h4 = 0;
     h5 = 0;
     h6 = 0;
     h1++;

     refid_next = sprintf("%s_sec_%s_%s", docid, ch, h1);
     
     if ((format == "multipage2")  || (format == "multipage3")) {
        printf("\nGo to \\ref index \"Home\", \\ref %s \"Previous\", \\ref %s \"Up\", \\ref %s \"Next\" \n\n *\/ \n", refid_prev, refid_act_up, refid_next); 
        printf("\/** \n");
        printf("\\anchor %s\n", refname);
        printf("\\page %s_sec_%s_%s %s\n", docid, ch, h1, title);        
        refid_prev   = refid_act;               
        refid_act    = sprintf("%s_sec_%s_%s", docid, ch, h1);    
        refid_act_up = sprintf("%s_sec_%s", docid, ch);                          
     } else if (format == "multipage1") {
        printf("\nBack to <a href=\"#top\">Top</a>\n");
        printf("\\anchor %s\n", refname);
        printf("\\section %s_sec_%s_%s %s\n", docid, ch, h1, title);     
     } else if (format == "singlepage") {
        printf("\nBack to <a href=\"#top\">Top</a>\n");
        printf("\\anchor %s\n", refname);
        printf("\\subsection %s_sec_%s_%s %s\n", docid, ch, h1, title);     
     }  

     next;   
}

#-------------------------------------------------------------------------------
# Convert heading level 3 to page, section, subsection, subsubsection  
# with unique identifier and unique tag for different output format
#-------------------------------------------------------------------------------
     
/^====[^=]/ {

     sub(/^====/,""); 
     sub(/==== *$/,""); 
     title = $0;
     gsub(/[ \/(),.:;-?]/, "");
     refname = sprintf("%s_%s", docid, $0);
     h3 = 0; 
     h4 = 0; 
     h5 = 0; 
     h6 = 0; 
     h2++;    

     refid_next = sprintf("%s_sec_%s_%s_%s", docid, ch, h1, h2);

     if (format == "multipage3") {
        printf("\nGo to \\ref index \"Home\", \\ref %s \"Previous\", \\ref %s \"Up\", \\ref %s \"Next\" \n\n *\/ \n", refid_prev, refid_act_up, refid_next); 
        printf("\/** \n");
        printf("\\anchor %s\n", refname);    
        printf("\\page %s_sec_%s_%s_%s %s\n", 
               docid, ch, h1, h2, title);
        refid_prev   = refid_act;               
        refid_act    = sprintf("%s_sec_%s_%s_%s", docid, ch, h1, h2);
        refid_act_up = sprintf("%s_sec_%s_%s", docid, ch, h1);
     } else if (format == "multipage2") {
#        printf("\nBack to <a href=\"#top\">Top</a>\n");
        printf("\\anchor %s\n", refname);
        printf("\\section %s_sec_%s_%s_%s %s\n", 
               docid, ch, h1, h2, title);     
     } else if (format == "multipage1") {
#        printf("\nBack to <a href=\"#top\">Top</a>\n");
        printf("\\anchor %s\n", refname);
        printf("\\subsection %s_sec_%s_%s_%s %s\n", 
               docid, ch, h1, h2, title);     
     } else if (format == "singlepage") {
#        printf("\nBack to <a href=\"#top\">Top</a>\n");
        printf("\\anchor %s\n", refname);    
        printf("\\subsubsection %s_sec_%s_%s_%s %s\n", 
               docid, ch, h1, h2, title);
     } 
     next;

} 

#-------------------------------------------------------------------------------
# Convert heading level 4 to section, subsection, subsubsection and bold
# heading with unique identifier and unique tag for different output format
#-------------------------------------------------------------------------------

/^=====[^=]/ {
     sub(/^=====/,""); 
     sub(/===== *$/,""); 
     title = $0;
     gsub(/[ \/(),.:;-?]/, "");
     refname = sprintf("%s_%s", docid, $0);
     h4 = 0; 
     h5 = 0; 
     h6 = 0; 
     h3++;

     if (format == "multipage3") {
        printf("\nBack to <a href=\"#top\">Top</a>\n\n\n");
        printf("\\anchor %s\n", refname);             
        printf("\\section %s_sec_%s_%s_%s_%s  %s\n", 
               docid, ch, h1, h2, h3, title); 
     } else if (format == "multipage2") {
#        printf("\nBack to <a href=\"#top\">Top</a>\n\n\n");
        printf("\\anchor %s\n", refname);             
        printf("\\subsection %s_sec_%s_%s_%s_%s  %s\n", 
               docid, ch, h1, h2, h3, title); 
     } else if (format == "multipage1") {
#        printf("\nBack to <a href=\"#top\">Top</a>\n\n\n");
        printf("\\anchor %s\n", refname);             
        printf("\\subsubsection %s_sec_%s_%s_%s_%s  %s\n", 
               docid, ch, h1, h2, h3, title); 
     } else if (format == "singlepage") {
#        printf("\nBack to <a href=\"#top\">Top</a>\n\n\n");
        printf("\\anchor %s\n", refname);             
        printf("\\paragraph %s_sec_%s_%s_%s_%s  %s\n", 
               docid, ch, h1, h2, h3, title); 
     }               
     next;
     
}

#-------------------------------------------------------------------------------
# Convert heading level 5 to section, subsection, subsubsection and bold
# heading with unique identifier and unique tag for different output format
#-------------------------------------------------------------------------------

/^======[^=]/ {
     sub(/^======/,""); 
     sub(/====== *$/,""); 
     title = $0;
     gsub(/[ \/(),.:;-?]/, "");
     refname = sprintf("%s_%s", docid, $0);
     h5 = 0; 
     h6 = 0; 
     h4++;
     
     if (format == "multipage3") {
#        printf("\nBack to <a href=\"#top\">Top</a>\n\n\n");
        printf("\\anchor %s\n", refname);             
        printf("\\subsection %s_sec_%s_%s_%s_%s_%s  %s\n", 
               docid, ch, h1, h2, h3, h4, title); 
     } else if (format == "multipage2") {
#        printf("\nBack to <a href=\"#top\">Top</a>\n\n\n");
        printf("\\anchor %s\n", refname);             
        printf("\\subsubsection %s_sec_%s_%s_%s_%s_%s  %s\n", 
               docid, ch, h1, h2, h3, h4, title); 
     } else if (format == "multipage1") { 
#        printf("\nBack to <a href=\"#top\">Top</a>\n\n\n");
        printf("\\anchor %s\n", refname);             
        printf("\\paragraph %s_sec_%s_%s_%s_%s_%s  %s\n", 
               docid, ch, h1, h2, h3, h4, title); 
     }  else if (format == "singlepage") {           
#        printf("\nBack to <a href=\"#top\">Top</a>\n\n\n");
        printf("\\anchor %s\n", refname);             
        printf("<b>%s</b>\n\n", title); 
     }        
     next;
}

#-------------------------------------------------------------------------------
# Convert heading level 6 to section, subsection, subsubsection and bold
# heading with unique identifier and unique tag for different output format
#-------------------------------------------------------------------------------

/^=======[^=]/ {
     sub(/^=======/,""); 
     sub(/======= *$/,""); 
     title = $0;
     gsub(/[ \/(),.:;-?]/, "");
     refname = sprintf("%s_%s", docid, $0);
     h6 = 0; 
     h5++;
     
     if (format == "multipage3") {
#        printf("\nBack to <a href=\"#top\">Top</a>\n\n\n");
        printf("\\anchor %s\n", refname);             
        printf("\\subsubsection %s_sec_%s_%s_%s_%s_%s_%s  %s\n", 
               docid, ch, h1, h2, h3, h4, h5, title); 
     } else if (format == "multipage2")  {
#        printf("\nBack to <a href=\"#top\">Top</a>\n\n\n");
        printf("\\anchor %s\n", refname);             
        printf("\\paragraph %s_sec_%s_%s_%s_%s_%s_%s  %s\n", 
               docid, ch, h1, h2, h3, h4, h5, title);              
     } else if ((format == "multipage1") || (format == "singlepage")) {
#        printf("\nBack to <a href=\"#top\">Top</a>\n\n\n");
        printf("\\anchor %s\n", refname);             
        printf("<b>%s</b>\n\n", title); 
     }               
     next;
}

#-------------------------------------------------------------------------------
# Convert heading level 6 to section, subsection, subsubsection and bold
# heading with unique identifier and unique tag for different output format
#-------------------------------------------------------------------------------

/^========[^=]/ {
     sub(/^========/,""); 
     sub(/======== *$/,""); 
     title = $0;
     gsub(/[ \/(),.:;-?]/, "");
     refname = sprintf("%s_%s", docid, $0);
     h6++; 
     
     if (format == "multipage3") {
#        printf("\nBack to <a href=\"#top\">Top</a>\n\n\n");
        printf("\\anchor %s\n", refname);             
        printf("\\paragraph %s_sec_%s_%s_%s_%s_%s_%s_%s  %s\n", 
               docid, ch, h1, h2, h3, h4, h5, h6, title); 
     } else {
#       printf("\nBack to <a href=\"#top\">Top</a>\n\n\n");
        printf("\\anchor %s\n", refname);             
        printf("<b>%s</b>\n", title);  
     }                  
     next;
}

#-------------------------------------------------------------------------------
#  Bold and italic text in wiki format
#-------------------------------------------------------------------------------

# Replace bold & italic mode
/'''''/ {
    while (match($0,/'''''/)) {
       sub(/'''''/,"<b><em>");
       if (sub(/'''''/,"</em></b>") == 0){
          msg = sprintf("WARNING: Bold & italic mode was not closed! - %s", $0);
          print msg > "/dev/stderr";
       }    
   }
}

#-------------------------------------------------------------------------------

# Replace bold mode
/'''/ {
    while (match($0,/'''/)) {
       sub(/'''/,"<b>");
       if (sub(/'''/,"</b>") == 0){
          msg = sprintf("WARNING: Bold mode was not closed! - %s", $0);
          print msg > "/dev/stderr";
       }    
   }
}

#-------------------------------------------------------------------------------

# Replace italic mode
/''/ {
    while (match($0,/''/)) {
       sub(/''/,"<em>");
       if (sub(/''/,"</em>") == 0){
          msg = sprintf("WARNING: Italic mode was not closed!- %s", $0);
          print msg > "/dev/stderr";
       }    
   }
}

#-------------------------------------------------------------------------------
#  Bulleted list in wiki format
#-------------------------------------------------------------------------------
/^\*[^*#]/ {
   if (!incode_html) {sub(/^\*/,"- ");}
}

/^\*\*[^*#]/ {
   if (!incode_html) {sub(/^\*\*/,"\t - "); inlist_wiki = 1;}
}

/^\*\*\*[^*#]/ {
   if (!incode_html) {sub(/^\*\*\*/,"\t\t - "); inlist_wiki = 1;}
}

#-------------------------------------------------------------------------------
#  Bulleted list in html format 
/<ul>/ {
   inlist_html++;
#   print "begin of ul:", inlist_html > "/dev/stderr";
}
/<\/ul>/ {
   inlist_html--;
#   print "end of ul:", inlist_html > "/dev/stderr";
}

#-------------------------------------------------------------------------------
#  Numbered list in wiki format
#-------------------------------------------------------------------------------
/^\#[^*#]/ {
   if (!incode_html) {sub(/^\#/,"-\# ");}
}

/^\#\#[^*#]/ {
   if (!incode_html) {sub(/^\#\#/,"\t -\# ");  inlist_wiki = 1;}
}

/^\#\#\#[^*#]/ {
   if (!incode_html) {sub(/^\#\#\#/,"\t\t -\# ");  inlist_wiki = 1;}
}

# ------------------------------------------------------------------------------
#  Numbered list in html format 
/<ol>/ {
   inlist_html++;
#   print "begin of ol:", inlist_html > "/dev/stderr";
}
/<\/ol>/ {
   inlist_html--;
#   print "end of ol:", inlist_html > "/dev/stderr";
}

#-------------------------------------------------------------------------------
#  Mixture of bulleted and numbered sublist in wiki format
#-------------------------------------------------------------------------------
/^\*\#[^*#]/ {
   if (!incode_html) {sub(/^\*\#/,"\t -\# "); inlist_wiki = 1;}
}

/^\#\*[^*#]/ {
   if (!incode_html) {sub(/^\#\*/,"\t - "); inlist_wiki = 1;}
}

/^\#\*\*[^*#]/ {
   if (!incode_html) {sub(/^\#\*\*/,"\t\t - "); inlist_wiki = 1;}
}

/^\*\#\*[^*#]/ {
   if (!incode_html) {sub(/^\*\#\*/,"\t\t - "); inlist_wiki = 1;}
}

/^\#\#\*[^*#]/ {
   if (!incode_html) {sub(/^\#\#\*/,"\t\t - "); inlist_wiki = 1;}
}

/^\#\*\#[^*#]/ {
   if (!incode_html) {sub(/^\#\*\#/,"\t\t -\# "); inlist_wiki = 1;}
}

/^\*\#\#[^*#]/ {
   if (!incode_html) {sub(/^\*\#\#/,"\t\t -\# "); inlist_wiki = 1;}
}

/^\*\*\#[^*#]/ {
   if (!incode_html) {sub(/^\*\*\#/,"\t\t -\# "); inlist_wiki = 1;}
}

#-------------------------------------------------------------------------------
#  Replace anchor link to doxygen format \anchor 
#-------------------------------------------------------------------------------
#<span id='Blade Resource Properties'></span>
/<span id=.*<\/span>/ {
    match($0,/<span id=.*<\/span>/ );
    string=substr($0, RSTART, RLENGTH); 
    sub(/<span id=['"]/,"", string); 
    sub(/['"]><\/span>/,"", string);
    gsub(/[ \/(),.:;-?]/,"", string);
    newstring = sprintf("\\anchor %s_%s\n", docid, string);
    sub(/<span id=.*<\/span>/, newstring)        
}

#-------------------------------------------------------------------------------
#  Image 
#-------------------------------------------------------------------------------
#[[Image:GettingStartedIDE_BladeResourceProperties.gif|frame|center| '''Blade Resource Properties''' ]]
/\[\[Image:/ {    
    match($0,/\[\[Image:[^\]]*\]\]/);
    string = substr($0, RSTART, RLENGTH);
    sub(/\[\[/,"", string); 
    sub(/Image:/,"", string); 
    sub(/\]\]/,"", string);
    num_arg = split(string, a, "|");
    if (num_arg == 1) {
       newstring = sprintf(\
       "\\latexonly \\includegraphics{%s} \\endlatexonly \n \\htmlonly <img src=\"%s\" alt=\"%s\"> \\endhtmlonly \n", \
       a[1], a[1],a[1]); 
    } else { 
       sub(/^ +/,"", a[num_arg]);
       sub(/ +$/,"", a[num_arg]);
       sub(/<b>/,"", a[num_arg]);
       sub(/<\/b>/,"", a[num_arg]);
       newstring = sprintf("\\image html %s \"%s\"\n\\image latex %s \"%s\"\n", 
                           a[1], a[num_arg], a[1], a[num_arg]);
    }
    sub(/\[\[Image:[^\]]*\]\]/, newstring);      
}

#-------------------------------------------------------------------------------
#  reference 
#-------------------------------------------------------------------------------
#[[#Resource Editor - With Hardware Resource | Resource Editor - With Hardware Resource]]
/\[\[\#.*\]\]/ {
    while (match($0, /\[\[\#[^\]]*\]\]/)) {
       match($0, /\[\[\#[^\]]*\]\]/);
       string=substr($0, RSTART, RLENGTH);
       sub(/\[\[\#/,"",string); 
       sub(/\]\]/,"",string);        
       split(string, a, "|");
       gsub(/[ \/(),.:;-?]/,"", a[1]);
       gsub(/^ +/,"", a[2]);
       newstring = sprintf("\\ref %s_%s \"%s\"", docid, a[1],a[2]);
       sub(/\[\[\#[^\]]*\]\]/, newstring)
    }
}

#-------------------------------------------------------------------------------
#[[Doc:sdkguide/infrastructure#Illustration of the usage of application type|Illustration of the usage of application type]].
#/\[\[Doc\:.*\]\]/ {
#   while (match($0, /\[\[Doc\:[^\]]*\]\]/)) {
#      string = substr($0, RSTART, RLENGTH);
#      if (match(string, /\#[^\]]*\]\]/)) {
#         substring=substr(string, RSTART, RLENGTH);
#         sub(/\#/,"",substring); 
#         sub(/\]\]/,"",substring);        
#         split(substring, a, "|");
#         gsub(/[ \/(),.:;-?]/,"", a[1]);
#         gsub(/^ +/,"", a[2]);
#         newstring = sprintf("\\ref %s_%s \"%s\"", docid, a[1],a[2]);
#         sub(/\[\[\Doc\:[^\]]*\]\]/, newstring)
#      }
#   }
#}

#-------------------------------------------------------------------------------
#[[System Test Plan Memory Management | Memory Management]]
/\[\[.*\|.*\]\]/ {
   while (match($0, /\[\[[^\]]*\]\]/)) {
         string = substr($0, RSTART, RLENGTH);
         sub(/\]\]/,"",string);        
         sub(/\[\[/,"",string);        
         split(string, a, "|");
         substring = a[2];
         gsub(/[ \/(),.:;-?]/,"", substring);
         newstring = sprintf("\\ref %s_%s \"%s\"", docid, substring, a[2]);
         sub(/\[\[.*\|.*\]\]/, newstring);               
   }
}

#-------------------------------------------------------------------------------
#[[System_Test_Plan_Memory_Management]]
/\[\[.*\]\]/ {
   while (match($0, /\[\[[^\]]*\]\]/)) {
         string = substr($0, RSTART, RLENGTH);
         sub(/ *\]\]/,"",string);        
         sub(/\[\[ */,"",string);        
         reftitle = string;
         refid = string;
         gsub(/_/," ", reftitle);
         gsub(/[ \/(),.:;-?]/,"", refid);
         newstring = sprintf("\\ref %s_%s \"%s\"", docid, refid, reftitle);
         sub(/\[\[.*\]\]/, newstring);               
   }
}

#-------------------------------------------------------------------------------
# Test case definition
# {{testcase | BIC-OSA-TSK-TC001 | OK    | This test is working }}
#-------------------------------------------------------------------------------
/\{\{testcase.*\}\}/ {
   while (match($0, /\{\{[^\}]*\}\}/)) {
         string = substr($0, RSTART, RLENGTH);
         sub(/\}\}/,"",string);        
         sub(/\{\{/,"",string);        
         split(string, a, "|");
         gsub(/\&/,"and",a[4]);        
#        newstring = sprintf("\\arg<b>%20s</b>%-10s%-s", a[2],a[3],a[4]);
#        Remove TBD - a[3]          
         newstring = sprintf("\\arg<b>%20s</b>%-s", a[2],a[4]);
         sub(/\{\{[^\}]*\}\}/, newstring);
   }
}
#-------------------------------------------------------------------------------
#  Remove TBDs
#-------------------------------------------------------------------------------
/'''''\[TBD *:.*\]'''''/ {
   gsub(/'''''\[TBD *:.*\]'''''/,""); 
}

/'''\[TBD *:.*\]'''/ {
   gsub(/'''\[TBD *:.*\]'''/,""); 
}

/\[TBD *:.*\]/ {
   gsub(/\[TBD *:.*\]/,""); 
}

#-------------------------------------------------------------------------------
/\(TBD *:.*\)/ {
   gsub(/\(TBD *:.*\)/,""); 
}

#-------------------------------------------------------------------------------
/\[Test program needs to be implemented\]/ {
   gsub(/\[Test program needs to be implemented\]/,""); 
}

#-------------------------------------------------------------------------------
#  Remove DONEs
#-------------------------------------------------------------------------------
/\[DONE\]/ {
   gsub(/\[DONE\]/,""); 
}

#-------------------------------------------------------------------------------
#  Remove Empty line character
#-------------------------------------------------------------------------------

#/<br>/ {
#   sub(/<br> */,"");
#}

#-------------------------------------------------------------------------------
#  Print tables 
#-------------------------------------------------------------------------------

/^\{\|/ {

   if (intable == 1) {
      msg = "WARNING: Table in table - Possibility of Error: Previous table was not closed!";
      print msg > "/dev/stderr";
   }    

   sub(/{\|/,""); 
   string = sprintf("<table %s>",$0);
   sub(/^.*$/,string);        
   intable = 1;
}

{if (intable) {

    if (match($0,/\|\+/)) {  
         sub(/\|\+/,""); 
         split($0, a, "|");         
         string = sprintf("<caption %s> %s </caption>",a[1],a[2]);
         sub(/^.*$/,string);        
    } 



   if (match($0,/\|\-/)) {  
      if (firstlineintable) {
         sub(/\|\-/,""); 
         string = sprintf("<tr %s>",$0);
         sub(/^.*$/,string);        
         firstlineintable = 0;
      }  else {
         sub(/\|\-/,""); 
         string = sprintf("<\/td><\/tr><tr %s>",$0);
         sub(/^.*$/,string);        
      }
      firsteleminlineintable = 1;
   } 
   

   if (match($0,/^ *\!/)) {
      sub(/\!/,"");
      if (match($0,/style/) || match($0,/align/) ||  match($0,/border/) || \
          match($0,/cellpadding/) ||  match($0,/cellspacing/)) {
          split($0, a, "|");
          if (firsttitleintable) {
             if  (firstlineintable) {
                string = sprintf("<tr><td %s> %s",a[1],a[2]);
                sub(/^.*$/,string);
                firsttitleintable = 0;
                firstlineintable = 0;
             } else {
                string = sprintf("<td %s> %s",a[1],a[2]);
                sub(/^.*$/,string);
                firsttitleintable=0;             
             }
          } else {
             string = sprintf("<\/td><td %s> %s",a[1],a[2]);
             sub(/^.*$/,string);
          }                   
       } else {
          if (firsttitleintable) {
             if  (firstlineintable) {
                string = sprintf("<tr><td> %s",$0);
                sub(/^.*$/,string);
                firsttitleintable = 0;
                firstlineintable = 0;
             } else {
                string = sprintf("<td> %s",$0);
                sub(/^.*$/,string);
                firsttitleintable=0;             
             }
          } else {
             string = sprintf("<\/td><td> %s",$0);
             sub(/^.*$/,string);
          }                        
       }
   }

   if (match($0,/^ *\|[^\+\}\|]/)) {
      sub(/\|/,"");
      if (match($0,/style/) || match($0,/align/) ||  match($0,/border/) || \
          match($0,/cellpadding/) ||  match($0,/cellspacing/)) { 
          split($0, a, "|");
          if (firsteleminlineintable) {
             if  (firstlineintable) {
                string = sprintf("<tr><td %s> %s",a[1],a[2]);
                sub(/^.*$/,string);
                firsteleminlineintable = 0;
                firstlineintable = 0;
             } else {
                string = sprintf("<td %s> %s",a[1],a[2]);
                sub(/^.*$/,string);
                firsteleminlineintable=0;             
             }
          } else {
             string = sprintf("<\/td><td %s> %s",a[1],a[2]);
             sub(/^.*$/,string);
          }                   
       } else {
          if (firsteleminlineintable) {
             if  (firstlineintable) {
                string = sprintf("<tr><td> %s",$0);
                sub(/^.*$/,string);
                firsteleminlineintable = 0;
                firstlineintable = 0;
             } else {
                string = sprintf("<td> %s",$0);
                sub(/^.*$/,string);
                firsteleminlineintable=0;             
             }
          } else {
             string = sprintf("<\/td><td> %s",$0);
             sub(/^.*$/,string);
          }                        
       }
   }
 
   if (match($0,/^ *\|$/)) {
      if (firsteleminlineintable) {
          if  (firstlineintable) {
             print "<tr><td>";            
             firsteleminlineintable = 0;
             firstlineintable = 0;
          } else {
             print "<td>";
             firsteleminlineintable=0;             
          }
       } else {
          print "<\/td><td>";          
       } 
       next;                  
   }

   if (match($0,/\|\|$/)) {  
        print "<\/td><td>"; 
        next;
   }
   
   if (match($0,/\!\!$/)) {  
        print "<\/td><td>"; 
        next;
   }

   if (match($0,/^$/)) {  
      next;
   }
     
}}

/\|\}/ {
   print "<\/td><\/tr><\/table>";
   intable = 0; 
   firstlineintable = 1;
   firsttitleintable = 1;
   next;
}

#-------------------------------------------------------------------------------
# Code section in wiki format
#-------------------------------------------------------------------------------

# If line starts with one or more spaces but not empty: this is a code line
# If you are not in code mode (incode = 0) set incode =1 and start code session
# with \code and print, otherwise just print 
#  
/^[ \t]+[^ \t]/ {
     if ((!intable) && (!inlist_html) && (!inlist_wiki) && (!incode_html)){
        if (!incode_wiki){
            incode_wiki = 1; print "\\code \n" $0; next;
        } else { 
            print; next;
        }
     }
}

# If we are in code mode (incode = 1) but line is empty or first character is
# not empty space switch off code mode: incode = 0 
/^[^ ]/ || /^$/ {
     if ((!intable) && (!inlist_html) && (!incode_html)) {
        if (incode_wiki) {
           incode_wiki = 0; print "\\endcode";
        }
    }
}  

#-------------------------------------------------------------------------------

# If HTML commands are used
/<code><pre>/ {
    gsub(/<code><pre>/," \\code "); 
    incode_html = 1;
}

/<\/pre><\/code>/ {
   gsub(/<\/pre><\/code>/," \\endcode "); 
   incode_html = 0;
}
 
#-------------------------------------------------------------------------------
#  Adding link to related documents 
#-------------------------------------------------------------------------------
/OpenClovis Installation Guide/ {
   if ( docid != "installguide") {
      sub(/OpenClovis Installation Guide/, " \\htmlonly <a href=\"../installguide/index.html\"> \\endhtmlonly OpenClovis Installation Guide \\htmlonly </a> \\endhtmlonly "); 
   }
}

/OpenClovis Release Notes/ {
   if ( docid != "relnotes") {
      sub(/OpenClovis Release Notes/," \\htmlonly <a href=\"../relnotes/index.html\"> \\endhtmlonly OpenClovis Release Notes \\htmlonly </a> \\endhtmlonly "); 
   }
}

/OpenClovis SA Forum Compliance/ {
   if ( docid != "safcompliance") {
      sub(/OpenClovis SA Forum Compliance/," \\htmlonly <a href=\"../safcompliance/index.html\"> \\endhtmlonly OpenClovis SA Forum Compliance \\htmlonly </a> \\endhtmlonly "); 
   }
}

/OpenClovis Sample Application Tutorial/ {
   if ( docid != "tutorial") {
      sub(/OpenClovis Sample Application Tutorial/," \\htmlonly <a href=\"../tutorial/index.html\"> \\endhtmlonly OpenClovis Sample Application Tutorial \\htmlonly </a> \\endhtmlonly "); 
   }
}

/OpenClovis SDK User Guide/ {
   if ( docid != "sdkguide") {
      sub(/OpenClovis SDK User Guide/," \\htmlonly <a href=\"../sdkguide/index.html\"> \\endhtmlonly OpenClovis SDK User Guide \\htmlonly </a> \\endhtmlonly "); 
   }
}

/OpenClovis IDE User Guide/ {
   if ( docid != "ideguide") {
      sub(/OpenClovis IDE User Guide/," \\htmlonly <a href=\"../ideguide/index.html\"> \\endhtmlonly OpenClovis IDE User Guide \\htmlonly </a> \\endhtmlonly "); 
   }
}

/OpenClovis Log Tool User Guide/ {
   if ( docid != "logtoolguide") {
      sub(/OpenClovis Log Tool User Guide/," \\htmlonly <a href=\"../logtoolguide/index.html\"> \\endhtmlonly OpenClovis Log Tool User Guide \\htmlonly </a> \\endhtmlonly "); 
   }
}

/OpenClovis Evaluation System User Guide/ {
   if ( docid != "evalguide") {
      sub(/OpenClovis Evaluation System User Guide/," \\htmlonly <a href=\"../evalguide/index.html\"> \\endhtmlonly OpenClovis Evaluation System User Guide \\htmlonly </a> \\endhtmlonly "); 
   }
}

/OpenClovis API Reference Guide/ {
   if ( docid != "apirefs") {
      sub(/OpenClovis API Reference Guide/," \\htmlonly <a href=\"../apirefs/index.html\"> \\endhtmlonly OpenClovis API Reference Guide \\htmlonly </a> \\endhtmlonly "); 
   }
}

/OpenClovis ASP Console Reference Guide/ {
   if ( docid != "aspconsole") {
      sub(/OpenClovis ASP Console Reference Guide/," \\htmlonly <a href=\"../aspconsole/index.html\"> \\endhtmlonly OpenClovis ASP Console Reference Guide \\htmlonly </a> \\endhtmlonly "); 
   }
}

#-------------------------------------------------------------------------------
#  Print lines (if it was a special line, it was already printed) 
#-------------------------------------------------------------------------------

{print; inlist_wiki = 0;}

#-------------------------------------------------------------------------------

END { 
   
   if (incode_wiki == 1) {
      incode_wiki = 0;
      print "\\endcode";
   }
   
   if (intable == 1) {
      msg = "WARNING: End of wiki page -  Table was not closed!";
      print msg > "/dev/stderr";
   }    
   
   if (inlist_html != 0) {
      msg = "WARNING: End of wiki page - HTML list is not closed (or closed many times)!";
      print msg > "/dev/stderr";
   }    

   if (incode_html == 1) {
      msg = "WARNING: End of wiki page - HTML code section is not closed!";
      print msg > "/dev/stderr";
   }    
      
   if ((format == "multipage1") || (format == "multipage2") || (format == "multipage3")) {
       printf("\nGo to \\ref index \"Home\", \\ref %s \"Previous\", \\ref %s \"Up\" \n\n *\/ \n", refid_prev, refid_act_up); 
   } else if (format == "singlepage") {
       printf("\nBack to <a href=\"#top\">Top</a>\n\*\/");
   }

}


