#!/usr/bin/python
import os, sys
import re
import pdb
import fnmatch
import errno
import types

# make sure they have a proper version of python
if sys.version_info[:3] < (2, 4, 3):
    print "Error: Must use Python 2.4.3 or greater for this script"
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
    print 'ImportError: Source files not found or invalid.\n' \
        'Your tarball may have been damaged.\n' \
        'Please contact %s for support' % SUPPORT_EMAIL
    sys.exit(1)

# ------------------------------------------------------------------------------
# Settings
# ------------------------------------------------------------------------------

THIRDPARTY_NAME_STARTS_WITH  = '3rdparty-base-1.23'                # Look for PKG starting with this name
THIRDPARTYPKG_DEFAULT        = '3rdparty-base-1.23.tar'            # search this package if no 3rdPartyPkg found
if determine_bit() == 64:
  THIRDPARTY_NAME_STARTS_WITH  = '3rdparty-base-1.23-x86_64'       # Look for PKG starting with this name
  THIRDPARTYPKG_DEFAULT        = '3rdparty-base-1.23-x86_64.tar'
SUPPORT_EMAIL                = 'support@openclovis.com'            # email for script maintainer
INSTALL_LOCKFILE             = '/tmp/.openclovis_installer'        # installer lockfile location

class ASPInstaller:
    """ Installer for OpenClovis SAFplus Availabliity Scalability Platform """
    
    def __init__(self):
        
        # ------------------------------------------------------------------------------
        # Construct local enviroment
        # ------------------------------------------------------------------------------

        self.DEBUG_ENABLED       = False  
        self.CUSTOM_OPENHPI      = False
        self.CUSTOM_OPENHPI_PKG  = None
        self.ASP_VERSION         = None
        self.ASP_REVISION        = None
        self.INSTALL_DIR         = None
        self.OS                  = None
        self.WORKING_DIR         = syscall('pwd')
        self.preinstallQueue     = []           # list of preinstall deps
        self.installQueue        = []           # list of deps to indicate what needs installing
        self.locks_out           = []           # list of all locks outstanding
        #self.THIRDPARTYPKG_PATH  = ''
        #self.PREFIX              = ''
        #self.BUILDTOOLS          = ''
        #self.INSTALL_DIR         = None
        self.PREINSTALL_ONLY     = False
        self.INSTALL_ONLY        = False
        self.STANDARD_ONLY       = False
        self.CUSTOM_ONLY         = False
        self.BUILD_DIR           = os.path.join(self.WORKING_DIR, 'workspace')
        self.LOG_DIR             = os.path.join(self.WORKING_DIR, 'log')
        #self.PREFIX_BIN          = ''
        self.DEV_BUILD           = False
        self.INSTALL_IDE         = True
        self.GPL                 = False
        self.DO_PREBUILD         = True 
        self.DO_SYMLINKS         = True
        self.HOME                = self.expand_path('~')
        self.OFFLINE             = False

        # ------------------------------------------------------------------------------
        self.NEED_TIPC_CONFIG    = False
        self.TIPC_CONFIG_VERSION = ''
        # ------------------------------------------------------------------------------
        
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
        self.parse_cl_options()
        
        
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

    
    def parse_cl_options(self):
        
        if '-h' in sys.argv or '--help' in sys.argv:
            self.usage()
            sys.exit(0)
        
        # check for debug flag
        if '--debug' in sys.argv:
            sys.argv.remove('--debug')
            self.DEBUG_ENABLED = True
        
        if '--custom' in sys.argv:
            sys.argv.remove('--custom')
            self.CUSTOM_ONLY = True
        
        if '--preinstall' in sys.argv:
            if os.getuid() != 0:
                self.feedback('\nYou must be root to run the preinstall phase', True)
            sys.argv.remove('--preinstall')
            self.PREINSTALL_ONLY = True

        if '--offline' in sys.argv:
            if os.getuid() != 0:
                self.feedback('\nYou must be root to run the preinstall phase', True)
            sys.argv.remove('--offline')
            self.PREINSTALL_ONLY = True
            self.OFFLINE = True
        
        if '--install' in sys.argv:
            sys.argv.remove('--install')
            self.INSTALL_ONLY = True
            
        
        if '--standard' in sys.argv:
            sys.argv.remove('--standard')
            self.STANDARD_ONLY = True
        
        if '--install-dir' in sys.argv:
            idx = sys.argv.index('--install-dir')
            try:
                # trailing slash is added
                self.INSTALL_DIR = self.expand_path(sys.argv[idx+1])
                self.debug('self.INSTALL_DIR set to: %s' % self.INSTALL_DIR)
                if not '/' in self.INSTALL_DIR:
                    raise IndexError
                sys.argv.pop(idx+1) # remove these args
                sys.argv.pop(idx)
            except IndexError:
                self.feedback('Error: Invalid options sent to installer\n       --install-dir must be followed by a valid path', True)
            
            # make sure we have trailing slash for consistency
            assert self.INSTALL_DIR[-1] == '/'
        
        
        if len(sys.argv) > 1:
            self.print_install_header()
            self.feedback('Warning: Unrecognized options sent to installer')
            for arg in sys.argv[1:]:
                print '\t' + arg.strip()
            self.feedback('Please press <enter> to continue or <ctrl-c> to quit this installer')
            self.get_user_feedback()
    
    
    def queuePreinstall(self):
        """ Prepare preinstall phase mimicking old preinstall-*.sh scripts """
        
        assert self.OS, "Error: Script OS failure"
        
        self.debug('queuePreinstall()')
        
        # check for root user (must be root for preinstall)
        if os.getuid() != 0:
            self.feedback('\nYou must be root to run the preinstall phase', True)
        
        
        # append all pre deps to be installed
        self.preinstallQueue.extend(self.OS.pre_dep_list)
        assert self.OS.pre_dep_list
        assert len(self.OS.pre_dep_list) > 0
        
        if len(self.preinstallQueue) == 0 or not self.OS.pre_dep_list:
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
            
    
    def usage(self):
        msg = '\ninstall.py - Installation tool for OpenClovis SAFplus %s\n\n' \
            'Installs OpenClovis SAFplus %s to a system intended for use\n' \
            'for OpenClovis SAFplus development.\n\n' \
            'Usage:\n' \
            '    %s [ --help ]                         # Prints this information\n' \
            '    %s [ --preinstall ]                   # Sets the script to run the preinstall phase only\n' \
            '    %s [ --install ]                      # Sets the script to run the install phase only\n' \
            '    %s [ --standard ]                     # Sets the script to do a standard install (both phases)\n' \
            '    %s [ --custom ]                       # Sets the script to do a custom install (ask user everything, Default)\n' \
            '    %s [ --install-dir /opt/clovis ]      # Sets the install directory (Default: /opt/clovis)\n' \
            % (self.ASP_VERSION, self.ASP_VERSION, sys.argv[0], sys.argv[0], sys.argv[0], sys.argv[0], sys.argv[0], sys.argv[0])
