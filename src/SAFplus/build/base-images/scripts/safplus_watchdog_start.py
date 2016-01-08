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
import shutil
# import pdb

# RemovePersistentDb = False
tipc_settings=None
 
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
    global tipc_settings
    # check whether watchdog exist 
    watchdog_pid = get_watchdog_pid()
    if not watchdog_pid:
        if safplus_tipc.has_tipc_plugin() and (safplus_tipc.ignore_tipc_settings() == False):
          if safplus_tipc.enforce_tipc_settings() == True:
            safplus_tipc.unload_tipc_module()
          safplus_tipc.load_config_tipc_module()
        set_ld_library_paths()

        codeBootFile = safplus.SAFPLUS_RUN_DIR + '/' + safplus.SAFPLUS_CODEBOOT_FILE 
        safplus.touch(codeBootFile)    # create 'safplus_codeboot' file to indicate start-up of SAFplus_AMF

        # setsid <prog> & daemonizes...
        cmd = 'setsid %s/safplus_watchdog.py &' % safplus.SAFPLUS_ETC_DIR
        os.system(cmd)
    else:
        safplus.fail_and_exit('SAFplus is already running on node [%s], pid [%s]' % (safplus.get_safplus_node_addr(), watchdog_pid))

def stop_watchdog():
    kill_watchdog() #Don't let safplus_watchdog monitor and spawn safplus_amf as in race condition
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
    print 'Usage : %s { start | stop | restart | status | zap | help } ' %\
          os.path.splitext(os.path.basename(sys.argv[0]))[0]
    print
    print
    print 'options can be one of the following : (these '\
          'options only work with start command, '\
          'e.g. etc/init.d/asp start -v etc.)'
    print

    l = ( ('-v', 'Be verbose'),
          ('--enforce-tipc-settings',
           'Use etc/asp.conf\'s TIPC settings '
           'overriding the system TIPC settings'),
          ('--ignore-tipc-settings',
           'Use systems TIPC settings '
           'ignoring the etc/asp.conf\'s settings'),
          ('--remove-persistent-db',
           'Delete all of the ASP persistent database files'),
          ('--log-level <level>',
           'Start ASP with particular log level. <level> is '
           '[trace|debug|info|notice|warning|error|critical]')
        )

    for o, h in l:
        print '%-30s: %s' % (o, h)

def check_py_version():
    min_req_version = (2, 3, 4)

    if sys.version_info < min_req_version:
        logging.critical('This script requires python version %s '\
                     'or later' % '.'.join(map(str, min_req_version)))
        logging.critical('You have python version %s' %\
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


def parse_command_line(args):
    # global RemovePersistentDb
    global tipc_settings
    import getopt
    optdict = {}
    try:
        opts, args = getopt.getopt(args, 'v', ['enforce-tipc-settings', 'ignore-tipc-settings', 'remove-persistent-db', 'log-level=',
                                   ])
    except getopt.GetoptError, e:
        print 'Command line parsing failed, error [%s]' % e
        watchdog_usage()
        sys.exit(1)

    for o, a in opts:
        if o == '-v':
            logging.getLogger().setLevel(logging.DEBUG)
            #for h in logging.handlers:
            #    h.setLevel(logging.DEBUG)
        elif o == '--enforce-tipc-settings':
            tipc_settings = 'enforce'
            logging.info("tipc settings is 'enforce'")
        elif o == '--ignore-tipc-settings':
            tipc_settings = 'ignore'
            logging.info("tipc settings is 'ignore'")
        elif o == '--remove-persistent-db':
            # RemovePersistentDb=True
            os.putenv('SAFPLUS_REMOVE_DB', 'TRUE')
        elif o == '--log-level':
            l = ['trace', 'debug',
                 'info', 'notice',
                 'warning', 'error',
                 'critical']
            if a.lower() in l:
                logging.info('SAFplus log level is [%s]' % a.lower())
                log_level = a.upper()
                os.putenv('CL_LOG_SEVERITY', log_level)
            else:
                logging.critical('Invalid SAFplus log level [%s]' % a)
                watchdog_usage()
                sys.exit(1)
    return optdict

def mkdir(d):
    try: # Make the directory
      os.makedirs(d)
    except OSError, e:  # Already exists
      import errno
      if e.errno != errno.EEXIST: raise    


def main(argv):
    check_py_version()
    safplus.import_os_adaption_layer()

    logging.basicConfig(format='%(levelname)s %(message)s', level=logging.INFO)

    if len(sys.argv) == 1 or sys.argv[1] == 'help' :
        watchdog_usage() 
        return 1

    parse_command_line(argv[2:])

    # This is handled during AMF restart    
    # if RemovePersistentDb:
    #  shutil.rmtree(safplus.SAFPLUS_DB_DIR,ignore_errors=True)

    mkdir(safplus.SAFPLUS_RUN_DIR)
    mkdir(safplus.SAFPLUS_LOG_DIR)
    mkdir(safplus.SAFPLUS_DB_DIR)

    watchdog_driver(argv[1])

if __name__ == '__main__':
    main(sys.argv)
