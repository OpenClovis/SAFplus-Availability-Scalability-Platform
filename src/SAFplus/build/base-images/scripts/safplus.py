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

import sys
import os
import time
import signal
import errno
import re
import glob
import commands
import subprocess
import pdb
import logging
import safplus_tipc

AmfName = "safplus_amf"
os_platform = None

SAFPLUS_REBOOT_FILE  = 'safplus_reboot'   #
SAFPLUS_STOP_FILE    = 'safplus_stop'     # File in created while SAF is gracefully finalizing and indicate watchdog not to start AMF. location is sandbox_dir/var/run
SAFPLUS_STATUS_FILE  = 'safplus_state'    # File to contain the status of SAFplus in form of int in location sandbox_dir/var/run
                                          # 0 -> running, 1 -> not running, 2 -> booting/shutting down.
SystemErrorNoSuchFileOrDir = 127
SAFPLUS_SHUTDOWN_WAIT_TIMEOUT  = 10

SAF_AMF_START_CMD_CHASSIS    = '-c'        # tag for Chassi number in start command
SAF_AMF_START_CMD_LOCAL_SLOT = '-l'        # tag for Slot number in start command
SAF_AMF_START_CMD_NODE_NAME  = '-n'        # tag for Node Name in start command

log = logging

def get_sandbox_dir():
    p = os.path.dirname(os.path.realpath(__file__))
    p = os.path.split(p)[0]
    return p

# Global variable to store sandbox directory
sandbox_dir = get_sandbox_dir()

def import_os_adaption_layer():
    global os_platform
    def on_platform(p):
        return p in sys.platform

    if on_platform('linux'):
        import safplus_linux as os_platform
    elif on_platform('qnx'):
        import safplus_qnx as os_platform
    else:
        fail_and_exit('AMF Watchdog Unknown platform')

def get_save_log_dir():
    p = os.getenv('ASP_PREV_LOG_DIR') or '/tmp/safplus_saved_logs'
    if os.path.exists(p):
        return p
    else:
        try:
            os.mkdir(p)
            return p
        except OSError, e:
            fail_and_exit('Failed to create directory, [%s]' % e)

SAFPLUS_BIN_DIR        = sandbox_dir + '/bin'            #
SAFPLUS_ETC_DIR        = sandbox_dir + '/etc'            #
SAFPLUS_SCRIPT_DIR     = sandbox_dir + '/etc/safplus.d'  #
SAFPLUS_VAR_DIR        = sandbox_dir + '/var'            #
SAFPLUS_LIB_DIR        = sandbox_dir + '/lib'            #
SAFPLUS_DB_DIR         = sandbox_dir + '/var/lib'        #
SAFPLUS_RUN_DIR        = sandbox_dir + '/var/run'        #
SAFPLUS_MODULES_DIR    = sandbox_dir + '/modules'        #
SAFPLUS_LOG_DIR        = sandbox_dir  + '/var/log'       #
SAFPLUS_LOG_CORE_DIR   = get_save_log_dir() + '/cores'   #
SAFPLUS_LOG_CRASH_DIR  = get_save_log_dir() + '/crash'   #
SAFPLUS_LOG_NORMAL_DIR = get_save_log_dir() + '/normal'  #


try:
    set
except NameError:
    import sets
    set = sets.Set

def touch(f):
    f = file(f,'a')
    f.close()

def safe_remove(f):
    try:
        os.remove(f)
    except Exception, e:
        log.debug("Exception %s while removing %s" % (str(e),f))
        os.system('rm -f %s' %f)

def remove_stop_file():
    stopFile = SAFPLUS_RUN_DIR + '/' + SAFPLUS_STOP_FILE
    safe_remove(stopFile)

def get_safplus_log_dir(): return SAFPLUS_LOG_DIR

def get_safplus_run_dir(): return SAFPLUS_RUN_DIR

def get_safplus_cores_dir(): return (get_dir(get_save_log_dir() + '/cores'))

def get_safplus_normal_log_dir(): return (get_dir(get_save_log_dir() + '/normal'))

def get_safplus_crash_log_dir(): return (get_dir(get_save_log_dir() + '/crash'))

def get_status_file(): return (SAFPLUS_RUN_DIR + '/' + SAFPLUS_STATUS_FILE + '-%s' %get_safplus_node_addr())

def system(cmd)   : return os_platform.system(cmd)

