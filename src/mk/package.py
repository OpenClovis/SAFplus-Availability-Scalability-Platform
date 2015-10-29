#!/usr/bin/env python
import os
import shutil
import tempfile
import subprocess
import time
import datetime
import re
import tarfile
import zipfile
import platform
import safplus_packager


def exe_shell_cmd(cmd):
    child = subprocess.Popen(cmd, shell=True, close_fds=True)
    while True:
        pid, sts = os.waitpid(child.pid, os.WNOHANG)
        if pid == child.pid:
            break
        else:
            time.sleep(0.00001)

    del child


def check_file_exists(file_path, skip=False):
    """ This function will check whether the given file available or not
    """
    if not os.path.exists(file_path) and not skip:
        safplus_packager.fail_and_exit(" Package generation failed due to non existence of {}".format(os.path.split(file_path)[1]))


class RPM:
    """ Class for creating a package for RPM package manager
    """

    def __init__(self, package_ver=1.0, prefix_loc="opt/safplus/target"):
        """ Preparing the environment for RPM package generation
            like creating the directory structure and
            exporting the some environment variables for building the
        """
        log.debug("Package Version is {}".format(package_ver))
        self.user_home_dir = os.path.expanduser('~')
        self.package_name = None
        self.package_ver = package_ver
        self.prefix_loc = prefix_loc
        self.archive_name = None
        self.rpm_spec_file_name = None
        self.rpm_build_top_dir = self.user_home_dir + os.path.sep + "rpmbuild"
        self.rpm_sub_dirs = ["BUILD", "BUILDROOT", "SPECS", "RPMS", "SRPMS", "SOURCES"]
        self.rpm_src_dir = self.rpm_build_top_dir + os.path.sep + "SOURCES"
        self.rpm_build_dir = self.rpm_build_top_dir + os.path.sep + "BUILD"
        self.rpm_specs_dir = self.rpm_build_top_dir + os.path.sep + "SPECS"
        self.rpm_rpm_dir = self.rpm_build_top_dir + os.path.sep + "RPMS"
        self.rpm_srpm_dir = self.rpm_build_top_dir + os.path.sep + "SRPMS"
        self.rpm_build_root_dir = self.rpm_build_top_dir + os.path.sep + "BUILDROOTS"
        self.create_rpm_dir_struct()

    def customise_spec_file(self):
        """
        Adding the package name and prefix location in the rpm spec file
        """
        d = dict()
        d["Name"] = "{}".format(self.package_name)
        d["Version"] = "{}".format(self.package_ver)
        d["Source0"] = "{}".format(self.archive_name)
        d["Prefix"] = "{}".format(self.prefix_loc)
        fh, abs_path = tempfile.mkstemp()
        log.debug("{}".format(d))
        with open(abs_path, 'w') as new_file:
            with open(os.path.join(self.rpm_specs_dir, self.rpm_spec_file_name), 'r') as old_file:
                for line in old_file:
                    key = line.split(":")[0]
                    if key in d:
                        line = key + ":\t" + d.get(key) + "\n"
                    new_file.write(line)
        os.close(fh)

        shutil.move(abs_path, os.path.join(self.rpm_specs_dir, self.rpm_spec_file_name))

    def rpm_build(self, archive_name, rpm_pkg_template_dir, rpm_makefile_name, rpm_spec_file_name):
        """
        :param archive_name: name of the archive including the full path
        :param rpm_pkg_template_dir: path to the rpm template directory
        :param rpm_makefile_name: makefile used to copy the generated binaries to the user given prefix location
        :param rpm_spec_file_name: useful for building the rpm to the given package
        :return:
        """
        self.archive_name = os.path.split(archive_name)[1]
        self.package_name = self.archive_name.split(".")[0]
        self.rpm_spec_file_name = os.path.split(rpm_spec_file_name)[1]
        log.debug("RPM SPEC file name is {}".format(self.rpm_spec_file_name))
        if os.path.exists(rpm_pkg_template_dir) and os.path.exists(os.path.join(rpm_pkg_template_dir, rpm_makefile_name)) and os.path.exists(os.path.join(rpm_pkg_template_dir, rpm_spec_file_name)):
            shutil.copy2(archive_name,  self.rpm_src_dir)
            shutil.copy2(os.path.join(rpm_pkg_template_dir, rpm_makefile_name), self.rpm_build_dir)
            shutil.copy2(os.path.join(rpm_pkg_template_dir, rpm_spec_file_name), self.rpm_specs_dir)
            self.customise_spec_file()
            exe_shell_cmd("rpmbuild -bb {}".format(os.path.join(self.rpm_specs_dir, self.rpm_spec_file_name)))
            rpm_file_list = safplus_packager.file_list(self.rpm_rpm_dir + os.sep + platform.machine(), "*.rpm")
            for rpm_file in rpm_file_list:
                shutil.copy2(rpm_file, os.path.split(archive_name)[0]) 
        else:
            log.error("Provide correct details like rpm_template dir or Makefile or rpm spec file location")

    def create_rpm_dir_struct(self):
        """ Creating a directory structure for rpm build
        """

        if os.path.exists(self.rpm_build_top_dir):
            log.info("RPM BUILD DIR is ALREADY existed ")
            shutil.rmtree(self.rpm_build_top_dir)
        log.info("Creating the TOP Directory for rpm build")
        os.makedirs(self.rpm_build_top_dir)
        #creating sub-directories like BUILD, BUILDROOOT, RPMS, SRPMS and SPECS in the TOP Directory
        for dir_name in self.rpm_sub_dirs:
            log.debug("dir_name is {}".format(dir_name))
            os.makedirs(os.path.join(self.rpm_build_top_dir, dir_name))

    def show(self):
        log.info("User_home_dir is {}".format(self.user_home_dir))
        log.info("Package Name is {}".format(self.package_name))
        log.info("Package Version is {}".format(self.package_ver))
        log.info("RPM TOP Dir is {}".format(self.rpm_build_top_dir))


