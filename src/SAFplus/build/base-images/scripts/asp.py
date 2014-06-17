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
import xml.dom.minidom
import glob
import commands
#import pdb

AmfName = "safplus_amf"

try:
    set
except NameError:
    import sets
    set = sets.Set

SystemErrorNoSuchFileOrDir = 127

def init_sys_asp():
    def on_platform(p):
        return p in sys.platform
    
    d  = {}

    if on_platform('linux'):
        s = __import__('asp_linux')
    elif on_platform('qnx'):
        s = __import__('asp_qnx')
    else: # ?
        fail_and_exit('AMF Watchdog Unknown platform')
        

    d['system'] = s.system
    d['Popen'] = s.Popen
    d['getMultiLink'] = s.getMultiLink
    d['get_kill_asp_cmd'] = s.get_kill_asp_cmd
    d['get_amf_watchdog_pid_cmd'] = s.get_amf_watchdog_pid_cmd
    d['get_cleanup_asp_cmd'] = s.get_cleanup_asp_cmd

    d['unload_tipc_cmd'] = s.unload_tipc_cmd
    d['load_tipc_cmd'] = s.load_tipc_cmd
    d['is_tipc_loaded_cmd'] = s.is_tipc_loaded_cmd
    d['get_amf_pid_cmd'] = s.get_amf_pid_cmd
    d['get_start_amf_watchdog_cmd'] = s.get_start_amf_watchdog_cmd
    d['shm_dir'] = s.shm_dir
    d['core_file_dir'] = s.core_file_dir
    d['core_file_regex'] = s.core_file_regex
    d['asp_shutdown_wait_timeout'] = s.asp_shutdown_wait_timeout
    d['build_tipc'] = '0'
       

    
    return d



def system(cmd): return sys_asp['system'](cmd)
def Popen(cmd): return sys_asp['Popen'](cmd)
def getMultiLink(): return sys_asp['getMultiLink']()

def proc_lock_file(cmd):
    if not is_root():
        return

    d = '/var/lock/safplus'
    if not os.path.exists(d):
        try:
            os.mkdir(d)
        except: pass

    f = 'safplus'
    asp_file = d + os.sep + f

    if cmd == 'touch':
        fn = touch_lock_file
    elif cmd == 'remove':
        fn = remove_lock_file
    else:
        assert(0)

    fn(asp_file)
    
def fail_and_exit(msg, lock_remove = True):
    log.critical(msg)
    if lock_remove:
        proc_lock_file('remove')
    sys.exit(1)

def execute_shell_cmd(cmd, err_msg, fail_on_error=True):
    ret, out, sig, core = system(cmd)

    if ret and fail_on_error:
        fail_and_exit('%s : attempted: [%s], output: [%s]'
                      % (err_msg, cmd, out))
    elif ret:
        log.warning('%s : attempted: [%s], output: [%s]'
                    % (err_msg, cmd, out))

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

def get_asp_db_dir():
    return asp_env['db_dir']

def get_asp_save_log_dir():
    return asp_env['save_log_dir']

def get_asp_normal_log_dir():
    return asp_env['normal_log_dir']

def get_asp_crash_log_dir():
    return asp_env['crash_log_dir']

def get_asp_cores_dir():
    return asp_env['core_files_dir']

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

def get_asp_valgrind_cmd():
    return asp_env['asp_valgrind_cmd']

def get_asp_cmd_marker_file():
    return asp_env['asp_cmd_marker_file']

def get_asp_ulimit_cmd():
    return asp_env['asp_ulimit_cmd']

def is_root():
    uid = os.getuid()
    return uid == 0

def is_system_controller():
    return bool(int(asp_env['system_controller']))

def is_simulation():
    return bool(int(asp_env['simulation']))

def is_valgrind_build():
    return bool(len(asp_env['asp_valgrind_cmd'].strip()))

def is_tipc_build(val=None):
    if val is None:
        val = asp_env['build_tipc']
    return bool(int(val))
    
def enforce_tipc_settings():
    return 'enforce_tipc_settings' in asp_env

def ignore_tipc_settings():
    return 'ignore_tipc_settings' in asp_env

def remove_persistent_db():
    return 'remove_persistent_db' in asp_env

def save_previous_logs():
    return bool(int(asp_env['save_prev_logs']))

def should_restart_asp():
    return bool(int(asp_env['restart_asp']))