def Popen(cmd)    : return os_platform.Popen(cmd)

def getMultiLink(): return os_platform.getMultiLink()

def get_safplus_node_name(): return safplus_getenv('NODENAME')

#def get_safplus_tipc_netid(): return safplus_getenv('TIPC_NETID', default='undefined')

def unload_tipc_cmd(): return os_platform.unload_tipc_cmd

def load_tipc_cmd(): return os_platform.load_tipc_cmd

def is_tipc_loaded_cmd(): return os_platform.is_tipc_loaded_cmd

def get_safplus_save_dir_margin(): return int(safplus_getenv('ASP_SAVE_DIR_SIZE_MARGIN', default=(5 * 1024)))

def get_safplus_save_dir_max_limit(): return int(safplus_getenv('ASP_SAVE_DIR_SIZE_MAX_LIMIT', default=(2 * 1024 * 1024)))

def get_core_file_size() : return safplus_getenv('ASP_CORE_FILE_SIZE', default='unlimited')

def get_safplus_valgrind_cmd(): return safplus_getenv('ASP_VALGRIND_CMD', default='')

def is_valgrind_build(): return bool(len(get_safplus_valgrind_cmd().strip()))

def is_disable_core(): return bool(int(safplus_getenv('ASP_DISABLE_CORE', default=0)))

def is_system_controller(): return safplus_getenv('SYSTEM_CONTROLLER')

def is_simulation(): return bool(int(safplus_getenv('ASP_SIMULATION', default='0')))
   
def is_tipc_build(): return bool(int(safplus_getenv('BUILD_TIPC', default='1')))

def save_previous_logs(): return int(safplus_getenv('ASP_SAVE_PREV_LOGS', default='0'))

def get_hpi_ip(): return safplus_getenv('SAHPI_UNSPECIFIED_DOMAIN_ID', default='undefined')

def fail_and_exit(msg):
    log.critical(msg)
    sys.exit(1)

def execute_shell_cmd(cmd, err_msg, fail_on_error=True):
    ret, out, sig, core = system(cmd)

    if ret and fail_on_error:
        fail_and_exit('%s : attempted: [%s], output: [%s]'
                      % (err_msg, cmd, out))
    elif ret:
        log.warning('%s : attempted: [%s], output: [%s]'
                    % (err_msg, cmd, out))

def get_dir(p):
    if os.path.exists(p):
        return p
    else:
        try:
            os.mkdir(p)
            return p
        except OSError, e:
            fail_and_exit('Failed to create directory, [%s]' % e)
                
def safplus_getenv(var, default=None):
    val = os.getenv(var) or default
    if val is None:
        fail_and_exit('The %s environment variable is not set in the %s/asp.conf file, '
                      'or the %s/asp.conf file has not been sourced.' % (var, SAFPLUS_ETC_DIR, SAFPLUS_ETC_DIR ))
    return val

def get_safplus_ulimit_cmd():     
    if is_disable_core():
        return 'ulimit -c 0'
    else:
        return 'ulimit -c ' + get_core_file_size()

def is_root():
    uid = os.getuid()
    return uid == 0

def log_safplus_env():
    log.debug('SAFplus environment information :')
    log.debug('sandbox directory : [%s]' % sandbox_dir)
    log.debug('Location of old logs : [%s]' % get_save_log_dir())
    log.debug('Link name : [%s]' % get_link_name())
    log.debug('TIPC build ? : %s' %bool(int(is_tipc_build())))
    if is_tipc_build():
        log.debug('TIPC netid : [%s]' % get_safplus_tipc_netid())
        log.debug('Tipc config command : [%s]' % get_tipc_config_cmd())
    log.debug('Node address : [%s]' % get_safplus_node_addr())
    log.debug('Node name : [%s]' % get_safplus_node_name())

    log.debug('System controller ? : %s' % is_system_controller() )
    log.debug('Simulation ? : %s' % is_simulation() )

