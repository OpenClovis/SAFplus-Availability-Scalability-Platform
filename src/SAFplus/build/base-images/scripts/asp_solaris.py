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
import subprocess
import pdb

SystemErrorNoSuchFileOrDir = 127

get_amf_pid_cmd = 'ps -o pid,comm -A'

shm_dir = os.getenv('ASP_SHM_DIR') or '/dev/shm'

core_file_dir = ''
core_file_regex = 'core.*'

asp_shutdown_wait_timeout = 30


def system(cmd):
    """Similar to the os.system call, except that both the output and
    return value is returned"""
    if sys.version_info[0:2] <= (2, 4):
        pipe=os.popen('%s' %cmd)
        output=pipe.read()
        sts=pipe.close()
        retval=0
        signal=0
        core=0
        if sts:
            retval = int(sts)
            signal = retval & 0x7f
            core   = ((retval & 0x80) !=0)
            retval = retval >> 8
        #print 'popen Command return value %s, Output:\n%s' % (str(retval),output)
        return (retval, output, signal, core)
    else :
        #print 'Executing command: %s' % cmd
        child = subprocess.Popen(cmd, shell=True,
                             stdout=subprocess.PIPE,
                             stderr=subprocess.PIPE,
                             close_fds=True)
        output = []
        while True:
            pid, sts = os.waitpid(child.pid, os.WNOHANG)
            output += child.stdout.readlines()
            if pid == child.pid:
                break
            else:
                time.sleep(0.00001)
        child.stdout.close()
        child.stderr.close()
        retval = sts
        signal = retval & 0x7f
        core   = ((retval & 0x80) !=0)
        retval = retval >> 8
        #print 'Subprocess Command return value %s, Output:\n%s' % (str(retval),output)
        del child
        return (retval, output, signal, core)

def fail_and_exit(msg):
    log.critical(msg)
    #sys.exit(1)

def execute_shell_cmd(cmd, err_msg, fail_on_error=True):
    ret, out, sig, core = system(cmd)

    if ret and fail_on_error:
        fail_and_exit('%s : attempted: [%s], output: [%s]'
                      % (err_msg, cmd, out))
    elif ret:
        log.warning('%s : attempted: [%s], output: [%s]'
                    % (err_msg, cmd, out))

def get_kill_asp_cmd(f):
    return 'killall -KILL %s 2> /dev/null' % f

def get_amf_watchdog_pid_cmd(p):
    return 'ps -e -o pid,comm | grep \'%s\'' % p

def get_start_amf_watchdog_cmd(p):
    return 'setsid %s/safplus_watchdog.py &' % p

def get_cleanup_asp_cmd(p):
    return 'rm -f /%s/CL*_%s' % (shm_dir, p)

def get_asp_sandbox_dir():
    return asp_env['sandbox_dir']

def get_asp_run_dir():
    return asp_env['run_dir']

def get_asp_etc_dir():
    return asp_env['etc_dir']

def get_asp_bin_dir():
    return asp_env['bin_dir']

def get_asp_status_file():
    return asp_env['status_file']

def get_asp_log_dir():
    return asp_env['log_dir']

def get_asp_save_log_dir():
    return asp_env['save_log_dir']

def get_asp_script_dir():
    return asp_env['script_dir']

def get_asp_node_name():
    return asp_env['node_name']

def get_asp_tipc_netid():
    return asp_env['tipc_netid']

def get_asp_tipc_config_cmd():
    return asp_env['tipc_config_cmd']

def get_asp_node_addr():
    return asp_env['node_addr']

def get_asp_link_name():
    return asp_env['link_name']

def get_asp_save_dir_margin():
    return asp_env['save_dir_size_margin']

def get_asp_save_dir_max_limit():
    return asp_env['save_dir_size_max_limit']

def is_root():
    uid = os.getuid()
    return uid == 0

def is_system_controller():
    return bool(int(asp_env['system_controller']))

def is_simulation():
    return bool(int(asp_env['simulation']))

def enforce_tipc_settings():
    if 'enforce_tipc_settings' in asp_env:
        return True
    else:
        return False

def ignore_tipc_settings():
    if 'ignore_tipc_settings' in asp_env:
        return True
    else:
        return False

def save_previous_logs():
    return bool(int(asp_env['save_prev_logs']))

def set_asp_env(k, v):
    asp_env[k] = v

def log_asp_env():
    log.debug('ASP environment information :')
    log.debug('sandbox directory : [%s]' % asp_env['sandbox_dir'])
    log.debug('Location of old logs : [%s]' % asp_env['save_log_dir'])
    log.debug('Link name : [%s]' % asp_env['link_name'])
    log.debug('TIPC netid : [%s]' % asp_env['tipc_netid'])
    log.debug('Tipc config command : [%s]' % asp_env['tipc_config_cmd'])
    log.debug('Node address : [%s]' % asp_env['node_addr'])
    log.debug('Node name : [%s]' % asp_env['node_name'])

    log.debug('System controller ? : %s' %\
              bool(int(asp_env['system_controller'])))
    log.debug('Simulation ? : %s' %\
              bool(int(asp_env['simulation'])))

