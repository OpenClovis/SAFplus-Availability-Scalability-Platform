# (@) bash
# source this into your shell to use a working directory.
# note: that your working directory (pwd) needs to be the same
# directory as this script for this to work properly.

function addpath() {
  m=$(echo "$1" | egrep "(^|:)$2(:|$)")
  [ -n "$m" ] && echo "$1" || echo "$2:$1" 
}

dn=$(pwd)
PYTHONPATH=$(addpath $PYTHONPATH $dn)
PATH=$(addpath $PATH "$dn/bin")
export PATH PYTHONPATH