def gen_safplus_run_env_file(run_file):
    """ Generates a run-time env file that carries all needed environment vars """
    try:
        f = file(run_file, 'w')
    except IOError:
        if not is_root():
            return
        fail_and_exit('Could not create file %s' % run_file)
    print >> f, '# This file is auto-generated when SAFplus is started, please do not modify'
    print >> f, 'ASP_DIR=%s' % sandbox_dir
    print >> f, 'ASP_BINDIR=%s' % SAFPLUS_BIN_DIR
    print >> f, 'ASP_APP_BINDIR=%s' % SAFPLUS_BIN_DIR
    print >> f, 'ASP_RUNDIR=%s' % SAFPLUS_RUN_DIR
    print >> f, 'ASP_DBDIR=%s' % SAFPLUS_DB_DIR
    print >> f, 'ASP_CONFIG=%s' % SAFPLUS_ETC_DIR
    print >> f, 'ASP_LOGDIR=%s' % SAFPLUS_LOG_DIR
    print >> f, 'ASP_NODENAME=%s' % get_safplus_node_name()
    print >> f, 'ASP_NODEADDR=%s' % get_safplus_node_addr()
    print >> f, 'ASP_MULTINODE=%s' % is_simulation()
    print >> f, 'ASP_SIMULATION=%s' % is_simulation()
    print >> f, 'BUILD_TIPC=%s' % is_tipc_build()
    f.close()
  
def get_safplus_node_addr(): 
    node_addr = safplus_getenv('DEFAULT_NODEADDR')

    # if AUTO_ASSIGN_NODEADDR is not defined or defined as "disabled", use default node address
    auto_mode = os.getenv('AUTO_ASSIGN_NODEADDR')
    if auto_mode == 'physical-slot':
        try:
            node_addr = get_physical_slot()
        except IpmiError:
            log.warning('Could not determine physical slot id. Will use default node address %s' % node_addr)
            log.warning('This is probably because IPMI was not available')

    return node_addr

def init_log():
    import logging

    logger = logging.getLogger('SAF')
    logger.setLevel(logging.DEBUG)

    console = logging.StreamHandler(sys.stdout)
    console.setLevel(logging.INFO)

    formatter = logging.Formatter('%(levelname)s %(message)s')
    console.setFormatter(formatter)

    logger.addHandler(console)
    return logger

def put_safplus_config():
    os.putenv('ASP_DIR', sandbox_dir)
    os.putenv('ASP_LOGDIR', SAFPLUS_LOG_DIR)
    os.putenv('ASP_RUNDIR', SAFPLUS_RUN_DIR)
    os.putenv('ASP_DBDIR', SAFPLUS_DB_DIR)
    os.putenv('ASP_SCRIPTDIR', SAFPLUS_SCRIPT_DIR)
    os.putenv('ASP_CPM_CWD', SAFPLUS_RUN_DIR)
    os.putenv('ASP_CONFIG', SAFPLUS_ETC_DIR)
    os.putenv('ASP_BINDIR', SAFPLUS_BIN_DIR)
    os.putenv('SAHPI_UNSPECIFIED_DOMAIN_ID', get_hpi_ip())
    #os.putenv('ASP_MULTINODE', is_simulation())
    #os.putenv('ASP_SIMULATION', is_simulation())
    #os.putenv('BUILD_TIPC', is_tipc_build())
    gen_safplus_run_env_file(SAFPLUS_ETC_DIR +'/asp_run.env')    
    return

def is_executable_file(f):
    import stat
    if not os.path.exists(f):
        return False

    is_file = stat.S_ISREG(os.stat(f)[stat.ST_MODE])
    
    return is_file and os.access(f, os.R_OK | os.X_OK)

def stop_led_controller():
    cmd = 'pkill lifesignal 2>/dev/null'
    os.system(cmd)

def start_led_controller():
    stop_led_controller()

    if os.path.exists('%s/lifesignal' % SAFPLUS_BIN_DIR):
        cmd = 'nohup %s/lifesignal 2>/dev/null &' % SAFPLUS_BIN_DIR
        os.system(cmd)

