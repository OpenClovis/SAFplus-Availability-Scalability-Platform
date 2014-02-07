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
import time
import logging
import signal 
import safplus_watchdog
import commands
#import pdb

def get_watchdog_pid():
    p = '%s/safplus_watchdog.py' % safplus.SAFPLUS_ETC_DIR
    cmd = safplus.get_pid_cmd(p)
    result = safplus.Popen(cmd)

    # Eliminate the incorrect lines
    psLine = filter(lambda x: not "grep" in x, result)

    if len(psLine) == 0: # Its already dead
      return 0
    try:
      wpid = int(psLine[0].split()[0])
    except Exception, e:
      print "Exception: %s" % str(e)
      print "CMD: %s" % cmd
      print "data: %s" % result
      raise
    return wpid

def get_watchdog_pid_cmd(p):
    return 'ps -e -o pid,cmd | grep \'%s\'' % p

def kill_watchdog():
    p = '%s/safplus_watchdog.py' % safplus.SAFPLUS_ETC_DIR
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

def set_ld_library_paths():
    safplus_dir = safplus.sandbox_dir

    p = [
        safplus_dir + '/lib',
        safplus_dir + '/lib/openhpi',
        '/usr/lib',
        '/usr/local/lib'
        ]

    old_ld_path = os.getenv('LD_LIBRARY_PATH')
    if old_ld_path:
        p.insert(0, old_ld_path)

    v = ':'.join(p)
    os.putenv('LD_LIBRARY_PATH', v)

def start_watchdog():
    # check whether watchdog exist 
    watchdog_pid = get_watchdog_pid()
    if not watchdog_pid:
        logging.info("Loading TIPC ")
        safplus_tipc.load_config_tipc_module()
        set_ld_library_paths()

        # setsid <prog> & daemonizes...
        cmd = 'setsid %s/safplus_watchdog.py &' % safplus.SAFPLUS_ETC_DIR
        os.system(cmd)
    else:
        safplus.fail_and_exit('SAFplus is already running on node [%s], pid [%s]' % (safplus.get_safplus_node_addr(), watchdog_pid))

def stop_watchdog():
    safplus.stop_amf()
 
def restart_safplus():
    # remove no restart file, to make sure a restart occurs
    safplus.remove_stop_file()
    safplus.stop_amf()
    # No need to start SAFplus, Watchdog will check and start AMF after 30 sec

def saf_status():
    safplus.get_safplus_status()

def saf_zap():
    kill_watchdog()    # Added as watchdog takes 30 sec for it to check and come down after stopping AMF
    safplus.zap_amf()

def watchdog_usage():
    print
    print 'Usage : %s { start | stop | restart | status | zap | help }' %\
          os.path.splitext(os.path.basename(sys.argv[0]))[0]
    print

def check_py_version():
    min_req_version = (2, 3, 4)

    if sys.version_info < min_req_version:
        log.critical('This script requires python version %s '\
                     'or later' % '.'.join(map(str, min_req_version)))
        log.critical('You have python version %s' %\
                     '.'.join(map(str, sys.version_info[0:3])))
        sys.exit(1)

def watchdog_driver(cmd):
    cmd_map = {'start'    : start_watchdog,
               'stop'     : stop_watchdog,
               'restart'  : restart_safplus,
               'status'   : saf_status,
               'zap'      : saf_zap,
               'help'     : watchdog_usage
               }
    if cmd_map.has_key(cmd):
        cmd_map[cmd]()
    else:
        safplus.fail_and_exit('Command [%s] not found !!' % cmd)

def main():
    check_py_version()
    safplus.import_os_adoption_layer()
    logging.basicConfig(filename='%s/amf_watchdog.log' % safplus.SAFPLUS_LOG_DIR, format='%(levelname)s %(message)s', level=logging.INFO)
    
    if len(sys.argv) >= 2:    
        if sys.argv[1] == 'help' :
            watchdog_usage() 
            sys.exit(1)  

    watchdog_driver(sys.argv[1])

if __name__ == '__main__':
    main()
