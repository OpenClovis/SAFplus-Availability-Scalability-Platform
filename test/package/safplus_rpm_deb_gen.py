#!/usr/bin/env python
import virtualbox
import time
import getpass
import sys
import os
import re
import pdb
import getopt
from logging import DEBUG
try:
    tae_dir = os.environ['TAE_DIR']
    print "TAE_DIR is {}".format(tae_dir)
except KeyError:
    print "export the TAE_DIR environment variable before the running {}script".format(os.path.split(sys.argv[0])[1])
    sys.exit(1)
sys.path.insert(0,tae_dir)
try:
   import openclovis.test as test
except ImportError:
   print "Provide the correct tae directory path"
   sys.exit(1)
del (test)
import openclovis.test.log as log
import openclovis.test.xml2dict as xml2dict
import openclovis.test.sourcetree as source_tree
from openclovis.test.sessionbash import SessionBash
CFG_VER = (1, 0, 0, 0)


def remove_colour_coding(string):
    """
    This function returns the string without color coding sequences
    :param string: contains the output along with color coding sequence
    :return:
    """
    re_exp = re.compile(r'\x1b[^m]*m')
    return re_exp.sub('', string)

class PkggenException(Exception):
    def __init__(self, reason):
        Exception.__init__(self)
        self.error_msg = reason

def execute_command(ssh_obj, cmd, error_msg, time_out=60, skip_error=False):
    """
    This function executes the given command on the remote machine and throws an PkggenException if the skip_error is False
    """
    ret = None
    err = False
    try:
        ret = ssh_obj.run_cmd(cmd, timeout = time_out)
    except:
        err = True
    if ret[0] == 1 and not skip_error:
        log.error(" cmd returns an error {}".format(ret[1]))
    elif err:
        raise PkggenException(error_msg)        
    return ret


class ClVirtualBox:
    """
    This class is responsible for controlling the virtual machine by using pyvbox api
    """
    def __init__(self):
        self.v_box = virtualbox.VirtualBox()
        self.v_box_constants = virtualbox.library
        self.vm_obj = None
        self.vm_session = None
        self.vm_snap_count = 0
        self.v_box_session = virtualbox.Session()

    def get_vm_object(self, vm_name):
        """
        This function checks/finds whether the given VM(virtual machine) name is registered
        with the virtual box or not.
        :param vm_name: Name of the Virtual machine
        :return: VM object if the given VM name is available
                 None if the given Virtual Name  is not found
        """
        try:
            self.vm_obj = self.v_box.find_machine(vm_name)
            log.info("Name:{0} State:{1} UUID:{2}".format(vm_name, self.vm_obj.state, self.vm_obj.__uuid__))
        except virtualbox.library.VBoxErrorObjectNotFound:
            log.error("Requested {} is not found".format(vm_name))

    def create_v_box_session(self):
        """

        :return:
        """
        self.v_box_session = virtualbox.Session()

    def check_vm_running(self):
        """
        This function checks whether the given Virtual Machine object is running or not
        :return: True if the machine is running
                 False otherwise
        """
        status = False
        if self.vm_obj.state == self.v_box_constants.MachineState.running:
            status = True
        return status

    def create_vm_session(self):
        """
        creates a session to the virtual machine which will be useful in controlling the virtual machine
        like power down the virtual machine, taking the snap shot etc
        """
        self.vm_session = self.vm_obj.create_session()

    def start_virtual_machine(self):
        """
        This Function starts the virtual machine and wait until the virtual machine comes up
        """
        progress = self.vm_obj.launch_vm_process()
        progress.wait_for_completion()
        # wait until the Virtual machine gui comes up
        time.sleep(60)
        self.create_vm_session()

    def check_and_lock_vm_session(self):
        """
        This Function locks the session if it is not locked
        """
        if self.vm_session is None:
            self.create_vm_session()
        log.info("Vm State {}".format(self.vm_obj.state))
        if self.vm_session.state != self.v_box_constants.SessionState.locked:
            self.vm_obj.lock_machine(self.vm_session, self.v_box_constants.LockType.vm)

    def power_down_vm(self):
        """
        This function will power off the virtual machine
        """
        self.check_and_lock_v_box_session()
        progress = self.v_box_session.console.power_down()
        progress.wait_for_completion(-1)
        log.info("VM STATE is {}".format(self.vm_obj.state))

    def get_vm_snap_shot_count(self):
        """
        Virtual machine snapshot count
        :return:
        """
        try:
            self.vm_snap_count = self.vm_obj.snapshot_count
            log.info("Snap Shot count is {}".format(self.vm_snap_count))
        except:
            log.warning("{} does not have any snap shot".format(self.vm_obj.name))

    def restore_vm_state_from_snapshot(self):
        """
        starts the virtual machine from the snap shot
        """
        self.v_box_session.console.restore_snapshot()
        pass

    def check_and_lock_v_box_session(self):
        """
        :return:
        """
        if self.v_box_session.state == self.v_box_constants.SessionState.locked:
            self.unlock_v_box_session()
        self.vm_obj.lock_machine(self.v_box_session, self.v_box_constants.LockType.shared)

    def unlock_v_box_session(self):
        """
        :return:
        """
        self.v_box_session.unlock_machine()

    def unlock_vm_session(self):
        """
        :return:
        """
        self.vm_session.unlock_machine()


