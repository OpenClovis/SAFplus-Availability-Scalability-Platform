#!/usr/bin/python
import os, sys
import re
import pdb
import fnmatch
import errno
import types
import urllib.request, urllib.error, urllib.parse
from xml.dom import minidom
from subprocess import Popen, PIPE

# make sure they have a proper version of python
if sys.version_info[:3] < (2, 4, 3):
    print("Error: Must use Python 2.4.3 or greater for this script")
    sys.exit(1)

# ------------------------------------------------------------------------------
# Custom Imports
# ------------------------------------------------------------------------------
try:
    sys.path.append('./src/install')
    from packages import *
    from objects import *
    from common import *
except ImportError:
    print('ImportError: Source files not found or invalid.\n' \
        'Your tarball may have been damaged.\n' \
        'Please contact %s for support' % SUPPORT_EMAIL)
    sys.exit(1)

# ------------------------------------------------------------------------------
# Settings
# ------------------------------------------------------------------------------

THIRDPARTY_NAME_STARTS_WITH  = '3rdparty-base-7.0.0'                # Look for PKG starting with this name
THIRDPARTYPKG_DEFAULT        = '3rdparty-base-7.0.0.tar'            # search this package if no 3rdPartyPkg found
if determine_bit() == 64:
  THIRDPARTY_NAME_STARTS_WITH  = '3rdparty-base-7.0.0'       # Look for PKG starting with this name
  THIRDPARTYPKG_DEFAULT        = '3rdparty-base-7.0.0.tar'
SUPPORT_EMAIL                = 'support@openclovis.com'            # email for script maintainer
INSTALL_LOCKFILE             = '/tmp/.openclovis_installer'        # installer lockfile location

