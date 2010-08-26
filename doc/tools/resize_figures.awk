#!/usr/bin/awk -f

################################################################################
#
#     RESIZE FIGURES IF NEEDED
#
################################################################################

BEGIN {

   MAXWIDTH=392.36 #This is with margin; full width is 472.03 
   MAXHEIGHT=520.0 #604.34 the real height, space left for caption
}
#------------------------------------------------------------------------------
# 
#------------------------------------------------------------------------------
/\\image latex/ {
    cmd = sprintf("identify %s", $3);   # $3 is the name of the image file
                                        # identify is an ImageMagick exe that
                                        # prints image geometry info
    cmd | getline output;               # we capture image info in var output
    split(output, res, " ");
    split(res[3], geo, "x");
    image_width  = geo[1];
    image_height = geo[2];
    
    widthratio = image_width/MAXWIDTH;
    heightratio = image_height/MAXHEIGHT;
    if (widthratio > heightratio) {
        if (widthratio > 1.0) {
            print $0, "width=\\textwidth";
        } else {
            print $0;
        }           
    } else {
        if (heightratio > 1.0) {
            print $0, "height=0.85\\textheight"; # 0.85 ~ 520.0/604.34
        } else {
            print $0;
        }           
    }
    next;
}

{
    print;
}