def proto_buf_rpm_installation(ssh_connection, working_dir):
    """
    This method install the protobuf related rpms into the remote machine because Centos7 yum repository doest not
    provides these libraries. Currently this method uses wget to download the rpms.
    :return:
    """
    rpm_dir = os.path.split(working_dir)[0] + os.sep + "protobuf_rpms"
    #Need to fix the installation of protobuf rpm packages on various Centos distributions. Currently this fuction
    # install the protobuf rpm packages on centos 7 distribution.  
    url_link = "http://cbs.centos.org/kojifiles/packages/protobuf/2.5.0/10.el7.centos/x86_64"
    protobuf_list = ["protobuf-2.5.0-10.el7.centos.x86_64.rpm", "protobuf-compiler-2.5.0-10.el7.centos.x86_64.rpm",
                     "protobuf-devel-2.5.0-10.el7.centos.x86_64.rpm", "protobuf-python-2.5.0-10.el7.centos.x86_64.rpm"]
    ret = ssh_connection.run_cmd("export PKG_CONFIG_PATH=$PKG_CONFIG_PATH;pkg-config --libs protobuf")
    if ret[0] == 1:
        ret = ssh_connection.run_cmd("rm -rf rpm_dir; mkdir -p rpm_dir")
        for item in protobuf_list:
            cmd = "wget -t 3 --directory-prefix={} {}".format(rpm_dir, os.path.join(url_link, item))
            log.debug("executing the command {} on {}".format(cmd, ssh_connection.ip))
            rc = ssh_connection.run_cmd(cmd)
            log.debug("ret {} {}".format(rc[0], rc))
            rc = ssh_connection.run_cmd("rpm -ivh {}".format(os.path.join(rpm_dir, item)))
            log.debug("ret {} {}".format(rc[0], rc))
    log.debug("ret {} {}".format(ret, ret[0]))


def install_pre_requisites(cont_obj, os_type, working_dir):
    """
    This function will install the prerequisites/dependencies libraries for the corresponding Linux distribution
    :param cont_obj:
    :param os_type: corresponding package handling utility will be selected.
           Ex: apt-get for Ubuntu
               yum for centos
    :param working_dir: is a directory where the downloaded google proto-buf related rpms are stored on the
           remote machine. Because some of the Linux distributions(like Centos7) does not
    """
    pkg_dict = dict()
    pkg_dict['ubuntu'] = ("apt-get", "dh-make dpkg-dev git g++ libprotoc-dev protobuf-compiler python-protobuf libdb-dev "
                                     "libbz2-dev libgdbm-dev libsqlite3-dev python-dev libboost1.55-all-dev libxml2-dev "
                                     "libsctp-dev lksctp-tools")
    pkg_dict['centos'] = ("yum", "git gcc bzip2-devel python-devel gcc-c++ libxml2-devel rpm-build"
                                 "boost-devel libdb-devel pexpect.noarch lksctp-tools lksctp-tools-devel"
                                 "gdbm-devel sqlite-devel rpm-build")
    os_type = os_type.lower()
    cmd = None
    if os_type in pkg_dict:
        pkg_utility, required_pkgs = pkg_dict.get(os_type)
        log.debug(" The following packages {} are going to installed on {} machine".format(required_pkgs, cont_obj.ip))
        cmd = "{0} -y install {1}".format(pkg_utility, required_pkgs)
    else:
        supported_oses = [item for item in pkg_dict.keys()]
        log.error("Unsupported os is provided. Supported oses are {}".format(supported_oses))

    if cmd:
        cont_obj.run_cmd(cmd, timeout=600)
        if os_type == "centos":
            proto_buf_rpm_installation(cont_obj, working_dir)


