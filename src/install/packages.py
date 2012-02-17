import os
import objects
from common import *

# ------------------------------------------------------------------------------

class OS:
    """ OS Super class """
    
    def __init__(self): 
        self.osobj                  = None
        self.supported              = True
        self.pre_dep_list           = []
        self.dep_list               = []
        self.pre_cmd                = ''
        self.apt                    = False
        self.yum                    = False
        self.pwd                    = syscall('pwd')
        
        self.bit = determine_bit()
        
        self.load_preinstall_deps()             # load up the preinstall dependency list (each os must implement)
        self.load_install_deps()                # load up install dependency list
        self.load_install_specific_deps()       # load up and custom install deps for a specific OS
        self.post_init()                        # overload post_init() as needed
    
    
    def post_init(self):
        """ overload as needed """
        self.name = 'N/A'
    
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
    
    
    def load_install_deps(self):
        """ this function loads the install() phase deps """
        
        # note - order of installation is dependent on list
        # creation at the end of this function
        
        # ------------------------------------------------------------------------------
        # gcc
        # ------------------------------------------------------------------------------
        EXPORT = ''
        
        gcc = objects.BuildDep()
        gcc.name          = 'gcc'
        gcc.version       = '3.2.3'
        gcc.pkg_name      = 'gcc-3.2.3.tar.gz'
        gcc.ver_test_cmd  = 'gcc -dumpversion 2>/dev/null'
        
        log = self.log_string_for_dep(gcc.name)
        
        gcc.build_cmds = [ """../configure --prefix=${PREFIX} --enable-shared --enable-threads=posix
            --disable-checking --with-system-zlib --enable-__cxa_atexit --disable-libunwind-exceptions
            --enable-java-awt=gtk --enable-languages=C,C++,Java""" + log,
                          'make' + log,
                          'make install' + log]
                          
        # ------------------------------------------------------------------------------
        # glibc
        # ------------------------------------------------------------------------------
        EXPORT = ''
        
        # *** These critical programs are missing or too old: as ld [error ubuntu 10.10 64]
        
        glibc = objects.BuildDep()
        glibc.name          = 'glibc'
        glibc.version       = '2.3.5'
        glibc.pkg_name      = 'glibc-2.3.5.tar.gz'
        
        log = self.log_string_for_dep(glibc.name)

        # actually need to compile a test program for this
        
        program = """ #include <stdio.h>
            #include <gnu/libc-version.h>
            int main (void) { puts (gnu_get_libc_version ()); return 0; } """
        
        glibc.ver_test_cmd  = """echo \"%s\" > /tmp/glibc-test.c 2>/dev/null;
            gcc /tmp/glibc-test.c -o /tmp/glibc-test 2>/dev/null;
            /tmp/glibc-test; """ % program
        
        
        
        glibc.build_cmds    = ['tar xfm "${THIRDPARTYPKG}" glibc-linuxthreads-2.3.5.tar.gz',
                               'tar zxf glibc-linuxthreads-2.3.5.tar.gz',
                               'rm -f glibc-linuxthreads-2.3.5.tar.gz',
                               'mv linuxthreads ..',
                               """../configure --prefix=${PREFIX} --build=i686-pc-linux-gnu --enable-add-ons=linuxthreads
                                   --disable-profile --without-cvs --without-__thread --without-sanity-check""" + log,
                               'make' + log,
                               'make install' + log]
        
        # ------------------------------------------------------------------------------
        # glib
        # ------------------------------------------------------------------------------
        EXPORT = 'export PKG_CONFIG_PATH=${PREFIX}/lib/pkgconfig/'
        
        # upgrade 2.28
        glib = objects.BuildDep()
        glib.name          = 'glib'
        glib.version       = '2.28.5'
        glib.pkg_name      = 'glib-2.28.5.tar.gz'
        glib.ver_test_cmd  = EXPORT + ' && pkg-config --modversion glib-2.0 2>/dev/null'
        
        log = self.log_string_for_dep(glib.name)
      
        # glib will only build if we dont use a /build/ dir
        glib.use_build_dir = False
      
        glib.build_cmds = ['./configure --prefix=${PREFIX}' + log,
                           'make' + log,
                           'make install' + log]
                                   
        # ------------------------------------------------------------------------------
        # openhpi
        # ------------------------------------------------------------------------------
        EXPORT = 'export PKG_CONFIG_PATH=${PREFIX}/lib/pkgconfig/'
        
        openhpi = objects.BuildDep()
        openhpi.name           = 'openhpi'
        openhpi.version        = '3.0.0'
        openhpi.pkg_name       = 'openhpi-3.0.0.oc.tar.gz'
        openhpi.ver_test_cmd   = EXPORT + ' && pkg-config --modversion openhpi 2>/dev/null'
        
        log = self.log_string_for_dep(openhpi.name)
        
        # special case
        initial_commands = ['cd ${PREFIX}', 
                            'rm -rf openhpi-*',
                            'tar xfm ${THIRDPARTYPKG} %s' % openhpi.pkg_name,
                            'tar zxf %s' % openhpi.pkg_name,
                            'rm -f %s' % openhpi.pkg_name,
                            'cd openhpi-*',
                            EXPORT + ' && ./configure --prefix=$PREFIX --with-varpath=$PREFIX/var/lib/openhpi' + log, 
                            'make' + log,
                            'make install' + log]
                            
        openhpi.build_cmds = [';'.join(initial_commands)]

        
        
        # ------------------------------------------------------------------------------
        # net-snmp
        # ------------------------------------------------------------------------------
        EXPORT = ''
        
        netsnmp = objects.BuildDep()
        netsnmp.name           = 'net-snmp'
        netsnmp.version        = '5.4.2'    
        netsnmp.pkg_name       = 'net-snmp-5.4.2.tar.gz'
        
        netsnmp.ver_test_cmd   = 'net-snmp-config --version 2>/dev/null'
        
        log = self.log_string_for_dep(netsnmp.name)
        
        netsnmp.use_build_dir = False

        netsnmp.build_cmds     = ["""./configure
                                    --prefix=${PREFIX}
                                    --with-default-snmp-version="2"
                                    --without-rpm
                                    --with-defaults
                                    --with-perl-modules=PREFIX=${PREFIX} """ + log,
                                  'make' + log,
                                  'make install' + log]
                
        
        
        # ------------------------------------------------------------------------------
        # openhpi-subagent
        # ------------------------------------------------------------------------------
        EXPORT = 'export PKG_CONFIG_PATH=${PREFIX}/lib/pkgconfig/ PATH=$PATH:${PREFIX}/bin'
        
        openhpisubagent = objects.BuildDep()
        openhpisubagent.name           = 'openhpi-subagent'
        openhpisubagent.version        = '2.3.4'    
        openhpisubagent.pkg_name       = 'openhpi-subagent-2.3.4.tar.gz'
        
        log = self.log_string_for_dep(openhpisubagent.name)
        
        # this is tricky because there is no version; we just test for its existence... install.py handles this special case
        
        openhpisubagent.use_build_dir = False
        
        
        openhpisubagent.build_cmds     = [EXPORT + ' && ./configure --prefix=${PREFIX} CFLAGS=-I${BUILDTOOLS}/local/include' + log,
                                          EXPORT + ' && make' + log, 
                                          EXPORT + ' && make install' + log]
        
                                          #'cp Makefile Makefile.bak' + log,
                                          #'sed --in-place -e "s;`net-snmp-config --prefix`;$PREFIX;g" Makefile' + log,

        # ------------------------------------------------------------------------------
        # TIPC
        # ------------------------------------------------------------------------------
        EXPORT = 'export KERNELDIR=/lib/modules/`uname -r`/build'
        
        TIPC = objects.BuildDep()
        TIPC.name           = 'tipc'
        TIPC.version        = '1.5.12'
        TIPC.pkg_name       = 'tipc-1.5.12.tar.gz'
        
        log = self.log_string_for_dep(TIPC.name)
        
        # tipc has a special case in install.py marked:                 # SPECIAL CASE, TIPC
        #TIPC.ver_test_cmd   = ':'
        
        
        TIPC.build_cmds     = [EXPORT + ' && make' + log,
                                'cp net/tipc/tipc.ko $PREFIX/modules',
                                'cp tools/tipc-config $PREFIX/bin',
                                'cp include/net/tipc/*.h $PREFIX/include']
               
        res = syscall('uname -r')
        
        if int(res.split('.')[2].split('-')[0]) < 16:
            pass
        else:
            TIPC.build_cmds.append('mkdir -p $PREFIX/include/linux >/dev/null 2>&1')
            TIPC.build_cmds.append('cp include/net/tipc/*.h $PREFIX/include/linux')
        

        # ------------------------------------------------------------------------------
        # TIPC_CONFIG
        # ------------------------------------------------------------------------------
        EXPORT = 'export KERNELDIR=/lib/modules/%s/build' % syscall('uname -r')
        
        TIPC_CONFIG = objects.BuildDep()
        TIPC_CONFIG.name           = 'tipc-config'
        TIPC_CONFIG.version        = '1.1.5'
        TIPC_CONFIG.pkg_name       = 'tipcutils-1.1.5.tar.gz' #default name, can change
        
        log = self.log_string_for_dep(TIPC_CONFIG.name)
        
        TIPC_CONFIG.use_build_dir = False

        TIPC_CONFIG.build_cmds = [EXPORT + ' && make',
                                  'mkdir -p $PREFIX/bin',
                                  'cp tipc-config $PREFIX/bin']


        # ------------------------------------------------------------------------------
        # JRE
        # ------------------------------------------------------------------------------
        EXPORT = ''
        
        JRE = objects.BuildDep()
        JRE.name           = 'JRE'
        JRE.version        = '1.6.0'
        JRE.pkg_name       = 'jre1.6.0_21.tar.gz'
        if self.bit == 64:
            JRE.pkg_name       = 'jre1.6.0_21-x86_64.tar.gz'
        
        log = self.log_string_for_dep(JRE.name)
       
        JRE.extract_install = True

        initial_commands = ['cd ${PREFIX}',
                          'tar xf ${THIRDPARTYPKG} %s' % JRE.pkg_name,
                          'tar zxf %s' % JRE.pkg_name,
                          'rm -f %s' % JRE.pkg_name]

        JRE.build_cmds = [';'.join(initial_commands)]


        # ------------------------------------------------------------------------------
        # ECLIPSE
        # ------------------------------------------------------------------------------
        EXPORT = ''
        
        ECLIPSE = objects.BuildDep()
        ECLIPSE.name           = 'eclipse'
        ECLIPSE.version        = '3.7.1'
        ECLIPSE.pkg_name       = 'eclipse-SDK-3.7.1-linux-gtk.tar.gz'
        if self.bit == 64:
            ECLIPSE.pkg_name       = 'eclipse-SDK-3.7.1-linux-gtk-x86_64.tar.gz'
       
        log = self.log_string_for_dep(ECLIPSE.name)
        
        ECLIPSE.extract_install = True

        initial_commands = ['cd ${PREFIX}',
                          'tar xf ${THIRDPARTYPKG} %s' % ECLIPSE.pkg_name,
                          'tar zxf %s' % ECLIPSE.pkg_name,
                          'rm -f %s' % ECLIPSE.pkg_name]

        ECLIPSE.build_cmds = [';'.join(initial_commands)]

            
        # ------------------------------------------------------------------------------
        # EMF
        # ------------------------------------------------------------------------------
        EXPORT = ''
        
        EMF = objects.BuildDep()
        EMF.name           = 'EMF'
        EMF.version        = '2.7.1'
        EMF.pkg_name       = 'emf-runtime-2.7.1.zip'
        
        log = self.log_string_for_dep(EMF.name)

        EMF.extract_install = True

        initial_commands = ['cd ${PREFIX}',
                          'tar xf ${THIRDPARTYPKG} %s' % EMF.pkg_name,
                          'unzip -qq -o -u %s' % EMF.pkg_name,                            
                          'rm -f %s' % EMF.pkg_name]

        EMF.build_cmds = [';'.join(initial_commands)]


        # ------------------------------------------------------------------------------
        # GEF
        # ------------------------------------------------------------------------------
        EXPORT = ''
        
        GEF = objects.BuildDep()
        GEF.name           = 'GEF'
        GEF.version        = '3.7.2'
        GEF.pkg_name       = 'GEF-runtime-3.7.2.zip'
        
        log = self.log_string_for_dep(GEF.name)

        GEF.extract_install = True
        
        initial_commands = ['cd ${PREFIX}',
                          'tar xf ${THIRDPARTYPKG} %s' % GEF.pkg_name,
                          'unzip -qq -o -u %s' % GEF.pkg_name,  
                          'rm -f %s' % GEF.pkg_name]

        GEF.build_cmds = [';'.join(initial_commands)]


        # ------------------------------------------------------------------------------
        # CDT
        # ------------------------------------------------------------------------------
        EXPORT = ''
        
        CDT = objects.BuildDep()
        CDT.name           = 'CDT'
        CDT.version        = '8.0.1'
        CDT.pkg_name       = 'cdt-master-8.0.1.zip'
        
        log = self.log_string_for_dep(CDT.name)

        CDT.extract_install = True
        
        initial_commands = ['cd ${PREFIX}',
                          'mkdir -p eclipse/cdt/eclipse',
                          'mkdir -p eclipse/links',
                          'cd eclipse/cdt/eclipse',
                          'echo "path=$PREFIX/eclipse/cdt" > $PREFIX/eclipse/links/cdt.link',
                          'tar xvf ${THIRDPARTYPKG} %s' % CDT.pkg_name,
                          'unzip -qq -o -u %s' % CDT.pkg_name,
                          'rm -f %s' % CDT.pkg_name]

        CDT.build_cmds = [';'.join(initial_commands)]
        
        
        # ------------------------------------------------------------------------------
        # Collect all the objects we built
        # ------------------------------------------------------------------------------
        
        
        # this list defines the order of installation
      
        self.dep_list = [gcc, glibc, glib, openhpi, netsnmp, openhpisubagent, TIPC, TIPC_CONFIG, JRE, ECLIPSE, EMF, GEF, CDT]
    
    
    def load_install_specific_deps(self):
        """ override this per OS to custom define 
            any install() objects """
        
        pass


