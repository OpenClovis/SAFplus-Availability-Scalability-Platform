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

import asp
import os
import sys
import signal
import time
import logging 

ASP_WATCHDOG_RESTART_FILE='safplus_restart_watchdog'
ASP_REBOOT_FILE = 'safplus_reboot'
ASP_RESTART_DISABLE_FILE = 'safplus_restart_disable'
ASP_NO_RESTART_FILE = 'safplus_no_restart'

SAFPLUS_RESTART_DELAY = 30  # How long to delay before restarting.  If the AMF is able to restart before keepalives find it dead this will cause major issues in the AMF.

def safe_remove(f):
    try:
        os.remove(f)
    except:
        os.system('rm -f %s' %f)

def safe_creat(f):
    try:
        os.creat(f)
    except:
        os.system('touch %s' %f)

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

def get_watchdog_pid_cmd(p):
    return 'ps -e -o pid,cmd | grep \'%s\'' % p

def kill_watchdog():
    p = '%s/safplus_watchdog.py' % asp.get_asp_etc_dir()
    cmd = get_watchdog_pid_cmd(p)
    result = os.popen(cmd)

    # Eliminate the incorrect lines
    psLine = filter(lambda x: not "grep" in x, result)

    if len(psLine) == 0: # Its already dead
      return
    try:
      wpid = int(psLine[0].split()[0])
    except Exception, e:
      print "Exception: %s" % str(e)
      print "CMD: %s" % cmd
      print "data: %s" % result
      raise

    try:
        os.kill(wpid, signal.SIGKILL)
    except OSError, e:
        if e.errno == errno.ESRCH:
            pass
        else:
            raise

def stop_watchdog():
    kill_watchdog()

def start_ams():
    cmd = 'setsid %s/asp.py &' %asp.get_asp_etc_dir()
    os.system(cmd)

def watchdog_loop():
    run_dir = asp.get_asp_run_dir()
    no_restart_file = run_dir + '/' + ASP_NO_RESTART_FILE
    watchdog_restart_file = run_dir + '/' + ASP_WATCHDOG_RESTART_FILE
    reboot_file  = run_dir + '/' + ASP_REBOOT_FILE
    restart_disable_file = run_dir + '/' + ASP_RESTART_DISABLE_FILE
    safe_remove(reboot_file)
    safe_remove(restart_disable_file)
    safe_remove(watchdog_restart_file)

    while True:
        try:
            pid = asp.get_amf_pid()
            if pid == 0:
                if not os.path.isfile(no_restart_file):                    # Restart AMF if restart file not found
                    print"Restart file not found : Starting AMF from Watchdog"
                    start_ams()
                else:                                                   # Kill watchdog if restart file exist 
                    print" Restart file FOUND : self kill Watchdog"
                    stop_watchdog()
            wdSleep(SAFPLUS_RESTART_DELAY)
        except :
            pass 

def main():
    #redirect_file()
    logging.basicConfig(filename='%s/amf_watchdog.log' % asp.get_asp_log_dir(), format='%(levelname)s %(message)s', level=logging.INFO)
    watchdog_loop()

if __name__ == '__main__':
    main()