def test_binary_package(remote_ssh, remote_dir):
    log.info(" Testing the safplus binary package")
    model_dir = "/{}/eval/basic".format(remote_ssh.user)
    image_name = os.path.split(model_dir)[1] + ".zip"
    remote_ssh.run_cmd("rm -rf {0};rm -rf /{2}/{1}; rm -rf /{2}/basic;mkdir -p {0}".format(model_dir, image_name, remote_ssh.user))
    remote_ssh.run_cmd("cp -r {}/examples/eval/basic/* {}".format(remote_dir, model_dir))
    ret = remote_ssh.run("cd {0}; export SAFPLUS_DIR={1}; export BUILD_SAFPLUS=0; make".format(model_dir, remote_dir))
    cmd = 'cd {0}; /opt/safplus/7.0/sdk/src/mk/safplus_packager.py -x  "(cp setup* *.xml {{image_dir}})" -p {0} {1}'.format(model_dir, image_name)
    remote_ssh.run(cmd)
    top_dir = "/root" + os.sep + image_name.split(".zip")[0]
    model_bin = top_dir + os.sep + "bin"
    model_lib = top_dir + os.sep + "lib"
    remote_ssh.run_cmd("cd /root; unzip {}".format(os.path.join(model_dir, image_name)))
    remote_ssh.run_cmd("cd {2}; source setup_env.sh;cd {0}; python {1}/dbalpy.py -x {2}/SAFplusAmf1Node1SG1Comp.xml safplusAmf;./safplus_amf &".format(model_bin, model_lib, top_dir), 300)
    time.sleep(5)
    amf_pid = remote_ssh.get_pid("{}/safplus_amf".format(model_bin))
    if amf_pid != 0:
         log.debug("safplus_amf pid is {}".format(amf_pid))
         remote_ssh.run_cmd("{}/safplus_cleanup".format(model_bin))
    else:
        log.error("Testing of binary package failed")

def copy_and_test(remote_ssh, pkg_ver, tae_dir):
    global builder_bash, cus_test_file, cus_test_path
    remote_file = None
    remote_dir = "/opt/safplus/{}/sdk".format(pkg_ver)
    if cus_test_file is not None:
        remote_file = "{}/test/safplusTest1Node.xml".format(remote_dir)
        ret = remote_ssh.run_cmd("ls -l {}".format(remote_file))
        if ret[0] == 0:
            remote_ssh.run_cmd("/bin/cp {0} {0}.org".format(remote_file))
        remote_path = "{}@{}:{}".format(remote_ssh.user, remote_ssh.ip, remote_file)
        source_path = "{}/{}".format(cus_test_path, cus_test_file)
        #pdb.set_trace()
        builder_bash.scp(source_path, remote_path, remote_ssh.password)
    
    test_binary_package(remote_ssh, remote_dir)
    remote_ssh.run_cmd("cd {}/test;export TAE_DIR={};make clean; make test".format(remote_dir, tae_dir), timeout=6000)
    if cus_test_file is not None:
        remote_ssh.run_cmd("/bin/cp {0}.org {0}".format(remote_file))


def uninstall_safplus_pkg(cont_obj, pkg_format):
    pkg_query_cmd = []
    pkg_remove_cmd = []
    if pkg_format == "rpm":
        pkg_query_cmd.append("rpm -qa | grep -i safplus")
        pkg_query_cmd.append("rpm -qa | grep -i safplus-src")
        pkg_remove_cmd.append("rpm -evh safplus")
        pkg_remove_cmd.append("rpm -evh safplus-src")
    elif pkg_format == "deb":
        pkg_query_cmd.append("dpkg -l safplus")
        pkg_query_cmd.append("dpkg -l safplus-src")
        pkg_remove_cmd.append("dpkg -P safplus")
        pkg_remove_cmd.append("dpkg -P safplus-src")
    for query_cmd, rm_cmd in zip(pkg_query_cmd, pkg_remove_cmd):
        ret = execute_command(cont_obj, query_cmd, "saflus {} package Query failed".format(pkg_format),skip_error=True)
        if ret[0] == 0:
            execute_command(cont_obj, rm_cmd, "safplus {} package uninstall failed".format(pkg_format))