def set_asp_env(k, v):
    asp_env[k] = v

def log_asp_env():
    log.debug('SAFplus environment information :')
    log.debug('sandbox directory : [%s]' % asp_env['sandbox_dir'])
    log.debug('Location of old logs : [%s]' % asp_env['save_log_dir'])
    log.debug('Link name : [%s]' % asp_env['link_name'])
    log.debug('TIPC build ? : %s' %bool(int(asp_env['build_tipc'])))
    if is_tipc_build():
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
        if not is_root():
            return
        fail_and_exit('Could not create file %s' % run_file)
    print >> f, '# This file is auto-generated when SAFplus is started, please do not modify'
    print >> f, 'ASP_DIR=%s' % d['sandbox_dir']
    print >> f, 'ASP_BINDIR=%s' % d['bin_dir']
    print >> f, 'ASP_APP_BINDIR=%s' % d['bin_dir']
    print >> f, 'ASP_RUNDIR=%s' % d['run_dir']
    print >> f, 'ASP_DBDIR=%s' % d['db_dir']
    print >> f, 'ASP_CONFIG=%s' % d['etc_dir']
    print >> f, 'ASP_LOGDIR=%s' % d['log_dir']
    print >> f, 'ASP_NODENAME=%s' % d['node_name']
    print >> f, 'ASP_NODEADDR=%s' % d['node_addr']
    print >> f, 'ASP_MULTINODE=%s' %d['simulation']
    print >> f, 'ASP_SIMULATION=%s' %d['simulation']
    print >> f, 'BUILD_TIPC=%s' %d['build_tipc']
    f.close()
    
def set_up_asp_config():
    def get_sandbox_dir():
        p = os.path.dirname(os.path.realpath(__file__))
        p = os.path.split(p)[0]

        return p

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
    d['build_tipc'] = asp_getenv('BUILD_TIPC', default='1')
    def set_tipc_build_env():
        path = sandbox + '/etc'  
        clTransportFile = path +'/clTransport.xml'
        if os.path.exists(clTransportFile):
            clTransport = xml.dom.minidom.parse(clTransportFile)            
            list_Node = clTransport.getElementsByTagName("type")
            d['build_tipc'] = '0'
            for node in list_Node :            
                child = node.firstChild
                defaultProtocol = child.data            
                if(defaultProtocol=='TIPC') :
                    d['build_tipc'] = '1'
        path = sandbox +'/lib'        
        if not os.path.exists(sandbox +'/lib'+'/libClTIPC.so'):
            d['build_tipc'] = '0'
        
    set_tipc_build_env()            
    class IpmiError(Exception): pass

    def get_physical_slot():
        # this assumes that we can access IPMI tool and the shelf manager via it
        cmd = '%s/bladeinfo -p 2>/dev/null || echo -1' % d['bin_dir']
        res = Popen(cmd)[0]
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

    def get_marker_file():
        return d['run_dir'] + os.sep + 'last_asp_cmd'

    def get_tipc_config_cmd():
        cmd = 'tipc-config > /dev/null 2>&1'
        tipc_config_cmd = 'tipc-config'
        ret, output, signal, core = system(cmd)
        if ret == SystemErrorNoSuchFileOrDir:
            log.debug('The tipc-config command is not in $PATH')
            tipc_config_cmd = d['bin_dir'] + os.sep + tipc_config_cmd
            if not os.path.exists(tipc_config_cmd):
                log.critical('The tipc-config command is not found in %s env !!' %
                         (d['bin_dir']))
                fail_and_exit('This indicates some serious configuration '
                         'problem while deploying SAFplus image on target.')
            else:
                return tipc_config_cmd
        else:
            return tipc_config_cmd
    
    d['status_file'] = get_status_file()
    d['asp_cmd_marker_file'] = get_marker_file()
    d['node_name'] = asp_getenv('NODENAME')


    if is_tipc_build(d['build_tipc']):        
        d['tipc_netid'] = asp_getenv('TIPC_NETID', default='undefined')
        d['tipc_config_cmd'] = get_tipc_config_cmd()

    d['link_name'] = asp_getenv('LINK_NAME', default='eth0')
    d['system_controller'] = asp_getenv('SYSTEM_CONTROLLER')
    d['simulation'] = asp_getenv('ASP_SIMULATION', default='0')
    d['asp_valgrind_cmd'] = asp_getenv('ASP_VALGRIND_CMD', default='')
    d['disable_core'] = bool(int(asp_getenv('ASP_DISABLE_CORE', default=0)))
    d['core_file_size'] = asp_getenv('ASP_CORE_FILE_SIZE', default='unlimited')

    if d['disable_core']:
        d['asp_ulimit_cmd'] = 'ulimit -c 0'
    else:
        d['asp_ulimit_cmd'] = 'ulimit -c ' + d['core_file_size']
        
    d['hpi_ip'] = asp_getenv('SAHPI_UNSPECIFIED_DOMAIN_ID', default='undefined')

    d['save_prev_logs'] = asp_getenv('ASP_SAVE_PREV_LOGS', default='0')
    if d['save_prev_logs'] is '0':
        d['core_files_dir'] = get_dir(d['save_log_dir'] + '/cores')
    else:
        d['crash_log_dir'] = get_dir(d['save_log_dir'] + '/crash')
        d['normal_log_dir'] = get_dir(d['save_log_dir'] + '/normal')

    d['save_dir_size_margin'] = int(asp_getenv('ASP_SAVE_DIR_SIZE_MARGIN',
                                               default=(5 * 1024)))
    d['save_dir_size_max_limit'] = int(asp_getenv('ASP_SAVE_DIR_SIZE_MAX_LIMIT',
                                                  default=(2 * 1024 * 1024)))
    d['restart_asp'] = asp_getenv('ASP_RESTART_ASP', default='1')
    
    os.putenv('ASP_LOGDIR', d['log_dir'])
    os.putenv('ASP_RUNDIR', d['run_dir'])
    os.putenv('ASP_DBDIR', d['db_dir'])
    os.putenv('ASP_SCRIPTDIR', d['script_dir'])
    os.putenv('ASP_CPM_CWD', d['run_dir'])

    os.putenv('ASP_DIR', d['sandbox_dir'])
    
    # This needs to be cleaned up
    os.putenv('SAHPI_UNSPECIFIED_DOMAIN_ID', d['hpi_ip'])

    os.putenv('ASP_CONFIG', d['etc_dir'])
    os.putenv('ASP_BINDIR', d['bin_dir'])
    os.putenv('ASP_MULTINODE', d['simulation'])
    os.putenv('ASP_SIMULATION', d['simulation'])
    os.putenv('BUILD_TIPC', d['build_tipc'])
    gen_asp_run_env_file(d['etc_dir']+'/asp_run.env', d)
    
    return d

    
