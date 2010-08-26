#!/bin/bash

###############################################################################
#
#              CONVERT WIKI PAGE INTO HTML AND PDF DOCUMENT
#
#------------------------------------------------------------------------------
#
# Usage:  ./convert_wiki_to_html_pdf.sh 
# All input parameters are defined in docconfig_general.sh, 
# update it before running the script
#
###############################################################################

function error () {
    echo "ERROR: $1"
    exit 1
}

# arg 1:    level
#     2:    message
function dbmsg () {
    if [ $1 -le $debug_level ]; then
        echo "$2"
    fi
}

usage="convert_wiki_to_html_pdf.sh - downloads wiki pages and creates html 
and pdf documents. 

Usage: 
convert_wiki_to_html_pdf.sh -c doc_config_file [-g] [-p]

Options:
 -c doc_config_file  mandatory option. File contains all parameters required
                     for generating the html and pdf docs.
 -g                  disables downloading wiki pages, converter uses wikipage 
                     content file (*.wiki) as a source file if it exists in 
                     local directory.
 -p                  skips creating pdf format"

debug_level=0       # default is no debug messages
do_wikigrab=1       # default is to do wikigrab, can be disabled by -g
do_pdf=1            # default is to create pdf, can be disabled by -p

DOCTOOLS_DIR=$(cd $(dirname $0); pwd)
dbmsg 1 "DOCTOOLS_DIR:      $DOCTOOLS_DIR"
got_config=0
while getopts hc:vgp OPTION ; do
    case "$OPTION" in
        c)  configfile="$OPTARG"
            if [ -f $configfile ]; then
                source $configfile
                got_config=1
            else 
                error "Config file $configfile not found!" 
            fi
            ;;
        g)  do_wikigrab=0
            ;;
        p)  do_pdf=0
            ;;
        v)  debug_level=1
            ;;
        h)  echo "$usage"
            exit 0
            ;;
    esac
done

if [ $got_config = 0 ]; then
    echo "ERROR: needs at least one config file"
    echo "$usage"
    exit 1
fi

# A few derived variables:
wikigrab="$DOCTOOLS_DIR/../../tools/wikigrab/wikigrab.py"
dbmsg 1 "wikigrab:        $wikigrab"

#------------------------------------------------------------------------------
# download the doc from wiki to file <DOCID>.wiki
#------------------------------------------------------------------------------
if [ $do_wikigrab = 1 ]; then
    rm "$DOCID".wiki
    result=0
    echo "Step 1: Downloading wiki page"
    if [ "$INPUT_WIKI_FORMAT" = "singlepage" ]; then
       echo $wikigrab -rg http://$WIKI_IP/wiki $WIKI_USER $WIKI_PW $WIKI_URL
       echo to file $DOCID.wiki
       $wikigrab -rg http://$WIKI_IP/wiki $WIKI_USER $WIKI_PW $WIKI_URL > $DOCID.wiki     
       result=$?   
    elif [ "$INPUT_WIKI_FORMAT" = "multipage" ]; then
       echo $wikigrab -rg http://$WIKI_IP/wiki $WIKI_USER $WIKI_PW $WIKI_URL
       echo to file $DOCID.wikisum
       $wikigrab -rg http://$WIKI_IP/wiki $WIKI_USER $WIKI_PW $WIKI_URL > $DOCID.wikisum
       result=$?
       echo awk to file.txt
       awk '/^ *<!--/ {next;}
            /^ *[^*]/ {next;}        
            /\[\[.*\]\]/ {
            match($0, /^\*+/);
            sub(/\** */,"");
            sub(/\[\[/,"");
            sub(/\]\]/,"");
            split($0, a, "|");
            gsub(/ /,"",a[1]);
            printf("%s=%d\n", a[1], RLENGTH-1);
       }' $DOCID.wikisum > file.txt
       echo Now convert to $DOCID.wiki
       for line in `cat file.txt`; do
         file=`echo $line | awk -F= '{print $1}'`
         indent=`echo $line | awk -F= '{print $2}'` 
         echo "$file"

         pad=`echo | awk '{print substr("==============", 0, n)}' n=$indent`         
         $wikigrab -rg http://$WIKI_IP/wiki $WIKI_USER $WIKI_PW $file | \
             sed -r "s/(=(=)+)/\1$pad/g" >> $DOCID.wiki
       done 
      #rm $DOCID.wikisum
      #rm file.txt
    elif [ "$INPUT_WIKI_FORMAT" = "xmlpage" ]; then
       $wikigrab -x  http://$WIKI_IP/wiki $WIKI_USER $WIKI_PW $WIKI_URL > $DOCID.wiki     
       result=$?   
    else 
      echo "Wrong <format_in> value: $INPUT_WIKI_FORMAT!"
      exit 1
    fi
    if [ $result = 1 ]; then
      exit 1
    fi
