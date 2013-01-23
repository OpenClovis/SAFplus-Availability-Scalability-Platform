#!/bin/bash

#
# This script reformat classes.html to have the same format as all others (Functions, Typedefs etc.)
#

# Reformat page by adding new line character in front of new commands 
sed -e 's/<div class="qindex">/\n<div class="qindex">/g' \
    -e 's/<div class="ah">/\n<div class="ah">/g' \
    -e 's/<a class="qindex"/\n<a class="qindex"/g' \
    -e 's/<a class="el"/\n<a class="el"/g' \
    -e 's/<a name=/\n<a name=/g' \
    -e 's/<tr><td>/\n<tr><td>/g' \
    -e 's/<\/td><\/tr>/\n<\/td><\/tr>/g' \
$1 > tmp.html && mv -f tmp.html $1

# Delete table related commands in alphabetical listings
sed -e '/<table align=/d'  \
    -e '/<a name=/d'  \
    -e '/<tr><td>/d' \
    -e '/<\/td><\/tr>/d'  \
    -e '/^$/d' \
    $1 > tmp.html && mv -f tmp.html $1

#------------------------------------------------------------------------------
# Replace Letter icon tabs
sed -e 's/<div class="qindex">/<div class="tabs"><ul>/g' \
    -e 's/<\/div><p>$/\n<\/ul><\/div><p>\&nbsp;/g' \
    -e 's/\&nbsp;|\&nbsp;//g' \
$1 > tmp.html && mv -f tmp.html $1

for f in A B C D E F G H I J K L M N O P Q R S T U V W X Y Z; do
sed -e "s/<a class=\"qindex\" href=\"#letter_$f\">$f<\/a>/<li><a href=\"#index_$f\"><span>$f<\/span><\/a><\/li>/g" \
$1 > tmp.html && mv -f tmp.html $1
done

# Remove 2nd Letter icon tabs at the end of the page
awk '/<div class="tabs">/{n++} n==4{skip=1} !skip{print}
/<\/div>/{if(skip){skip=0; n++}}' $1 > tmp.html && mv -f tmp.html $1

#------------------------------------------------------------------------------

# Replace Letter icon in alphabetical listing
for f in A B C D E F G H I J K L M N O P Q R S T U V W X Y Z; do
sed -e "s/<div class=\"ah\">\&nbsp;\&nbsp;$f\&nbsp;\&nbsp;<\/div>/<\/ul>\n<p><h3><a class=\"anchor\" name=\"index_$f\">- $f -<\/a><\/h3><ul>/g" \
$1 > tmp.html && mv -f tmp.html $1
done

#Add the last closing </ul>
sed 's/<hr size=/<\/ul>\n<hr size=/g' \
$1 > tmp.html && mv -f tmp.html $1

#Remove the extra closing </ul> at the beginning
awk '/<ul>/{n++} /<\/ul>/{n--; if (n<0) {n=0;next}} {print}' $1 > tmp.html && mv -f tmp.html $1

#------------------------------------------------------------------------------

# Creating bulleted list
sed -e 's/<a class="el"/<li><a class="el"/g' \
    -e 's/<\/a>\&nbsp;\&nbsp;\&nbsp;/<\/a><\/li>/g' \
$1 > tmp.html && mv -f tmp.html $1