def init_log():
    import logging

    logger = logging.getLogger('ASP')
    logger.setLevel(logging.DEBUG)

    console = logging.StreamHandler(sys.stdout)
    console.setLevel(logging.INFO)

    formatter = logging.Formatter('%(levelname)s '
                                  '%(message)s')
    console.setFormatter(formatter)

    logger.addHandler(console)
    return logger

def is_executable_file(f):
    import stat
    is_file = stat.S_ISREG(os.stat(f)[stat.ST_MODE])
    
    return is_file and os.access(f, os.R_OK | os.X_OK)

def stop_amf_watchdog():
    '''Depends on ps utility returning the first line
    to be that of the watchdog process.
    '''

    p = '%s/safplus_watchdog.py' % get_asp_etc_dir()
    cmd = sys_asp['get_amf_watchdog_pid_cmd'](p)
    result=Popen(cmd)
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

def start_amf_watchdog(stop_watchdog=True):
    if stop_watchdog == True:
        stop_amf_watchdog()
    log.info('Starting AMF watchdog...')
    cmd = sys_asp['get_start_amf_watchdog_cmd'](get_asp_etc_dir())
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
        cmd = 'ldd %s/safplus_cm | grep -c libopenhpi' %\
              get_asp_bin_dir()
        return int(Popen(cmd)[0]) > 0

    def cm_requires_openhpid():
        cmd = 'ldd %s/safplus_cm | grep -c libopenhpimarshal' %\
              get_asp_bin_dir()
        return int(Popen(cmd)[0]) > 0

    if os.getenv('SAHPI_UNSPECIFIED_DOMAIN_ID') != "UNDEFINED":
        os.putenv('OPENHPI_UID_MAP',
                  '%s/openhpi_uid.map' % get_asp_run_dir())
        os.putenv('OPENHPI_CONF',
                  '%s/openhpi.conf' % get_asp_etc_dir())

        if not os.path.exists('%s/safplus_cm' % get_asp_bin_dir()):
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
                if not os.path.exists('%s/openhpid' % get_asp_bin_dir()):
                    fail_and_exit('Need to start openhpid but it cannot '
                                  'be found in %s' % get_asp_bin_dir())
                pidfile=''
                if os.getuid()!=0 :
                    pidfile='-f %s/openhpi.pid' %get_asp_run_dir()
                cmd = '%s/openhpid -c $OPENHPI_CONF %s'  %(get_asp_bin_dir(),pidfile)
                os.system(cmd)

        cmd = '(sleep 60; setsid %s/hpiSubagent -s > '\
              '/dev/null 2>&1) &' % get_asp_bin_dir()
        os.system(cmd)