class ASPInstaller:
    """ Installer for OpenClovis SAFplus Availabliity Scalability Platform """
    
    def __init__(self):
        
        # ------------------------------------------------------------------------------
        # Construct local enviroment
        # ------------------------------------------------------------------------------

        self.DEBUG_ENABLED       = False  


        self.ASP_VERSION         = None
        self.ASP_REVISION        = None
        #self.INSTALL_DIR         = None
        self.OS                  = None
        self.WORKING_ROOT        = syscall('pwd')
        self.WORKING_DIR         = syscall('pwd')+'/src/3rdparty/base'        
        #self.INSTALL_DIR         = self.WORKING_DIR+'/src/3rdparty/base/'
        self.preinstallQueue     = []           # list of preinstall deps
        self.ide_preinstallQueue = []           # list of preinstall deps for ide
        self.preinstallPipQueue  = []           # list of preinstall pip deps
        self.installQueue        = []           # list of deps to indicate what needs installing
        self.locks_out           = []           # list of all locks outstanding
        self.PREINSTALL_ONLY     = False
        self.INSTALL_ONLY        = False
        self.STANDARD_ONLY       = False


        self.LOG_DIR             = os.path.join(self.WORKING_DIR, 'log')

        self.DEV_BUILD           = False
        self.INSTALL_IDE         = True
        self.GPL                 = False
        self.DO_PREBUILD         = True 
        self.DO_SYMLINKS         = True
        self.HOME                = self.expand_path('~')
        self.OFFLINE             = False
        self.INTERNET		 = True	


        self.GENERAL_LOG_PATH = os.path.join(self.LOG_DIR, 'general.log')
        self.OS = determine_os()
        
            
        self.nuke_dir(self.LOG_DIR) # clean and make log directory
        self.create_dir(self.LOG_DIR)
        
        
        self.debug('Determined OS to be: %s, %d-bit' % (self.OS.name, self.OS.bit))
        
        
        if (sys.argv[0].count('/') >= 2 or len(sys.argv[0]) > len('./install.py')):
            self.feedback('Error: Please invoke this sript from its own directory as "./install.py"', True)
        
        # look in our VERSION file to get ASP_VERSION and ASP_REVISION
        try:
            fh = open('VERSION', 'r')
            fdata = fh.readlines()
            fh.close()
            
            self.ASP_VERSION = fdata[1].split('=')[1].strip()
            self.ASP_REVISION = fdata[3].split('=')[1].strip()
            
            if not self.ASP_VERSION or not self.ASP_REVISION:
                raise Exception # Invalid VERSION file format

            if self.ASP_REVISION == 'dev':
                # we have a development install
                self.debug('Development install detected')
                self.DEV_BUILD = True
        
        except:
            msg = 'Error: \'%s\' file not found or invalid.\n' \
                    'Your tarball may have been damaged.\n' \
                    'Please contact %s for support' % (os.path.join(self.WORKING_DIR, 'VERSION'), SUPPORT_EMAIL)
            self.feedback(msg, True, False)
        
        
        # set some flags that may have been passed from command line
        #self.parse_cl_options()
        
        
        # make sure os is supported
        if not self.DEBUG_ENABLED and (not self.OS or not self.OS.supported):
            msg = 'Error: This OS is not supported by this script\n' \
                'Please contact %s for support' % SUPPORT_EMAIL
            self.feedback(msg, True, False)
            
        if self.DEBUG_ENABLED:
            self.nuke_dir(self.BUILD_DIR) # clean old builds only for debugging (otherwise lets keep them to save time if error)

    
    def __del__(self):
        # if we are dieing we want to silenty try to remove our lock
        self.remove_lock(INSTALL_LOCKFILE)   
    
    def queuePreinstall(self):
        """ Prepare preinstall phase mimicking old preinstall-*.sh scripts """
        
        assert self.OS, "Error: Script OS failure"
        
        self.debug('queuePreinstall()')
        
        #check for root user (must be root for preinstall)
        if os.getuid() != 0:
            self.feedback('\nYou must be root to run the preinstall phase', True)
        
        
        # append all pre deps to be installed
        self.preinstallQueue.extend(self.OS.pre_dep_list)
        self.ide_preinstallQueue.extend(self.OS.ide_pre_dep_list)
        self.preinstallPipQueue.extend(self.OS.pre_pip_dep_list)
        assert self.OS.pre_dep_list and self.OS.pre_pip_dep_list and self.OS.ide_pre_dep_list
        assert len(self.OS.pre_dep_list) > 0 and len(self.OS.pre_pip_dep_list) > 0 and len(self.OS.ide_pre_dep_list)>0
        
        if len(self.preinstallQueue) == 0 or not self.OS.pre_dep_list or len(self.preinstallPipQueue) == 0 or not self.OS.pre_pip_dep_list or len(self.ide_preinstallQueue) == 0 or not self.OS.ide_pre_dep_list:
            self.feedback('This feature has not yet been fully implemented', True) # fixme
    
    
    def grab_workspace_logs(self):
        """ does a find workspace/ -name '*.log' and
        copies all found logs into our log directory """
        
        cmd = "find %s -type f -name '*.log' 2>/dev/null" % os.path.join(self.WORKING_DIR, 'workspace')
        
        files = syscall(cmd)
        
        # workaround as to syscall returning different types
        # convert files to list if not
        if type(files) == type('string'):
            files = [files]
        
        
        for filepath in files:
        
            farr = filepath.split('workspace/')
            
            # ex workspace/openhpi-2.14.0/build/config.log => openhpi-2.14.0-config.log
            # ex workspace/glib-2.12.6/config.log => glib-2.12.6-config.log

            try:
                new_name = farr[1].replace('/build/', '-').replace('/', '-')
            except:
                # something went horribly wrong
                self.debug('Something went horribly wrong with grab_workspace_logs()')
                #if self.DEBUG_ENABLED:
                #    assert 0, 'fixme'
            
            new_path = os.path.join(self.LOG_DIR, new_name)
            copy_cmd = 'cp "%s" "%s"' % (filepath.rstrip(), new_path.rstrip())
            #print copy_cmd
            syscall(copy_cmd)
            
            
    def tar_log_directory(self):
    
        cmd = 'cd %s; rm -f installer-logs.tgz; tar cfz installer-logs.tgz log/ 2>/dev/null;' % self.WORKING_DIR
        syscall(cmd)
        self.feedback('Created tarball "installer-logs.tgz" located in "%s"' % self.WORKING_DIR)
        self.feedback('Please upload this tarball to %s for help with this issue' % SUPPORT_EMAIL)
            
    
    def expand_path(self, path):
        assert self.WORKING_DIR
        assert '/' in path or path == '~'
        path = os.path.expanduser(path)
        path = path.replace('./', self.WORKING_DIR + '/')
        path = path.rstrip('/') + '/'
        assert path[-1] == '/'
        assert '/' in path
        assert not '~' in path
        return path
    
    
    def launchGUI(self):
        """ Prepare install phase mimicking old install.sh script """
        
        assert self.OS, "Error: Script OS failure"
        
        #KERNEL_VERSION  = syscall('uname -r')   
        self.PACKAGE_NAME    = 'sdk-' + self.ASP_VERSION
        #self.WORKING_ROOT    = os.path.dirname(self.WORKING_DIR)
        #PROCESSOR       = syscall('uname -m')
        #INET_ACCESS     = syscall('ping -c1 -W1 google.com')
        
        # Custom openhpi pkg?
        #if '-p' in sys.argv:
        #    
        #    CUSTOM_OPENHPI = 1
        #    
        #    for i in range(len(sys.argv)):
        #        if sys.argv[i] == '-p':
        #            try:
        #                CUSTOM_OPENHPI_PKG = sys.argv[i+1].strip()
        #                break
        #            except:
        #                self.feedback('Error: Missing openhpi-package-tarball path. (-p option specified)', True)
        #    
        #    if not os.path.isfile(CUSTOM_OPENHPI_PKG):
        #        self.feedback('Error: Invalid custom openhpi-package-tarball path. Remove -p to use default.', True)
        
        
        
        # ------------------------------------------------------------------------------
        # Set install lock
        # ------------------------------------------------------------------------------
        
        success = self.get_lock(INSTALL_LOCKFILE)
        
        if not success:
            msg = 'Error: There is another OpenClovis install in progress\n' \
                'This script cannot continue\n' \
                'If you feel this is an error, please execute\n' \
                '    rm -f \'%s\'\n' \
                'Contact %s for support' % (INSTALL_LOCKFILE, SUPPORT_EMAIL)
            self.feedback(msg, True)
        
        
        # define some essential packages we most definitely need...
        # fixme, maybe we should install these if they aren't found?
        #essentials = ('perl', 'md5sum')
        
        #fatal = False
        
        #for pkg in essentials:
        #    success = syscall('which ' + pkg)
        #    if not success:
        #        self.feedback('\'%s\' missing and is necessary for installation' % pkg)
        #        fatal = True
        
        # if they were missing an essential package
        #if fatal:
        #    msg = '\nError: Cannot find the above package(s) in your $PATH environment variable\n' \
        #        '       You may need to install the above or properly set your $PATH variable\n'
        #    self.feedback(msg, True)
        
        
        # ------------------------------------------------------------------------------
        # Begin Interactive Installer
        # ------------------------------------------------------------------------------
        
        if not (self.PREINSTALL_ONLY or self.INSTALL_ONLY or self.STANDARD_ONLY):
            
                       
            self.feedback('Welcome to the OpenClovis SAFplus %s Installer\n' % self.ASP_VERSION)
            self.feedback('This program helps you to install:')
            self.feedback('    - Required 3rd-party Packages')
            self.feedback('    - The OpenClovis SAFplus Availabiltiy Scalability Platform\n')
            self.feedback('Installation Directory Prerequisites')
            self.feedback('    - At least 512MB free disk space')
            self.feedback('    - Write permission to the installation directory\n')
            self.feedback('Note: You may experience slow installation if the target installation')
            self.feedback('      directory is mounted from a remote file system (e.g., NFS).\n')
            self.feedback('Please press <enter> to continue or <ctrl-c> to quit this installer')
            
            self.get_user_feedback()
            
            # ------------------------------------------------------------------------------
            # Selection of installation type
            # ------------------------------------------------------------------------------
            
            if not self.STANDARD_ONLY:
                
                # Show this picker unless they already specified
                
                self.print_install_header()
                
                self.feedback('Select the installation opions [default is 1]:\n')
                #self.feedback('    1) Standard          -  Select all default options')
                self.feedback('    1) Continue the installation   -  install reprequisites and 3rdparty (must be root).')
                self.feedback('    2) Cancel the installation')
                
                strin = self.get_user_feedback('\nPlease choose an installation option: ')
                
                if strin == '1':
                    # default install (fast install)
                    # go_to_standard_install # fix
                    self.STANDARD_ONLY = True
                    self.debug('Standard Install Selected')
                elif strin == '2':                    
                    self.debug('User canceled the installation')
                    return
                else:
                    self.STANDARD_ONLY = True
                #elif strin == '3':
                #    self.INSTALL_ONLY = True
                #    self.debug('Install Only Selected')
                #else: # nothing or 2
                    # Custom install
                #    self.CUSTOM_ONLY = True
                #    self.debug('Custom Install Selected')
                
        # ------------------------------------------------------------------------------   
        # Begin prompting for various installation options
        # ------------------------------------------------------------------------------
        

        # could be set via CL
        #if not self.INSTALL_DIR:
        #    self.INSTALL_DIR = '/opt/clovis'
        #    if self.DEBUG_ENABLED:
        #        self.INSTALL_DIR = '/tmp/clovis'
        #else:
        #    self.debug('self.INSTALL_DIR already set (via cl) to %s' %  self.INSTALL_DIR)
        
        
        
        # ------------------------------------------------------------------------------
        # Preinstall Dependencies
        # ------------------------------------------------------------------------------
        
        if self.STANDARD_ONLY:
            # standard needs a queue here
            self.queuePreinstall()
            self.doPreinstall()
            
               
        #if not self.INSTALL_ONLY:
        #    self.doPreinstall()
        
        # we may return here because we are done if preinstall only
        #if self.PREINSTALL_ONLY:
        #    return
        
                      
        # ------------------------------------------------------------------------------
        # 3rd party package validation      
        # ------------------------------------------------------------------------------
        #WORKING_ROOT = self.WORKING_ROOT
        self.feedback('Working Directory set to [%s]' %self.WORKING_DIR)

        os.chdir ('%s' % self.WORKING_DIR)
        ThirdPartyList = fnmatch.filter(os.listdir(self.WORKING_DIR),"%s.*tar" % THIRDPARTY_NAME_STARTS_WITH)
        self.feedback('List %s' %(ThirdPartyList))
        Pkg_Found = 0   # a flag to check for 3rdPartyPkg presence
        max_ver = 0
        if len (ThirdPartyList) == 0:
           thirdPartyPkg =  THIRDPARTYPKG_DEFAULT  
        elif len (ThirdPartyList) == 1:
           Pkg_Found = 1
           thirdPartyPkg = ThirdPartyList[0]
        else:
           Pkg_Found = 1
           for ThirdParty in ThirdPartyList:
               sub_vers = re.sub("\D", "", re.sub (THIRDPARTY_NAME_STARTS_WITH, "", ThirdParty))
               if sub_vers:
                  sub_vers = int (sub_vers)
                  if sub_vers > max_ver:
                     max_ver = sub_vers
                     thirdPartyPkg = ThirdParty

        self.THIRDPARTYPKG_PATH = os.path.join(self.WORKING_DIR, thirdPartyPkg)
        #THIRDPARTYMD5 = os.path.splitext(thirdPartyPkg)[0] + '.md5'
        #THIRDPARTYMD5_PATH = os.path.join(WORKING_ROOT, THIRDPARTYMD5)
        
        # do they have the third party pkg tarball?
        if not Pkg_Found:
            self.feedback('Error: Cannot find \'%s\' in directory \'%s\'. The installation stops\n' % (thirdPartyPkg, self.WORKING_DIR),True)
            return
        
        self.feedback('tar xfm %s'  % self.THIRDPARTYPKG_PATH)
        ret = syscall('tar xfm %s' % self.THIRDPARTYPKG_PATH)
        os.system('make')
        os.system('chmod -R 757 %s/target'%self.WORKING_ROOT)
        #Build the source
        os.chdir ('%s/src' % self.WORKING_ROOT)
        os.system('make')
    
    def doPreinstall(self):
        """ 
            After queueing up preinstall phase deps, this function is used to install them 
            Returns True on success
            """
        
        if len(self.preinstallQueue) == 0 or len(self.preinstallPipQueue) == 0:
            self.debug('Warning: doPreinstall called with 0 items')
            return        
        
        assert self.OS.apt != self.OS.yum, 'install script error: cannot use apt AND yum'
        
        self.debug('doPreinstall() %d items to install' % (len(self.preinstallQueue)+len(self.preinstallPipQueue)))
        
        if self.OS.apt and not syscall('which apt-get'):
            self.feedback('Error: Could not find apt-get, cannot continue.\n\tMay be a problem with $PATH', True)
        elif self.OS.yum and not syscall('which yum'):
            self.feedback('Error: Could not find yum, cannot continue.\n\tMay be a problem with $PATH', True)
        
        self.feedback('Beginning preinstall phase... please wait. This may take up to 5 minutes.')
        
        install_str = ''
        install_lst = []        
        for dep in self.preinstallQueue:
            if type(dep.name) == type(""):  # A string means its a simple dependency
              install_str += '%s ' % dep.name
            else:
              install_lst.append(dep)      # This is a complex dependency that must be installed individually

        ide_install_str = ''
        ide_install_lst = []        
        for dep in self.ide_preinstallQueue:
            if type(dep.name) == type(""):  # A string means its a simple dependency
              ide_install_str += '%s ' % dep.name
            else:
              ide_install_lst.append(dep)      # This is a complex dependency that must be installed individually

        pip_install_str = ''
        pip_install_lst = []        
        for dep in self.preinstallPipQueue:
            if type(dep.name) == type(""):  # A string means its a simple dependency
              pip_install_str += '%s ' % dep.name
            else:
              pip_install_lst.append(dep)
        
        if self.OS.apt:
            if self.OFFLINE:
               res = syscall ("dpkg -i -R preinstall")
               print(str (res))
            
            # lets build a string in order to pass apt-get ALL our deps at once
            # this is MUCH faster than doing them one at a time although errors
            # are much harder to detect

            else:
               syscall('apt-get update;')      
               self.debug('Installing via apt-get: ' + install_str)
               (retval, result, signal, core) = system('apt-get -y --force-yes install %s' % install_str)
               self.debug("Result: %d, output: %s" % (retval, str(result)))

               if "Could not get lock" in "".join(str(result)):
                 self.feedback("Could not get the lock, is another package manager running?\n", fatal=True)
               if retval != 0:
                 self.feedback("\n\nPreinstall via apt-get was not successful.  You may need to install some of the following packages yourself.\n%s\n\nOutput of apt-get was:\n%s" % (install_str,"".join(result)), fatal=True)
               self.debug('Installing via apt-get: ' + ide_install_str)
               (retval, result, signal, core) = system('apt-get -y --force-yes install %s' % ide_install_str)
               self.debug("Result: %d, output: %s" % (retval, str(result)))

               if "Could not get lock" in "".join(str(result)):
                 self.feedback("Could not get the lock, is another package manager running?\n", fatal=True)
               if retval != 0:
                 self.feedback("\n\nPreinstall via apt-get for ide was not successful.  You may need to install some of the following packages yourself.\n%s\n\nOutput of apt-get was:\n%s" % (install_str,"".join(result)), fatal=True)

               syscall('pip install --upgrade pip;')
               self.debug('Installing via pip: ' + pip_install_str)
               (retval, result, signal, core) = system('pip install %s' % pip_install_str)
               self.debug("Result: %d, output: %s" % (retval, str(result)))
               if retval != 0:
                 self.feedback("\n\nPreinstall via pip was not successful.  You may need to install some of the following packages yourself.\n%s\n\nOutput of apt-get was:\n%s" % (pip_install_str,"".join(result)), fatal=True)

            self.feedback('Successfully installed preinstall dependencies.')
        
        
        else:
            if self.INTERNET :
                instCmd = 'yum -y install %s 2>&1'
                cmd = instCmd % install_str
                self.debug('Yum Installing: ' + cmd)
                result = syscall(cmd)            
                self.debug(str(result))         
                #yum -y update kernel            

                for dep in install_lst:
                  #pdb.set_trace()
                  if type(dep.name) is type([]):  # Any one of these to successfully install is ok
                    for name in dep.name:
                      cmd = instCmd % name
                      self.debug('Yum Installing: ' + cmd)
                      result = syscall(cmd)
                      self.debug(str(result))

                cmd = instCmd % ide_install_str
                self.debug('Yum Installing: ' + cmd)
                result = syscall(cmd)            
                self.debug(str(result)) 

                for dep in ide_install_lst:
                  #pdb.set_trace()
                  if type(dep.name) is type([]):  # Any one of these to successfully install is ok
                    for name in dep.name:
                      cmd = instCmd % name
                      self.debug('Yum Installing: ' + cmd)
                      result = syscall(cmd)
                      self.debug(str(result))

                instCmd = 'pip3 install %s 2>&1'
                cmd = instCmd % pip_install_str
                self.debug('Yum Installing: ' + cmd)
                result = syscall(cmd)            
                self.debug(str(result))         
                #yum -y update kernel            

                for dep in pip_install_lst:
                  #pdb.set_trace()
                  if type(dep.name) is type([]):  # Any one of these to successfully install is ok
                    for name in dep.name:
                      cmd = instCmd % name
                      self.debug('pip3 Installing: ' + cmd)
                      result = syscall(cmd)
                      self.debug(str(result))

                self.feedback('Successfully installed preinstall dependencies.')

            else:
                os.chdir ('%s' %(os.path.dirname(self.WORKING_DIR)))
                if not os.path.exists(PRE_INSTALL_PKG):
                    self.feedback('Error : No %s.tar.gz package found' % self.PRE_INSTALL_PKG,True)                   
                syscall('tar zxf %s' % PRE_INSTALL_PKG)
                self.feedback('install preinstall dependencies without internet')
                os.chdir(self.PRE_INSTALL_PKG)
                pre_Install_List = fnmatch.filter(os.listdir(self.PRE_INSTALL_PKG),"*.rpm")
                self.feedback('There are %d items to install' % len(pre_Install_List))
                strin=''
                for pre_Install in pre_Install_List:
                    self.feedback('install pkg :  %s' %(pre_Install))
                    #self.debug('Installing: ' + cmd)
                    result = syscall('rpm -Uvh --nodeps %s' %(pre_Install))
                    self.debug(str(result))
                self.feedback('Successfully installed preinstall dependencies.')
                if self.TIPC == False:
                    os.chdir(self.WORKING_DIR)	
                    syscall('rm -rf %s/%s'%((os.path.dirname(self.WORKING_DIR)),PRE_INSTALL_PKG_NAME))	                    
                os.chdir(self.WORKING_DIR)
        return True
      
    def debug(self, msg):
        """ Prints a debug message
            Also dumps to log file """
        assert self.GENERAL_LOG_PATH
        msg = "[DEBUG] " + msg
        if self.DEBUG_ENABLED:
            print(msg)
        try:
            fh = open(self.GENERAL_LOG_PATH, 'a')
            fh.write(msg + '\n')
            fh.close()
        except:
            if self.DEBUG_ENABLED:
                print("[DEBUG] Could not locate log path '%s'" % self.GENERAL_LOG_PATH)
    
    
    
    def exit_script(self, tarlogs=False):
        """ Exit the script with the proper exit value """
        if tarlogs:
            self.grab_workspace_logs()
            self.tar_log_directory()
        #self.cleanup()
        sys.exit(1)
        
    
    def feedback(self, msg, fatal=False, dotarlogs=True):
        """ Prints feedback to the user and
            is optionally fatal, and
            optionally tars up the log directory
            """
        
        print(msg)
        
        try:
            fh = open(self.GENERAL_LOG_PATH, 'a')
            fh.write(msg + '\n')
            fh.close()
        except:
            if self.DEBUG_ENABLED:
                print("[DEBUG] Could not locate log path '%s'" % self.GENERAL_LOG_PATH)
            pass
        
        
        if fatal:
            self.exit_script(dotarlogs)
    
    
    def create_dir(self, path):
        """ 
            Attempts to create dirs @ path,
            making parent dirs as necessary
            """
        
        if os.path.exists(path):
            return
        
        syscall('mkdir -p "%s" 2>/dev/null' % path)
        
        if not os.path.exists(path):
            if not os.access(os.path.dirname(path), os.W_OK):
                self.feedback(ERROR_WPERMISSIONS % os.path.dirname(path), True)            
            else:
                self.feedback(ERROR_MKDIR_FAIL % path, True)
        else:
            self.debug('Created dir: ' + path)
    
    
    def nuke_dir(self, path):
        """ 
            Attempts to nuke a directory
            """
        
        if not os.path.exists(path):
            return
        
        syscall('rm -rf "%s" 2>/dev/null' % path)
        
        if os.path.exists(path):
            if not os.access(os.path.dirname(path), os.W_OK):
                self.feedback(ERROR_WPERMISSIONS % os.path.dirname(path), True)            
            else:
                self.feedback(ERROR_RMDIR_FAIL % path, True)
        else:
            self.debug('Nuked dir: ' + path)
    
    
    
    def get_lock(self, lock):
        """ 
            Attempts to get a file lock
            returns True/False depending on success 
            """
        
        # already locked?
        if os.path.isfile(lock):
            return False
        
        # lock it
        try:
            fh = open(lock, 'w')
            fh.write('Locked @: ' + syscall('date'))
            fh.close()
        except:
            fh.close()
            return False
        
        self.debug('Setting lock %s' % lock)
        self.locks_out.append(lock)
        return True
    
    
    def remove_lock(self, lock):
        """ 
            Attempts to remove a lock file
            returns True if found and removed
            returns False if not found or not removed
        INSTALLIDE      = True
            """
        
        # if not even locked
        if not os.path.isfile(lock):
            return False
        
        self.debug('Removing lock %s' % lock)
        
        # remove lock
        try:
            os.remove(lock)
            self.locks_out.remove(lock)
        except:
            return False
        
        return True
    
    
    def print_install_header(self):
        """ UI header used during install """
        
        title = 'OpenClovis SAFplus %s %s Installer - %s %s-bit' % \
            (self.ASP_VERSION, self.ASP_REVISION, self.OS.name, self.OS.bit)
        
        if self.DEBUG_ENABLED:
            title += ' - DEBUG'
        
        CONST_WIDTH = max(80, len(title))
        
        
        space = ' '*round(((CONST_WIDTH-1-len(title))/2))

        os.system('clear')  # clear screen
        print(('-'*CONST_WIDTH))
        print((space + title + space))
        print(('-'*CONST_WIDTH))
    
    
    def get_user_feedback(self, prompt=''):
        """ Return information provided by a user at a given prompt
            Exit cleanly if they kill the application during this time
            """
        
        try:
            return input(prompt).strip()
        except KeyboardInterrupt:
            self.feedback(WARNING_INCOMPLETE, True, False)
    
    
    def version_compare(self, neededver, installver):
        """ 
            Returns True if 'neededver' is greater than 'installver'
            IE. a True return indicates a package needs installation
            """
        
        if not installver or installver == 'None':
            return True
        if not neededver:
            return False
        
        nv = neededver.split('.')
        iv = installver.split('.')
        
        while len(iv) < len(nv):
            iv.append(0)
        
        # do checking                                                               
        for x, y in zip(iv, nv):
            if x != y:
                return int(x) < int(y)
        
        return False



def main():
    # allocate an ASP installer object
    installer = ASPInstaller()
    installer.launchGUI()
    print("Script exited cleanly.")
    sys.exit(0)

def Test():
    main()

# Get everything started
if __name__ == '__main__':
    main()

