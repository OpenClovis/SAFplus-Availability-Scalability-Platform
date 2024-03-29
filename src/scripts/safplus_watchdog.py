#!/usr/bin/python3

import time
import os
import sys
import traceback
import logging
import safplus as asp
import subprocess

ASP_RESTART_FILE = 'safplus_restart'
ASP_WATCHDOG_RESTART_FILE='safplus_restart_watchdog'
ASP_REBOOT_FILE = 'safplus_reboot'
ASP_RESTART_DISABLE_FILE = 'safplus_restart_disable'
ASP_LOAD_CLUSTER_MODEL_FILE="safplus_load_cluster_model"

SAFPLUS_RESTART_DELAY = 20  # How long to delay before restarting.  If the AMF is able to restart before keepalives find it dead this will cause major issues in the AMF.

fileLogger=None

def asp_admin_stop():
    cmd = 'cat %s' % asp.get_asp_cmd_marker_file()
    ret, out, signal, core = asp.system(cmd)
    last_asp_cmd = out[0][:-1]

    return last_asp_cmd in ['stop', 'zap']

def configWatchdogLog():
    logging.basicConfig(filename='%s/amf_watchdog.log' % asp.get_asp_log_dir(), format='%(levelname)s %(asctime)s.%(msecs)d %(message)s', level=logging.DEBUG, datefmt='%a %d %b %Y %H:%M:%S')
    os.chmod('%s/amf_watchdog.log' % asp.get_asp_log_dir(), 0o644);
    global fileLogger
    fileLogger = logging.getLogger()

def get_amf_pid():
    while True:
        valid = subprocess.getstatusoutput("pidof safplus_amf");
        if valid[0] == 0:
            return valid[1]    
        else:
            break
        logging.warning('There is more than one AMF pid. Try again...')
        time.sleep(0.25)     
    return 0

def wait_until_amf_up():
    amf_pid = 0
    
    for i in range(10):
        amf_pid = get_amf_pid()
        if amf_pid:
            break

        time.sleep(3)

    return amf_pid

def safe_remove(fileName):
    try:
        os.remove(fileName)
    except:
        os.system('rm -f %s' %fileName)

def amf_watchdog_loop():
    monitor_interval = 5
    ppid = wait_until_amf_up()
    run_dir = asp.get_asp_run_dir()
    node_name = asp.get_asp_node_name()
    restart_file = run_dir + '/' + ASP_RESTART_FILE
    watchdog_restart_file = run_dir + '/' + ASP_WATCHDOG_RESTART_FILE
    reboot_file = asp.get_asp_bin_dir()+ '/' + ASP_REBOOT_FILE
    restart_disable_file = run_dir + '/' + ASP_RESTART_DISABLE_FILE
    load_cluster_model_file = run_dir + '/' + ASP_LOAD_CLUSTER_MODEL_FILE
    safe_remove(restart_file)
    safe_remove(reboot_file)
    safe_remove(restart_disable_file)
    safe_remove(load_cluster_model_file)
    if not ppid:
        logging.critical('ASP did not come up successfully...')
        sys.exit(1)

    global fileLogger
    while True:
        try:
            ppid = get_amf_pid()
            if not ppid:
                logging.critical('SAFplus watchdog invoked on %s' % time.strftime('%a %d %b %Y %H:%M:%S'))
                is_restart = os.access(restart_file, os.F_OK)
                is_forced_restart = os.access(watchdog_restart_file, os.F_OK)
                generate_db = False
                if os.access(load_cluster_model_file, os.F_OK):
                    generate_db = True
                if is_restart or is_forced_restart:
                    safe_remove(restart_file)
                    safe_remove(watchdog_restart_file)
                    logging.debug('SAFplus watchdog stopping SAFplus for %d sec' % SAFPLUS_RESTART_DELAY)
                    asp.zap_asp(False)
                    time.sleep(SAFPLUS_RESTART_DELAY)
                    logging.debug('SAFplus watchdog restarting SAFplus...')
                    asp.save_asp_runtime_files()
                    cmd = asp.get_amf_command(generate_db)
                    os.system(cmd)
                    asp.create_asp_cmd_marker('start')
                    if asp.reconfigWdLog:
                        fileLogger.handlers = []
                        configWatchdogLog()
                        asp.reconfigWdLog = False
                elif os.access(reboot_file, os.F_OK):
                    safe_remove(reboot_file)
                    # if getenv("ASP_NODE_REBOOT_DISABLE", 0) != 0:
                    #     logging.debug('SAFplus watchdog would normally reboot %s, but ASP_NODE_REBOOT_DISABLE is set' % node_name)
                    #     os.system("rm -f /dev/shm/SAFplus*")
                    #     asp.zap_asp()
                    #     sys.exit(1)
                    # else:
                    logging.debug('SAFplus watchdog rebooting %s...' % node_name)
                    # asp.run_custom_scripts('reboot')
                    asp.proc_lock_file('remove')
                    os.system('reboot')
                elif os.access(restart_disable_file, os.F_OK):
                    safe_remove(restart_disable_file)
                    logging.debug('SAFplus watchdog ignoring failure of %s '
                                  'as node failfast/failover recovery action '
                                  'was called on it and ASP_NODE_REBOOT_DISABLE '
                                  'environment variable is set for it.'
                                  % node_name)
                    #os.system("rm -f /dev/shm/SAFplus*") # it is deleted in asp_zap()
                    asp.zap_asp()
                    sys.exit(1)
                else:
                    logging.debug('SAFplus watchdog invocation default case')
                    if not asp_admin_stop():
                        asp.zap_asp(False)
                        if asp.should_restart_asp():
                            asp.zap_asp(False)
                            logging.debug('SAFplus watchdog stopping SAFplus for %d sec' % SAFPLUS_RESTART_DELAY)
                            time.sleep(SAFPLUS_RESTART_DELAY)
                            asp.save_asp_runtime_files()
                            cmd = asp.get_amf_command(generate_db)
                            os.system(cmd)
                            asp.create_asp_cmd_marker('start')
                            if asp.reconfigWdLog:
                                fileLogger.handlers = []
                                configWatchdogLog()
                                asp.reconfigWdLog = False
                        else:
                            asp.proc_lock_file('remove')
                            sys.exit(1)
                    else:
                        logging.debug('SAFplus watchdog ignoring node failure, as it might be due to shutdown request from logd')
                        asp.proc_lock_file('remove')
                        sys.exit(1)
        except Exception as e:
            logging.critical('SAFplus watchdog received exception %s' %str(e))
            logging.critical('traceback: %s',traceback.format_exc())
        time.sleep(monitor_interval)

def main():
    configWatchdogLog()
    amf_watchdog_loop()

if __name__ == '__main__':
    main()