def start_amf():
    def is_amf_in_valgrind_list():
        val_filter = os.getenv("ASP_VALGRIND_FILTER")
        if val_filter:
            val_filter = val_filter.split()
            return "safplus_amf" in val_filter

        return True
        
    # chassis_id = get_asp_chassis_id()
    node_addr = get_asp_node_addr()
    node_name = get_asp_node_name()
    
    log.info('Starting AMF...')
    # chassis id is hardcoded to 0 as it is not used yet
    if is_valgrind_build() and is_amf_in_valgrind_list():
        cmd = '%s; ' % get_asp_ulimit_cmd() +\
              '%s ' % get_asp_valgrind_cmd() +\
              '--log-file=%s/%s.%d ' % (get_asp_log_dir(), AmfName,int(time.time())) +\
              '%s/%s -c 0 -l %s -n %s' %\
              (get_asp_bin_dir(), AmfName, node_addr, node_name)
    else:
        cmd = '%s; ' % get_asp_ulimit_cmd() +\
              '%s/%s -c 0 -l %s -n %s' %\
              (get_asp_bin_dir(), AmfName, node_addr, node_name)

    os.system(cmd)

def run_custom_scripts(cmd):
    d = get_asp_script_dir()
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

# Stone 9/20/2013 -- do not put conditional behavior deep within a library function.
#                    if cmd == 'start':
#                        fail_and_exit('Custom script execution failure')

def setup_gms_config():
    return

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

    snmpd_exe = get_asp_bin_dir() + os.sep + 'snmpd'
    if os.path.exists(snmpd_exe):
        set_snmp_conf_path()
        snmpd_modules_flag = os.getenv("SNMP_MODULES")
        if snmpd_modules_flag is None: snmpd_modules_flag = ""
        else: snmpd_modules_flag = "-I " + snmpd_modules_flag

        log.info('Starting SNMP daemon...')
        cmd = 'setsid %s %s -DH -Lo -f -C -c %s/etc/snmpd.conf '\
              '>/dev/null 2>&1 &' % (snmpd_exe, snmpd_modules_flag, get_asp_sandbox_dir())
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
    if not is_tipc_build():
        return

    tipc_netid = get_asp_tipc_netid()
    node_addr = get_asp_node_addr()
    num,link_name = getMultiLink()
    log.info('num of bearer : %d ...' %(num))
    tipcCfg = os.getenv('CL_TIPC_CFG_PARAMS')
    if tipcCfg is None: tipcCfg = ""    
    cmd = '%s -netid=%s -addr=1.1.%s %s -be=eth:%s' % (get_asp_tipc_config_cmd(), tipc_netid, node_addr, tipcCfg, link_name[0])
    log.debug('TIPC command is [%s]' % cmd)
    ret, output, signal, core = system(cmd)
    if ret:
        output_buf = ''.join(output)
        if 'unable to enable bearer' in output_buf:
            msg = ''.join(['Failed to configure the tipc module. ',
                           
                           'System is configured to use %s, but tipc '
                           'cannot use this interface. ' % link_name,
                           
                           'Does it exist? To change the interface, '
                           'edit the LINK_NAME and linkName fields in '
                           '%s/asp.conf and %s/clGmsConfig.xml.' %\
                           (get_asp_etc_dir(), get_asp_etc_dir())])
            
            # Try to remove the tipc module if we failed to configure
            # tipc.  Otherwise it will work in the next run, but only
            # in "local" mode.
            num,link_name = getMultiLink()    
            cmd = 'tipc-config -bd=eth:%s' %(link_name[0])
            ret, output, signal, core = system(cmd)
            system("rmmod tipc")  
            fail_and_exit(msg)

        elif 'TIPC module not installed' in output_buf:
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
    for x in range(1,num) :       
        cmd = '%s -be=eth:%s' %\
          (get_asp_tipc_config_cmd(),link_name[x])
        log.debug('enable bearer name : %s ...' %(cmd))
        ret, output, signal, core = system(cmd)        