def generate_pkg(dist_dict, sources, source_tree_time_outs, pkg_ver, pkg_rel):
    vm = ClVirtualBox()
    vm.get_vm_object(dist_dict.name)
    builder = None
    if vm.vm_obj is not None:
        vm.get_vm_snap_shot_count()
        """if vm.vm_snap_count != 0:
            if vm.check_vm_running():
                vm.power_down_vm()
                vm.unlock_v_box_session()
            vm.check_and_lock_v_box_session()
            vm.restore_vm_state_from_snapshot()
            vm.unlock_v_box_session()
        """
        if not vm.check_vm_running():
            vm.start_virtual_machine()
        else:
            vm.create_vm_session()
        log.info("Virtual Machine'{}' state is {}".format(vm.vm_obj.name, vm.vm_obj.state))
        try:
            builder = SessionBash("{}_machine".format(vm.vm_obj.name), dist_dict.credentials.ip, dist_dict.credentials.user, dist_dict.credentials.password)
            builder.connect()
            execute_command(builder, "mkdir -p {0}; cd {0}".format(dist_dict.working_dir), "Creation of working directory failed")
            install_pre_requisites(builder, dist_dict.os, dist_dict.working_dir)
            source = dict()
            for (s, details) in sources.items():
                log.info("Populating %s" % s)
                d = dist_dict.working_dir + os.sep + s
                source[s] = source_tree.create(details.source, builder, d, source_tree_time_outs)
                source[s].populate()
            s_dir = dist_dict.working_dir + os.sep + "SAFplus"
            execute_command(builder, "cd {}; export PKG_VER={}; export PKG_REL={};make clean; make {}".format(s_dir, pkg_ver, pkg_rel,dist_dict.pkg_format),"{} package generation failed".format(dist_dict.pkg_format),18000)
            ret = execute_command(builder, "ls {}/build".format(s_dir)," package generation failed")
            log.debug(" Generated SAFplus {} packages are {}".format(dist_dict.pkg_format, remove_colour_coding(ret[1])))
            ret = execute_command(builder, "uname -m", "Failed to get machine type")
            arch = ret[1].strip()
            if dist_dict.pkg_format == "deb":
                if arch == "x86_64":
                    arch = "amd64"
                else:
                    arch = "i386"
            log.debug("Arch is {}".format(arch))
            pkg_name = []
            install_cmd = []
            if dist_dict.pkg_format == "rpm":
                dist_tag = execute_command(builder, "rpm --showrc | grep -w 'dist'", "failed to get the RPM dist tag")
                dist_str = remove_colour_coding(dist_tag[1])
                dist_str = dist_str.split("\t")[1].rstrip()
                log.debug("dist_str is {}".format(dist_str))
                pkg_name.append("{}/build/safplus-{}-{}{}.{}.{}".format(s_dir, pkg_ver, pkg_rel, dist_str, arch, dist_dict.pkg_format))
                pkg_name.append("{}/build/safplus-src-{}-{}{}.{}.{}".format(s_dir, pkg_ver, pkg_rel, dist_str, arch, dist_dict.pkg_format))
                install_cmd.append("rpm -ivh --replacefiles {}".format(pkg_name[0]))
                install_cmd.append("rpm -ivh --replacefiles {}".format(pkg_name[1]))
            elif dist_dict.pkg_format == "deb":
                pkg_ver_list = str(pkg_ver).split('.')
                if len(pkg_ver_list) == 1:
                    pkg_ver_list.insert(1, "0")
                pkg_name.append("{}/build/safplus_{}-{}-{}_{}.{}".format(s_dir, pkg_ver_list[0], pkg_ver_list[1], pkg_rel, arch, dist_dict.pkg_format))
                pkg_name.append("{}/build/safplus-src_{}-{}-{}_{}.{}".format(s_dir, pkg_ver_list[0], pkg_ver_list[1], pkg_rel, arch, dist_dict.pkg_format))
                install_cmd.append("dpkg -i {}".format(pkg_name[0]))
                install_cmd.append("dpkg -i {}".format(pkg_name[1]))
            log.debug("Generated package name is {}".format(pkg_name))
            uninstall_safplus_pkg(builder, dist_dict.pkg_format)
            for pkg_ins_cmd in install_cmd:
                execute_command(builder, pkg_ins_cmd, "Failed to install the {}".format(dist_dict.pkg_format))
            copy_and_test(builder, pkg_ver, "{}/TAE".format(dist_dict.working_dir))
            builder.run_cmd("cd {}; rm -rf log log_* stamps_* tae_run_*".format("/opt/safplus/7.0/sdk/test"))
        except PkggenException, msg:
            log.error("SAFplus package generation due to {}".format(msg.error_msg))
        except Exception as e:
            log.error(" error is {}".format(e))
        if builder is not None:
            builder.finalize()
        #vm.power_down_vm()
        vm.unlock_vm_session()

    else:
        print "The Given Virtual Machine is not registered with Virtual box ruuning under {} user".format(getpass.getuser())
        print "Possible reasons are Invalid Virtual Machine Name \n" \
              " or Check under which user account, the Given virtual Machine is registered with virtual box"


