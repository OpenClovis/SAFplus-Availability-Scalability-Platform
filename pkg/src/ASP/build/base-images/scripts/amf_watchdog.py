#!/usr/bin/env python
###############################################################################
#
# Copyright (C) 2002-2009 by OpenClovis Inc. All  Rights Reserved.
# 
# The source code for  this program is not published  or otherwise 
# divested of  its trade secrets, irrespective  of  what  has been 
# deposited with the U.S. Copyright office.
# 
# This program is  free software; you can redistribute it and / or
# modify  it under  the  terms  of  the GNU General Public License
# version 2 as published by the Free Software Foundation.
# 
# This program is distributed in the  hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied  warranty  of 
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
# General Public License for more details.
# 
# You  should  have  received  a  copy of  the  GNU General Public
# License along  with  this program. If  not,  write  to  the 
# Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
#
###############################################################################

import asp
import os
import sys
import time

ASP_RESTART_FILE = 'asp_restart'
ASP_WATCHDOG_RESTART_FILE='asp_restart_watchdog'
ASP_REBOOT_FILE = 'asp_reboot'
ASP_RESTART_DISABLE_FILE = 'asp_restart_disable'

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

    while True:
        pid = asp.get_amf_pid()
        if pid == 0:
            asp.log.critical('AMF watchdog invoked on %s' %\
                             time.strftime('%a %d %b %Y %H:%M:%S'))
            is_restart = os.access(restart_file, os.F_OK)
            is_forced_restart = os.access(watchdog_restart_file, os.F_OK)
            if is_restart or is_forced_restart:
                safe_remove(restart_file) 
                safe_remove(watchdog_restart_file)
                asp.log.debug('AMF watchdog restarting ASP...')
                asp.zap_asp()
                ## give time for pending ops to complete
                ## we unload the TIPC module and let ASP start reload it, 
                ## since its been observed with tipc 1.5.12 that ASP starts 
                ## after a link re-establishment results in multicast link
                ## retransmit failures due to pending ACK thereby resulting
                ## in all the TIPC links being reset.

                asp.start_asp(stop_watchdog=False)
                asp.create_asp_cmd_marker('start')
                sys.exit(1)
            elif os.access(reboot_file, os.F_OK):
                safe_remove(reboot_file)
                asp.log.debug('AMF watchdog rebooting %s...'
                              % asp.get_asp_node_name())
                asp.run_custom_scripts('reboot')
                os.system('reboot')
            elif os.access(restart_disable_file, os.F_OK):
                safe_remove(restart_disable_file)
                asp.log.debug('AMF watchdog ignoring failure of %s '
                              'as node failfast/failover recovery action '
                              'was called on it and ASP_NODE_REBOOT_DISABLE '
                              'environment variable is set for it.'
                              % asp.get_asp_node_name())
                asp.zap_asp()
                sys.exit(1)
            else:
                asp.zap_asp()

                if not asp_admin_stop():
                    if asp.should_restart_asp():
                        asp.start_asp(stop_watchdog=False)
                        asp.create_asp_cmd_marker('start')

                sys.exit(1)

        time.sleep(monitor_interval)

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

    redirect_to = '%s/amf_watchdog.log' % asp.get_asp_log_dir()

    os.open(redirect_to, os.O_RDWR | os.O_CREAT)
    os.dup2(0, 1)
    os.dup2(0, 2)

def main():
    asp.check_py_version()
    redirect_file()
    amf_pid = asp.wait_until_amf_up()
    if not amf_pid:
        asp.log.critical('ASP did not come up successfully...')
        asp.log.critical('Cleaning up...')
        asp.kill_asp()
        sys.exit(1)
    amf_watchdog_loop()

if __name__ == '__main__':
    main()
