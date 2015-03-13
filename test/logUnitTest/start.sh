#!/bin/bash
export LD_LIBRARY_PATH=`pwd`:$LD_LIBRARY_PATH
export PYTHONPATH=`pwd`:$PYTHONPATH
./testLog