def unload_tipc_module():
    if not is_tipc_build():
        return

    log.info('Unloading TIPC ...')
    num,link_name = getMultiLink()
    for x in range(0,num) :
        cmd = 'tipc-config -bd=eth:%s' %(link_name[x])
        log.debug('disable bearer :%s ...' %(cmd))
        ret, output, signal, core = system(cmd)
    cmd = sys_asp['unload_tipc_cmd']
    ret, output, signal, core = system(cmd)
    if ret:
        if 'not found' in ''.join(output):
            cmd2 = 'rmmod tipc'
            ret, output2, signal, core = system(cmd2)
            if ret:
                log.warning('Failed to remove TIPC module: attempted: %s and %s,'
                            ' output was: %s and %s' %\
                            (cmd, cmd2, output, output2))

def load_tipc_module():
    if not is_tipc_build():
        return

    cmd = sys_asp['load_tipc_cmd']
    ret, output, signal, core = system(cmd)        
    if ret:
        sandbox = get_asp_sandbox_dir()
        log.debug('Trying to load TIPC module from the sandbox modules '\
                  'directory [ %s/modules ]' % sandbox)
        cmd = 'insmod %s/modules/tipc.ko' % sandbox
        ret, output, signal, core = system(cmd)
        if ret:
            fail_and_exit('Failed to load TIPC module: attempted: %s, '
                          'output was: %s' % (cmd, ''.join(output)))

