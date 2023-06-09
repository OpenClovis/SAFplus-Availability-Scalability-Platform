import pdb
import os
import objects
from common import *
from distutils.version import *

cmp_version = lambda x, y: LooseVersion(x).__cmp__(y) #Equal 0, greater 1, lesser -1

# ------------------------------------------------------------------------------

class OS:
    """ OS Super class """
    
    def __init__(self): 
        self.osobj                  = None
        self.supported              = True
        self.pre_dep_list           = []
        self.ide_pre_dep_list           = []
        self.pre_pip_dep_list       = []
        self.dep_list               = []
        self.pre_cmd                = ''
        self.apt                    = False
        self.yum                    = False
        self.pwd                    = syscall('pwd')


        try:
          self.gccVer                 = [int(x) for x in syscall('gcc --version')[0].split()[3].split(".")]
        except IndexError:  # Most likely no gcc installed
          self.gccVer = None

        self.kernelVerString = syscall('uname -r')
        self.kernelVer       = self.kernelVerString.split(".")
        self.kernelVer[0] = int(self.kernelVer[0])
        self.kernelVer[1] = int(self.kernelVer[1])
        self.kernelVer[2] = int((self.kernelVer[2].split("-"))[0])


        self.bit = determine_bit()

        self.pre_init()                         # overload pre_init() as needed

        self.load_preinstall_deps()             # load up the preinstall dependency list (each os must implement)
        #self.load_install_deps()                # load up install dependency list
        #self.load_install_specific_deps()       # load up and custom install deps for a specific OS

        self.post_init()                         # overload post_init() as needed


    def pre_init(self):
        """ overload as needed """
        self.name = 'N/A'

    def post_init(self):
        """ overload as needed """
        pass
    
    def load_preinstall_deps(self):
        """ overload and load preinstall() deps here """
        assert 0, 'Script Error: Each OS class needs to implement load_preinstall_deps()'
    
    def log_string_for_dep(self, dep):
        # returns a string to append onto a build command for a given dep to pipe
        # its output into the proper logging directory
        # piping of our logging output
        log_string = ' >> %s 2>&1' % os.path.join(os.path.join(self.pwd, 'log'), dep + '.log')
        # ex 'make ' + ' > /root/log/openhpi.log 2>&1'
        return log_string   
    
    def load_install_specific_deps(self):
        """ override this per OS to custom define 
            any install() objects """
        
        pass


# ------------------------------------------------------------------------------
class Ubuntu14(OS):
    """ Ubuntu Distro class """
    def pre_init(self):
        self.name = 'Ubuntu'
        self.apt = True
    
    def load_preinstall_deps(self):
        
        deps =  ['build-essential',
                 'autoconf',
                 'python3.4-dev',
                 'pkg-config',
                 'libtool',
                 'curl',
                 'mesa-common-dev',
                 'freeglut3-dev',
                 'libglew-dev',
                 'libglm-dev',
                 'libgtk-3-dev',
                 'libcairo2-dev',
                 'librsvg2-dev',
                 'python-rsvg'
                ]
        
        
        for name in deps:
            D = objects.RepoDep(name)
            self.pre_dep_list.append(D)

#-------------------------------------------------------------------------------
class Ubuntu16(OS):
    """ Ubuntu Distro class """
    def pre_init(self):
        self.name = 'Ubuntu'
        self.apt = True
    
    def load_preinstall_deps(self):
        
        deps =  ['build-essential',
                 'autoconf',
                 'python3.5-dev',
                 'python-pip',
                 'pkg-config',
                 'libtool',
                 'curl',
                 'python3-pip'                 
                ]

        ide_deps = ['libffi-dev',
                'libgtk-3-dev',
                'gir1.2-rsvg-2.0',
                'python3-cairo',
                'python-gi-cairo',
                'python3-gi'
                ]

        pip_deps =  ['genshi', 'watchdog', 'paramiko', 'wxPython==4.1.1']
        
        for name in deps:
            D = objects.RepoDep(name)
            self.pre_dep_list.append(D)

        for name in ide_deps:
            D = objects.RepoDep(name)
            self.ide_pre_dep_list.append(D)

        for name in pip_deps:
            D = objects.RepoDep(name)
            self.pre_pip_dep_list.append(D)
# ------------------------------------------------------------------------------

class Ubuntu18(OS):
    """ Ubuntu Distro class """
    def pre_init(self):
        self.name = 'Ubuntu'
        self.apt = True

    def load_preinstall_deps(self):

        deps =  ['build-essential',
                 'autoconf',
                 'python3.6-dev',
                 #'python-pip',
                 'pkg-config',
                 'libtool',
                 'curl',
                 'python3-pip'
                ]

        ide_deps = ['libffi-dev',
                'libgtk-3-dev',
                'gir1.2-rsvg-2.0',
                'python3-cairo',
                'python-gi-cairo',
                'python3-gi'
                ]

        pip_deps =  ['genshi', 'watchdog', 'paramiko', 'wxPython==4.1.1']

        for name in deps:
            D = objects.RepoDep(name)
            self.pre_dep_list.append(D)

        for name in ide_deps:
            D = objects.RepoDep(name)
            self.ide_pre_dep_list.append(D)

        for name in pip_deps:
            D = objects.RepoDep(name)
            self.pre_pip_dep_list.append(D)
# ------------------------------------------------------------------------------

