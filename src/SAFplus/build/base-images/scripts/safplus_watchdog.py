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

ASP_RESTART_FILE = 'safplus_restart'
ASP_WATCHDOG_RESTART_FILE='safplus_restart_watchdog'
ASP_REBOOT_FILE = 'safplus_reboot'
ASP_RESTART_DISABLE_FILE = 'safplus_restart_disable'

SAFPLUS_RESTART_DELAY = 30  # How long to delay before restarting.  If the AMF is able to restart before keepalives find it dead this will cause major issues in the AMF.

def getenv(varName, default):
    env_value = os.getenv(varName)
    if env_value == None:
        return default
    elif env_value == '1' or env_value.lower() == 'yes' or env_value.lower() == 'true':
        return 1
    elif env_value == '0' or env_value.lower() == 'no' or env_value.lower() == 'false':
        return 0
    else:
        return env_value

def safe_remove(f):
    try:
        os.remove(f)
    except:
        os.system('rm -f %s' %f)

def asp_admin_stop():
    cmd = 'cat %s' % asp.get_asp_cmd_marker_file()
    ret, out, signal, core = asp.system(cmd)
    last_asp_cmd = out[0][:-1]

    return last_asp_cmd in ['stop', 'zap']

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

# now includes openhpid as well
def amf_watchdog_loop():
    monitor_interval = 5
    run_dir = asp.get_asp_run_dir()
    restart_file = run_dir + '/' + ASP_RESTART_FILE
    watchdog_restart_file = run_dir + '/' + ASP_WATCHDOG_RESTART_FILE
    reboot_file  = run_dir + '/' + ASP_REBOOT_FILE
    restart_disable_file = run_dir + '/' + ASP_RESTART_DISABLE_FILE
    safe_remove(restart_file)
    safe_remove(reboot_file)
    safe_remove(restart_disable_file)
    seen_openhpid = False

    while True:
        try:
            pid = asp.get_amf_pid()
            if pid == 0:
                logging.critical('AMF watchdog invoked on %s' % time.strftime('%a %d %b %Y %H:%M:%S'))
                is_restart = os.access(restart_file, os.F_OK)
                is_forced_restart = os.access(watchdog_restart_file, os.F_OK)
                if is_restart or is_forced_restart:
                    safe_remove(restart_file) 
                    safe_remove(watchdog_restart_file)
                    logging.debug('AMF watchdog restarting ASP...')
                    asp.zap_asp(False)
                    ## give time for pending ops to complete
                    ## we unload the TIPC module and let ASP start reload it, 
                    ## since its been observed with tipc 1.5.12 that ASP starts 
                    ## after a link re-establishment results in multicast link
                    ## retransmit failures due to pending ACK thereby resulting
                    ## in all the TIPC links being reset.
                    wdSleep(SAFPLUS_RESTART_DELAY)
                    asp.start_asp(stop_watchdog=False, force_start=True)
                    asp.create_asp_cmd_marker('start')
                    sys.exit(1)
                elif os.access(reboot_file, os.F_OK):
                    safe_remove(reboot_file)
                    if getenv("ASP_NODE_REBOOT_DISABLE", 0) != 0:
                        asp.zap_asp()
                        sys.exit(1)
                    else:
                        logging.debug('AMF watchdog rebooting %s...' % asp.get_asp_node_name())
                        asp.run_custom_scripts('reboot')
                        asp.proc_lock_file('remove')
                        os.system('reboot')
                elif os.access(restart_disable_file, os.F_OK):
                    safe_remove(restart_disable_file)
                    logging.debug('AMF watchdog ignoring failure of %s '
                                  'as node failfast/failover recovery action '
                                  'was called on it and ASP_NODE_REBOOT_DISABLE '
                                  'environment variable is set for it.'
                                  % asp.get_asp_node_name())
                    asp.zap_asp()
                    sys.exit(1)
                else:
                    logging.debug('AMF watchdog invocation default case')
    
                    if not asp_admin_stop():
                        asp.zap_asp(False)
                        if asp.should_restart_asp():
                            wdSleep(SAFPLUS_RESTART_DELAY)
                            asp.start_asp(stop_watchdog=False, force_start = True)
                            asp.create_asp_cmd_marker('start')
                        else:
                            asp.proc_lock_file('remove')
                    sys.exit(1)
            else:
                # pid is nonzero => amf is up
                # handle openhpid here
                openhpid_pid = asp.get_openhpid_pid()

                if seen_openhpid:

                    if openhpid_pid == 0:
                        # openhpid is DOWN and we have seen it before
                        # we should bring it back
                        logging.debug('AMF watchdog expected openhpid but did not find it. Restarting openhpid...')

                        # zap it to make sure its DEAD
                        os.popen('killall openhpid 2>/dev/null')
                        #time.sleep(1)
                        asp.start_openhpid()
                    else:
                        logging.debug('AMF watchdog openhpid pid(%d) found as expected, nothing to do.' % openhpid_pid)

                else:
                    if openhpid_pid != 0:
                        seen_openhpid = True



            wdSleep(monitor_interval)
        except:
            pass
 
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
  
    for fd in range(0, maxfd):
        try:
            os.close(fd)
        except OSError:
            pass
    # We don't redirect stdout, stderr to amf_watchdog.log, so comment out these code
    #redirect_to = '%s/amf_watchdog.log' % asp.get_asp_log_dir()

    #redirect_to = '%s/amf_watchdog.log' % asp.get_asp_log_dir()

    #os.open(redirect_to, os.O_RDWR | os.O_CREAT)
    #os.dup2(0, 1)
    #os.dup2(0, 2)

def main():
    asp.check_py_version()
    redirect_file()
    logging.basicConfig(filename='%s/amf_watchdog.log' % asp.get_asp_log_dir(), format='%(levelname)s %(message)s', level=logging.INFO)    
    amf_pid = asp.wait_until_amf_up()
    if not amf_pid:
        logging.critical('ASP did not come up successfully...')
        logging.critical('Cleaning up...')
        asp.kill_asp()
        sys.exit(1)
    amf_watchdog_loop()

if __name__ == '__main__':
    main()