def start_hpi_subagent():
    def cm_is_openhpi_based():
        cmd = 'ldd %s/safplus_cm | grep -c libopenhpi' % SAFPLUS_BIN_DIR
        return int(Popen(cmd)[0]) > 0

    def cm_requires_openhpid():
        cmd = 'ldd %s/safplus_cm | grep -c libopenhpimarshal' % SAFPLUS_BIN_DIR
        return int(Popen(cmd)[0]) > 0

    if os.getenv('SAHPI_UNSPECIFIED_DOMAIN_ID') != "UNDEFINED":
        os.putenv('OPENHPI_UID_MAP', '%s/openhpi_uid.map' % SAFPLUS_RUN_DIR)
        os.putenv('OPENHPI_CONF', '%s/openhpi.conf' % SAFPLUS_ETC_DIR)

        if not os.path.exists('%s/safplus_cm' % SAFPLUS_BIN_DIR):
            cm_err_msg_list = [
                               'Note: Chassis manager will not be started '
                               'because safplus_cm is not found.',
                               'However, the chassis manager\'s IP is '
                               'configured in target.conf.',
                               'If your system contains a chassis manager, '
                               'you may reconfigure your build to',
                               'use the chassis manager (see ./configure --help). '
                               'Doing so will provide a',
                               'greater level of integration between the SAFplus and '
                               'your hardware.  You may', 
                               'remove this warning by removing the CMM_IP field '
                               'from the target.conf file.'
                               ]
            for cm_err_msg in cm_err_msg_list:
                log.warning(cm_err_msg)
        elif cm_is_openhpi_based():
            if cm_requires_openhpid(): 
                if not os.path.exists('%s/openhpid' % SAFPLUS_BIN_DIR):
                    fail_and_exit('Need to start openhpid but it cannot '
                                  'be found in %s' % SAFPLUS_BIN_DIR)

                cmd = '%s/openhpid -c $OPENHPI_CONF' % SAFPLUS_BIN_DIR
                os.system(cmd)

        cmd = '(sleep 60; setsid %s/hpiSubagent -s > '\
              '/dev/null 2>&1) &' % SAFPLUS_BIN_DIR
        os.system(cmd)

def run_custom_scripts(cmd):
    d = SAFPLUS_SCRIPT_DIR
    scriptOk = True
    if os.path.isdir(d):
        for f in os.listdir(d):
            f = os.path.abspath(d + '/' + f)
            if is_executable_file(f):
                st = os.system('%s %s' % (f, cmd))
                if os.WEXITSTATUS(st):
                    log.info('Script [%s] exitted abnormally with status [%d].' % (f, os.WEXITSTATUS(st)))
                    scriptOk = False
    return scriptOk 

def start_snmp_daemon():
    def set_snmp_conf_path():
        safplus_dir = sandbox_dir
        etc_dir = SAFPLUS_ETC_DIR
        p = [
            safplus_dir + '/share/snmp',
            etc_dir,
            '/usr/local/etc/snmp',
            '/usr/local/share/snmp',
            '/usr/local/lib/snmp'
            ]

        old_snmp_conf_path = os.getenv('SNMPCONFPATH')
        if old_snmp_conf_path:
            p.insert(0, old_snmp_conf_path)

        v = ':'.join(p)

        os.putenv('SNMPCONFPATH', v)

    snmpd_exe = SAFPLUS_BIN_DIR + os.sep + 'snmpd'
    if os.path.exists(snmpd_exe):
        set_snmp_conf_path()
        snmpd_modules_flag = os.getenv("SNMP_MODULES")
        if snmpd_modules_flag is None: snmpd_modules_flag = ""
        else: snmpd_modules_flag = "-I " + snmpd_modules_flag

        log.info('Starting SNMP daemon...')
        cmd = 'setsid %s %s -DH -Lo -f -C -c %s/etc/snmpd.conf '\
              '>/dev/null 2>&1 &' % (snmpd_exe, snmpd_modules_flag, sandbox_dir)
        if int(os.system(cmd)):
            fail_and_exit('Failed to start snmp daemon')