# ------------------------------------------------------------------------------
class Ubuntu(OS):
    """ Ubuntu Distro class """
    
    def post_init(self):
        self.name = 'Ubuntu'
        self.apt = True
    
    def load_preinstall_deps(self):
        
        deps =  ['build-essential',
                 'linux-headers-' + syscall('uname -r'),
                 'gettext',
                 'uuid-dev',
                 'bison',
                 'flex',
                 'gawk',
                 'pkg-config',
                 'libglib2.0-dev',
                 'libgdbm-dev',
                 'libdb4.6-dev',
                 'libsqlite3-0',
                 'libsqlite3-dev',
                 'e2fsprogs',
                 'libperl-dev',
                 'libltdl3-dev',
                 'e2fslibs-dev',
                 'libsnmp-dev',
                 'zlib1g-dev']
        
        
        for name in deps:
            D = objects.RepoDep(name)
            self.pre_dep_list.append(D)


# ------------------------------------------------------------------------------
class RedHat4(OS):
    
    def post_init(self):
        self.name = 'Red Hat 4'
        self.yum = True
    
    def load_preinstall_deps(self):
        deps =  ['pkgconfig', 
                 'libtool',
                 'libtool-libs',
                 'gcc',
                 'gcc-c++',
                 'gettext',
                 'kernel-devel',
                 'kernel-headers',
                 'perl-devel',
                 'db4',
                 'db4-devel',
                 'db4-utils',
                 'e2fsprogs',
                 'e2fsprogs-devel',
                 'gdbm',
                 'gdbm-devel',
                 'sqlite', 
                 'sqlite-devel']

        for name in deps:
                D = objects.RepoDep(name)
                self.pre_dep_list.append(D)




