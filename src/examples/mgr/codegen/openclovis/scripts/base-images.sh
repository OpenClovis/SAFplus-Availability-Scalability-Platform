#!/bin/bash
# Shell script utility to read a file line line and execute cp command.
#

# Function to execute cp command for each line
processLine(){
  line="$@" # get all args
  length=${#line}
  filetype=${line:0:3}
  let "length = $length - 4"
  filepath=${line:4:$length}
  if [ "$filetype" == "lib" ]
  then
  cp -r ${filepath} ${MODEL_LIB}
  fi
  if [ "$filetype" == "bin" ]
  then
  cp -r ${filepath} ${MODEL_BIN}
  fi
}

### Main script stars here ###
MODEL_NAME="$(basename $(dirname $(dirname $(dirname $1))))"
PROJECT_AREA="$(dirname $(dirname $(dirname $(dirname $(dirname $1)))))"
FILE="$PROJECT_AREA/$MODEL_NAME/src/build/base-images/scripts/copy_list"

# make sure file exist and readable
if [ ! -f $FILE ]; then
        echo "$FILE : does not exists"
        exit 1
elif [ ! -r $FILE ]; then
        echo "$FILE: can not read"
        exit 2
fi

while read line
do
        # use $line variable to process line in processLine() function
        processLine $line
done < $FILE