def save_safplus_runtime_files():

    def get_current_time():
        import time
        return time.strftime('%Y_%m_%d_%H_%M_%S')

    def is_empty_dir(d):
        if not os.path.exists(d):
            return True
        else:
            return len(os.listdir(d)) == 0

    def is_core_present():
        res = False
        loc = os_platform.core_file_dir()
        core_regex = os_platform.core_file_regex()

        if not loc: loc = SAFPLUS_RUN_DIR

        cmd = 'find %s -name \'%s\'' % (loc, core_regex)
        r, o, s, c = system(cmd)
        if r:
            log.warning('Executing [%s] command failed, ret = %s' % (cmd, r))
            res = False
        else:
            res = bool(o)
        
        return res

    def can_save_dir(d, log_dir):
 
        def dir_size(d, defsize=10*1024):
            l = Popen('du -sk %s' % d)
            if len(l) != 1:
                log.critical('The command \`du -sk\' did not return '
                             'expected output, returning %sKb as the value'
                             % defsize)
                return defsize

            s = l[0][:-1].split()[0]

            return int(s)
        
        def dir_free_space(d, defsize=10*1024):
            cmd = 'df -Pk %s' % d
            
            l = Popen(cmd)
            if len(l) == 1: # if the size cannot be determined
              log.warn('Cannot determine available space at "%s" (could it be a network mount?)  Assuming plenty of room.' % d)
              return defsize

            if len(l) != 2:
                log.critical('The command "%s" did not return '
                             'the expected output, returning %sKb as the free space.  It returned "%s"'
                             % (cmd,defsize,l))

                return defsize

            s = l[1].split()[3]

            return int(s)

        max_limit = get_safplus_save_dir_max_limit()
        cur_save_dir_dir_size = dir_size(d)
        save_log_dir = log_dir
        cur_save_log_dir_size = dir_size(save_log_dir)

        if max_limit < cur_save_dir_dir_size + cur_save_log_dir_size:
            msg = 'Maximum size limit exceeded, while trying to save [%s]\n'\
                  'Maximum limit on space occupied by logs in [%s] = %sKb\n'\
                  'Actual space occupied now by [%s] = %sKb, '\
                  'size of [%s] = %sKb'\
                  % (d,
                     save_log_dir,
                     max_limit,
                     save_log_dir,
                     cur_save_log_dir_size,
                     d,
                     cur_save_dir_dir_size)
            return (False, msg)

        save_dir_margin = get_safplus_save_dir_margin()
        cur_save_dir_margin = abs(dir_free_space(save_log_dir) - cur_save_dir_dir_size)

        if save_dir_margin >= cur_save_dir_margin:
            msg = 'Minimum size margin exceeded, while trying to save [%s]\n'\
                  'Minimum margin = %sKb, actual margin now = %sKb' % (d, save_dir_margin, cur_save_dir_margin)
            return (False, msg)
        else:
            return (True, '')

    def rm_runtime_files():
        p = re.compile(r'core.\d+$')
        l = glob.glob('%s/*' % SAFPLUS_RUN_DIR)
        l = [e for e in l if not p.search(e)]
        for e in l:
            execute_shell_cmd('rm -rf \'%s\'' % e, 'Failed to delete [%s]' % e)

    def save_cores():
        run_dir = SAFPLUS_RUN_DIR
        cores_dir = get_safplus_cores_dir()

        can_save, err_msg = can_save_dir(run_dir, cores_dir)
        if not can_save:
            log.critical(err_msg)
            execute_shell_cmd('rm -f %s/*', run_dir)
        else:
            cur_time = get_current_time()
            for e in glob.glob('%s/*' % run_dir):
                core_file_name = os.path.split(e)[1]
                execute_shell_cmd('mv -f %s %s/%s_%s' %\
                                  (e, cores_dir, core_file_name, cur_time),
                                  'Failed to move core file [%s]' % e)

    if save_previous_logs():
         for d in ['log', 'run']:
            src = getattr(sys.modules[__name__], 'get_safplus_%s_dir' % d)()

            if is_core_present():
                log_dir = get_safplus_crash_log_dir()
            else:
                log_dir = get_safplus_normal_log_dir()

            dst = log_dir + '/%s_' % d +\
                  '%s_' % get_safplus_node_name() +\
                  '%s' % get_current_time()

            if not is_empty_dir(src):
                can_save, err_msg = can_save_dir(src, log_dir)
                if not can_save:
                    log.critical(err_msg)
                    cmd = 'rm -rf %s' % src
                    execute_shell_cmd(cmd, 'Failed to delete [%s]' % src)
                    log.info('Deleted previous %s directory' % src)
                else:
                    cmd = 'cp -Ppr %s %s' % (src, dst)
                    execute_shell_cmd(cmd, 'Failed to copy [%s] to [%s]'
                                      % (src, dst), fail_on_error=False)
                
                    cmd = 'rm -rf %s' % src
                    execute_shell_cmd(cmd, 'Failed to delete [%s]' % src)
                    log.info('Saved previous %s directory in %s' % (d, dst))

                try:
                    os.mkdir(src)
                except OSError, e:
                    if e.errno == errno.EEXIST:
                        pass
                    else:
                        raise
    else:
        rm_runtime_files()
        save_cores()
            
    if 0: # remove_persistent_db:  This is removed during the initial startup (but should it be removed whenever the AMF is restarted?)
        safplus_db_dir = SAFPLUS_DB_DIR
        cmd = 'rm -rf %s/*' % safplus_db_dir
        execute_shell_cmd(cmd, 'Failed to delete [%s]' % safplus_db_dir)
        log.info('Deleted persistent files in %s directory' % safplus_db_dir)