# ------------------------------------------------------------------------------
class RedHat5(OS):
    
    def post_init(self):
        self.name = 'Red Hat 5'
        self.yum = True
    
    def load_preinstall_deps(self):
        deps =  ['pkgconfig', 
                 'libtool',
                 'libtool-libs',
                 'gcc',
                 'gcc-c++',
                 'gettext',
                 'kernel-devel',
                 'kernel-headers',
                 'perl-devel',
                 'db4',
                 'db4-devel',
                 'db4-utils',
                 'e2fsprogs',
                 'e2fsprogs-devel',
                 'gdbm',
                 'gdbm-devel',
                 'sqlite', 
                 'sqlite-devel']
            
            
        for name in deps:
            D = objects.RepoDep(name)
            self.pre_dep_list.append(D)


# ------------------------------------------------------------------------------
class CentOS4(OS):
    
    def post_init(self):
        self.name = 'CentOS 4'
        self.yum = True
    
    def load_preinstall_deps(self):
        deps =  ['pkgconfig', 
                 'libtool',
                 'libtool-libs',
                 'gcc',
                 'gcc-c++',
                 'gettext',
                 'kernel-devel',
                 'kernel-headers',
                 'perl-devel',
                 'db4',
                 'db4-devel',
                 'db4-utils'
                 'e2fsprogs',
                 'e2fsprogs-devel',
                 'gdbm',
                 'gdbm-devel',
                 'sqlite', 
                 'sqlite-devel']
            
            
        for name in deps:
            D = objects.RepoDep(name)
            self.pre_dep_list.append(D)