def gen_asp_run_env_file(run_file, d):
    """ Generates a run-time env file that carries all needed environment vars """
    try:
        f = file(run_file, 'w')
    except IOError:
        fail_and_exit('Could not create file %s' % run_file)
    print >> f, '# Hello Shri This file is auto-generated when ASP is started, please do not modify'
    print >> f, 'ASP_DIR=%s' % d['sandbox_dir']
    print >> f, 'ASP_BINDIR=%s' % d['bin_dir']
    print >> f, 'ASP_RUNDIR=%s' % d['run_dir']
    print >> f, 'ASP_DBDIR=%s' % d['db_dir']
    print >> f, 'ASP_CONFIG=%s' % d['etc_dir']
    print >> f, 'ASP_LOGDIR=%s' % d['log_dir']
    print >> f, 'LD_PRELOAD_32=/opt/SUNWtipc/lib/libtipcsocket.so.1'
    print >> f, 'LD_PRELOAD_64=/opt/SUNWtipc/lib/amd64/libtipcsocket.so.1'
    print >> f, 'ASP_NODENAME=%s' % d['node_name']
    print >> f, 'ASP_NODEADDR=%s' % d['node_addr']
    f.close()
    
def set_up_asp_config():

    def get_sandbox_dir():
        p = os.path.dirname(__file__)
        p = os.path.split(p)[0]

        return p

    def get_save_log_dir():
        p = os.getenv('ASP_PREV_LOG_DIR') or '/tmp/asp_saved_logs'
        if os.path.exists(p):
            return p
        else:
            try:
                os.mkdir(p)
                return p
            except OSError, e:
                fail_and_exit('Failed to create directory, [%s]' % e)

    def get_dir(p):
        if os.path.exists(p):
            return p
        else:
            try:
                os.mkdir(p)
                return p
            except OSError, e:
                fail_and_exit('Failed to create directory, [%s]' % e)
                
    d = {}

    sandbox = get_sandbox_dir()
    d['sandbox_dir'] = sandbox
    d['bin_dir'] = sandbox + '/bin'
    d['etc_dir'] = sandbox + '/etc'
    d['lib_dir'] = sandbox + '/lib'
    d['modules_dir'] = sandbox + '/modules'
    d['var_dir'] = get_dir(sandbox + '/var')
    d['log_dir'] = get_dir(d['var_dir'] + '/log')
    d['run_dir'] = get_dir(d['var_dir'] + '/run')
    d['db_dir'] = get_dir(d['var_dir'] + '/lib')
    d['save_log_dir'] = get_save_log_dir()
    d['script_dir'] = d['etc_dir'] + '/asp.d'
    
    def asp_getenv(var, default=None):
        val = os.getenv(var) or default
        if val is None:
            fail_and_exit('The %s environment variable is not set in the %s/asp.conf file, '
                          'or the %s/asp.conf file has not been sourced.' %
                          (var, d['etc_dir'], d['etc_dir']))
        return val
            
    class IpmiError(Exception): pass

    def get_physical_slot():
        # this assumes that we can access IPMI tool and the shelf manager via it
        cmd = '%s/bladeinfo -p 2>/dev/null || echo -1' % d['bin_dir']
        res = os.popen(cmd).readlines()[0]
        if res.startswith('-1'):
            log.debug('Could not determine physical slot id in auto mode because could not run bladeinfo -p. '
                      'Please check IPMI access manually by running bladeinfo -p, '
                      'or use an explicit node address (DEFAULT_NODEADDR) in asp.conf.')
            raise IpmiError
        res = res.split()
        if len(res)!=2 or res[0].strip() != 'physical_slot:':
            log.debug('Could not determine physical slot id in auto mode due to bladeinfo -p did not '
                      'return useful information. '
                      'Please check IPMI access manually by running bladeinfo -p, '
                      'or use an explicit node address (DEFAULT_NODEADDR) in asp.conf.')
            raise IpmiError
        slot = res[1].strip()
        try:
            slot = int(slot)
        except ValueError:
            log.debug('Physical slot could not be determined in auto mode. '
                      'Please check IPMI access manually by running bladeinfo -p, '
                      'or use an explicit node address (DEFAULT_NODEADDR) in asp.conf.')
            raise IpmiError
        return slot            
        
    node_addr = asp_getenv('DEFAULT_NODEADDR')

    # if AUTO_ASSIGN_NODEADDR is not defined or defined as "disabled", we
    # are going to use the default node address
    auto_mode = os.getenv('AUTO_ASSIGN_NODEADDR')
    if auto_mode == 'physical-slot':
        try:
            node_addr = get_physical_slot()
        except IpmiError:
            log.warning('Could not determine physical slot id. Will use default node address %s' % node_addr)
            log.warning('This is probably because IPMI was not available')

    d['node_addr'] = node_addr

    def get_status_file():
        return d['run_dir'] + '/aspstate-%s' % d['node_addr']

    def get_tipc_config_cmd():

        cmd = 'tipc-config > /dev/null 2>&1'
        tipc_config_cmd = 'tipc-config'
        
        ret, output, signal, core = system(cmd)

        if ret == SystemErrorNoSuchFileOrDir:
            log.debug('The tipc-config command is not in $PATH')
            tipc_config_cmd = d['bin_dir'] + os.sep + tipc_config_cmd
            if not os.path.exists(tipc_config_cmd):
                log.critical('The tipc-config command is not found in %s !!' %
                             d['bin_dir'])
                log.critical('This indicates some serious configuration '
                             'problem while deploying ASP image on target.')
                sys.exit(1)
            else:
                return tipc_config_cmd
        else:
            return tipc_config_cmd
    
    d['status_file'] = get_status_file()
    d['node_name'] = asp_getenv('NODENAME')
    d['tipc_netid'] = asp_getenv('TIPC_NETID', default='undefined')
    d['tipc_config_cmd'] = get_tipc_config_cmd()
    d['link_name'] = asp_getenv('LINK_NAME', default='eth0')
    d['system_controller'] = asp_getenv('SYSTEM_CONTROLLER')
    d['simulation'] = asp_getenv('ASP_SIMULATION', default='0')
    d['hpi_ip'] = asp_getenv('SAHPI_UNSPECIFIED_DOMAIN_ID', default='undefined')
    d['save_prev_logs'] = asp_getenv('ASP_SAVE_PREV_LOGS', default=1)
    d['save_dir_size_margin'] = int(asp_getenv('ASP_SAVE_DIR_SIZE_MARGIN',
                                          default=(5 * 1024)))
    d['save_dir_size_max_limit'] = int(asp_getenv('ASP_SAVE_DIR_SIZE_MAX_LIMIT',
                                             default=(2 * 1024 * 1024)))
    
    os.putenv('ASP_LOGDIR', d['log_dir'])
    os.putenv('ASP_RUNDIR', d['run_dir'])
    os.putenv('ASP_DBDIR', d['db_dir'])
    os.putenv('ASP_CPM_CWD', d['run_dir'])

    os.putenv('ASP_DIR', d['sandbox_dir'])
    
    # This needs to be cleaned up
    os.putenv('SAHPI_UNSPECIFIED_DOMAIN_ID', d['hpi_ip'])

    os.putenv('ASP_CONFIG', d['etc_dir'])
    os.putenv('ASP_BINDIR', d['bin_dir'])
    
    gen_asp_run_env_file(d['etc_dir']+'/asp_run.env', d)
    
    return d

    