def load_config_tipc_module():
    if not is_tipc_build():
        return

    def is_tipc_netid_defined():
        return get_asp_tipc_netid() != 'undefined'

    def is_tipc_loaded():
        cmd = sys_asp['is_tipc_loaded_cmd']
        l = Popen(cmd)
        l = [e[:-1] for e in l]
        l = [e for e in l if 'grep' not in e]
        c = len(l)
        return bool(c)

    def is_tipc_configured():

        if not is_tipc_loaded():
            return False

        tipc_config_cmd = get_asp_tipc_config_cmd()
        
        bearers = Popen('%s -b' % tipc_config_cmd)
        bearers = [e[:-1] for e in bearers[1:] if e != 'No active bearers\n']
        if not bearers:
            return False

        return True

    def is_tipc_properly_configured():
        tipc_config_cmd = get_asp_tipc_config_cmd()
        
        tipc_addr = Popen('%s -addr' % tipc_config_cmd)[0]
        tipc_addr = tipc_addr.split(':')[1].strip()[1:-1]

        if tipc_addr != '1.1.%s' % get_asp_node_addr():
            log.debug('System configured TIPC address : %s, '
                      'user configured TIPC address : %s' %
                      (tipc_addr, '1.1.%s' % get_asp_node_addr()))
            return False

        tipc_netid = Popen('%s -netid' % tipc_config_cmd)[0]
        tipc_netid = tipc_netid.split(':')[1].strip()

        if tipc_netid != get_asp_tipc_netid():
            log.debug('System configured netid : %s, '
                      'user configured netid : %s' %
                      (tipc_netid, get_asp_tipc_netid()))
            return False

        bearers = Popen('%s -b' % tipc_config_cmd)
        bearers = [e[:-1] for e in bearers[1:]]
        num,link_name= getMultiLink()
        tipc_bearer = 'eth:%s' % link_name[0]

        if tipc_bearer not in bearers:
            log.debug('Configured bearer %s not in bearer list %s' %
                      (tipc_bearer, bearers))
            return False

        return True

    def tipc_failed_state(tipc_state):
        if tipc_state == (0, 0, 0):
            msg = '\n'.join([ 'TIPC is not configured for this SAFplus build.',

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
            unload_tipc_module()
            load_tipc_module()
            config_tipc_module()
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
                                      
                                      'Please start SAFplus with either '
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

    time.sleep(5) ## delay for possible tipc unload/reload bugs resulting in tipc split brain
    if not is_root():
        if not is_tipc_loaded():
            fail_and_exit('SAFplus is not being run in root user mode '
                          'and TIPC module is not loaded.\n'
                          'Please run SAFplus as either root '
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

def save_asp_runtime_files():

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
        loc = sys_asp['core_file_dir']
        core_regex = sys_asp['core_file_regex']

        if not loc: loc = get_asp_run_dir()

        cmd = 'find %s -name \'%s\'' % (loc, core_regex)
        r, o, s, c = system(cmd)
        if r:
            log.warning('Executing [%s] command failed, ret = %s' %\
                        (cmd, r))
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

        max_limit = get_asp_save_dir_max_limit()
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

        save_dir_margin = get_asp_save_dir_margin()
        cur_save_dir_margin = abs(dir_free_space(save_log_dir) - cur_save_dir_dir_size)

        if save_dir_margin >= cur_save_dir_margin:
            msg = 'Minimum size margin exceeded, while trying to save [%s]\n'\
                  'Minimum margin = %sKb, actual margin now = %sKb'\
                  % (d, save_dir_margin, cur_save_dir_margin)
            return (False, msg)
        else:
            return (True, '')

    def rm_runtime_files():
        p = re.compile(r'core.\d+$')
        l = glob.glob('%s/*' % get_asp_run_dir())
        l = [e for e in l if not p.search(e)]
        for e in l:
            execute_shell_cmd('rm -rf \'%s\'' % e,
                              'Failed to delete [%s]' % e)

    def save_cores():
        run_dir = get_asp_run_dir()
        cores_dir = get_asp_cores_dir()

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
            src = getattr(sys.modules[__name__], 'get_asp_%s_dir' % d)()

            if is_core_present():
                log_dir = get_asp_crash_log_dir()
            else:
                log_dir = get_asp_normal_log_dir()

            dst = log_dir + '/%s_' % d +\
                  '%s_' % get_asp_node_name() +\
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
            
    if remove_persistent_db():
        asp_db_dir = get_asp_db_dir()
        cmd = 'rm -rf %s/*' % asp_db_dir
        execute_shell_cmd(cmd, 'Failed to delete [%s]' % asp_db_dir)
        log.info('Deleted persistent files in %s directory' % asp_db_dir)

def check_if_root():
    if not is_root():
        fail_and_exit('Please run this script as a root, '
                      'you are running this script as [%s]' %
                      os.environ['USER'])

def touch_lock_file(asp_file):
    try:
        f = os.open(asp_file, os.O_CREAT | os.O_EXCL | os.O_RDWR)
    except OSError, e:
        if e.errno != errno.EEXIST:
            raise
        fail_and_exit("SAFplus instance already running. "
                      "To force startup, try removing %s lockfile at your own risk and retry." % asp_file, 
                      False)

def remove_lock_file(asp_file):
    cmd = '[ -f %s ] && rm -f %s' % (asp_file, asp_file)
    system(cmd)

def force_restart_safplus():
    save_asp_runtime_files()
    if is_tipc_build():
        load_config_tipc_module()
    set_ld_library_paths()
    if is_system_controller():
        start_snmp_daemon()
    if is_simulation():
        setup_gms_config()
    run_custom_scripts('start')
    start_amf()
    if is_system_controller() and not is_simulation():
        start_hpi_subagent()
    if not is_simulation():
        start_led_controller()


def start_asp(stop_watchdog=True, force_start = False):
    try:
        if False and not force_start:
            proc_lock_file('touch')
        check_asp_status(not force_start)
        kill_asp(False)
        cleanup_asp()
        save_asp_runtime_files()
        if is_tipc_build():
            load_config_tipc_module()
        set_ld_library_paths()
        if is_system_controller():
            start_snmp_daemon()
        if is_simulation():
            setup_gms_config()
        run_custom_scripts('start')
        start_amf()
        if is_system_controller() and not is_simulation():
            start_hpi_subagent()
        if not is_simulation():
            start_led_controller()
        start_amf_watchdog(stop_watchdog)
    except:
        raise

def start_openhpid(stop_watchdog=False):
    try:
        if not os.path.exists('%s/openhpid' % get_asp_bin_dir()):
            fail_and_exit('Need to start openhpid but it cannot be found in %s' % get_asp_bin_dir())
        else:
            cmd = '%s/openhpid -c $OPENHPI_CONF' % get_asp_bin_dir()
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
    """ attempts to get openhpid pid,
        if found, returns pid
        if not found, returns 0 """

    try:
        l = Popen('ps aux | grep -i openhpid | grep -vF "grep"')

        if l:
            # pid found
            pid = int(l[0].split()[1])
            return pid

    except:
        pass

    return 0

def get_pid_for_this_sandbox(pid):
    proc_file = '/proc/%s/cwd' % pid
    
    try:
        cwd = os.readlink(proc_file)
    except OSError, e:
        log.debug('Failed to read [%s] : %s' %\
                      (proc_file, e))
        cwd = ''

    if cwd.endswith(' (deleted)'):
        log.critical('Contents of this sandbox directory were deleted '
                     'without stopping SAFplus which was started '
                     'from it.')
        fail_and_exit('Please kill all the SAFplus processes manually '
                          'before proceeding any further.')

    return cwd == get_asp_run_dir()
    
def get_amf_pid(watchdog_pid = False):
    while True:
        valid = commands.getstatusoutput("/bin/pidof %s" % AmfName);
        if valid[0] == 0:
            if len(valid[1].split())==1:          
                return int(valid[1])
        else:
            break
        log.warning('There is more than one AMF pid. Try again...')
        time.sleep(0.25)
    if watchdog_pid:
         valid = commands.getstatusoutput("/bin/pidof safplus_watchdog.py");
         if valid[0] == 0:
            return int(valid[1])
    return 0;
    
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
        t = sys_asp['asp_shutdown_wait_timeout']
        for i in range(t/6):
            amf_pid = get_amf_pid()
            if amf_pid == 0:
                break
            time.sleep(6)

    amf_pid = get_amf_pid()
    if amf_pid == 0:
        log.warning('SAFplus is not running on node [%s]. Cleaning up anyway...' %
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
    if not is_simulation():
        time.sleep(1)
        unload_tipc_module()

def cleanup_asp():
    cmd_list = [sys_asp['get_cleanup_asp_cmd']( get_asp_node_addr())]

    for cmd in cmd_list:
        os.system(cmd)

def kill_asp(lock_remove = True):
    amf_pid = get_amf_pid() # Dummy, to guard against deletion of sandbox

    b = get_asp_bin_dir()

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

    # Insert amf binary at the top of the list for kills so there is no false recovery from amf which could also
    # result in a node failfast resulting in node reboots
    try:
        apps.remove(AmfName)
        apps.insert(0, AmfName)
    except: pass

    for f in apps:
        f = os.path.abspath(b + '/' + f)
        if is_executable_file(f):
            cmd = sys_asp['get_kill_asp_cmd'](f)
            ret = os.system(cmd)
            log.info("Killed [%s] with command [%s] -> %d" % (f,cmd, ret))
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
    if lock_remove:
        proc_lock_file('remove')

def zap_asp(lock_remove = True):
    try:
      run_custom_scripts('zap')
    except Exception, e:
      logging.critical('%s: run_custom_scripts(zap) received exception %s' % (time.strftime('%a %d %b %Y %H:%M:%S'),str(e)))
      logging.critical('traceback: %s',traceback.format_exc())

    kill_asp(lock_remove)
    if not is_simulation():
        time.sleep(2) ## delay to give time for the zapped processes to exit
        unload_tipc_module()

def restart_asp():
    pass

def start_asp_console():
    os.putenv('ASP_CPM_LOGFILE', 'console')
    start_asp()
    
def check_asp_status(watchdog_pid = False):
    v = is_asp_running(watchdog_pid)
    if v == 0:
        fail_and_exit('SAFplus is already running on node [%s], pid [%s]' % (get_asp_node_addr(), get_amf_pid(watchdog_pid)), False)
    elif v == 2:
        fail_and_exit('SAFplus is still booting up/shutting down on node [%s], '
                      'pid [%s]. '
                      'Please give \'stop\' or \'zap\' command and '
                      'then continue' %\
                      (get_asp_node_addr(), get_amf_pid(watchdog_pid)), False)
    elif v == 3:
        fail_and_exit('SAFplus is already running with pid [%s], but it was not '
                      'started from this sandbox [%s]. Please check if '
                      'SAFplus was already started from some other '
                      'sandbox directory.' %\
                      (get_amf_pid(watchdog_pid), get_asp_sandbox_dir()))

def get_asp_status(to_shell=True):
    v = is_asp_running(watchdog_pid = True)

    if v == 0:
        log.info('SAFplus is running on node [%s], pid [%s]' %\
                 (get_asp_node_addr(), get_amf_pid(True)))
    elif v == 1:
        log.info('SAFplus is not running on node [%s]' %\
                 get_asp_node_addr())
    elif v == 2:
        log.info('SAFplus is booting up/shutting down')
    elif v == 3:
        log.info('SAFplus is running with pid [%s], but it was not '
                 'started from this sandbox [%s].' %\
                 (get_amf_pid(True), get_asp_sandbox_dir()))

    # Hack, hack, hack !!!
    # I (vshenoy) don't know of any other way to propagate this
    # value back to shell
    if to_shell:
        sys.exit(v)
    else:
        return v
    
def is_asp_running(watchdog_pid = False):
    '''
    Return value meaning :
    0 -> running
    1 -> not running,
    2 -> booting/shutting down
    3 -> running without status file, this is a bug.
    '''

    amf_pid = get_amf_pid(watchdog_pid)

    if amf_pid == 0:
        return 1
    
    asp_status_file = get_asp_status_file()

    if os.path.exists(asp_status_file):
        t = int(Popen('cat %s' % asp_status_file)[0])
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
          ('--remove-persistent-db',
           'Delete all of the SAFplus persistent database files'),
          ('--asp-log-level <level>',
           'Start SAFplus with particular log level. <level> is '
           '[trace|debug|info|notice|warning|error|critical]')
        )

    for o, h in l:
        print '%-30s:  %s' % (o, h)

def create_asp_cmd_marker(cmd):
    execute_shell_cmd('echo "%s" > %s' %\
                      (cmd,get_asp_cmd_marker_file()),
                      'Failed to create [%s] marker' % cmd)

def asp_driver(cmd):

    cmd_map = {'start' : start_asp,
               'stop' : stop_asp,
               'restart': restart_asp,
               'console' : start_asp_console,
               'status' : get_asp_status,
               'zap' : zap_asp,
               'help' : usage
               }

    if cmd_map.has_key(cmd):
        if cmd in ['zap','stop']:
            create_asp_cmd_marker(cmd)
            cmd_map[cmd]()
        else:
            cmd_map[cmd]()
            create_asp_cmd_marker(cmd)
    else:
        fail_and_exit('Command [%s] not found !!' % cmd)

def sanity_check():
    def check_for_root_files(c):
        cmd = 'find %s -uid 0 -type %s' % (get_asp_sandbox_dir(), c)
        l = Popen(cmd)

        if c == 'f':
            t = 'files'
        elif c == 'd':
            t = 'directories'
        else:
            assert(0)

        if l:
            fail_and_exit('Some of the %s in the sandbox '
                          'are owned by root. \nThis indicates that '
                          'previously SAFplus may have been/is running '
                          'in root user mode. \n'
                          'Please delete those files to continue '
                          'running/querying SAFplus as normal user or '
                          'run/query SAFplus as root.' % t)

    def check_for_root_shms():
        cmd = 'find %s -uid 0 -type f -name \'CL_*\'' % sys_asp['shm_dir']
        l = Popen(cmd)

        if l:
            fail_and_exit('Some of the shared memory segments '
                          'in /dev/shm are owned by root. \n'
                          'This indicates that previously SAFplus may '
                          'have been/is running in root user mode. \n'
                          'Please delete those files to continue '
                          'running/querying SAFplus as normal user or '
                          'run/query SAFplus as root.')

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
                                    'remove-persistent-db',
                                    'asp-log-level=',
                                   ])
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
        elif o == '--remove-persistent-db':
            set_asp_env('remove_persistent_db', True)
        elif o == '--asp-log-level':
            l = ['trace', 'debug',
                 'info', 'notice',
                 'warning', 'error',
                 'critical']
            if a.lower() in l:
                log_level = a.upper()
                os.putenv('CL_LOG_SEVERITY', log_level)
            else:
                log.critical('Invalid SAFplus log level [%s]' % a)
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

def main():
    check_py_version()
    if len(sys.argv) < 2:
        usage()
        sys.exit(1)
    else:
        parse_command_line()

    log_asp_env()

    if not is_root():
        if sys.argv[1] not in [ 'status' ]:
            log.info('SAFplus is being run in non-root user mode.')
            sanity_check()

    asp_driver(sys.argv[1])

log = init_log()
sys_asp = init_sys_asp()
asp_env = set_up_asp_config()

if __name__ == '__main__':
    main()