class CentOS8(OS):
    
    def pre_init(self):
        self.name = 'CentOS 8'
        self.yum = True
    
    def load_preinstall_deps(self):
        deps =  ['gcc',
                 'gcc-c++',
                 'make',
                 'dnf python3',                 
                 'autoconf',
                 'python3-devel',
                 'pkg-config',
                 'libtool',
                 'curl']
        ide_deps = ['mesa-libGLU-devel',
                 'freeglut',
                 'gtk3-devel',
                 'cairo',
                 'cairo-devel',
                 'librsvg2-devel',
                 'librsvg2-tools',
                 'libjpeg-turbo-devel',
                 'libpng12'
                 ]
        pip_deps =  ['wxpython', 'genshi', 'watchdog', 'paramiko']
            
        for name in deps:
            D = objects.RepoDep(name)
            self.pre_dep_list.append(D)

        for name in ide_deps:
            D = objects.RepoDep(name)
            self.ide_pre_dep_list.append(D)

        for name in pip_deps:
            D = objects.RepoDep(name)
            self.pre_pip_dep_list.append(D)


# ------------------------------------------------------------------------------
class Debian10(OS):
    
    def pre_init(self):
        self.name = 'Debian'
        self.apt = True
    
    def load_preinstall_deps(self):
        
        deps =  ['build-essential',
                 'autoconf',
                 'python3.7-dev',
                 'python-pip',
                 'pkg-config',
                 'libtool',
                 'curl'
                ]
        ide_deps = ['mesa-common-dev',
                 'freeglut3-dev',
                 'libglew-dev',
                 'libglm-dev',
                 'libgtk-3-dev',
                 'python-wxgtk3.0',
                 'libcairo2-dev',
                 'librsvg2-dev',
                 'python-cairo'
                ]

        pip_deps =  ['genshi', 'watchdog', 'paramiko']

        for name in deps:
            D = objects.RepoDep(name)
            self.pre_dep_list.append(D)

        for name in ide_deps:
            D = objects.RepoDep(name)
            self.ide_pre_dep_list.append(D)

        for name in pip_deps:
            D = objects.RepoDep(name)
            self.pre_pip_dep_list.append(D)

# ------------------------------------------------------------------------------

class Debian9(OS):
    
    def pre_init(self):
        self.name = 'Debian'
        self.apt = True
    
    def load_preinstall_deps(self):
        
        deps =  ['build-essential',
                 'autoconf',
                 'python3.5-dev',
                 'python-pip',
                 'pkg-config',
                 'libtool',
                 'curl'                 
                ]
        ide_deps =  ['mesa-common-dev',
                 'freeglut3-dev',
                 'libglew-dev',
                 'libglm-dev',
                 'libgtk-3-dev',
                 'python-wxgtk3.0',
                 'libcairo2-dev',
                 'librsvg2-dev',
                 'python-rsvg'
                ]

        pip_deps =  ['genshi', 'watchdog', 'paramiko']

        for name in deps:
            D = objects.RepoDep(name)
            self.pre_dep_list.append(D)

        for name in ide_deps:
            D = objects.RepoDep(name)
            self.ide_pre_dep_list.append(D)

        for name in pip_deps:
            D = objects.RepoDep(name)
            self.pre_pip_dep_list.append(D)

# ------------------------------------------------------------------------------
class Other(OS):
    
    def pre_init(self):
        self.name = 'Other'
        self.supported = False
        self.apt = False
        self.yum = False
    
    def load_preinstall_deps(self):
        pass

# ------------------------------------------------------------------------------
# ------------------------------------------------------------------------------
# ------------------------------------------------------------------------------

def determine_os():
    """ 
        Attempts to determine the users operating system 
        and return the proper OS object
        Returns None if unable to determine OS 
        """
    hostos = syscall('uname -s').lower()
    
    if hostos == 'linux':
        
        # CentOS or RedHat or Fedora    
        if os.path.isfile('/etc/redhat-release'):
            try:
                fh = open('/etc/redhat-release')
                fdata = fh.read().lower()
                fh.close()
            except:
                return None
            
            #if 'fedora' in fdata: return Fedora()
            
            if 'centos' in fdata:
                #if 'release 4' in fdata: return CentOS4()
                #if 'release 5' in fdata: return CentOS5()
                #if 'release 6' in fdata: return CentOS6()
                if 'release 8' in fdata: return CentOS8()
            else:              
                # must be redhat
                #if 'release 4' in fdata: return RedHat4()
                #if 'release 5' in fdata: return RedHat5()
                #if 'release 6' in fdata: return RedHat6()
                return None
        
             
        # Ubuntu
        if os.path.isfile('/etc/lsb-release'):
            try:
                fh = open('/etc/lsb-release')
                fdata = fh.read().lower()
                fh.close()
            except:
                return None
            
            if 'ubuntu' in fdata:
                #if '14.' in fdata: # ubuntu 14 is not supported. Need newwer one than it, e.g. 16, 18 or 20...
                #    return Ubuntu14()
                if '16.' in fdata:
                    return Ubuntu16()
                elif '18.' in fdata:
                    return Ubuntu18()
            
        # Debian
        if os.path.isfile('/etc/debian_version'):
            print ('Debian OS')
            try:
                fh = open('/etc/debian_version')
                fdata = fh.read().lower()
                fh.close()
                #print (fdata)
                if 'buster' in fdata or '10.' in fdata: 
                    print ('Debian 10')
                    return Debian10()
                if 'stretch' in fdata or '9.' in fdata: return Debian9()
                #if '8.' in fdata: return Debian8()                
            except:
                print ('NN')
                return None 
    
    # OSX/Solaris, both unsupported
    elif hostos == 'darwin' or hostos == 'solaris':
        return Other()
    print ('NNN')
    return None

