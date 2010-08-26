#!/usr/bin/awk -f

################################################################################
#
#                      Reformat wiki based latex pages 
#
################################################################################
# Usage:
#    %s index.tex  
#
# Example:
#
################################################################################

BEGIN {
   n = 0;
   want_printing = 0;
   inpreface = 0;
   inappendix = 0;
   intable = 0;
   intablewcaption = 0;
   n_rows_intable = 0;
}

#-------------------------------------------------------------------------------
# Remove Table of contents on top created for HTML format
#-------------------------------------------------------------------------------
/\label{/ {
   n++;
   if (n == 2) {
     want_printing = 1;
   }
}

#-------------------------------------------------------------------------------
# Remove jumps to the Top
#-------------------------------------------------------------------------------

/Back to \\href{#top}{\\tt Top}/ {
   gsub(/Back to \\href{#top}{\\tt Top}/,""); 
}

#-------------------------------------------------------------------------------
# Replace headings 0 1 2 3 
#-------------------------------------------------------------------------------

/\\subsection{/ {
     sub(/\\subsection{/,"\\chapter{"); 
}

/\\subsubsection{/ {
     sub(/\\subsubsection{/,"\\section{"); 
}

/\\paragraph{/ {
     sub(/\\paragraph{/,"\\subsection{"); 
}

/\\subparagraph{/ {
     sub(/\\subparagraph{/,"\\subsubsection{"); 
}

#-------------------------------------------------------------------------------
# Do not number Preface, but include in table of contents 
# Sections and Subsections in Preface are not numbered and not included in toc.
#-------------------------------------------------------------------------------

/\\chapter{/ {
     if (match($0, /\\chapter{Preface}/)) {      
        inpreface = 1;
        sub(/\\chapter{Preface}/,"\\chapter\*{Preface} \\addcontentsline{toc}{chapter}{Preface}");
     } else {
        inpreface = 0; 
     }   
}

{ if (inpreface) {
     if (match($0,/\\section{/)) {
        sub(/\\section{/, "\\section\*{");
     } 
     if (match($0,/\\subsection{/)) {
        sub(/\\subsection{/, "\\subsection\*{");
     } 
     
}}  

      
# Replacing above part by this will include the unnumbered sections and subsections in toc, 
# but it did not look good
#{ if (inpreface) {
#     if (match($0,/\\section{[^}]*}/)) {
#        title = substr($0, RSTART, RLENGTH);
#        sub(/\\section{/, "", title);
#        sub(/}/, "", title);
#        string = sprintf("\\section\*{%s} \\addcontentsline{toc}{section}{%s}", title, title);
#        sub(/\\section{[^}]*}/, string);
#     } 
#     if (match($0,/\\subsection{[^}]*}/)) {
#        subtitle = substr($0, RSTART, RLENGTH);
#        sub(/\\subsection{/, "", subtitle);
#        sub(/}/, "", subtitle);
#        string = sprintf("\\subsection\*{%s} \\addcontentsline{toc}{subsection}{%s}", subtitle, subtitle);
#        sub(/\\subsection{[^}]*}/, string);
#     } 
#
#}}        

#-------------------------------------------------------------------------------
# Do not number Appendix, but include in table of contents 
# Sections and Subsections in Appendix are not numbered and not included in toc.
#-------------------------------------------------------------------------------

/\\chapter{/ {
     if (match($0, /\\chapter{Appendix[^}]*}/)) { 
        title = substr($0, RSTART, RLENGTH);
        sub(/\\chapter{/, "", title);
        sub(/}/, "", title);
        string = sprintf("\\chapter\*{%s} \\addcontentsline{toc}{chapter}{%s}", title, title);
        sub(/\\chapter{Appendix[^}]*}/, string);          
        inappendix = 1;
     } else {
        inappendix = 0; 
     }   
}

{ if (inappendix) {
     if (match($0,/\\section{/)) {
        sub(/\\section{/, "\\section\*{");
     } 
     if (match($0,/\\subsection{/)) {
        sub(/\\subsection{/, "\\subsection\*{");
     } 
     
}}  

#-------------------------------------------------------------------------------
# For tables with no caption that have less than or equal 2 rows, it keeps 
# table together, does not let it flow over next page
# i.e: add \begin{table}[h]  and \end{table}
#-------------------------------------------------------------------------------

# we set variable intablewcaption if it is a table with caption--------------------------------------------- 
/\\begin{table}\[h\]/ {
   intablewcaption = 1;
}

/\\end{table}/ {
   intablewcaption = 0;
}

# find where table starts-------------------------------------------------------
/\\begin{TabularC}/ {
   intable = 1;
   n_rows_intable = 0;   
   if ( ! intablewcaption) {
      contentintable = $0;
      next;
   }
}

# at the end of table without caption, if it has less rows than three modify ---
# and print otherwise leave as it is -------------------------------------------
/\\end{TabularC}/ {
   intable = 0;
   if ( ! intablewcaption) {         
      contentintable = sprintf("%s \n %s", contentintable, $0); 
      if ( n_rows_intable<3 ) {
            sub(/\\begin{TabularC}/, "\\begin{table}\[H\]\\begin{TabularC}", contentintable);
            sub(/\\end{TabularC}/, "\\end{TabularC}\\end{table}", contentintable);
      }
      print contentintable;
      next;         
   }      
}

# if we are in table without caption, count rows and save them------------------
{  if ( intable &&  ! intablewcaption) {
      contentintable = sprintf("%s \n %s", contentintable, $0); 
      if (match($0, /\\\\hline/)) {
        ++n_rows_intable;
      }
      next;
   }
}

#-------------------------------------------------------------------------------
# Keep tables in place
#-------------------------------------------------------------------------------

/\\begin{table}\[h\]/ {
     sub(/\\begin{table}\[h\]/,"\\begin{table}\[H\]"); 
}

#-------------------------------------------------------------------------------
#  Print lines 
#-------------------------------------------------------------------------------

{ if (want_printing) {print;}}

#-------------------------------------------------------------------------------

END { 
}


