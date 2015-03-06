#!/usr/bin/awk -f
#
# count lines in a C program
#

BEGIN {
    # get largest file-name
    maxlen = 4;
    for (i=1; i<ARGC; i++) {
        if (length(ARGV[i])>maxlen) { maxlen = length(ARGV[i]); }
    }
    fmt = sprintf("%%%ds : %%7s %%7s ( %%7s )\n", maxlen);
    printf fmt, "file", "lines", "C-lines", "preproc. directives"
}

{
    if (file == "") {
        file = FILENAME
    }
    if (file != FILENAME) {
        printf fmt, file, nl, cl, ppd
        file = FILENAME
        tnl += nl
        tcl += cl
        tppd += ppd
        nl = 0
        cl = 0
        ppd = 0
    }
    nl++
    if ($0 == "") { ; }
    else if ($1 ~ /^\/\//) { ; }
    else if ($1 ~ /^\/\*/ && $NF ~ /\*\/$/) { ; }
    else if ($0 ~ /\/\*/ && $0 !~ /\*\//) { in_comment = 1 }
    else if ($0 !~ /\/\*/ && $0 ~ /\*\//) { in_comment = 0 }
    else if (in_comment) { ; }
    else if ($1 ~ /^#/) { ppd++ }
    else { cl++ }
}
 
END {
    printf fmt, file, nl, cl, ppd
    tnl += nl
    tcl += cl
    tppd += ppd
    printf fmt, "Total:", tnl, tcl, tppd, tcl-tppd
}