def init_log():
    import logging

    logger = logging.getLogger('ASP')
    logger.setLevel(logging.DEBUG)

    console = logging.StreamHandler()
    console.setLevel(logging.INFO)

    formatter = logging.Formatter('%(levelname)s '
                                  '%(message)s')
    console.setFormatter(formatter)

    logger.addHandler(console)
    return logger

def is_executable_file(f):
    import stat
    is_file = stat.S_ISREG(os.stat(f)[stat.ST_MODE])
    
    return is_file and os.access(f, os.R_OK | os.EX_OK)

def stop_amf_watchdog():
    '''Depends on ps utility returning the first line
    to be that of the watchdog process.
    '''

    p = '%s/safplus_watchdog.py' % get_asp_etc_dir()
    cmd = 'ps -e -o pid,args | grep \'%s\'' % p
    wpid = int(os.popen(cmd).readlines()[0].split()[0])
    try:
        os.kill(wpid, signal.SIGKILL)
    except OSError, e:
        import errno
        if e.errno == errno.ESRCH:
            pass
        else:
            raise

def start_amf_watchdog(stop_watchdog = True):
    if stop_watchdog == True:
	stop_amf_watchdog()
    #pdb.set_trace()
    log.info('Starting AMF watchdog...')
    cmd = '%s/safplus_watchdog.py' % get_asp_etc_dir()
    os.system(cmd)

def stop_led_controller():
    cmd = 'pkill lifesignal 2>/dev/null'
    os.system(cmd)

def start_led_controller():
    stop_led_controller()

    if os.path.exists('%s/lifesignal' % get_asp_bin_dir()):
        cmd = 'nohup %s/lifesignal 2>/dev/null &' %\
              get_asp_bin_dir()
        os.system(cmd)

