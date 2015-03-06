#!/usr/bin/awk -f

BEGIN {
    FS = "/";
}

# Filter out lines conatining only //-style comment
/^[:space:]*\/\// {
    next;
}

# Filter out empty lines
in_comment == 0 && NF == 0 {
    next;
}



{
#    printf "Raw line:|%s|\n", $0;
#    printf "Split line (%d fields):|", NF
#    for (i=1; i<=NF; i++) {
#        printf "%s|", $i;
#    }
#    printf "\n";
#    printf "Processed line:"
    printed = 0;
    for (i=1; i<=NF; i++) {
        if (i>1 && substr($i, 1, 1) == "*") {
            in_comment = 1;
        }
        if (in_comment == 0) {
            if (match($i, "[^[:space:]]") > 0)
            {
                printed += 1;
                printf "%s", $i;
            }
        }
        if (i<NF && substr($i, length($i), 1) == "*") {
            in_comment = 0;
        }
    }
    if (printed)
    {
        printf "\n";
    }
}
