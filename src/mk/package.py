#!/usr/bin/env python
import os
import shutil
import tempfile
import subprocess
import time
from safplus_packager import log

def exe_shell_cmd(cmd):
    child = subprocess.Popen(cmd, shell=True, close_fds=True)
    while True:
        pid, sts = os.waitpid(child.pid, os.WNOHANG)
        if pid == child.pid:
            break
        else:
            time.sleep(0.00001)

    del child

class RPM:
    """ Class for creating a package for RPM package manager
    """

    def __init__(self, package_ver=1.0, prefix_loc="root/safplus"):
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
        d = {}
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
            shutil.copy2(os.path.join(rpm_pkg_template_dir, rpm_makefile_name), self.rpm_build_dir  )
            shutil.copy2(os.path.join(rpm_pkg_template_dir, rpm_spec_file_name), self.rpm_specs_dir)
            self.customise_spec_file()
            exe_shell_cmd("rpmbuild -bb {}".format(os.path.join(self.rpm_specs_dir, self.rpm_spec_file_name)))
        else:
            log.error("Provide correct details like rpm_template dir or Makefile or rpm spec file location")
        #os.chdir(self.rpm_build_top_dir)
        

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


if __name__ == "__main__":
    test()   

def test():
    log.debug("Hello")
    rpm_pack = RPM()
    rpm_pack.show()
    rpm_pack.rpm_build("/home/openclovis/gitcodebase/openclovis_install/SAFplus/src/mk/SAFplus.tar.gz",
                       "/home/openclovis/gitcodebase/openclovis_install/SAFplus/src/mk/pkg_templates/rpm/",
                       "Makefile", "package.spec")
