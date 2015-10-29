#!/bin/bash
# This script sets up paths so that SAFplus can find required libraries
# and plugins.

# Set your selected backplane interface here
export SAFPLUS_BACKPLANE_INTERFACE=eth0

MY_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
export PATH=$MY_DIR/bin:$PATH
export LD_LIBRARY_PATH=$MY_DIR/lib:$MY_DIR/plugin:$LD_LIBRARY_PATH
export PYTHONPATH=$MY_DIR/lib:$PYTHONPATH
