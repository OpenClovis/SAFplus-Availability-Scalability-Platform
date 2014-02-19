#!/usr/bin/env python
# Copyright (C) 2002-2012 OpenClovis Solutions Inc.  All Rights Reserved.
# This file is available  under  a  commercial  license  from  the
# copyright  holder or the GNU General Public License Version 2.0.
#
# The source code for  this program is not published  or otherwise 
# divested of  its trade secrets, irrespective  of  what  has been 
# deposited with the U.S. Copyright office.
# 
# This program is distributed in the  hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied  warranty  of 
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
# General Public License for more details.
# 
# For more  information,  see the  file COPYING provided with this
# material.

import safplus
import safplus_tipc
import os
import sys
import signal
import time
import logging
import subprocess
#import pdb

SAFPLUS_POLL_INTERVAL = 10  # How long to delay for watchdog to check for the presene of SAFplus_AMF. If SAFplus_AMF is not running and safplus_stop file is not present, watchdog will start SAFplus_AMF

def wdSleep(amt):
    """Sleep and refuse to let anything kick me awake"""
    start = time.time()
    while (time.time() - start) < amt:
      leftover = amt - (time.time() - start)
      try:
        time.sleep(leftover)
      except:
        try:
          select.select([],[],[],leftover)
        except:
          pass

def watchdog_loop():
    run_dir = safplus.SAFPLUS_RUN_DIR
    reboot_file  = run_dir + '/' + safplus.SAFPLUS_REBOOT_FILE
    stop_file    = run_dir + '/' + safplus.SAFPLUS_STOP_FILE
    safplus.safe_remove(reboot_file)
    safplus.remove_stop_file()
    amfproc = None

    while True:
        try:
            #pid = safplus.get_amf_pid()
            #if pid == 0:
            if not amfproc:
                if os.path.isfile(stop_file):   # Kill watchdog if stop file exists        
                    logging.info("Stop file exists: SAFplus is stopping")
                    return
                else:                           # Restart AMF if stop file not found
                    logging.info("Stop file not found: Starting AMF from watchdog")
                    safplus.kill_amf()  # when AMF dies, kill all its children to make sure there are no orphaned processes hanging around.  This only kills binaries in the bin directory, rather than all children...
                    amfproc = safplus.cleanup_and_start_ams()
            # Python 3:
#            try:
#              Python 3: amfproc.wait(SAFPLUS_POLL_INTERVAL)
#              del amfproc
#              amfproc = None
#            except subprocess.TimeoutExpired, e:
#              pass

#            if amfproc.poll():
#              del amfproc
#              amfproc = None
#            wdSleep(SAFPLUS_POLL_INTERVAL)
            amfproc.wait() 
            del amfproc
            amfproc = None

        except Exception, e:
            print "Exception: %s" % str(e)
            time.sleep(5)

def main():
    safplus.import_os_adaption_layer()
    logging.basicConfig(filename='%s/amf_watchdog.log' % safplus.SAFPLUS_LOG_DIR, format='%(levelname)s %(message)s', level=logging.DEBUG)
    watchdog_loop()

if __name__ == '__main__':
    main()
