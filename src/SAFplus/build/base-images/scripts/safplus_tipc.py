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
import safplus
import logging
import os
import time
import signal
import errno
import re
import glob
import commands
import safplus_watchdog_start
#import pdb

log = logging

def system(cmd)   : return safplus.system(cmd)

def Popen(cmd)    : return safplus.Popen(cmd)

def getMultiLink(): return safplus.getMultiLink()

def get_safplus_node_name(): return safplus.safplus_getenv('NODENAME')

def get_safplus_tipc_netid(): return safplus.safplus_getenv('TIPC_NETID', default='undefined')

def is_system_controller(): return safplus.safplus_getenv('SYSTEM_CONTROLLER')

def is_simulation(): return bool(int(safplus.safplus_getenv('ASP_SIMULATION', default='0')))
   
def enforce_tipc_settings(): return safplus_watchdog_start.TipcSettings=='enforce'

def ignore_tipc_settings(): return safplus_watchdog_start.TipcSettings=='ignore'

def get_link_name(): return safplus.safplus_getenv('LINK_NAME', default='eth0')

def execute_shell_cmd(cmd, err_msg, fail_on_error=True):
    ret, out, sig, core = system(cmd)

    if ret and fail_on_error:
        safplus.fail_and_exit('%s : attempted: [%s], output: [%s]'
                      % (err_msg, cmd, out))
    elif ret:
        log.warning('%s : attempted: [%s], output: [%s]'
                    % (err_msg, cmd, out))

def is_root():
    uid = os.getuid()
    return uid == 0

def get_physical_slot():
    # this assumes that we can access IPMI tool and the shelf manager via it
    cmd = '%s/bladeinfo -p 2>/dev/null || echo -1' % safplus.SAFPLUS_BIN_DIR
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

def get_tipc_config_cmd():

    cmd = 'tipc-config > /dev/null 2>&1'
    tipc_config_cmd = 'tipc-config'

    ret, output, signal, core = system(cmd)
    if ret == safplus.SystemErrorNoSuchFileOrDir:
        log.debug('The tipc-config command is not in $PATH')
        tipc_config_cmd = safplus.SAFPLUS_BIN_DIR + os.sep + tipc_config_cmd
        if not os.path.exists(tipc_config_cmd):
            log.critical('The tipc-config command is not found in %s !!' % safplus.SAFPLUS_BIN_DIR)
            safplus.fail_and_exit('This indicates some serious configuration '
                         'problem while deploying SAFplus image on target.')
        else:
            return tipc_config_cmd
    else:
        return tipc_config_cmd
    
def checkTipc(): 
    p = os.path.dirname(os.path.realpath(__file__))
    p = os.path.split(p)[0]      
    filePath = p + '/etc/clTransport.xml'
    try:
      searchfile = open(filePath, "r")
    except IOError:  # If clTransport.xml does not even exist, then default to TIPC
      return True
    res=False
    override = None
    for line in searchfile:
        if ("TIPC" in line) and ("default" in line):
            res=True 
        if ("node" in line) and ("name" in line) and ("protocol" in line) and (os.getenv('NODENAME') in line): 
          if ("TIPC" in line):
            override= True
          else:
            override= False

    if not override is None:
      res = override

    searchfile.close()
    log.debug("checkTipc: %s" % str(res))
    return res

def is_tipc_build(): return bool(int(safplus.safplus_getenv('BUILD_TIPC', default='1')))

def config_tipc_module():
    if not is_tipc_build():
        log.warning('Transport protocol : UDP only')
        return
    tipc_netid = get_safplus_tipc_netid()
    node_addr = safplus.get_safplus_node_addr()
    num,link_name = getMultiLink()
    log.info('num of bearer : %d ...' %(num))
    tipcCfg = os.getenv('CL_TIPC_CFG_PARAMS')
    if tipcCfg is None: tipcCfg = ""    
    cmd = '%s -netid=%s -addr=1.1.%s %s -be=eth:%s' % (get_tipc_config_cmd(), tipc_netid, node_addr, tipcCfg, link_name[0])
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
                           (safplus.SAFPLUS_ETC_DIR, safplus.SAFPLUS_ETC_DIR)])
            
            # Try to remove the tipc module if we failed to configure tipc.
            # Otherwise it will work in the next run, but only in "local" mode.
            num,link_name = getMultiLink()    
            cmd = 'tipc-config -bd=eth:%s' %(link_name[0])
            ret, output, signal, core = system(cmd)
            system("rmmod tipc")  
            safplus.fail_and_exit(msg)

        elif 'TIPC module not installed' in output_buf:
            msg = ''.join(['Failed to configure the tipc module. ',                           
                           'The tipc kernel module is not loaded. ',                           
                           'Use \'lsmod | grep tipc\' to see that '
                           'it is not loaded.'])
            safplus.fail_and_exit(msg)

        else:
            msg1 = ''.join(['Failed to configure the tipc module. ',                           
                            'Executed \'%s\'. ' % cmd,                           
                            'Received unknown tipc-config error: %s' % output_buf])

            msg2 = '\n'.join(['Please check that: ',                             
                              '1. The tipc kernel module is loaded. '
                              '(lsmod | grep tipc)',                              
                              '2. The tipc-config command is in your $PATH.',                              
                              '3. Values for TIPC_NETID, DEFAULT_NODEADDR '
                              'and LINK_NAME are correct in %s/asp.conf.' % safplus.SAFPLUS_ETC_DIR])
            safplus.fail_and_exit(msg1 + msg2)
    for x in range(1,num) :       
        cmd = '%s -be=eth:%s' % (get_tipc_config_cmd(),link_name[x])
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
    cmd = safplus.unload_tipc_cmd()
    ret, output, signal, core = system(cmd)
    if ret:
        if 'not found' in ''.join(output):
            cmd2 = 'rmmod tipc'
            ret, output2, signal, core = system(cmd2)
            if ret:
                log.warning('Failed to remove TIPC module: attempted: %s and %s, output was: %s and %s' % (cmd, cmd2, output, output2))