def start_hpi_subagent():
    def cm_is_openhpi_based():
        cmd = 'ldd %s/asp_cm | grep -c libopenhpi' %\
              get_asp_bin_dir()
        return int(os.popen(cmd).readlines()[0]) > 0

    def cm_requires_openhpid():
        cmd = 'ldd %s/asp_cm | grep -c libopenhpimarshal' %\
              get_asp_bin_dir()
        return int(os.popen(cmd).readlines()[0]) > 0

    if os.getenv('SAHPI_UNSPECIFIED_DOMAIN_ID') != "UNDEFINED":
        os.putenv('OPENHPI_UID_MAP',
                  '%s/openhpi_uid.map' % get_asp_run_dir())
        os.putenv('OPENHPI_CONF',
                  '%s/openhpi.conf' % get_asp_etc_dir())

        if not os.path.exists('%s/asp_cm' % get_asp_bin_dir()):
            cm_err_msg_list = [
                               'Note: Chassis manager will not be started '
                               'because asp_cm is not found.',
                               'However, the chassis manager\'s IP is '
                               'configured in target.conf.',
                               'If your system contains a chassis manager, '
                               'you may reconfigure your build to',
                               'use the chassis manager (see ./configure --help). '
                               'Doing so will provide a',
                               'greater level of integration between the ASP and '
                               'your hardware.  You may', 
                               'remove this warning by removing the CMM_IP field '
                               'from the target.conf file.'
                               ]
            for cm_err_msg in cm_err_msg_list:
                log.warning(cm_err_msg)
        elif cm_is_openhpi_based():
            if cm_requires_openhpid(): 
                if not os.path.exists('%s/openhpid' % get_asp_bin_dir()):
                    fail_and_exit('Need to start openhpid but it cannot '
                                  'be found in %s' % get_asp_bin_dir())

                cmd = '%s/openhpid -c $OPENHPI_CONF' % get_asp_bin_dir()
                os.system(cmd)

        cmd = '(sleep 60; %s/hpiSubagent -s > /dev/null 2>&1) &' % get_asp_bin_dir()
        os.system(cmd)

def start_amf():
    
    # chassis_id = get_asp_chassis_id()
    node_addr = get_asp_node_addr()
    node_name = get_asp_node_name()
    
    log.info('Starting AMF...')
    # chassis id is hardcoded to 1 as it is not used yet
    cmd = 'ulimit -c unlimited; %s/safplus_amf -c 0 -l %s -n %s' %\
          (get_asp_bin_dir(), node_addr, node_name)

    os.system(cmd)

def run_custom_scripts(cmd):
    d = get_asp_script_dir()
    
    if os.path.isdir(d):
        for f in os.listdir(d):
            f = os.path.abspath(d + '/' + f)
            if is_executable_file(f):
                os.system('%s %s' % (f, cmd))

def setup_gms_config():
    os.putenv('ASP_MULTINODE', '1')

