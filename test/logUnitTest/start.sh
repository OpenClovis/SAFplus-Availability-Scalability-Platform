#!/bin/bash
export LD_LIBRARY_PATH=`pwd`/lib:$LD_LIBRARY_PATH
export PYTHONPATH=`pwd`/lib:`pwd`/bin:`pwd`/test:$PYTHONPATH
./test/testLog