def start_amf():
    def is_amf_in_valgrind_list():
        val_filter = os.getenv("ASP_VALGRIND_FILTER")
        if val_filter:
            val_filter = val_filter.split()
            return "safplus_amf" in val_filter

        return True

    # Create log file to save any error while starting SAFplus_AMF. location : sandbox_dir/var/log/Node_Name.log
    file_name_path = '%s/%s.log' %(SAFPLUS_LOG_DIR, get_safplus_node_name())
    log_file = open(file_name_path, 'a')
     
    cmd = '%s/%s' %(SAFPLUS_BIN_DIR, AmfName)
    chassi_count = '0' 
    log.info('Starting AMF (%s)...' % cmd)

    process = subprocess.Popen([cmd,\
                                SAF_AMF_START_CMD_CHASSIS, chassi_count,\
                                SAF_AMF_START_CMD_LOCAL_SLOT, get_safplus_node_addr(),\
                                SAF_AMF_START_CMD_NODE_NAME, get_safplus_node_name()],\
                                stdout=log_file, stderr=log_file, shell=False)
    return process

def cleanup_and_start_ams():

    log.debug("Cleanup SAFplus")
    cleanup_safplus()                 # remove shared mem

    try:
        log.debug("Save Previous Logs ")
        save_safplus_runtime_files()      # save previous core, log, files and clear DB files 
    except Exception, e:
        log.warning("Exception when generating crash dump: %s" % str(e))

    if is_system_controller():
            start_snmp_daemon()

    run_custom_scripts('start')
    process = start_amf()

    if is_system_controller() and not is_simulation():
        start_hpi_subagent()
    if not is_simulation():
        start_led_controller()
    return process

def start_openhpid(stop_watchdog=False):
    try:
        if not os.path.exists('%s/openhpid' % SAFPLUS_BIN_DIR):
            fail_and_exit('Need to start openhpid but it cannot be found in %s' % SAFPLUS_BIN_DIR)
        else:
            cmd = '%s/openhpid -c $OPENHPI_CONF' % SAFPLUS_BIN_DIR
            log.debug('Restarting openhpid with cmd: ' + cmd)
            ret = os.system(cmd)

            log.debug('cmd exited with: ' + str(ret))

            # lets make sure it came up
            time.sleep(1)
            pid = get_openhpid_pid()
            if pid == 0:
                log.critical('failed to restart openhpid, did not come up...')
            else:
                log.debug('openhpid restarted successfully with pid: %d' % pid)

    except:
        raise Exception

def get_openhpid_pid():
    """ attempts to get openhpid pid, if found, returns pid else returns 0 """
    try:
        l = Popen('ps aux | grep -i openhpid | grep -vF "grep"')

        if l: # pid found
            pid = int(l[0].split()[1])
            log.debug('found openhpid pid(%d)' % pid)
            return pid

    except:
        pass

    return 0

def get_amf_pid():
    while True:
        valid = commands.getstatusoutput("/bin/pidof %s" % AmfName);
        if valid[0] == 0:
            if len(valid[1].split())==1:          
                return int(valid[1])
        else:
            break
        log.warning('there are more than one AMF pid. Try again...')
        time.sleep(0.25)
    return 0;

def stop_amf():
    stopFile = SAFPLUS_RUN_DIR + '/' + SAFPLUS_STOP_FILE 
    touch(stopFile)    # create 'safplus_stop' file to indicate graceful shutdown of SAFplus_AMF

    def wait_for_safplus_shutdown():
        t = SAFPLUS_SHUTDOWN_WAIT_TIMEOUT
        for i in range(t/6):
            amf_pid = get_amf_pid()
            if amf_pid == 0:
                break
            time.sleep(6)

    amf_pid = get_amf_pid()
    if amf_pid == 0:
        log.warning('SAFplus is not running on node [%s]. Cleaning up anyway...' % get_safplus_node_addr())
        kill_amf()
        run_custom_scripts('stop')
    else:
        log.info('Stopping SAFplus AMF...')
        os.kill(amf_pid, signal.SIGINT)
        log.debug('SAFplus is running with pid: %d' % amf_pid)
        log.info('Waiting for SAFplus AMF to shutdown...')
        wait_for_safplus_shutdown()
        amf_pid = get_amf_pid()
        if amf_pid: kill_amf()
        run_custom_scripts('stop')


    if not is_simulation():
        safplus_tipc.unload_tipc_module()