def start_snmp_daemon():
    def set_snmp_conf_path():
        asp_dir = get_asp_sandbox_dir()
        etc_dir = get_asp_etc_dir()
        p = [
            asp_dir + '/share/snmp',
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

    set_snmp_conf_path()

    log.info('Starting SNMP daemon...')
    cmd = '%s/snmpd -DH -Lo -f -C -c %s/etc/snmpd.conf '\
          '>/dev/null 2>&1 &' % (get_asp_bin_dir(), get_asp_sandbox_dir())
    if int(os.system(cmd)):
        fail_and_exit('Failed to start snmp daemon')

def set_ld_library_paths():
    asp_dir = get_asp_sandbox_dir()

    p = [
        asp_dir + '/lib',
        asp_dir + '/lib/openhpi',
        '/usr/lib',
        '/usr/local/lib'
        ]

    old_ld_path = os.getenv('LD_LIBRARY_PATH')
    if old_ld_path:
        p.insert(0, old_ld_path)

    v = ':'.join(p)

    os.putenv('LD_LIBRARY_PATH', v)

def config_tipc_module():
    tipc_netid = get_asp_tipc_netid()
    node_addr = get_asp_node_addr()
    link_name = get_asp_link_name()
    
    cmd = '%s -netid=%s -addr=1.1.%s -be=eth:%s' %\
          (get_asp_tipc_config_cmd(), tipc_netid, node_addr, link_name)

    log.debug('TIPC command is [%s]' % cmd)
    ret, output, signal, core = system(cmd)
    output_buf = ''.join(output)
    if ret and 'unable to enable bearer' not in output_buf:
	#In solaris even when tipc configuration is successful, we get
	#unable to enable bearer output. So we should ignore it
        #if 'unable to enable bearer' in output_buf:
        #    msg = ''.join(['Failed to configure the tipc module. ',
        #                   
        #                   'System is configured to use %s, but tipc '
        #                   'cannot use this interface. ' % link_name,
        #                   
        #                   'Does it exist? To change the interface, '
        #                   'edit the LINK_NAME and linkName fields in '
        #                   '%s/asp.conf and %s/clGmsConfig.xml.' %\
        #                   (get_asp_etc_dir(), get_asp_etc_dir())])
        #    
        #    # Try to remove the tipc module if we failed to configure
        #    # tipc.  Otherwise it will work in the next run, but only
        #    # in "local" mode.
        #    unload_tipc_module()
        #    fail_and_exit(msg)

        if 'TIPC module not installed' in output_buf:
            msg = ''.join(['Failed to configure the tipc module. ',
                           
                           'The tipc kernel module is not loaded. ',
                           
                           'Use \'lsmod | grep tipc\' to see that '
                           'it is not loaded.'])
            fail_and_exit(msg)

        else:
            msg1 = ''.join(['Failed to configure the tipc module. ',
                           
                            'Executed \'%s\'. ' % cmd,
                           
                            'Received unknown tipc-config error: %s' %\
                            output_buf])

            msg2 = '\n'.join(['Please check that: ',
                             
                              '1. The tipc kernel module is loaded. '
                              '(lsmod | grep tipc)',
                              
                              '2. The tipc-config command is in your $PATH.',
                              
                              '3. Values for TIPC_NETID, DEFAULT_NODEADDR '
                              'and LINK_NAME are correct in %s/asp.conf.' %\
                              get_asp_etc_dir()])
            fail_and_exit(msg1 + msg2)

def unload_tipc_module():
    cmd = 'modinfo |grep tipc'
    ret, output, signal, core = system(cmd)
    if len(output) != 0:
	for line in output:
	    cmd = 'modunload -i ' + line.split()[0]
    	    ret, output, signal, core = system(cmd)
	    if ret != 0:
		log.critical('Failed to unload tipc module')

def load_tipc_module():
    cmd = 'modinfo |grep -c tipc'
    ret, output, signal, core = system(cmd)        
    if output[0].strip() == '0':
        sandbox = get_asp_sandbox_dir()
        log.debug('Trying to load TIPC module from the sandbox modules '\
                  'directory [ %s/modules ]' % sandbox)
        cmd = 'modload /usr/kernel/drv/tipc'
        ret, output, signal, core = system(cmd)
        if ret:
            fail_and_exit('Failed to load TIPC module: attempted: %s, '
                          'output was: %s' % (cmd, ''.join(output)))

def load_config_tipc_module():

    def is_tipc_netid_defined():
        return get_asp_tipc_netid() != 'undefined'

    def is_tipc_loaded():
        cmd = 'modinfo | grep -c tipc'
        c = int(os.popen(cmd).readlines()[0][:-1])
        return bool(c)

    def is_tipc_configured():
        if not is_tipc_loaded():
            return False

        tipc_config_cmd = get_asp_tipc_config_cmd()
        
        bearers = os.popen('%s -b' % tipc_config_cmd).readlines()
        bearers = [e[:-1] for e in bearers[1:] if e != 'No active bearers\n']
        if not bearers:
            return False

        return True

    def is_tipc_properly_configured():
        tipc_config_cmd = get_asp_tipc_config_cmd()
        
        tipc_addr = os.popen('%s -addr' % tipc_config_cmd).readlines()[0]
        tipc_addr = tipc_addr.split(':')[1].strip()[1:-1]

        if tipc_addr != '1.1.%s' % get_asp_node_addr():
            log.debug('System configured TIPC address : %s, '
                      'user configured TIPC address : %s' %
                      (tipc_addr, '1.1.%s' % get_asp_node_addr()))
            return False

        tipc_netid = os.popen('%s -netid' % tipc_config_cmd).readlines()[0]
        tipc_netid = tipc_netid.split(':')[1].strip()

        if tipc_netid != get_asp_tipc_netid():
            log.debug('System configured netid : %s, '
                      'user configured netid : %s' %
                      (tipc_netid, get_asp_tipc_netid()))
            return False

        bearers = os.popen('%s -b' % tipc_config_cmd).readlines()
        bearers = [e[:-1] for e in bearers[1:]]
        tipc_bearer = 'eth:%s' % get_asp_link_name()

        if tipc_bearer not in bearers:
            log.debug('Configured bearer %s not in bearer list %s' %
                      (tipc_bearer, bearers))
            return False

        return True

    def tipc_failed_state(tipc_state):
        if tipc_state == (0, 0, 0):
            msg = '\n'.join([ 'TIPC is not configured for this ASP build.',

                              'Please manually load and configure TIPC module '
                              'to proceed.'])
        elif tipc_state == (0, 0, 1):
            msg = '\n'.join([ 'This is an invalid state %s, '
                              'and indicates a bug.' % tipc_state])
        elif tipc_state == (1, 0, 1):
            msg = '\n'.join([ 'This is an invalid state %s, '
                              'and indicates a bug.' % tipc_state])

        fail_and_exit(msg)

    def tipc_valid_state(tipc_state):
        if tipc_state == (0, 1, 0):
            msg = '\n'.join([ 'TIPC module is loaded but not configured.',
                              
                              'Please configure tipc manually using tipc-config '
                              'command to proceed.',
                              
                              'If tipc-config is not in your $PATH, '
                              'you can find it in %s' % get_asp_bin_dir()])
            fail_and_exit(msg)
        elif tipc_state == (0, 1, 1):
            pass
        elif tipc_state == (1, 0, 0):
            #unload_tipc_module()
            #load_tipc_module()
            #config_tipc_module()
            log.debug('Ignoring tipc module loading in solaris')
        elif tipc_state == (1, 1, 0):
            config_tipc_module()
        elif tipc_state == (1, 1, 1):
            if not is_tipc_properly_configured():
                log.debug('TIPC configuration mismatch.')
                if enforce_tipc_settings():
                    log.debug('The --enforce-tipc-settings option '
                              'is set, so using configuration in '
                              '%s/asp.conf.' % get_asp_etc_dir())
                    unload_tipc_module()
                    load_tipc_module()
                    config_tipc_module()
                elif ignore_tipc_settings():
                    log.debug('The --ignore-tipc-settings option '
                              'is set, so using system TIPC configuration...')
                else:
                    msg = '\n'.join([ 'TIPC is loaded and configured but '
                                      'the configuration does not match '
                                      'with that in %s/asp.conf' %
                                      get_asp_etc_dir(),
                                      
                                      'Please start ASP with either '
                                      '--enforce-tipc-settings to use '
                                      'TIPC configuration in %s/asp.conf '
                                      'or --ignore-tipc-settings to use '
                                      'system TIPC configuration '
                                      'to proceed further.' %
                                      get_asp_etc_dir()])

                    fail_and_exit(msg)
                    

    # Tipc module loading and configuring state machine :-(
    # (tipc_netid_defined ?, tipc_loaded ?, tipc_configured ?)

    tipc_state_machine = {
        (0, 0, 0) : tipc_failed_state,
        (0, 0, 1) : tipc_failed_state,
        (0, 1, 0) : tipc_valid_state,
        (0, 1, 1) : tipc_valid_state,
        (1, 0, 0) : tipc_valid_state,
        (1, 0, 1) : tipc_failed_state,
        (1, 1, 0) : tipc_valid_state,
        (1, 1, 1) : tipc_valid_state,
    }


    if not is_root():
        if not is_tipc_loaded():
            fail_and_exit('ASP is not being run in root user mode '
                          'and TIPC module is not loaded.\n'
                          'Please run ASP as either root '
                          'or load and configure TIPC module properly '
                          'as root to continue.')
        else:
            log.info('TIPC module is loaded, assuming that it is '
                     'configured properly and continuing...')
    else:
        if is_simulation():
            if not is_tipc_loaded():
                load_tipc_module()
        else:
            tipc_state = is_tipc_netid_defined(), \
                         is_tipc_loaded(), \
                         is_tipc_configured()
            tipc_state = tuple([int(e) for e in tipc_state])
            log.debug('TIPC state : %s' % str(tipc_state))
            tipc_state_machine[tipc_state](tipc_state)

def check_sys_config():
    os.system('%s/check_sys.sh' % get_asp_bin_dir())
    
def save_asp_runtime_files():

    def get_current_time():
        import time
        return time.strftime('%Y_%m_%d_%H_%M_%S')

    def is_empty_dir(d):
        if not os.path.exists(d):
            return True
        else:
            return len(os.listdir(d)) == 0

    def can_save_dir(d):
    
        def dir_size(d, defsize=10*1024):
            l = os.popen('du -sk %s' % d).readlines()
            if len(l) != 1:
                log.critical('The command \`du -shk\' did not return '
                             'expected output, returning %sKb as the value'
                             % defsize)
                return defsize

            s = l[0][:-1].split()[0]

            return int(s)

        def dir_free_space(d, defsize=10*1024):
            l = os.popen('df -k %s' % d).readlines()
            if len(l) != 2:
                log.critical('The command \`df -Phk\' did not return '
                             'expected output, returning %sKb as the free space'
                             % defsize)

                return defsize
            
            s = l[1].split()[3]

            return int(s)

        max_limit = get_asp_save_dir_max_limit()
        cur_save_dir_dir_size = dir_size(d)
        save_log_dir = get_asp_save_log_dir()
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

        save_dir_margin = get_asp_save_dir_margin()
        cur_save_dir_margin = abs(dir_free_space(save_log_dir) - cur_save_dir_dir_size)

        if save_dir_margin >= cur_save_dir_margin:
            msg = 'Minimum size margin exceeded, while trying to save [%s]\n'\
                  'Minimum margin = %sKb, actual margin now = %sKb'\
                  % (d, save_dir_margin, cur_save_dir_margin)
            return (False, msg)
        else:
            return (True, '')

    for d in ['log', 'run']:
        src = getattr(sys.modules[__name__], 'get_asp_%s_dir' % d)()

        dst = get_asp_save_log_dir() + '/%s_' % d +\
              '%s_' % get_asp_node_name() +\
              '%s' % get_current_time()

        if not is_empty_dir(src):
            if save_previous_logs():
                can_save, err_msg = can_save_dir(src)

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
            else:
                cmd = 'rm -rf %s' % src
                execute_shell_cmd(cmd, 'Failed to delete [%s]' % src)
                
                log.info('Deleted previous %s directory' % src)

            os.mkdir(src)

def check_if_root():
    if not is_root():
        fail_and_exit('Please run this script as a root, '
                      'you are running this script as [%s]' %
                      os.environ['USER'])

def touch_lock_file(asp_file):
    cmd = 'touch %s' % asp_file
    ret, output, signal, core = system(cmd)
    if ret:
        fail_and_exit('Failed to touch lock file: attempted: %s '
                      'output was: %s' % (cmd, output))

def remove_lock_file(asp_file):
    cmd = '[ -f %s ] && rm -f %s' % (asp_file, asp_file)
    system(cmd)

def proc_lock_file(cmd):
    if not is_root():
        return

    d = '/var/lock/subsys'
    if not os.path.exists(d):
        return

    f = 'asp'
    asp_file = d + os.sep + f

    if cmd == 'touch':
        fn = touch_lock_file
    elif cmd == 'remove':
        fn = remove_lock_file
    else:
        assert(0)

    fn(asp_file)
    
def start_asp(stop_watchdog = True):
    try:
        check_asp_status()
        kill_asp()
        cleanup_asp()
        #pdb.set_trace()
        save_asp_runtime_files()
        check_sys_config()
        load_config_tipc_module()
        set_ld_library_paths()
        if is_system_controller():
            start_snmp_daemon()
        if is_simulation():
            setup_gms_config()
        proc_lock_file('touch')
        run_custom_scripts('start')
        start_amf()
        if is_system_controller() and not is_simulation():
            start_hpi_subagent()
        if not is_simulation():
            start_led_controller()
        start_amf_watchdog(stop_watchdog)
    except:
        raise

def get_amf_pid():
    def get_pid_for_this_sandbox(pid):
        proc_file = '/usr/bin/pwdx %s' % pid
        
        try:
            ret, cwd, signal, core = system(proc_file)
	    cwd = cwd[0].split()[1]
        except OSError, e:
            log.error('Failed to read [%s] : %s' %\
                      (proc_file, e))
            cwd = ''

        if cwd.endswith(' (deleted)'):
            log.critical('Contents of this sandbox directory were deleted '
                         'without stopping ASP which was started '
                         'from it.')
            log.critical('Please kill all the ASP processes manually '
                         'before proceeding any further.')
            sys.exit(1)

        return cwd == get_asp_run_dir()
    
    l = os.popen('ps -A | grep safplus_amf').readlines()
    l = [int(e.split()[0]) for e in l]

    l = filter(get_pid_for_this_sandbox, l)

    if len(l) == 0:
        return 0
    else:
        return int(l[0])
    
def wait_until_amf_up():
    amf_pid = 0
    
    for i in range(3):
        amf_pid = get_amf_pid()
        if amf_pid:
            break

        time.sleep(10)

    return amf_pid
    
def stop_asp():
    def wait_for_asp_shutdown():
        for i in range(5):
            amf_pid = get_amf_pid()
            if amf_pid == 0:
                break
            time.sleep(6)

    amf_pid = get_amf_pid()
    if amf_pid == 0:
        log.warning('ASP is not running on node [%s]. Cleaning up anyway...' %
                    get_asp_node_addr())
    else:
        log.info('Stopping AMF...')
        os.kill(amf_pid, signal.SIGINT)

    log.info('Stopping AMF watchdog...')
    stop_amf_watchdog()

    log.info('Waiting for AMF to shutdown...')
    wait_for_asp_shutdown()

    run_custom_scripts('stop')

    kill_asp()

    proc_lock_file('remove')

def cleanup_asp():
    asp_node_addr = get_asp_node_addr()
    cmd_list = ['rm -f /dev/shm/CL*_%s' % asp_node_addr,
                'rm -f /tmp/.SHM*_%s' % asp_node_addr,
                'rm -f /tmp/.CL*_%s' % asp_node_addr]

    for cmd in cmd_list:
        os.system(cmd)

def kill_asp():
    amf_pid = get_amf_pid() # Dummy, to guard against deletion of sandbox
    if amf_pid != 0:
       os.system('pkill -P %s 2> /dev/null' % amf_pid)
       os.system('kill -9 %s 2> /dev/null' % amf_pid)
       stop_led_controller()
       proc_lock_file('remove')

def restart_asp():
    pass

def start_asp_console():
    os.putenv('ASP_CPM_LOGFILE', 'console')
    start_asp()
    
def check_asp_status():
    v = is_asp_running()
    if v == 0:
        fail_and_exit('ASP is already running on node [%s], pid [%s]' %\
                      (get_asp_node_addr(), get_amf_pid()))
    elif v == 2:
        fail_and_exit('ASP is still booting up/shutting down on node [%s], '
                      'pid [%s]. '
                      'Please give \'stop\' or \'zap\' command and '
                      'then continue' %\
                      (get_asp_node_addr(), get_amf_pid()))
    elif v == 3:
        fail_and_exit('ASP is running on node [%s], pid [%s] '
                      'without status file, which indicates a bug'
                      (get_asp_node_addr(), get_amf_pid()))

def get_asp_status(to_shell=True):
    v = is_asp_running()

    if v == 0:
        log.info('ASP is running on node [%s], pid [%s]' %\
                 (get_asp_node_addr(), get_amf_pid()))
    elif v == 1:
        log.info('ASP is not running on node [%s]' %\
                 get_asp_node_addr())
    elif v == 2:
        log.info('ASP is booting up/shutting down')
    elif v == 3:
        log.info('ASP is running without the status file')

    # Hack, hack, hack !!!
    # I (vshenoy) don't know of any other way to propagate this
    # value back to shell
    if to_shell:
        sys.exit(v)
    else:
        return v
    
def is_asp_running():
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
    
    asp_status_file = get_asp_status_file()

    if os.path.exists(asp_status_file):
        t = int(os.popen('cat %s' % asp_status_file).readlines()[0])
        asp_up = bool(t)
        if asp_up:
            return 0
        else:
            return 2
    else:
        return 3
    
def usage():
    print
    print 'Usage : %s {start|stop|restart|console|status|zap|help} [options]' %\
          os.path.splitext(os.path.basename(sys.argv[0]))[0]
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
          ('--asp-log-level <level>',
           'Start ASP with particular log level. <level> is '
           '[trace|debug|info|notice|warning|error|critical]'))

    for o, h in l:
        print '%-30s:  %s' % (o, h)