def usage():
    """ print the usage and supported options of the script.
    """
    prog_name = os.path.split(sys.argv[0])[1]
    print"""
The {0} script generates a SAFplus RPM/DEBIAN packages for the linux distribution details provided in the packager xml file.
{0} script runs test cases on the generated SAFPlus packages in the remote machine and upload the test results to the report server
If the installed safplus source code directory path is different than the default safplus code directory path in the
src/test/safplusTest1Node.xml file, you can use -o option for override/customise the deafult safplusTest1Node.xml file""".format(prog_name)
    print"""
Syntax {0} -S <packager xml file> [-C <customize test xml file>]""".format(prog_name)
    print"""
Options:
-S or --script=<file>
         Use a multi-distribution packger xml file.
-C or --customise=<xml file>
         For customise/overwrites the default safplusTest1Node.xml present in the src/test
         and execute the tests  based on the new safplusTest1Node.xml
    """


def get_option_value(arg_val):

    if arg_val and arg_val.startswith("="):
        arg_val = arg_val[1:]
    return arg_val


builder_bash = None
cus_test_path = None
cus_test_file = None
log.level = DEBUG
log.increase_verbosity()

if __name__ == '__main__':
    cfgFile = None
    try:
        opts, args = getopt.getopt(sys.argv[1:], "hS:C:", ["help", "script=", "customise="])
    except getopt.GetoptError as err:
        log.error("{}".format(err))
        usage()
        sys.exit(0)
    if not opts and not args:
        log.error("Unrecognised options to the script")
        usage()
        sys.exit(0)

    for opt, arg in opts:
        if opt in ("-h", "--help"):
            usage()
            sys.exit(0)
        elif opt in ("-S", "--script"):
            cfgFile = get_option_value(arg)
            log.info("Given script file name is {}".format(cfgFile))
        elif opt in ("-C", "--customise"):
            cus_test_file = get_option_value(arg)
    if cus_test_file is not None:
        cus_test_path = os.path.split(cus_test_file)[0]
        if not cus_test_path:
            cus_test_path = os.getcwd()
        cus_test_path = os.path.abspath(cus_test_path)
        log.info("Customise test xml directory is {}".format(cus_test_path))
        cus_test_file = os.path.split(cus_test_file)[1]
        log.info("Customise test xml file name is {}".format(cus_test_file))
    try:
        cfg = xml2dict.decode_file(cfgFile, '', None)  # we accept any XML first
    except xml2dict.BadConfigFile:
        log.critical('Config file [%s] not valid (either not XML or not a valid XML)' % cfgFile)
        log.critical('If this file is a testcase filter file mistakenly treated as a TAE')
        log.critical('config file, you may consider renaming it (not using .cfg extension)')
        raise
    root_tag = cfg.keys()[0]  # since it's XML, there is always one root tag
    log.info("Root tag is {}".format(root_tag))
    #sys.exit(0)
    if cfg[root_tag].ver != ('%d.%d.%d.%d' % CFG_VER):
        log.critical('Config file version mismatch. Expecting %d.%d.%d.%d' % CFG_VER)
        sys.exit(1)
    cfg = cfg.packager
    log.debug("Generating The SAFplus RPM/Debian package for Version {} Release {}".format(cfg.pkg_version, cfg.pkg_release))
    log.debug("Content are {}".format(cfg))
    builder_bash = SessionBash("builder", cfg.builder.ip, cfg.builder.user, cfg.builder.password)
    builder_bash.connect()
    log.debug("Distributions are {} length {}".format(cfg.distributions, len(cfg.distributions)))
    for key, value in cfg.distributions.items():
        log.debug("Vm Name {} ip Address {} user {} pkg_format {}".format(value.name, value.credentials.ip, value.credentials.user, value.pkg_format))
        generate_pkg(value, cfg.source, cfg.env_cfg, cfg.pkg_version, cfg.pkg_release)

    builder_bash.finalize()
