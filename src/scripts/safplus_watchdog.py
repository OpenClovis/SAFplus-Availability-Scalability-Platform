#!/usr/bin/python

import commands
import time
import os
import signal
import sys
import traceback
import logging

ASP_REBOOT_FILE = 'safplus_reboot'

def init_log():
    logger = logging.getLogger('ASP')
    logger.setLevel(logging.DEBUG)

    console = logging.StreamHandler(sys.stdout)
    console.setLevel(logging.INFO)

    formatter = logging.Formatter('%(levelname)s '
                                  '%(message)s')
    console.setFormatter(formatter)

    logger.addHandler(console)
    return logger

def set_up_asp_config():
    def get_sandbox_dir():
        p = os.path.dirname(os.path.realpath(__file__))
        p = os.path.split(p)[0]

        return p

    def get_dir(p):
        if os.path.exists(p):
            return p
        else:
            try:
                os.mkdir(p)
            except OSError, e:
                if e.errno != errno.EEXIST:
                    fail_and_exit('Failed to create directory, [%s]' % e)
            return p
                
    d = {}
    sandbox = get_sandbox_dir()
    d['bin_dir'] = sandbox + '/bin'
    d['etc_dir'] = sandbox + '/etc'
    d['lib_dir'] = sandbox + '/lib'
    d['var_dir'] = get_dir(sandbox + '/var')
    d['log_dir'] = get_dir(d['var_dir'] + '/log')

    return d

def get_asp_log_dir():
    return asp_env['log_dir']

def get_asp_bin_dir():
	return asp_env['bin_dir']

def configWatchdogLog():
    logging.basicConfig(filename='%s/amf_watchdog.log' % get_asp_log_dir(), format='%(levelname)s %(asctime)s.%(msecs)d %(message)s', level=logging.DEBUG, datefmt='%a %d %b %Y %H:%M:%S')
    os.chmod('%s/amf_watchdog.log' % get_asp_log_dir(), 0644);

def get_amf_pid():
    while True:
        valid = commands.getstatusoutput("pidof safplus_amf");
        if valid[0] == 0:
            return valid[1]    
        else:
            break
        log.warning('There is more than one AMF pid. Try again...')
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

def get_childs_pid(ppid):
    cmd = "ps --forest -o pid -g $(ps -o sid= -p %s)" % ppid
    processChild = os.popen(cmd).read()
    processes = []
    start = False
    for p in processChild.split('\n'):
        pid = p.strip()
        if start and pid:
            processes.append(pid)
        if pid == str(ppid):
            start = True
    return processes

def safe_remove(fileName):
    try:
        os.remove(fileName)
    except:
        os.system('rm -f %s' %fileName)

def amf_watchdog_loop():
    monitor_interval = 0.1
    ppid = wait_until_amf_up()
    rebootFile = get_asp_bin_dir()+ '/' + ASP_REBOOT_FILE
    safe_remove(rebootFile)
    if not ppid:
        logging.critical('ASP did not come up successfully...')
        #logging.critical('Cleaning up...')
        sys.exit(1)
    processChild = get_childs_pid(ppid)
    while True:
        try:
            ppid = get_amf_pid()
            if not ppid:
                for pid in processChild:
                    try:
                        os.kill(int(pid), signal.SIGINT)
                    except: pass
                sys.exit(1)
            else:
                processChild = get_childs_pid(ppid)
            if os.access(rebootFile,os.F_OK):
                safe_remove(rebootFile)
                os.system('reboot')
        except Exception,e:
            logging.critical('SAFplus watchdog received exception %s' %str(e))
            logging.critical('traceback: %s',traceback.format_exc())
        time.sleep(monitor_interval)

def main():
    configWatchdogLog()
    amf_watchdog_loop()

log = init_log()
asp_env = set_up_asp_config()

if __name__ == '__main__':
    main()
