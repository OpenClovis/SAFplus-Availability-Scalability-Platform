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
import time
import logging 
import safplus_watchdog
import commands

def touch(f):
    f = file(f,'a')
    f.close()

def get_watchdog_pid():
    p = '%s/safplus_watchdog.py' % asp.get_asp_etc_dir()
    cmd = asp.sys_asp['get_amf_watchdog_pid_cmd'](p)
    result = asp.sys_asp['Popen'](cmd)

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

def start_watchdog():
    # check here for watchdog exist 
    watchdog_pid = get_watchdog_pid()

    if not watchdog_pid:
        # setsid <prog> & daemonizes...
        cmd = 'setsid %s/safplus_watchdog.py &' % asp.get_asp_etc_dir()
        os.system(cmd)
    else:
        asp.fail_and_exit('SAFplus is already running on node [%s], pid [%s]' % (asp.get_asp_node_addr(), watchdog_pid), False)

def stop_watchdog():
    stopFile = asp.get_asp_run_dir() + '/' + asp.SAFPLUS_STOP_FILE 
    touch(stopFile)    # create restart file to prevent improper asp going down
    asp.stop_amf()
    # Unnecessary: watchdog will quit when it sees the stop file: safplus_watchdog.stop_watchdog()
    # remove restart file before start
    #if os.path.isfile(saf_no_restart_file):
    #    safplus_watchdog.safe_remove(saf_no_restart_file)
    #    print"REMOVED restart file %s" % saf_no_restart_file
 
def restart_asp():
    # remove no restart file, to make sure a restart occurs
    asp.remove_stop_file()
    asp.stop_amf()
    # No need to start SAFplus, Watchdog will check and start ASP after 30 sec

def saf_status():
    asp.get_asp_status()

"""
def redirect_file():
    UMASK = 0
    WORKDIR = asp.get_asp_run_dir()
    MAXFD = 1024

    os.chdir(WORKDIR)
    os.umask(UMASK)

    import resource
    maxfd = resource.getrlimit(resource.RLIMIT_NOFILE)[1]
    if maxfd == resource.RLIM_INFINITY:
        maxfd = MAXFD  
    for fd in range(0, maxfd):  commenting as files are not created to close
        try:
            os.close(fd)
        except OSError:
            pass
"""

def watchdog_usage():
    print
    print 'Usage : %s {start|stop|restart|status|help  _____WIP_____ }' %\
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
               'restart'  : restart_asp,
               'status'   : saf_status,
               'help'     : watchdog_usage
               }
    if cmd_map.has_key(cmd):
        #create_watchdog_cmd_marker(cmd)  # for printing the executing line and for displaying err
        cmd_map[cmd]()
    else:
        asp.fail_and_exit('Command [%s] not found !!' % cmd)

def main():
    check_py_version()
    #redirect_file() commenting as the files need not be closed
    logging.basicConfig(filename='%s/amf_watchdog.log' % asp.get_asp_log_dir(), format='%(levelname)s %(message)s', level=logging.INFO)
    
    if len(sys.argv) >= 2:    
        if sys.argv[1] == 'help' :
            watchdog_usage() 
            sys.exit(1)  

    watchdog_driver(sys.argv[1])

if __name__ == '__main__':
    main()