class DEBIAN:
    """ DEBIAN class for creating a .deb package for DEBIAN based package management systems
    """
    def __init__(self, prefix_loc="/opt/safplus/target", release_number=1, package_ver="1.0"):
        """ Preparing the environment for Debian package generation like
            creation of top directory for debian package generation
        """
        self.user_home_dir = os.path.expanduser('~')
        self.prefix_loc = prefix_loc
        self.release_number = release_number
        self.build_top_dir = self.user_home_dir + os.sep + "deb_build"
        if os.path.exists(self.build_top_dir):
            print " {} is already existed".format(self.build_top_dir)
            shutil.rmtree(self.build_top_dir)
        os.mkdir(self.build_top_dir)
        self.pkg_name = None
        self.pkg_version = package_ver
        self.req_archive_name = None
        self.req_dir_name = None
        self.deb_dir = None
        self.deb_control_file = None
        self.deb_postrm_file = None
        self.deb_rule_file = None
        self.deb_change_log_file = None
        self.deb_makefile = None

    def prepare_env(self, archive_file, machine_type):
        """ Rename the archive name to <package_name>_<version> format ( SAFplus.tgz -> SAFplus_1.0.tgz)
            extract the archive, rename the package directory to <package_name>-<version>, copy the required configuration
            files into <package_name>-<version>/debian.
        """
        archive_name, compress_format, pkg_suffix = safplus_packager.get_compression_format(os.path.split(archive_file)[1])
        log.debug("Archive name is {}".format(archive_name))
        log.debug("Archive compress format is {}".format(compress_format))
        log.debug("Archive extension is {}".format(pkg_suffix))
        mach_type_arch = False
        if len(machine_type.split("-")) > 1:
            machine_type = machine_type.split("-")[0]
        if machine_type in archive_name:
            archive_name = archive_name.split("_{}".format(machine_type))[0]
            mach_type_arch = True
        self.pkg_name = archive_name.split("_")[0].lower()
        pkg_ver = self.pkg_version
        if len(archive_name.split("_")) > 1:
            self.pkg_version = archive_name.split("_")[1]
        if not re.match(r'[^a-zA-Z]\d*.\d*', self.pkg_version):
            self.pkg_version = pkg_ver
        log.debug("PKG NAME:{} VERSION:{}".format(self.pkg_name, self.pkg_version))
        self.req_archive_name = self.build_top_dir + os.sep + self.pkg_name + "_" + self.pkg_version + pkg_suffix
        log.debug("Required archive name format:{}".format(self.req_archive_name))
        shutil.copy2(archive_file, self.req_archive_name)
        self.extract_archive(compress_format)
        #Rename the extracted directory to <package_name>-<package_version>
        self.req_dir_name = self.build_top_dir + os.sep + self.pkg_name + "-" + self.pkg_version
        log.debug("Required archive directory:{}".format(self.req_dir_name))
        if mach_type_arch:
            shutil.move(self.build_top_dir + os.sep + archive_name + "_" + machine_type, self.req_dir_name)
        else:
            shutil.move(self.build_top_dir + os.sep + archive_name, self.req_dir_name)
        self.deb_dir = self.req_dir_name + os.sep + "debian"
        os.mkdir(self.deb_dir)
        log.debug("Debian directory {}".format(self.deb_dir))

    def extract_archive(self, compress_format="gztar"):
        """ extract_archive function the uncompress the archive file into the
            debian build directory
        """
        archive_mode = {'tar': 'r', 'gztar': 'r:gz', 'bztar': 'r:bz2'}
        if compress_format != 'zip':
            t = tarfile.open(self.req_archive_name, archive_mode.get(compress_format))
            t.extractall(self.build_top_dir + os.sep)
            t.close()
        else:
            z = zipfile.ZipFile(self.req_archive_name)
            z.extractall(self.build_top_dir)
            z.close()

    def check_req_deb_files(self):
        self.deb_control_file = self.deb_dir + os.sep + "control"
        check_file_exists(self.deb_control_file)
        self.deb_postrm_file = self.deb_dir + os.sep + "postrm"
        check_file_exists(self.deb_postrm_file, True)
        self.deb_rule_file = self.deb_dir + os.sep + "rules"
        check_file_exists(self.deb_rule_file)
        self.deb_change_log_file = self.deb_dir + os.sep + "changelog"
        check_file_exists(self.deb_change_log_file)
        self.deb_makefile = self.deb_dir + os.sep + "Makefile"
        check_file_exists(self.deb_makefile)

    def deb_build(self, archive_file, deb_pkg_template_dir, machine_type=None, third_party_dir=None):
        """
        :param archive_file: name of the archive including the full path
        :param deb_pkg_template_dir: path to the debian template directory
        :param machine_type: target machine type
        :parm kernel_version: target kerenel version
        :param third_party_dir: path to the third party libraries like boost
        """

        if machine_type is None:
            machine_type = platform.machine()
        if third_party_dir is None:
            # Need to fix the third party library location
            # if we are using the distribution provided libraries, deb_build may fail
            third_party_dir = os.path.abspath(os.path.dirname(__file__) + os.sep + "../../target/{}/install".format(machine_type))
        log.debug("Archive file is {}".format(archive_file))
        self.prepare_env(archive_file, machine_type)
        if os.path.exists(deb_pkg_template_dir):
            safplus_packager.copy_dir(deb_pkg_template_dir, self.deb_dir)
            self.check_req_deb_files()
            self.customise_deb_files(archive_file, machine_type, third_party_dir)
        else:
            print "Provide correct details like deb_template dir {}".format(deb_pkg_template_dir)
        pass

    def customise_postrm(self):
        """ Adding the prefix install location to the postrm script which will be useful in deleting the
            package from the target file system
        """
        fh, abs_path = tempfile.mkstemp()
        with open(abs_path, 'w') as new_file:
            new_file.write("#!/bin/sh\n")
            new_file.write("prefix={}\n".format(self.prefix_loc))
            with open(self.deb_postrm_file, 'r') as old_file:
                for line in old_file.readlines():
                    new_file.write(line)

        os.close(fh)
        shutil.move(abs_path, self.deb_postrm_file)
        os.chmod(self.deb_postrm_file, 0o755)

    def customise_control_file(self, machine_type):
        """ This function will add the source, package name and machine type to the control file
            for building the debian package generation
        """
        d = dict()
        d["Source"] = "{}".format(self.pkg_name)
        d["Package"] = "{}".format(self.pkg_name)
        if machine_type == "x86_64":
            d["Architecture"] = "amd64"
        else:
            d['Architecture'] = "i386"
        fh, abs_path = tempfile.mkstemp()
        with open(abs_path, 'w') as new_file:
            with open(self.deb_control_file, 'r') as old_file:
                for line in old_file:
                    key = line.split(":")[0]
                    if key in d:
                        line = key + ": " + d.get(key) + "\n"
                    new_file.write(line)
        os.close(fh)
        shutil.move(abs_path, self.deb_control_file)

    def customise_rules_file(self, third_party_dir):
        fh, abs_path = tempfile.mkstemp()
        with open(abs_path, 'w') as new_file:
            new_file.write("#!/usr/bin/make -f\n")
            new_file.write("export PACKAGENAME ?={}\n".format(self.pkg_name))
            new_file.write("export PREFIX ?= {}\n".format(self.prefix_loc))
            new_file.write("export DESTDIR ?= {}\n".format(self.deb_dir + os.sep + self.pkg_name))
            new_file.write("export THIRDPARTY_DIR ?= {}\n".format(third_party_dir))
            with open(self.deb_rule_file, 'r') as old_file:
                for line in old_file:
                    new_file.write(line)
        os.close(fh)
        shutil.move(abs_path, self.deb_rule_file)
        os.chmod(self.deb_rule_file, 0o755)

    def customise_changelog_file(self):
        """This function adds the package name version number, release number and date of modification
           to the changelog file
        """
        pkg_version = self.pkg_version.split(".")
        str_buf = "{} ({}-{}-{})".format(self.pkg_name, pkg_version[0], pkg_version[1], self.release_number)
        f = open(self.deb_change_log_file, 'r')
        for line in f.readlines():
            str_buf = str_buf + str(line)
        f.close()
        #Need to fix the adding of timestamp to changelog file
        #Currently This logic is adding the timestamp to end of the file.
        #For future package genration with different versions it will be a problem
        str_buf = str_buf[0:-2] + "{}\n".format(datetime.datetime.now().strftime(" %a, %d %b %Y %H:%M:%S -%f")[:-2])
        f = open(self.deb_change_log_file, 'w')
        f.write(str_buf)
        f.close()

    def customise_deb_files(self, archive_file, machine_type, third_party_dir):
        shutil.move(self.deb_makefile, self.req_dir_name)
        self.customise_control_file(machine_type)
        self.customise_rules_file(third_party_dir)
        self.customise_changelog_file()
        self.customise_postrm()
        os.chdir(self.req_dir_name)
        exe_shell_cmd("dpkg-buildpackage -us -uc -b")
        deb_file_list = safplus_packager.file_list(self.build_top_dir, "*.deb")
        for deb_file in deb_file_list:
            shutil.copy2(deb_file, os.path.split(archive_file)[0])


def test_deb():
    log.debug("Hello")
    deb_pack = DEBIAN()
    deb_pack.deb_build("/home/openclovis/gitcodebase/openclovis_install/s7/src/mk//SAFplus.tar.gz",
                       "/home/openclovis/gitcodebase/openclovis_install/s7/src/mk/pkg_templates/deb/")


def test_rpm():
    log.debug("Hello")
    rpm_pack = RPM()
    rpm_pack.show()
    rpm_pack.rpm_build("/home/openclovis/gitcodebase/openclovis_install/SAFplus/src/mk/SAFplus.tar.gz",
                       "/home/openclovis/gitcodebase/openclovis_install/SAFplus/src/mk/pkg_templates/rpm/",
                       "Makefile", "package.spec")


log = safplus_packager.log
if __name__ == "__main__":
    test_deb()
    test_rpm()