def load_tipc_module():
    if not is_tipc_build():
        return
    cmd = safplus.load_tipc_cmd()
    log.warning('Executing [%s]' % cmd)
    ret, output, signal, core = system(cmd)        
    if ret:
        sandbox = safplus.sandbox_dir
        log.debug('Trying to load TIPC module from the sandbox modules directory [ %s/modules ]' % sandbox)
        cmd = 'insmod %s/modules/tipc.ko' % sandbox
        ret, output, signal, core = system(cmd)
        if ret:
            safplus.fail_and_exit('Failed to load TIPC module: attempted: %s, output was: %s' % (cmd, ''.join(output)))

def is_tipc_loaded():
    cmd = safplus.is_tipc_loaded_cmd()
    l = Popen(cmd)
    l = [e[:-1] for e in l]
    l = [e for e in l if 'grep' not in e]
    c = len(l)
    return bool(c)

def load_config_tipc_module():
    if not is_tipc_build():
        log.debug("skipping tipc: plugin not built")
        return
    logging.info("Loading TIPC")

    def is_tipc_netid_defined():
        return get_safplus_tipc_netid() != 'undefined'

    def is_tipc_configured():
        if not is_tipc_loaded():
            return False

        tipc_config_cmd = get_tipc_config_cmd()        
        bearers = Popen('%s -b' % tipc_config_cmd)
        bearers = [e[:-1] for e in bearers[1:] if e != 'No active bearers\n']
        if not bearers:
            return False

        return True

    def is_tipc_properly_configured():
        tipc_config_cmd = get_tipc_config_cmd()
        
        tipc_addr = Popen('%s -addr' % tipc_config_cmd)[0]
        tipc_addr = tipc_addr.split(':')[1].strip()[1:-1]

        if tipc_addr != '1.1.%s' % safplus.get_safplus_node_addr():
            log.debug('System configured TIPC address : %s, user configured TIPC address : %s' % (tipc_addr, '1.1.%s' % safplus.get_safplus_node_addr()))
            return False

        tipc_netid = Popen('%s -netid' % tipc_config_cmd)[0]
        tipc_netid = tipc_netid.split(':')[1].strip()

        if tipc_netid != get_safplus_tipc_netid():
            log.debug('System configured netid : %s, user configured netid : %s' % (tipc_netid, get_safplus_tipc_netid()))
            return False

        bearers = Popen('%s -b' % tipc_config_cmd)
        bearers = [e[:-1] for e in bearers[1:]]
        num,link_name= getMultiLink()
        tipc_bearer = 'eth:%s' % link_name[0]

        if tipc_bearer not in bearers:
            log.debug('Configured bearer %s not in bearer list %s' % (tipc_bearer, bearers))
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

        safplus.fail_and_exit(msg)

    def tipc_valid_state(tipc_state):
        if tipc_state == (0, 1, 0):
            msg = '\n'.join([ 'TIPC module is loaded but not configured.',                              
                              'Please configure tipc manually using tipc-config '
                              'command to proceed.',
                              
                              'If tipc-config is not in your $PATH, '
                              'you can find it in %s' % safplus.SAFPLUS_BIN_DIR])
            safplus.fail_and_exit(msg)
        elif tipc_state == (0, 1, 1):
            pass
        elif tipc_state == (1, 0, 0):
            load_tipc_module()
            config_tipc_module()
        elif tipc_state == (1, 1, 0):
            config_tipc_module()
        elif tipc_state == (1, 1, 1):
            if not is_tipc_properly_configured():  # To_Be_Done : Need to verify for enforced TIPC setting
                log.debug('TIPC configuration mismatch.')
                enforce_tipc_settings()
                log.debug('The --enforce-tipc-settings option '
                              'is set, so using configuration in '
                              '%s/asp.conf.' % safplus.SAFPLUS_ETC_DIR)
                unload_tipc_module()
                load_tipc_module()
                config_tipc_module()
                """
                elif ignore_tipc_settings():
                    log.debug('The --ignore-tipc-settings option '
                              'is set, so using system TIPC configuration...')
                else:
                    msg = '\n'.join([ 'TIPC is loaded and configured but '
                                      'the configuration does not match '
                                      'with that in %s/asp.conf' %
                                      safplus.SAFPLUS_ETC_DIR,
                                      
                                      'Please start SAFplus with either '
                                      '--enforce-tipc-settings to use '
                                      'TIPC configuration in %s/asp.conf '
                                      'or --ignore-tipc-settings to use '
                                      'system TIPC configuration '
                                      'to proceed further.' %
                                      safplus.SAFPLUS_ETC_DIR])

                    safplus.fail_and_exit(msg)
                """    

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
            safplus.fail_and_exit('SAFplus is not being run in root user mode '
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