#            '    %s [ -p <openhpi-package-tarball> ]   # Allows use of specified OpenHPI package\n' \

        
        self.feedback(msg, False)
    
    
    def queueInstall(self):
        # ------------------------------------------------------------------------------   
        # Determine which packages this user needs
        # ------------------------------------------------------------------------------   
          
        # for each dep this OS requires,
        for dep in self.OS.dep_list:
            
            if self.DEBUG_ENABLED:
                assert dep.pkg_name, "Error: Dependency is missing pkg_name"
                assert dep.version, "Error: Dependency is missing version"
                assert dep.name, "Error: Dependency is missing name"
                assert dep.build_cmds, "Error: Dependency is missing build_cmds"
            
            
            if dep.force_install:
                self.installQueue.append(dep)
                continue
            
            # do we need to install this?
            if dep.ver_test_cmd:
                if '${' in dep.ver_test_cmd:
                    assert self.PREFIX
                    dep.ver_test_cmd = dep.ver_test_cmd.replace('${PREFIX}', self.PREFIX)
                                    
                dep.installedver = syscall(dep.ver_test_cmd)
                
                if not dep.installedver:
                    dep.installedver = 'None'
                
                assert type(dep.installedver) == type('string'), dep.installedver
                
                if self.version_compare(dep.version, dep.installedver):
                    # if so, add it to list to be installed
                    self.installQueue.append(dep)
                    
            else:
                # special cases, no ver_test_cmd!
                dep.installedver = 'N/A'
                
                if dep.name == 'openhpi-subagent':
                    # openhpi-subagent ONLY gets installed if
                    # either openhpi or netsnmp will be installed                    
                    
                    if os.path.isfile('/usr/local/bin/hpiSubagent') or os.path.isfile('/usr/bin/hpiSubagent') or os.path.isfile('%s/hpiSubagent' % self.PREFIX_BIN):
                        # already installed somewhere
                        # do we need reinstallation?
                        for qdep in self.installQueue:
                            if qdep.name in ('net-snmp', 'openhpi'):
                            
