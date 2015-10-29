#!/bin/bash
# This script sets up paths so that SAFplus can find required libraries
# and plugins.

# Set your selected backplane interface here
export SAFPLUS_BACKPLANE_INTERFACE=eth0

MY_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
SAFPLUS_DIR=/opt/safplus/7.0/sdk
MA=i686-linux-gnu
export PATH=$MY_DIR/bin:$SAFPLUS_DIR/target/$MA/bin:$PATH
export LD_LIBRARY_PATH=$MY_DIR:$MY_DIR/lib:$MY_DIR/plugin:$SAFPLUS_DIR/target/$MA/lib::$SAFPLUS_DIR/target/$MA/plugin:$LD_LIBRARY_PATH
export PYTHONPATH=$MY_DIR/lib:$SAFPLUS_DIR/target/$MA/lib:$PYTHONPATH