def asp_driver(cmd):
    cmd_map = {'start' : start_asp,
               'stop' : stop_asp,
               'restart': restart_asp,
               'console' : start_asp_console,
               'status' : get_asp_status,
               'zap' : kill_asp,
               'help' : usage
               }

    if cmd_map.has_key(cmd):
        cmd_map[cmd]() 
    else:
        fail_and_exit('Command [%s] not found !!' % cmd)

def sanity_check():
    def check_for_root_files(c):
        cmd = 'find %s -uid 0 -type %s' % (get_asp_sandbox_dir(), c)
        l = os.popen(cmd).readlines()

        if c == 'f':
            t = 'files'
        elif c == 'd':
            t = 'directories'
        else:
            assert(0)

        if l:
            fail_and_exit('Some of the %s in the sandbox '
                          'are owned by root. \nThis indicates that '
                          'previously ASP may have been/is running '
                          'in root user mode. \n'
                          'Please delete those files to continue '
                          'running/querying ASP as normal user or '
                          'run/query ASP as root.' % t)

    def check_for_root_shms():
        cmd = 'find %s -uid 0 -type f -name \'CL_*\'' % '/dev/shm'
        l = os.popen(cmd).readlines()

        if l:
            fail_and_exit('Some of the shared memory segments '
                          'in /dev/shm are owned by root. \n'
                          'This indicates that previously ASP may '
                          'have been/is running in root user mode. \n'
                          'Please delete those files to continue '
                          'running/querying ASP as normal user or '
                          'run/query ASP as root.')

    check_for_root_files('f')
    check_for_root_files('d')

    check_for_root_shms()