#                                if not os.path.isfile('/usr/lib/libelf.so'):
#                                     cmd = 'ln -fs /usr/lib/libelf.so.1 /usr/lib/libelf.so'
#                                     self.debug('calling cmd: ' + cmd)
#                                     ret_code = cli_cmd(cmd)
                   
                            
                                self.installQueue.append(dep)
                                continue
                    else:
                        #do install
                        self.installQueue.append(dep)
                        continue
                     
                           
                # SPECIAL CASE, TIPC
                elif dep.name == 'tipc':
                    cmd = '/sbin/modinfo tipc > /dev/null 2>&1;'

                    self.debug('calling cmd: ' + cmd)
                    ret_code = cli_cmd(cmd)
                        
                    if ret_code == 1:
                        
                        # fixme (try/except)
                        if int(syscall('uname -r').split('.')[2]) < 15:
                            # do install
                            dep.installedver = 'None'
                            self.installQueue.append(dep)
                        else:
                            
                            msg = '[ERROR] No TIPC module was found on this system, nor can one be installed\n' \
                                  '        by this installer.  Please recompile your kernel with TIPC support\n' \
                                  '        enabled as a kernel module and run this program again.\n' \
                                  'Please contact %s for support\n' % SUPPORT_EMAIL
                            self.feedback(msg, True, False)
                        
                    else: #ret_code == 0
                        assert ret_code == 0
                        self.NEED_TIPC_CONFIG = True
                        
                        
                        dep.installedver = syscall('/sbin/modinfo tipc | grep \'^version\' | tr -s " " | cut -d\  -f 2') # fixme, does this work

                    
                elif dep.name == 'tipc-config' and self.NEED_TIPC_CONFIG:
                    cmd = 'which tipc-config 2>/dev/null'
                    self.debug('calling cmd: ' + cmd)
                    ret_code = cli_cmd(cmd)
                    
                    if ret_code != 0:
                        # install tipc_config
                        TIPC_MODULE_VERSION = syscall('/sbin/modinfo tipc | grep \'^version\' | tr -s " " | cut -d\  -f 2')
                        TIPC_MAJOR_VERSION = int(TIPC_MODULE_VERSION.split('.')[0])
                        TIPC_MINOR_VERSION = int(TIPC_MODULE_VERSION.split('.')[1])
                        
                        if TIPC_MAJOR_VERSION != 1:
                            # dont install
                            dep.installedver = 'Warning: incompatible version, won\'t install'
                            continue
                            
                        if TIPC_MINOR_VERSION == 5:
                            dep.installedver = 'Installed'
                            continue
                            
                        elif TIPC_MINOR_VERSION == 6:
                            self.TIPC_CONFIG_VERSION = 'tipcutils-1.0.4.tar.gz'                            
                            dep.pkg_name =  self.TIPC_CONFIG_VERSION
                        elif TIPC_MINOR_VERSION == 7:
                            self.TIPC_CONFIG_VERSION = 'tipcutils-1.1.5.tar.gz'
                            dep.pkg_name =  self.TIPC_CONFIG_VERSION
                        
                        self.installQueue.append(dep)
                        continue

                    
                    else: #ret_code non zero
                        # tipc config installed
                        dep.installedver = 'Installed'
                        continue
                        
                else:
                    # just add it to queue
                    self.installQueue.append(dep)
                    continue
                       
                #else:
                continue
                  
    
    
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
        self.WORKING_ROOT    = os.path.dirname(self.WORKING_DIR)
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
        essentials = ('perl', 'md5sum')
        
        fatal = False
        
        for pkg in essentials:
            success = syscall('which ' + pkg)
            if not success:
                self.feedback('\'%s\' missing and is necessary for installation' % pkg)
                fatal = True
        
        # if they were missing an essential package
        if fatal:
            msg = '\nError: Cannot find the above package(s) in your $PATH environment variable\n' \
                '       You may need to install the above or properly set your $PATH variable\n'
            self.feedback(msg, True)
        
        
        # ------------------------------------------------------------------------------
        # Begin Interactive Installer
        # ------------------------------------------------------------------------------
        
        if not (self.PREINSTALL_ONLY or self.INSTALL_ONLY or self.STANDARD_ONLY):
            
            self.print_install_header()
            
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
            
            if not (self.STANDARD_ONLY or self.CUSTOM_ONLY):
                
                # Show this picker unless they already specified
                
                self.print_install_header()
                
                self.feedback('Installation Type:\n')
                self.feedback('    1) Standard          -  Select all default options')
                self.feedback('    2) Custom            -  Recommended')
                self.feedback('    3) Preinstall Only   -  Only does the preinstall phase')
                self.feedback('    4) Install Only      -  Only does the install phase')
                
                strin = self.get_user_feedback('\nPlease choose an installation option [default: 2]: ')
                
                if strin == '1':
                    # default install (fast install)
                    # go_to_standard_install # fix
                    self.STANDARD_ONLY = True
                    self.debug('Standard Install Selected')
                elif strin == '3':
                    self.PREINSTALL_ONLY = True
                    self.debug('Preinstall Only Selected')
                elif strin == '4':
                    self.INSTALL_ONLY = True
                    self.debug('Install Only Selected')
                else: # nothing or 2
                    # Custom install
                    self.CUSTOM_ONLY = True
                    self.debug('Custom Install Selected')
                
        # ------------------------------------------------------------------------------   
        # Begin prompting for various installation options
        # ------------------------------------------------------------------------------
        

        # could be set via CL
        if not self.INSTALL_DIR:
            self.INSTALL_DIR = '/opt/clovis'
            if self.DEBUG_ENABLED:
                self.INSTALL_DIR = '/tmp/clovis'
        else:
            self.debug('self.INSTALL_DIR already set (via cl) to %s' %  self.INSTALL_DIR)
        
        if not (self.PREINSTALL_ONLY or self.STANDARD_ONLY):
        
            self.print_install_header()
                
            strin = self.get_user_feedback('Enter the installation root directory [default: %s]: ' % self.INSTALL_DIR)
            
            if strin:
                # they provided a path. expand '~' and './' references
                self.INSTALL_DIR = self.expand_path(strin)
            else:
                # accept default
                pass

            if not os.access(self.WORKING_DIR, os.W_OK):
                msg =  ERROR_WPERMISSIONS % self.WORKING_DIR
                msg += 'Copy the contents of this folder to a directory where you have write permission and try again.'
                self.feedback(msg, True)

        
        if not self.PREINSTALL_ONLY:
            self.BUILDTOOLS      = os.path.join(self.INSTALL_DIR, 'buildtools')
            self.PACKAGE_ROOT    = os.path.join(self.INSTALL_DIR, self.PACKAGE_NAME)
            self.ASP_PREBUILD_DIR= os.path.join(self.PACKAGE_ROOT, 'prebuild')
            self.PREFIX          = os.path.join(self.BUILDTOOLS, 'local')
            self.PREFIX_BIN      = os.path.join(self.PREFIX, 'bin')
            self.PREFIX_LIB      = os.path.join(self.PREFIX, 'lib')
            self.IDE_ROOT        = os.path.join(self.PACKAGE_ROOT, 'IDE')        # IDE is installed here
            self.DOC_ROOT        = os.path.join(self.PACKAGE_ROOT, 'doc')        # DOCS are copied here
            self.BIN_ROOT        = os.path.join(self.PACKAGE_ROOT, 'bin')        # scripts are copied here    
            self.LIB_ROOT        = os.path.join(self.PACKAGE_ROOT, 'lib')        # copy rc scripts in this directory
            self.ASP_ROOT        = os.path.join(self.PACKAGE_ROOT, 'src/SAFplus')    # ASP sources are copied here
            self.MODULES         = os.path.join(self.PREFIX, 'modules')
            self.ECLIPSE         = os.path.join(self.PACKAGE_ROOT, 'eclipse')
            self.ECLIPSE_ROOT    = os.path.join(self.PREFIX, 'eclipse')
            self.ESC_ECLIPSE_DIR = syscall("echo %s/eclipse | sed -e 's;/;\\\/;g'" % self.PACKAGE_ROOT)
 
            # check for GPL
            if self.INSTALL_IDE and os.path.isdir('src/IDE'):
                self.GPL = True

            # setup self.NET_SNMP_CONFIG
            NET_CONFIG = os.path.join(self.PREFIX_BIN, 'net-snmp-config')
            self.NET_SNMP_CONFIG = 'net-snmp-config'
            if os.path.isdir(self.PREFIX) and os.path.isfile(NET_CONFIG):
                self.NET_SNMP_CONFIG = NET_CONFIG
            
            #self.CACHE_DIR = "${HOME}/.clovis/$PACKAGE_NAME"
            self.CACHE_DIR = self.join_list_as_path([self.HOME, '.clovis', self.PACKAGE_NAME])
            self.CACHE_FILE = "install.cache"

            # make all these dirs
            for DIR in (self.INSTALL_DIR, self.BUILDTOOLS, self.PACKAGE_ROOT, self.PREFIX, self.PREFIX_BIN, self.PREFIX_LIB, self.IDE_ROOT, 
                        self.ASP_ROOT, self.DOC_ROOT, self.BIN_ROOT, self.LIB_ROOT, self.BUILD_DIR, self.MODULES, 
                        self.CACHE_DIR, self.ECLIPSE, self.ECLIPSE_ROOT):
                
                # attempt to create each of these dirs
                self.create_dir(DIR)

                

            if not os.access(self.INSTALL_DIR, os.W_OK):
                self.feedback(ERROR_WPERMISSIONS % self.INSTALL_DIR)
                self.feedback(ERROR_CANT_CONTINUE, True)

        
        # ------------------------------------------------------------------------------
        # Preinstall Dependencies
        # ------------------------------------------------------------------------------
        
        if (self.CUSTOM_ONLY):
            
            self.print_install_header()
            self.queuePreinstall()
            
            self.feedback('Here are the preinstall dependencies:\n')
            
            depnames = []
            
            for dep in self.preinstallQueue:
                depnames.append(dep.name)
            
            # this is used to sort our word list
            # by length of word so that we can
            # print the dep list in a nicer way
            def bylength(word1, word2):
                return len(word1) - len(word2)
            
            depnames.sort(cmp=bylength)
            
            #self.debug(str(depnames))
            
            # this may look odd but it is simply
            # printing the dep names in a nice way
            while len(depnames):
                print depnames[0],
                depnames.pop(0) # O(n)
                if len(depnames):
                    print ' ' + depnames[-1]
                    depnames.pop() # removes last item
            
            print '\n\n',
            
            cmd = 'apt-get install'
            if self.OS.yum:
                cmd = 'yum install'
            
            
            self.feedback('These will be installed via the "%s" command. Some of these may already be installed.\n' % cmd)
            self.feedback('Please press <enter> to continue or <ctrl-c> to quit this installer')
            self.get_user_feedback()
        
        elif self.PREINSTALL_ONLY or self.STANDARD_ONLY:
            # standard needs a queue here
            self.queuePreinstall()
        
        
        if not self.INSTALL_ONLY:
            self.doPreinstall()
        
        # we may return here because we are done if preinstall only
        if self.PREINSTALL_ONLY:
            return
        
        # ------------------------------------------------------------------------------
        # Install Dependencies
        # ------------------------------------------------------------------------------
        
        try:
            longest = len(self.OS.dep_list[0].name)
        except IndexError:
            msg = 'Error: OS dependency list empty.\n' \
                'Your tarball may have been damaged.\n' \
                'Please contact %s for support\n' % SUPPORT_EMAIL
            self.feedback(msg, True)
        
        self.queueInstall()
        
        if (self.CUSTOM_ONLY):
            
            self.print_install_header()
            
            self.feedback('Here are the install dependencies for %s %d-bit:\n' % (self.OS.name, self.OS.bit))            
            
            for dep in self.OS.dep_list:
                l = len(dep.name)
                if l > longest:
                    longest = l
            
            
            head = 'Module' + ' '*(8+longest-6) + 'Needed     (Installed)'
            
            self.feedback(head)
            self.feedback('-'*len(head))
            
            for dep in self.OS.dep_list:
                if dep in self.installQueue:
                    self.feedback(dep.name + ' '*(8+longest - len(dep.name)) + dep.version + '*' + ' '*(10-len(dep.version)) + '(%s)' % dep.installedver.strip())
                else:
                    self.feedback(dep.name + ' '*(8+longest - len(dep.name)) + dep.version + ' '*(10-len(dep.version)) + ' (%s)' % dep.installedver.strip())
            
            
            self.feedback('\n(* needs install)')
            self.feedback('\nPlease press <enter> to continue or <ctrl-c> to quit this installer')
            self.get_user_feedback()
        
        
        # ------------------------------------------------------------------------------
        # 3rd party package validation      
        # ------------------------------------------------------------------------------
        WORKING_ROOT = self.WORKING_ROOT
        self.feedback('Working root set to [%s], package root at [%s]' %(WORKING_ROOT, self.PACKAGE_ROOT) )

        os.chdir ('%s' % WORKING_ROOT)
        ThirdPartyList = fnmatch.filter(os.listdir(WORKING_ROOT),"%s.*tar" % THIRDPARTY_NAME_STARTS_WITH)

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

        self.THIRDPARTYPKG_PATH = os.path.join(WORKING_ROOT, thirdPartyPkg)
        THIRDPARTYMD5 = os.path.splitext(thirdPartyPkg)[0] + '.md5'
        THIRDPARTYMD5_PATH = os.path.join(WORKING_ROOT, THIRDPARTYMD5)
        
        # do they have the third party pkg tarball?
        if not Pkg_Found:
            self.print_install_header()
            
            self.feedback('Error: Cannot find \'%s\' in directory \'%s\'\n' % (thirdPartyPkg, WORKING_ROOT))
            
            #THIRDPARTYPKG_FTPURL = os.path.join('ftp://ftp.openclovis.com/pub/release/', thirdPartyPkg)        
            #THIRDPARTYMD5_FTPURL = os.path.join('ftp://ftp.openclovis.com/pub/release/', THIRDPARTYMD5)        
            THIRDPARTYPKG_FTPURL = os.path.join("https://github.com/downloads/OpenClovis/SAFplus-Availability-Scalability-Platform/", thirdPartyPkg) 
            # attempt to download the package. Requires wget
            
            cmd = ''
            if syscall('which wget 2>/dev/null'):
                cmd = 'wget -t 3 --no-check-certificate '
            elif syscall('which curl 2>/dev/null'):
                cmd = 'curl -OL '
            
            
            if cmd:
                strin = self.get_user_feedback('Would you like to attempt download of this necessary package? <y|n> [y]: ')
                
                if not strin or strin.lower().startswith('y'):
                    
                    if not os.access(WORKING_ROOT, os.W_OK):
                        self.feedback(ERROR_WPERMISSIONS % WORKING_ROOT)
                        self.feedback('Error: Script could not continue', True)    
                    
                    try:
                        # cleanup inturrupted files
                        if os.path.exists(thirdPartyPkg):
                            os.remove(thirdPartyPkg)
                        syscall('%s "%s"' % (cmd, THIRDPARTYPKG_FTPURL))
                        syscall('mv "%s" "%s" 2>/dev/null' % (thirdPartyPkg, self.THIRDPARTYPKG_PATH))
                    except KeyboardInterrupt:
                        try:
                            # cleanup inturrupted files
                            os.remove(thirdPartyPkg)
                            os.remove(self.THIRDPARTYPKG_PATH)
                        except:
                            pass
                        self.feedback(WARNING_INCOMPLETE, True)
            
            # did it download successfully?
            if not os.path.exists(self.THIRDPARTYPKG_PATH):
                msg  = 'Error: You need %s to continue, script will now exit\n' \
                    '       Please check \'%s\' for this necessary package,\n' \
                    '       and place it in \'%s\'' % (thirdPartyPkg, THIRDPARTYPKG_FTPURL, WORKING_ROOT)
                self.feedback(msg, True)
            
            if not os.access(self.THIRDPARTYPKG_PATH, os.R_OK):
                self.print_install_header()
                msg  = 'Error: Invalid permission (cannot read from) \'%s\'\n' \
                    'Please set the proper permissions so that this script may continue\n' % self.THIRDPARTYPKG_PATH
                self.feedback(msg, True)
            
            # should preform MD5 validation?
            if os.path.exists(THIRDPARTYMD5_PATH):
                self.feedback('Please wait, performing checksum validation...')
                try:
                    # is there a better way to do this?
                    md5sum1 = syscall('md5sum ' + self.THIRDPARTYPKG_PATH).strip(' ')[0]
                    md5sum2 = syscall('cat ' + THIRDPARTYMD5_PATH).strip(' ')[0]
                    
                    if md5sum1 == md5sum2:
                        self.debug('Checksum validation passed')
                    else:
                        raise Exception
                except:
                    msg =  'Error: \'%s\' checksum validation failed.\n' % thirdPartyPkg
                    msg += 'Your tarball may have been damaged.\n'
                    msg += 'Please contact %s for support' % SUPPORT_EMAIL
                    self.feedback(msg, True)
            else:
                self.feedback('Warning: Unable to find \'%s\'. Proceeding without checksum validation...' % THIRDPARTYMD5_PATH)
        
        # doInstallation
        self.doInstallation()
    
    
    def doPreinstall(self):
        """ 
            After queueing up preinstall phase deps, this function is used to install them 
            Returns True on success
            """
        
        if len(self.preinstallQueue) == 0:
            self.debug('Warning: doPreinstall called with 0 items')
            return
        
        assert self.OS.apt != self.OS.yum, 'install script error: cannot use apt AND yum'
        
        self.debug('doPreinstall() %d items to install' % len(self.preinstallQueue))
        
        if self.OS.apt and not syscall('which apt-get'):
            self.feedback('Error: Could not find apt-get, cannot continue.\n\tMay be a problem with $PATH', True)
        elif self.OS.yum and not syscall('which yum'):
            self.feedback('Error: Could not find yum, cannot continue.\n\tMay be a problem with $PATH', True)
        
        self.feedback('Beginning preinstall phase... please wait. This may take up to 5 minutes.')
        
        install_str = ''
        for dep in self.preinstallQueue:
            install_str += '%s ' % dep.name
        
        if self.OS.apt:
            if self.OFFLINE:
               res = syscall ("dpkg -i -R preinstall")
               print str (res)
            
            # lets build a string in order to pass apt-get ALL our deps at once
            # this is MUCH faster than doing them one at a time although errors
            # are much harder to detect

            else:
               syscall('apt-get update;')
            
               self.debug('Apt-Get Installing: ' + install_str)
               result = syscall('apt-get -y --force-yes install %s' % install_str)
            
               self.debug(str(result))
            self.feedback('Successfully installed preinstall dependencies.')
        
        
        else:
            cmd = 'yum -y install %s 2>&1' % install_str
            self.debug('Yum Installing: ' + cmd)
            result = syscall(cmd)            
            self.debug(str(result))
            
            #yum -y update kernel
            
            self.feedback('Successfully installed preinstall dependencies.')
        
        
        return True                
    
    
    
    def doInstallation(self):
        """ 
            After queueing up install phase deps, this function is used to install them 
            """
        
        self.debug('doInstallation() %d items to install' % len(self.installQueue))
        
        for dep in self.installQueue:
            
            self.feedback('Beginning configure, build, and install of: %s %s' % (dep.name, dep.version))

            if not dep.extract_install:
                os.chdir(self.BUILD_DIR)                                            # move into build dir
                
                syscall('tar xfm "%s" %s' % (self.THIRDPARTYPKG_PATH, dep.pkg_name))    # pull out of pkg
                syscall('tar zxf %s' % dep.pkg_name)                                    # extract
                syscall('rm -f %s' % dep.pkg_name)                                      # remove .gz
                
                exdir = dep.pkg_name.replace('.tar.gz', '').replace('.tgz', '')
            
                os.chdir(exdir)                                                         # move into extracted folder

                # if the dep wants to do configure in a /build/ directory
                if dep.use_build_dir:
                    self.create_dir('build')                                            # create a build dir
                    os.chdir('build')                                                   # move into build dir

            # For some reason these build commands had to be deferred (they may rely on previously build stuff, or preinstall)
            if type(dep.build_cmds) == types.FunctionType:
                dep.build_cmds = dep.build_cmds()
                
            # execute commands to build package
            for cmd in dep.build_cmds:
                
                cmd = self.parse_unix_vars(cmd)               
                cmd = cmd.replace('\n', ' ')
                
                self.debug('calling cmd: ' + cmd)
                ret_code = cli_cmd(cmd)
                if (ret_code != 0):
                    self.feedback("[FATAL] Got bad retcode (%d) from command '%s'" % (ret_code, cmd), True)
                self.debug('got ret code: ' + str(ret_code))
    
            self.feedback('%s %s was installed successfully' % (dep.name, dep.version))

        # everything installed successfully, finishup
        self.debug('Install phase complete')
        self.postInstallation()

        self.feedback("=======================================================================")
        self.feedback("Installation of ${blue_bold}OpenClovis SDK$blue is now complete")
        self.feedback("")
        self.feedback("Next steps:")
        self.feedback(" -  Run 'cl-create-project-area' to create a new project area and start")
        self.feedback("    working with the SDK")
        self.feedback(" -  Run 'cl-ide' to start the OpenClovis IDE")
        self.feedback(" -  To read more information, please consult with the user documentation")
        self.feedback("    installed under %s/doc." % self.PACKAGE_ROOT)
        self.feedback("=======================================================================")
        self.feedback("")



    def postInstallation(self):
        """ sets up everyone once install is done """
        self.debug('Starting postInstallation()')

        # install crossbuild
        # prebuild

        assert self.PACKAGE_ROOT

        if os.path.isdir(self.PACKAGE_ROOT):
            self.feedback('OpenClovis is already installed. Overwrite?')
            self.feedback('Responding with \'no\' will leave the existing SDK intact and proceed to the')

            strin = self.get_user_feedback('installation of other utilities.  Overwrite existing SDK? <y|n> [n]: ')
                
            if not strin or strin.lower().startswith('n'):
                pass
                # no, do not overwrite
                self.DO_PREBUILD = False

            else:
                # yes
                self.feedback('Cleaning up...')

                ret = cli_cmd('rm -rf "%s" >/dev/null 2>&1' % self.PACKAGE_ROOT)
                if ret != 0:
                    self.feedback('[ERROR] cannot remove "%s"' % self.PACKAGE_ROOT, True)

                ret = cli_cmd('mkdir -p %s' % self.PACKAGE_ROOT)
                if ret != 0:
                    self.feedback('[ERROR] failed to create $PACKAGE_ROOT directory' % self.PACKAGE_ROOT, True)

                if not os.path.isdir(self.PACKAGE_ROOT):
                    self.feedback('[ERROR] failed to create "%s"' % self.PACKAGE_ROOT, True)
                ret = cli_cmd('rm -rf "%s" >/dev/null 2>&1' % self.join_list_as_path([self.ECLIPSE_ROOT, 'plugins', 'com.clovis.*']))
                if ret != 0:
                    self.feedback('[ERROR] failed to remove clovis plugins from %s/plugins' % self.ECLIPSE_ROOT, True)

                self.feedback('Done cleaning.')
                
                self.install_ASP()
        else:
            # PACKAGE_ROOT not created
            ret = cli_cmd('mkdir -p %s' % self.PACKAGE_ROOT)
            if ret != 0:
                self.feedback('[ERROR] failed to create $PACKAGE_ROOT directory' % self.PACKAGE_ROOT, True)


        self.install_utilities()

        if self.DO_PREBUILD:
            self.prebuild()

        if self.DO_SYMLINKS:
            self.do_symlinks()
  

    def install_ASP(self):
        self.feedback('Installing SAFplus...')

        cmds = ['cd $WORKING_DIR',
                'tar cf - src/SAFplus src/ASP src/examples |( cd $PACKAGE_ROOT; tar xfm -)',
                'cp VERSION $PACKAGE_ROOT',
                """sed -e "s;buildtools_dir:=/opt/clovis/buildtools;buildtools_dir:=$BUILDTOOLS;g"
                    -e "s;NET_SNMP_CONFIG = net-snmp-config;NET_SNMP_CONFIG = $NET_SNMP_CONFIG;g"
                    $PACKAGE_ROOT/src/SAFplus/build/common/templates/make-cross.mk.in >
                $PACKAGE_ROOT/src/SAFplus/build/common/templates/make-cross.mk.in.modified""",
                """mv  -f $PACKAGE_ROOT/src/SAFplus/build/common/templates/make-cross.mk.in.modified
                       $PACKAGE_ROOT/src/SAFplus/build/common/templates/make-cross.mk.in"""]

        self.run_command_list(cmds) 

        # IDE Installation
        if self.INSTALL_IDE:
            self.install_IDE()

        # LogViewer
        if os.path.isdir(os.path.join(self.WORKING_DIR, 'log-viewer')):
            self.feedback('Installing LogViewer...')

            cmds = ['cd $WORKING_DIR',
            'tar cf - logviewer |(cd $PACKAGE_ROOT; tar xfm -)']

            if self.GPL:
               cmds.append('tar cf - src/logviewer |(cd $PACKAGE_ROOT; tar xfm -)') 

            cmds.append('cd $PACKAGE_ROOT')
            cmds.append('sed -i "s;\$PWD;$PACKAGE_ROOT\/logviewer;" logviewer/logViewer.sh')
            
            self.run_command_list(cmds)

        self.feedback('Copying documents...')

        self.ESC_PKG_ROOT = syscall("echo %s | sed -e 's;/;\\\/;g'" % self.PACKAGE_ROOT)
        self.ESC_PKG_NAME = syscall("echo %s | sed -e 's/\./\\\./g'" % self.PACKAGE_NAME)

        cmds = ['export ESC_PKG_ROOT=%s' % self.ESC_PKG_ROOT,
                'export ESC_PKG_NAME=%s' % self.ESC_PKG_NAME,
                'export ESC_ECLIPSE_DIR=%s' % self.ESC_ECLIPSE_DIR,
                'cd $WORKING_DIR',
                'tar cf - doc | (cd $PACKAGE_ROOT; tar xfm -) >/dev/null 2>&1',
                'cd $WORKING_DIR/templates/bin',
                'mkdir -p $BIN_ROOT 2>/dev/null',
                """sed -e "s;@CL_SDK@;$ESC_PKG_NAME;g"
                    -e "s;@CL_SDKDIR@;$ESC_PKG_ROOT;g"
                    $WORKING_DIR/templates/bin/cl-create-project-area.in > $BIN_ROOT/cl-create-project-area""",
                """sed -e "s;@CL_SDK@;$ESC_PKG_NAME;g"
                    -e "s;@CL_SDKDIR@;$ESC_PKG_ROOT;g"
                    $WORKING_DIR/templates/bin/cl-ide.in > $BIN_ROOT/cl-ide""",
                """sed -e "s;@CL_SDK@;$ESC_PKG_NAME;g"
                    -e "s;@CL_SDKDIR@;$ESC_PKG_ROOT;g"
                    $WORKING_DIR/templates/bin/cl-log-viewer.in > $BIN_ROOT/cl-log-viewer""",
                """sed -e "s;@CL_SDK@;$ESC_PKG_NAME;g"
                -e "s;@CL_SDKDIR@;$ESC_PKG_ROOT;g"
                $WORKING_DIR/templates/bin/cl-generate-source.in > $BIN_ROOT/cl-generate-source""",
                """sed -e "s;@CL_SDK@;$ESC_PKG_NAME;g"
                    -e "s;@CL_SDKDIR@;$ESC_PKG_ROOT;g"
                    $WORKING_DIR/templates/bin/cl-validate-project.in > $BIN_ROOT/cl-validate-project""",
                """sed -e "s;@CL_SDK@;$ESC_PKG_NAME;g"
                    -e "s;@CL_SDKDIR@;$ESC_PKG_ROOT;g"
                    $WORKING_DIR/templates/bin/cl-migrate-project.in > $BIN_ROOT/cl-migrate-project""",
                'ln -s $PACKAGE_ROOT/src/SAFplus/scripts/cl-create-wrs-toolchain $BIN_ROOT/.',
                'ln -s $PACKAGE_ROOT/src/SAFplus/scripts/cl-create-wrs2.0-toolchain $BIN_ROOT/.',
                'chmod +x $BIN_ROOT/*',
                'cd - >/dev/null 2>&1',
                'cd $BIN_ROOT',
                'ln -s $PACKAGE_ROOT/logviewer/icons icons',
                'ln -s $PACKAGE_ROOT/logviewer/config config',
                'cd - >/dev/null 2>&1',
                'mkdir -p $LIB_ROOT 2>/dev/null',
                """sed -e "s;@SDKDIR@;$ESC_PKG_ROOT;g"
                -e "s;@ECLIPSE@;$ESC_ECLIPSE_DIR;g"
                    $WORKING_DIR/templates/lib/clconfig.rc.in > $LIB_ROOT/clconfig.rc """]


        self.run_command_list(cmds)

        # runtime accessability check
        assert self.LIB_ROOT
        filename = '%s/clutils.rc' % self.LIB_ROOT
        if os.path.isfile(filename) and not os.access(filename, os.W_OK):
            self.feedback('[WARNING] Cannot save run-time utilities in "%s": permission denied' % filename)

        self.feedback('Done.')

             

    def install_IDE(self):
        self.feedback('Starting IDE installation...')

        cmds = ['cd $WORKING_DIR',
                'tar cf - $IDE | ( cd $PACKAGE_ROOT; tar xfm -)']

        if self.GPL:
            cmds.append('tar cf - src/$IDE | ( cd $PACKAGE_ROOT; tar xfm -)')
        
        self.feedback("Linking Eclipse in %s..." % self.PACKAGE_ROOT)
        cmds.append('cd $PACKAGE_ROOT')
        cmds.append('rm -rf eclipse/plugins/*clovis* >/dev/null 2>&1') # remove redundant clovis plugins if any
        cmds.append('cp -rl $ECLIPSE_ROOT .')
        cmds.append("sed -e '/-showsplash\|org.eclipse.platform/d' eclipse/eclipse.ini > eclipse/eclipse_ini.tmp")
        cmds.append('rm eclipse/eclipse.ini')
        cmds.append('mv eclipse/eclipse_ini.tmp eclipse/eclipse.ini')
        cmds.append('cd %s/plugins' %self.IDE_ROOT)
        cmds.append('mv -f * $PACKAGE_ROOT/eclipse/plugins >/dev/null 2>&1')
        cmds.append('cd %s' %self.IDE_ROOT)
        cmds.append('rm -rf plugins')
        # update config.ini        
        cmds.append('cd %s/scripts' %self.IDE_ROOT)
        cmds.append('cp -rf config.ini $ECLIPSE/configuration')

        self.run_command_list(cmds)

        # Delete help cache if the build we are installing is newer than
        # the build we last installed
        assert self.CACHE_DIR
        VERSION_CACHE = '%s/eclipse/VERSION' % self.CACHE_DIR
        NOW_VERSION = '%s/VERSION' % self.PACKAGE_ROOT

        # is there a cache?
        if os.path.isfile(VERSION_CACHE):

            cachefdata = self.read_file(VERSION_CACHE)
            nowfdata = self.read_file(NOW_VERSION)
            do_nothing = False

            try:
                CACHE_BUILD_NUMBER = cachefdata[0].split('=')[1].strip()
                NOW_BUILD_NUMBER = nowfdata[0].split('=')[1].strip()

                if CACHE_BUILD_NUMBER == NOW_BUILD_NUMBER:
                    do_nothing = True

            except:
                # assume to just replace
                do_nothing = False

                if do_nothing:
                    # do nothing :)
                    pass
                else:
                    cmds = ['cp $PACKAGE_ROOT/VERSION $CACHE_DIR/eclipse/.',
                            'rm -rf $CACHE_DIR/eclipse/org.eclipse.help.base']

                    self.run_cmd_list(cmds)

        else:
            ECLIPSE_CACHE_DIR = os.path.join(self.CACHE_DIR, 'eclipse')
            if not os.path.isdir(ECLIPSE_CACHE_DIR):
                ret = cli_cmd('mkdir -p %s' % ECLIPSE_CACHE_DIR)
                if ret != 0:
                    self.feedback('[ERROR] Could not create "%s". Check permissions.' % ECLIPSE_CACHE_DIR, True)
                self.feedback('Created "%s" directory' % ECLIPSE_CACHE_DIR)

            cmds = ['cp $PACKAGE_ROOT/VERSION $CACHE_DIR/eclipse/.',
                    'rm -rf $CACHE_DIR/eclipse/org.eclipse.help.base']
            self.run_command_list(cmds)


    def install_utilities(self):
        self.feedback('Starting utilities installation...')

        filename = os.path.join(self.LIB_ROOT, 'clutils.rc')
        try:
            fh = open(filename, 'w')
            fh.write(CLUTILS_RC)
            fh.close()
        except: 
            self.feedback('[ERROR] Could not open "%s"' % filename, True)

        self.create_dir(self.BIN_ROOT)


    def prebuild(self):
        # ask about this early on
        strin = self.get_user_feedback('Build SAFplus libraries for the local machine and/or installed crossbuild toolchains ? <y|n> [y]: ')

        if not strin or strin.lower().startswith('y'):
           strin = self.get_user_feedback('Where to build ? [default: %s]: ' % self.ASP_PREBUILD_DIR)
           if strin:
           # they provided a path. expand '~' and './' references
              self.ASP_PREBUILD_DIR = self.expand_path(strin)
           else:
                # accept default
              pass

           self.create_dir(self.ASP_PREBUILD_DIR)
           self.feedback('The following installed build tool packages are found:')
           cmd = '%s' % self.BUILDTOOLS
           os.chdir (cmd)
           tool = syscall ('ls -1d *')
           print tool
           cmd = '%s' % self.ASP_PREBUILD_DIR
           os.chdir (cmd)
           self.feedback ('Select the crossbuild tool(s) to build from the above list, [Default: local]') 
           strin = self.get_user_feedback()
           if strin == None or strin == "":
               strin = "local"

           builds = re.split('\W+', strin)
           for b in builds:
             if b == 'local':
               syscall ("%s/src/SAFplus/configure --with-asp-build > build.log" % self.PACKAGE_ROOT)
             else: 
               syscall ("%s/src/SAFplus/configure --with-asp-build --with-cross-build=%s > build.log" % (self.PACKAGE_ROOT, b) )
           
             self.feedback('Building asp %s' % b)
             cmd = 'asp/build/%s' % b
             os.chdir (cmd)
             os.system ('make asp-libs')
             os.system ('make asp-install')


    def do_symlinks(self):
        # ask about this early on

        self.feedback('A few binaries are installed in %s.' % self.BIN_ROOT)
        self.feedback('For convenience, you can add the above directory to your PATH defination.')
        self.feedback('Alternatively, we can create symlinks for you (from a binary direcory that is already in you path).\n')
        strin = self.get_user_feedback('Create symbolic links for items in %s ? <y|n> [y]: '  % self.BIN_ROOT)

        self.DEFAULT_SYM_LINK = '/usr/local/bin'

        if not strin or strin.lower().startswith('y'):
           strin = self.get_user_feedback('Where to create the symbolic links ? [default: %s]: ' % self.DEFAULT_SYM_LINK)
           if strin:
           # they provided a path. expand '~' and './' references
              self.DEFAULT_SYM_LINK = self.expand_path(strin)
              if not os.path.isdir(self.DEFAULT_SYM_LINK):
                 self.create_dir(self.DEFAULT_SYM_LINK)
           else:
                # accept default
              pass


        for dirname, dirnames, filenames in os.walk(self.BIN_ROOT):
          for filename in filenames:
            if filename == 'cl-log-viewer': continue
            
            src = os.path.join(dirname, filename)
            dst = os.path.join(self.DEFAULT_SYM_LINK, filename)
            #self.feedback("%s -> %s" % (src,dst))
            try:
              os.remove(dst)  # remove it since it may point to another SDK version
            except OSError,e:
              pass # its ok if the file does not exist
            try:
              os.symlink(src,dst)
            except OSError,e:  
              if e.errno == errno.EPERM or e.errno == errno.EEXIST:  # EEXIST means that os.remove() failed for some reason
                self.feedback('No permission to change %s' % dst)
              else:
                self.feedback('Cannot create symlink %s, error %s' % (dst,str(e)))  

        self.feedback('Symbolic links for the  binaries are created in %s\n' % self.DEFAULT_SYM_LINK)


    def read_file(self, filepath):
        """ returns a list of the contents of filepath
        empty list is returned if file does not exist """
        if not os.path.isfile(filepath):
            return []

        try:
            fh = open(filepath, 'r')
            fdata = fh.readlines()
            fh.close()
        except:
            self.feedback('[ERROR] Could not read "%s"' % VERSION_CACHE, True)

        return fdata

        

    def run_command_list(self, cmds):
        """ takes a list of unix commands,
        injects the proper values, and runs them, exiting on failure"""

        for i in range(len(cmds)):
            cmds[i] = self.parse_unix_vars(cmds[i])
            assert '$' not in cmds[i]

        cmd = ' && '.join(cmds)
        ret = cli_cmd(cmd)
        if ret != 0:
            self.feedback('[ERROR] command failed: "%s"' % cmd)



    def parse_unix_vars(self, line):
        """ takes a string as input
        replaces unix variables with their equivalent
        returns new string """

        line = line.replace('\n', ' ')

        if '$' not in line:
            return line
        
        self.ESC_PKG_ROOT = syscall("echo %s | sed -e 's;/;\\\/;g'" % self.PACKAGE_ROOT)
        self.ESC_PKG_NAME = syscall("echo %s | sed -e 's/\./\\\./g'" % self.PACKAGE_NAME)
        #self.ESC_ECLIPSE_DIR = syscall("echo %s/eclipse | sed -e 's;/;\\\/;g'" % self.PACKAGE_ROOT)
        olist = ['PREFIX', 'thirdPartyPkg', 'BUILDTOOLS', 'NET_SNMP_CONFIG', 'PACKAGE_ROOT', 'BIN_ROOT', 'LIB_ROOT', 'WORKING_DIR', 'ESC_PKG_ROOT', 'ESC_PKG_NAME', 'IDE', 'ASP', 'PACKAGE_NAME', 'HOME', 'CACHE_DIR', 'IDE_ROOT', 'ECLIPSE_ROOT', 'ECLIPSE', 'ESC_ECLIPSE_DIR', 'PATH']
        rlist = [self.PREFIX, self.THIRDPARTYPKG_PATH, self.BUILDTOOLS, self.NET_SNMP_CONFIG, self.PACKAGE_ROOT, self.BIN_ROOT, self.LIB_ROOT, self.WORKING_DIR, self.ESC_PKG_ROOT, self.ESC_PKG_NAME, 'IDE', 'ASP', self.PACKAGE_NAME, self.HOME, self.CACHE_DIR, self.IDE_ROOT, self.ECLIPSE_ROOT, self.ECLIPSE, self.ESC_ECLIPSE_DIR, os.getenv('PATH') + os.defpath]
        
	assert len(rlist) == len(olist)

        for r in rlist:
            assert r

        for orig, rep in zip(olist, rlist):
            line = line.replace('${%s}' % orig.upper(), rep)
            line = line.replace('$%s' % orig.upper(), rep)


        assert '$' not in line, line

        return line
 


    def join_list_as_path(self, alist):
        """ takes as input a list of strings
        and returns a string with all of members of 'alist'
        joined together using os.path.join """
        ret = ''
        for l in alist:
            ret = os.path.join(ret, l)

        return ret
    

    def cleanup(self):
        """ cleans up files generated by this installer """ 
        
        self.debug('Cleaning up...')
        
        clean = []
        
        # add all outstanding locks to be cleaned
        clean.extend(self.locks_out)
        
        # install.pyc file
        pyc = sys.path[0] + sys.argv[0].lstrip('.') + 'c'
        clean.append(pyc)
        
        # add any other files that need to be cleaned here
        
        for cleanme in clean:
            try:
                os.remove(cleanme)
                self.debug('Removed %s' % cleanme)
            except:
                pass
    
    
    def debug(self, msg):
        """ Prints a debug message
            Also dumps to log file """
        assert self.GENERAL_LOG_PATH
        msg = "[DEBUG] " + msg
        if self.DEBUG_ENABLED:
            print msg
        try:
            fh = open(self.GENERAL_LOG_PATH, 'a')
            fh.write(msg + '\n')
            fh.close()
        except:
            if self.DEBUG_ENABLED:
                print "[DEBUG] Could not locate log path '%s'" % self.GENERAL_LOG_PATH
    
    
    
    def exit_script(self, tarlogs=False):
        """ Exit the script with the proper exit value """
        if tarlogs:
            self.grab_workspace_logs()
            self.tar_log_directory()
        self.cleanup()
        sys.exit(1)
        
    
    def feedback(self, msg, fatal=False, dotarlogs=True):
        """ Prints feedback to the user and
            is optionally fatal, and
            optionally tars up the log directory
            """
        
        print msg
        
        try:
            fh = open(self.GENERAL_LOG_PATH, 'a')
            fh.write(msg + '\n')
            fh.close()
        except:
            if self.DEBUG_ENABLED:
                print "[DEBUG] Could not locate log path '%s'" % self.GENERAL_LOG_PATH
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
        
        space = ' '*((CONST_WIDTH-1-len(title))/2)
        
        os.system('clear')  # clear screen
        
        print('-'*CONST_WIDTH)
        print(space + title + space)
        print('-'*CONST_WIDTH)
    
    
    def get_user_feedback(self, prompt=''):
        """ Return information provided by a user at a given prompt
            Exit cleanly if they kill the application during this time
            """
        
        try:
            return raw_input(prompt).strip()
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
    print "Script exited cleanly."
    sys.exit(0)

def Test():
    main()

# Get everything started
if __name__ == '__main__':
    main()