# ------------------------------------------------------------------------------
class CentOS5(OS):
    
    def post_init(self):
        self.name = 'CentOS 5'
        self.yum = True
    
    def load_preinstall_deps(self):
        deps =  ['pkgconfig', 
                 'libtool',
                 'libtool-libs',
                 'gcc',
                 'gcc-c++',
                 'gettext',
                 'kernel-devel',
                 'kernel-headers',
                 'perl-devel',
                 'db4',
                 'db4-devel',
                 'db4-utils'
                 'e2fsprogs',
                 'e2fsprogs-devel',
                 'gdbm',
                 'gdbm-devel',
                 'sqlite', 
                 'sqlite-devel']
            
            
        for name in deps:
            D = objects.RepoDep(name)
            self.pre_dep_list.append(D)


# ------------------------------------------------------------------------------
class Fedora(OS):
    
    def post_init(self):
        self.name = 'Fedora'
        self.yum = True
    
    def load_preinstall_deps(self):
        deps =  ['libtool-ltdl-devel',
                    'gcc',
                    'gcc-c++',
                    'gettext',
                    'kernel-devel',
                    'kernel-headers',
                    'e2fsprogs',
                    'e2fsprogs-devel',
                    'net-snmp-devel',
                    'libuuid',
                    'libuuid-devel',
                    'perl-devel',
                    'gdbm-devel',
                    'db4-devel',
                    'db4-utils',
                    'sqlite', 
                    'sqlite-devel']

                    
        for name in deps:
            D = objects.RepoDep(name)
            self.pre_dep_list.append(D)