def cleanup_safplus():
    node_addr = get_safplus_node_addr()
    cmd_list = os_platform.get_cleanup_safplus_cmd(node_addr)

    try:
        os.system(cmd_list)
    except Exception, e:
      print "Exception: %s" % str(e)
      print "CMD: %s" % cmd_list
      # print "data: %s" % result
      raise

def kill_amf():
    log.info('Killing SAFplus...')
    b = SAFPLUS_BIN_DIR

    if is_valgrind_build():
        l = Popen('ps -eo pid,cmd | grep valgrind')
        l = [e.split() for e in l]
        l = [e[0] for e in l if e[1] == 'valgrind']

        pid_cwd_list = []
        for e in l:
            cmdline = Popen('cat /proc/%s/cmdline' % e)[0]
            if 'valgrind' in cmdline:
                pid_cwd_list.append([e, cmdline])

        pid_cwd_list = [[e, cmdline.split('\x00')] for e, cmdline in pid_cwd_list]
        pid_cwd_list = [[e, [k for k in cmdline if b in k]] for e, cmdline in pid_cwd_list]
        pid_cwd_list = [[int(e), ''.join(k)] for e, k in pid_cwd_list]
        pid_cwd_list = [[pid, exe] for pid, exe in pid_cwd_list if exe]

    apps = os.listdir(b)

    # Insert amf binary at the top of the list for kills so there is no false recovery from amf 
    # which could also result in a node failfast resulting in node reboots
    try:
        apps.remove(AmfName)
        apps.insert(0, AmfName)
    except: pass

    for f in apps:
        f = os.path.abspath(b + '/' + f)
        if is_executable_file(f):
            cmd = os_platform.get_kill_amf_cmd(f)
            os.system(cmd)
        if is_valgrind_build():
            for pid, exe in pid_cwd_list:
                if exe == f:
                    try:
                        os.kill(pid, signal.SIGKILL)
                    except OSError, e:
                        if e.errno == errno.ESRCH:
                            pass
                        else:
                            raise

    stop_led_controller()

def zap_amf():
    stopFile = SAFPLUS_RUN_DIR + '/' + SAFPLUS_STOP_FILE 
    touch(stopFile)        # create 'safplus_stop' file to indicate gracefull shutdown of SAFplus_AMF

    run_custom_scripts('zap')
    kill_amf()

    if not is_simulation():
        safplus_tipc.unload_tipc_module()

def is_amf_running():
    '''
    Return value meaning :
    0 -> running
    1 -> not running,
    2 -> booting/shutting down
    3 -> running without status file, this is a bug.
    '''
    amf_pid = get_amf_pid()
    if amf_pid == 0:
        return 1
    
    safplus_status_file = get_status_file()

    if os.path.exists(safplus_status_file):
        t = int(Popen('cat %s' % safplus_status_file)[0])
        safplus_up = bool(t)
        if safplus_up:
            return 0
        else:
            return 2
    else:
        return 3

def get_safplus_status(to_shell=True):
    v = is_amf_running()
    if v == 0:
        print 'SAFplus is running on node [%s], pid [%s]' % (get_safplus_node_addr(), get_amf_pid() )
    elif v == 1:
        print 'SAFplus is not running on node [%s]' % get_safplus_node_addr()
    elif v == 2:
        print 'SAFplus is booting up/shutting down'
    elif v == 3:
        print 'SAFplus is running with pid [%s], but it was not started from this sandbox [%s].' % (get_amf_pid(), sandbox_dir)

    if to_shell:
        sys.exit(v)
    else:
        return v 

def get_pid_cmd(p):
    cmd = os_platform.get_pid_cmd(p)
    return cmd

def main():
    start_asp()

# log = init_log()  # Note, this logging config is interesting, but the users of this lib should call init_log to configure it... going with simplicity right now
put_safplus_config()    # load evn variables related to SAF execution

if __name__ == '__main__':
    main()