def parse_command_line():
    import getopt

    try:
        opts, args = getopt.getopt(sys.argv[2:],
                                   'v',
                                   ['enforce-tipc-settings',
                                    'ignore-tipc-settings',
                                    'asp-log-level='])
    except getopt.GetoptError, e:
        print 'Command line parsing failed, error [%s]' % e
        usage()
        sys.exit(1)

    for o, a in opts:
        if o == '-v':
            import logging

            for h in log.handlers:
                h.setLevel(logging.DEBUG)
        elif o == '--enforce-tipc-settings':
            set_asp_env('enforce_tipc_settings', True)
        elif o == '--ignore-tipc-settings':
            set_asp_env('ignore_tipc_settings', True)
        elif o == '--asp-log-level':
            l = ['trace', 'debug',
                 'info', 'notice',
                 'warning', 'error',
                 'critical']
            if a.lower() in l:
                log_level = a.upper()
                os.putenv('CL_LOG_SEVERITY', log_level)
            else:
                log.critical('Invalid ASP log level [%s]' % a)
                usage()
                sys.exit(1)

def check_py_version():
    min_req_version = (2, 3, 4)

    if sys.version_info < min_req_version:
        log.critical('This script requires python version %s '\
                     'or later' % '.'.join(map(str, min_req_version)))
        log.critical('You have python version %s' %\
                     '.'.join(map(str, sys.version_info[0:3])))
        sys.exit(1)

def add_solaris_specific_path():
    #Append tipc-config path to PATH
    p = [
	 '/opt/SUNWtipc/sbin/'
        ]

    old_ld_path = os.getenv('PATH')
    if old_ld_path:
        p.insert(0, old_ld_path)

    v = ':'.join(p)

    os.putenv('PATH', v)

    #export LD_PRELOAD for tipc
    preload_32='/opt/SUNWtipc/lib/libtipcsocket.so.1'
    preload_64='/opt/SUNWtipc/lib/amd64/libtipcsocket.so.1'
    os.putenv('LD_PRELOAD_32',preload_32)
    os.putenv('LD_PRELOAD_64',preload_64)

def main():
    check_py_version()

    if len(sys.argv) < 2:
        usage()
        sys.exit(1)
    else:
        parse_command_line()

    log_asp_env()
    #pdb.set_trace()
    if not is_root():
        log.info('ASP is being run in non-root user mode.')
        sanity_check()
    #pdb.set
    asp_driver(sys.argv[1])

log = init_log()
add_solaris_specific_path()
asp_env = set_up_asp_config()

if __name__ == '__main__':
    main()