fi # do_wikigrab

#------------------------------------------------------------------------------
# convert downloaded wiki page to doxygen format  
#------------------------------------------------------------------------------
echo "Step 2: Converting wiki page to doxygen page"

#------------------------------------------------------------------------------
# clean directory 
#
rm "$DOCID".txt
rm "$DOCID".tag

#------------------------------------------------------------------------------
# convert wiki format to doxygen format: <doc-name>.wiki---><doc-name>.txt 
#
echo "Step 2A: Basic converting wiki format to doxygen format: output format is $OUTPUT_HTML_FORMAT"
$DOCTOOLS_DIR/convert_wiki_to_doxygen.awk $DOCID $OUTPUT_HTML_FORMAT $DOCID.wiki > $DOCID.txt

#------------------------------------------------------------------------------
# resize figures if needed in the latex output
# 
echo "Step 2B: Resizing figures to fit page in pdf format"
$DOCTOOLS_DIR/resize_figures.awk $DOCID.txt > tmp.txt && mv -f tmp.txt $DOCID.txt

#------------------------------------------------------------------------------
# create table of contents saved in toc.txt 
#
echo "Step 2C: Creating the Table of Contents"
$DOCTOOLS_DIR/add_toc_to_doc.py "$PRODUCT_NAME $DOCTITLE" $OUTPUT_HTML_FORMAT $DOCID.txt tmp.txt && mv -f tmp.txt $DOCID.txt

#------------------------------------------------------------------------------
# create output html format
#------------------------------------------------------------------------------
echo "Step 3: Create html output at $HTMLDIR, doxygen config is $DOXYFILE"
rm $HTMLDIR/*.html
sed -e "s/DATE/on `date` /g" $DOCTOOLS_DIR/clovis_footer.html.in > clovis_footer.html
sed -e "s/DOCTITLE/$PRODUCT_NAME $DOCTITLE/g" $DOCTOOLS_DIR/clovis_header.html.in > clovis_header.html
cp $DOCTOOLS_DIR/clovisdoc.css clovisdoc.css
#sed -e "s/^PROJECT_NUMBER\s*=\s*/PROJECT_NUMBER = \"Rev$REVISION\"/g" $DOXYFILE > Doxyfile
doxygen $DOXYFILE
cp $DOCTOOLS_DIR/OpenClovis_Logo.png $DOCTOOLS_DIR/bg9.png $HTMLDIR
cp *.png $HTMLDIR
(cd $HTMLDIR; $DOCTOOLS_DIR/reformat_wiki_html_pages.sh)
cp latex/index.tex latex/index.tex.orig
cp latex/refman.tex latex/refman.tex.orig
echo "html pages are created"
 
#------------------------------------------------------------------------------
# create output pdf format
#------------------------------------------------------------------------------
if [ $do_pdf = "1" -a "$OUTPUT_HTML_FORMAT" = "singlepage" ]; then
    echo "Step 4: Create pdf output"
    cp latex/index.tex.orig latex/index.tex
    cp latex/refman.tex.orig latex/refman.tex
    cp $DOCTOOLS_DIR/refman.tex latex
    $DOCTOOLS_DIR/reformat_wiki_latex_pages.awk latex/index.tex > latex/tmp.tex; 
    mv latex/tmp.tex latex/index.tex;
    cp $DOCTOOLS_DIR/docparams.tex latex/tmp.tex
    sed -e "s/DOCUMENT/$DOCTITLE/g" \
	    -e "s/PRODUCT_NAME/$PRODUCT_NAME/g" \
	    -e "s/RELEASE/$RELEASE/g" \
	    -e "s/PARTNUMBER/$PARTNUMBER/g" \
	    -e "s/REVISION/$REVISION/g" \
        -e "s/DATE/`date +%Y-%B-%d`/g" latex/tmp.tex > latex/docparams.tex
    if [ "$OUTPUT_PDF_FORMAT" = "book" ]; then    
       cp $DOCTOOLS_DIR/clovisdoc.sty latex/clovisdoc.sty
    elif [ "$OUTPUT_PDF_FORMAT" = "article" ]; then
       cp $DOCTOOLS_DIR/clovisdoc.sty.article latex/clovisdoc.sty
    else 
       error "The output pdf format $OUTPUT_PDF_FORMAT is not valid!"     
    fi
    cp $DOCTOOLS_DIR/doxygen.sty latex
    cp $DOCTOOLS_DIR/openclovis_logo.png latex
    cp $DOCTOOLS_DIR/bg3.png latex
    cp *.png latex
    (cd latex; make; make; make)
    mkdir -p $PDFDIR
    pdf_file=$PDFDIR/"$COMMON_PREFIX"_"$DOCID"_"$RELEASE".pdf
    cp latex/refman.pdf $pdf_file
    echo "PDF file $pdf_file created"
fi