# ------------------------------------------------------------------------------
class SUSE(OS): # uses YAST #fixme
    
    # SUSE is not supported as of 4/8/2011
    
    def post_init(self):
        self.name = 'SUSE'
        self.supported = False
        
    def load_preinstall_deps(self):
        pass
        
#        deps =  ['build-essential',
#                 'linux-headers-' + syscall('uname -r'),
#                 'gettext',
#                 'uuid-dev',
#                 'bison',
#                 'flex',
#                 'gawk',
#                 'pkg-config',
#                 'libglib2.0-dev',
#                 'libgdbm-dev',
#                 'libdb4.6-dev',
#                 'libsqlite3-0',
#                 'libsqlite3-dev',
#                 'e2fsprogs',
#                 'libperl-dev',
#                 'libltdl3-dgetev',
#                 'e2fslibs-dev']
#        
#        for name in deps:
#            D = objects.RepoDep(name)
#            self.pre_dep_list.append(D)

# ------------------------------------------------------------------------------
class Debian(OS):
    
    def post_init(self):
        self.name = 'Debian'
        self.apt = True
    
    def load_preinstall_deps(self):
        
        deps =  ['build-essential',
                 'linux-headers-' + syscall('uname -r'),
                 'gettext',
                 'uuid-dev',
                 'bison',
                 'flex',
                 'gawk',
                 'libglib2.0-dev',
                 'libgdbm-dev',
                 'libdb4.6-dev',
                 'libsqlite3-0',
                 'libsqlite3-dev',
                 'e2fsprogs',
                 'libperl-dev',
                 'libltdl3-dev',
                 'e2fslibs-dev',
                 'unzip',
                 'libsnmp-dev',
                 'zlib1g-dev',
                 'psmisc']
        
        if determine_bit() == 64:
            deps.append('ia32-libs')
        
        for name in deps:
            D = objects.RepoDep(name)
            self.pre_dep_list.append(D)


# ------------------------------------------------------------------------------
class Other(OS):
    
    def post_init(self):
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
            
            if 'fedora' in fdata: return Fedora()
            
            if 'centos' in fdata:
                if 'release 4' in fdata: return CentOS4()
                if 'release 5' in fdata: return CentOS5()
            else: 
                # must be redhat
                if 'release 4' in fdata: return RedHat4()
                if 'release 5' in fdata: return RedHat5()
        
        # SUSE
        if os.path.isfile('/etc/SuSE-release'):
            return SUSE()
        
        # Ubuntu
        if os.path.isfile('/etc/lsb-release'):
            try:
                fh = open('/etc/lsb-release')
                fdata = fh.read().lower()
                fh.close()
            except:
                return None
            
            if 'ubuntu' in fdata:
                return Ubuntu()
        
        # Debian
        if os.path.isfile('/etc/debian_version'):
            return Debian()
    
    # OSX/Solaris, both unsupported
    elif hostos == 'darwin' or hostos == 'solaris':
        return Other()
    
    return None
