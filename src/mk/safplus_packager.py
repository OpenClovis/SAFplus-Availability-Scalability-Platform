#!/usr/bin/env python3
import os
import sys
import shutil
import platform
import glob
import logging
import re
import getopt
import errno
import subprocess
import pdb
from xml.dom import minidom

def parseXML(imagesConfig, targetName = ""):
    dom = minidom.parse(imagesConfig);
    dictConfigure = {"slot" : "", "netInterface" : "", "name" : ""}
    for n in dom.childNodes:
        for n1 in n.childNodes:
            if n1.nodeType != n1.TEXT_NODE:
                if n1.tagName == targetName:
                    slot = n1.getElementsByTagName("slot")[0]
                    netInterface = n1.getElementsByTagName("netInterface")[0]
                    dictConfigure["slot"] = slot.firstChild.data
                    dictConfigure["netInterface"] = netInterface.firstChild.data
                    dictConfigure["name"] = n1.tagName
    return dictConfigure

def updateSetupFile(dictConfigure, tarGet = ""):
    if tarGet == "" or not check_dir_exists(tarGet):
        return ""
    s = open(tarGet).read()
    if dictConfigure['name'] != "":
        s = s.replace('ASP_NODENAME=node0', 'ASP_NODENAME=%s' % dictConfigure['name'] )
    if dictConfigure['netInterface'] != "":
        s = s.replace('SAFPLUS_BACKPLANE_INTERFACE=eth0', 'SAFPLUS_BACKPLANE_INTERFACE=%s' % dictConfigure['netInterface'] )
    if dictConfigure['slot'] != "Payload":
        s = s.replace('export SAFPLUS_SYSTEM_CONTROLLER=0', '')
    f = open(tarGet, 'w')
    f.write(s)
    f.close()
    return ""

def createArchiveByCommand(tar_dir, tarFileName):
    """ Create an archive with a given archive name and archive format for the provided directory
    """
    if check_dir_exists(os.path.join(tar_dir, tarFileName)):
        cmd = 'cd %s; tar -zcf %s.tar.gz %s' % (tar_dir, tarFileName, tarFileName)
        try:
            log.info("Creating tarball: %s" % os.path.join(tar_dir, tarFileName) + ".tar.gz")
            ret = subprocess.call(cmd, shell=True)
        except Exception as e:
            pdb.set_trace
    return os.path.join(tar_dir, tarFileName) + ".tar.gz"

def file_list(dir_name, pattern='*'):
    """ Returns a list of file names having same extension present in the directory based on the pattern.
        Default this function returns a list of files present in the directory.
    """
    file_names_list = []
    if check_dir_exists(dir_name):
        pattern = "{}/{}".format(dir_name, pattern)
        file_names_list = glob.glob(pattern)
        for file_name in file_names_list:
            if os.path.isdir(file_name):
                file_names_list.remove(file_name)

    return file_names_list


def fail_and_exit(errmsg):
    """ log the error message and exit from the script.
    """
    log.error("{}".format(errmsg))
    sys.exit(-1)


def check_dir_exists(dir_name):
    """ Return true if the given dir_name existed in the file system otherwise it will return false
    """
    return os.path.exists(dir_name)


def unusedcreatedir(dir_name):
    try:
        os.mkdir(dir_name, 0o755)
    except OSError as e:
        if e.errno != errno.EEXIST:
            raise
            fail_and_exit(" Failed to create the dir {}".format(dir_name))


def check_and_createdir(dir_name):
    if not check_dir_exists(dir_name):
        os.makedirs(dir_name)
        #createdir(dir_name)
    return True


def log_init():
    """ log_init() initializes a log object to standard output with required format
        and returns a logging object
    """
    logging.basicConfig(filemode='a', format='%(asctime)s %(levelname)s: %(message)s',
                        datefmt='%H:%M:%S', level=logging.DEBUG)
    return logging.getLogger("UPDATE REPO")


# make_archive() is available from python 2.7 version onwards
def create_archive(tar_name, ext, tar_dir, arch_format='gztar'):
    """ Create an archive with a given archive name and archive format for the provided directory
    """
    #log.info("NAM = {}".format(tar_name))
    #log.info("EXT = {}".format(ext))
    #log.info("DIR = {}".format(tar_dir))
    if check_dir_exists(tar_dir):
        gen_tar_name = shutil.make_archive(tar_name, arch_format, tar_dir)
    # Rename this archive if create_archive gave us the wrong name
        if gen_tar_name != tar_name + ext:
          os.rename(gen_tar_name, tar_name + ext)
        log.info("Archive {} generated successfully".format(tar_name + ext))
    else:
        fail_and_exit("Archive directory {} does not exist".format(tar_dir))
    return tar_name

def copy_dir(src, dst, recursion=0):
    """This function will recursively copy the files and sub-directories from src directory to destination directory
       suppose the sub-directory in source directory contains only object and  header( like .h .hxx .hpp .ipp) files
       copy_dir() skips the creation of sub-directory in the destination directory
    """
    if not check_dir_exists(dst):
        files_list = file_list(src)
        p = re.compile(r'(\w+)\.(h\w+|i\w+|o)')
        obj_header_files_count = len([e for e in files_list if p.search(e)])
        if len(files_list) != 0 and len(files_list) == obj_header_files_count:
            return
        os.makedirs(dst)
    # log.debug(recursion*" " + src)
    for item in os.listdir(src):
        s = os.path.join(src, item)
        d = os.path.join(dst, item)
        if os.path.isdir(s):
            copy_dir(s, d,recursion+3)
        else:
            if not os.path.exists(d) or os.stat(src).st_mtime - os.stat(dst).st_mtime > 1:
                # Below regular expression skips the copying of object files and header files
                # into the destination directory
                if not re.search(r'(\w+)\.(h\w+|i\w+|o)', s):
                    #log.debug((recursion+1)*" " + s + " -> " + d)
                    shutil.copy(s, d)
                    # mark any python or shell scripts as executable
                    if re.search(r'(\w+).(py|sh)', d):
                        os.chmod(d, 0o755)


def get_compression_format(archive_name):
    """ Returns archive_name without extension and compression method from the archive_name
    """
    default_compress_format = {'tar': 'tar', 'gz': 'gztar', 'bz2': 'bztar', 'tgz': 'gztar', 'zip': 'zip'}
    compress_format = "gztar"
    sub_strs  = archive_name.split('.')
    compress_extension = sub_strs[-1]
    index = None
    archive_suffix = ".tar.gz"
    if compress_extension in default_compress_format:
        compress_format = default_compress_format.get(compress_extension)
        if compress_extension != "tar":
            try:
                index = sub_strs.index("tar")
            except ValueError:
                pass

        if index is None:
            archive_suffix = "."+ compress_extension
        else:
            archive_suffix = ".tar." + compress_extension
        # log.debug("Archive suffix is {}".format(archive_suffix))
        archive_name = archive_name.split(archive_suffix)[0]
    return archive_name, compress_format, archive_suffix


def package_dirs(target_dir, tar_dir, yum_package, debian_package):
    """ package_dirs is a helper function which will copy the directories/files from source location to destination
        location based on the directories name. This function will identify the directory names like (lib or plugin,
        bin or sbin, test and share).
    """
    if check_dir_exists(target_dir):
        for dir_name, subdir_list, filename_list in os.walk(target_dir):
            if yum_package or debian_package:
                if os.path.relpath(dir_name, target_dir).startswith("install"):
                    #SKip the copying of 3rdparty utilities into the tarball if the script is invoked with -y or -d flags
                    continue
            if os.path.basename(dir_name) == 'lib':
                tar_lib_dir = tar_dir+"/lib"
                copy_dir(dir_name, tar_lib_dir)
            elif os.path.basename(dir_name) == 'plugin':
                tar_plugin_dir = tar_dir+"/plugin"
                copy_dir(dir_name, tar_plugin_dir)
            elif os.path.basename(dir_name) == 'bin' or os.path.basename(dir_name) == 'sbin':
                tar_bin_dir = tar_dir+"/bin"
                copy_dir(dir_name, tar_bin_dir)
            elif os.path.basename(dir_name) == 'test':
                tar_test_dir = tar_dir + "/test"
                copy_dir(dir_name, tar_test_dir)
            elif os.path.basename(dir_name) == "share":
                tar_share_dir = tar_dir+'/share'
                copy_dir(dir_name, tar_share_dir)
            elif os.path.basename(dir_name) == "etc":
                tar_share_dir = tar_dir+'/etc'
                copy_dir(dir_name, tar_share_dir)
            else:
                pass
    else:
        log.info(" {} does not exist".format(target_dir))


def get_image_dir_path(tar_name):
    """ Return the image directory path from the tar_name.
        Default this function will the current working as a image directory if it failed to extract the path
        from tar_name.
    """
    image_dir_path = os.path.split(tar_name)[0]

    if not image_dir_path:
        image_dir_path = os.getcwd()
    image_dir_path = os.path.abspath(image_dir_path)
    # redundant with later log: log.info("Image directory is {}/image".format(image_dir_path))
    return image_dir_path


def get_image_file_name(tar_name):
    image_file_name = os.path.split(tar_name)[1]
    if not image_file_name:
        image_file_name = tar_name
    # log.debug("Image File Name is {}".format(image_file_name))
    return image_file_name


def package(base_dir, tar_name, prefix_dir, machine=None, pre_build_dir=None,execute=None, yum_package = False, debain_package = False, pkg_ver = 1.0, pkg_rel = 1):
    """ This function packages the model related binaries, libraries, test examples and 3rd party utilities
       into an archive to the given target platform.
    """
    if base_dir:
        log.info("SAFPlus Model directory is {}".format(base_dir))

    # Break the output file name into its components so we can ...
    image_dir_path = get_image_dir_path(tar_name)
    modelDir = os.path.normpath("{}/../".format(image_dir_path))

    image_dir = "{}/images".format(modelDir)

    tar_name = get_image_file_name(tar_name)
    tar_name, compress_format, archive_suffix = get_compression_format(tar_name)
    #image_stage_dir = image_dir + os.sep + os.path.splitext(get_image_file_name(tar_name))[0]
    image_stage_dir = image_dir + os.sep + tar_name

    # Blow away the old staging directory if it exists and recreate it
    #if check_dir_exists(image_dir):
    #    shutil.rmtree(image_dir)
        # There's no need to copy it somewhere else... nobody needs it.
        #log.info("Backing up the already existed image directory")
        #image_backup_dir = "{}/images_backup".format(image_dir_path)
        #if check_dir_exists(image_backup_dir):
        #    shutil.rmtree(image_backup_dir)
        #shutil.move(image_dir, image_backup_dir)
    if not os.access(image_stage_dir, os.F_OK):
        os.makedirs(image_stage_dir)
   
        

    # If the user does not supply the crossbuild into, assume the local machine -- so get the local machine's data
    if not machine:
        machine = get_target_machine_type() # target processor compiler type, e.g. x86_64-linux-gnu, i686-linux-gnu

    # Print out what was discovered so user can visually verify.
    # log.info("Target platform machine type:{}".format(machine))
    # log.info("Target platform image directory is {}".format(image_dir))
    if yum_package:
        log.info("Packaging the Model/SAFplus in RPM for {} version {} release".format(pkg_ver, pkg_rel))
    if debain_package:
        log.info("Packaging the Model/SAFplus in DEBIAN for {} version {} release".format(pkg_ver, pkg_rel))
    #tar_dir = "{}/{}".format(image_dir, tar_name)
    #check_and_createdir(tar_dir)
    # log.info("Packaging files from {0} and {1}".format(pre_build_dir,base_dir));

    if pre_build_dir:  # Create the actual prebuilt dir by combining what the user passed with the given machine type.
        target_dir = "{0}/target/{1}".format(pre_build_dir, machine)
        if check_dir_exists(target_dir):
            log.info("SAFPlus binaries, libraries and third party utilities related to target platform are present in {}".format(target_dir))
            package_dirs(target_dir, image_stage_dir, yum_package, debain_package)
        else:
            fail_and_exit("Specified {} does not exists".format(target_dir))
    
    #target_dir = "{0}/target/bin".format(image_dir_path + "/..")
    target_dir = "{0}/bin".format(target_dir)
    if check_dir_exists(target_dir):
        log.info("Components' binaries presented in {}".format(target_dir))
        package_dirs(target_dir, image_stage_dir, yum_package, debain_package)
    else:
        fail_and_exit("Specified {} does not exists".format(target_dir))

    if base_dir:
        target_dir = "{0}/target/{1}".format(base_dir, machine)
        if check_dir_exists(target_dir):
            log.info("Model related binaries, libraries and  utilities related to target platform are present in {}".format(target_dir))
            package_dirs(target_dir, image_stage_dir, yum_package, debain_package)
        else:
            fail_and_exit("Specified {} does not exists".format(target_dir))

    # log.info("Packaging complete")
    if execute:
      tmp = execute.format(image_dir=image_stage_dir)
      log.info("Executing user supplied script: %s" % tmp)
      try:
        ret = subprocess.call(tmp, shell=True)
      except Exception as e:
        pdb.set_trace()
    #   log.info("script execution complete, returned %d", ret)

    imagesConfig = os.path.join(modelDir+'/configs', "imagesConfig.xml")
    if check_dir_exists(imagesConfig):
        # read information from imagesConfig.xml
        dictConfigure = parseXML(imagesConfig, tar_name)
        setupFile = os.path.join(image_dir, tar_name, "bin") + "/setup"
        # modify information follow imagesConfig.xml
        updateSetupFile(dictConfigure, setupFile)

    # log.info("Archive name is {0} Archive compression format is {1}".format(tar_name, compress_format))
    # put the tarball exactly where the requested on the command line: tar_name = os.path.join(image_dir, tar_name)
    tarFileName = tar_name
    tar_name = os.path.join(image_dir, tar_name)
    #image_dir = "{}/../images".format(image_dir_path)
    # gen_tar_name = create_archive(tar_name, archive_suffix, image_dir , compress_format)
    gen_tar_name = createArchiveByCommand(image_dir, tarFileName)
    log.info("Archive {} generated successfully".format(gen_tar_name))

    # select the corresponding package generation class method from the package module
    if yum_package:
        from package import RPM
        rpm_gen = RPM(prefix_dir, pkg_ver, pkg_rel)
        rpm_template_dir = os.path.abspath(os.path.dirname(__file__) + os.sep + "pkg_templates/rpm")
        rpm_gen.rpm_build(tar_name, rpm_template_dir, "Makefile", "package.spec")
    if debain_package:
        from package import DEBIAN
        deb_gen = DEBIAN(prefix_dir, pkg_ver, pkg_rel)
        deb_template_dir = os.path.abspath(os.path.dirname(__file__) + os.sep + "pkg_templates/deb")
        deb_gen.deb_build(tar_name, deb_template_dir, machine)

def get_target_machine_type():
    global target_mch_compiler_type
    if target_mch_compiler_type is None:
        try:
            target_mch_compiler_type = subprocess.check_output("g++ -dumpmachine", shell=True)
            target_mch_compiler_type = target_mch_compiler_type.rstrip()
        except subprocess.CalledProcessError as e:
            print ("Script is aborted due to the following error{}".format(e))
            sys.exit(-1)
    return target_mch_compiler_type


def usage():
    """ print the usage and supported options of the script.
    """
    target_platform = platform.system()
    target_machine = get_target_machine_type()
    target_machine = target_machine.rstrip()
    progName = os.path.split(sys.argv[0])[1]
    print ("""
The {0} script generates an archive for the given project/prebuild  directory
to a given target machine compiler type """.format(os.path.split(sys.argv[0])[1]))
    print ("""
If you are producing a cross build, use the -b flag to copy files from
the appropriate target/<-m flag> directory to produce an archive for
your target architecture.""")
    print ("""
The {0} script generates an Debian/RPM package for a given Project directory with the default
package version number 1.0 and default release number 1.
use the -y flag to generate the RPM package.
use the -d flag to generate the DEBIAN PACKAGE.
For generating a RPM/Debian package with custom version number and release number
use th -r flag anf -v flag along with -y/-d flags.
    """)

    print ("""
Syntax {} [-p <pathToProject>] [-s <pathToSAFplus>] [-m <target machine compiler Type>] [[-o] <outputFile>]""".format(sys.argv[0]))
    print ("""
Options:
  -h or --help: This help
  -m or --target-machine=<target machine compiler Type>
     Ex: i686-linux-gnu, x86_64-linux-gnu
     Default value is {tgtMachine}
  -p or --project-dir=<PathtoProject/Applicationdir>
  -s or --safplus-dir=<PathtoSafplusLibraryDir>
  -x or --execute="string"
     Execute this string on the bash prompt after copying files but before creating the archive
  -o or --output=<archive name>
     Output file name. Can also be supplied as the first non-flag argument.
     Extension selects the format (.tgz, .tar.gz, or .zip)
     default output file is {tp}_{tm}
  -y or --yum
     Generates the RPM package for the given project/safplus directory 
  -d or --debian
     Generates the debian package for the given project/safplus directory 
  -i or --install_dir
     Installation directory/location for the target RPM/Debian Package 
     default installation directory for model is /opt/<model_name>
  -v or --pkg-version=<pkg-version number>
     script generates the RPM/Debian package with the given version number  and given release number
     default value is 1.0
  -r or --pkg-release=<pkg-release number>
     when a minor update happens, RPM/Debian package need to rebuilt with incremented release number
     default valus is 1
""".format(tgtMachine=target_machine,tm=target_machine.split('-')[0],tp=target_platform))

    print ("""Example usage:

Package SAFplus into safplus.zip:
$ {sp} safplus.zip

Package SAFplus and a project together into myApp.tgz
$ {sp} [TODO]

Crossbuild packaging:
$ {sp} -s {s} -m {m} crossPkg.zip

Package the model and safplus libraries into an rpm <model_name>-1.0-1.{m}.rpm:
$ {sp} -y -p <path to a model dir> <model_name>.tgz

Package the model along with safplus libraries into a rpm for version 2.0 with a release 2:
$ {sp} -p <path to model dir> -v 2.0 -r 2 -y <model_name>.tgz

Crossbuild RPM package generation:
$ {sp} -s {s} -m {m} -y crossPkg.tgz""".\
        format(s=os.path.abspath(os.path.split(sys.argv[0])[0] +("/../..")), m=target_machine,
               sp=progName))


def parser(args):
    """Read the command line arguments, parse them, and return a N-tuple of all the info"""
    model_dir = None
    tar_name = None
    execute = None
    yum_package = False
    debian_package = False
    install_dir = None
    pkg_ver = None
    pkg_rel = None

    target_platform = platform.system()
    target_machine = get_target_machine_type()
    pre_build_dir = None

    try:
        opts, args = getopt.getopt(args, "hm:s:a:o:p:x:ydi:v:r:", ["help", "project-dir=", "target-machine=",
                                                           "tar-name=", "safplus-dir=","execute=", "yum", "debian", "install_dir=",
                                                           "pkg-version", "pkg-pkgrelease"])
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
            sys.exit()
        elif opt in ("-x", "--execute="):
            execute =  get_option_value(arg)
        elif opt in ("-p", "--project-dir="):
            model_dir = get_option_value(arg)
            # log.info("Project dir is {}".format(model_dir))
        elif opt in ("-o", "--output"):
            tar_name = get_option_value(arg)
            # log.debug("Archive name is {}".format(tar_name))
        elif opt in ("-a", "--target-machine"):
            target_machine = get_option_value(arg)
            # log.info("target-machine is {}".format(target_machine))
        elif opt in ("-s", "--safplus-dir"):
            pre_build_dir = get_option_value(arg)
            # log.info("SAFplus dir {}".format(pre_build_dir))
        elif opt in ("-y", "--yum"):
            yum_package = True
            # log.info("RPM PACKAGE ")
        elif opt in ("-d", "--debian"):
            debian_package = True
            # log.info("DEBIAN PACKAGE")
        elif opt in ("-i", "--install_dir"):
            install_dir = get_option_value(arg)
            log.info(" Package Installation directory on the target Machine is {}".format(install_dir))
        elif opt in ("-v", "--pkg-version"):
            pkg_ver = get_option_value(arg)
        elif opt in ("-r", "--pkg-release"):
            pkg_rel = get_option_value(arg)
        else:
            pass

    if len(args) >= 1:
      tar_name = args[0]
    tg_platform = None

    if len(target_machine.split("-")) > 1:
        tg_platform = target_machine.split('-')[0]
    #Below logic is useful to check whether the archive name starts with safplus or not.
    #RPM/Debian packages generated by using this script will  overwrites the installed safplus packages on the system.
    if debian_package or yum_package:
        if tar_name is not None :
            if tar_name.lower().startswith("safplus"):
                fail_and_exit("Archive name {} starts with safplus. It may overwrite the safplus debian packages installed on the system".format(tar_name))
        else:
            if model_dir is not None and check_dir_exists(model_dir):
                pass
            else:
                fail_and_exit(" For RPM/Debian packages generation Archive name is mandatory and does not starts with safplus") 
    if debian_package or yum_package:
        if model_dir is not None:
            if check_dir_exists(model_dir):
                install_dir = "/opt" + os.sep + os.path.split(model_dir)[1]
            else:
                fail_and_exit("Model directory {} does not exists".format(model_dir))

    if install_dir is not None:
        if yum_package:
            log.debug("Installation directory for a generated Rpm package is {}".format(install_dir))
        elif debian_package:
            log.debug("Installation directory for a generated debian package is {}".format(install_dir))

    if tar_name is None:
        if model_dir is not None and check_dir_exists(model_dir):
            tar_name = os.path.split(model_dir)[1] + "_{}".format(tg_platform)
        else:
            tar_name = "safplus_{}".format(tg_platform)
    elif not re.search(r'(\w+).(tar|zip|tgz|gz)', tar_name):
        tar_name = "{}_{}".format(tar_name, tg_platform)
    else:
        pass
    return model_dir, tar_name, target_machine, pre_build_dir,execute,yum_package, debian_package, install_dir, pkg_ver, pkg_rel


def get_option_value(arg_val):

    if arg_val and arg_val.startswith("="):
        arg_val = arg_val[1:]
    return arg_val


def main():
    # log.debug("Command line Arguments are {}".format(sys.argv))
    model_dir, tar_name, target_machine, pre_build_dir,execute, yum_package, debian_package, prefix_dir, pkg_ver, pkg_rel = parser(sys.argv[1:])
    if pre_build_dir is None:
        pre_build_dir = os.path.abspath(os.path.dirname(__file__) + os.sep + '..' + os.sep + '..')
    if prefix_dir is None:
        prefix_dir = "/opt/safplus/target"
    if pkg_ver is None:
         if yum_package or debian_package:
             pkg_ver = "1.0"
    if pkg_rel is None:
         if yum_package or debian_package:
             pkg_rel = "1"
    # log.info("%s, %s, %s, %s, %s, %s" % (model_dir, tar_name, target_machine, target_kernel, pre_build_dir,execute))
    package(model_dir, tar_name, prefix_dir, target_machine, pre_build_dir,execute, yum_package, debian_package, pkg_ver, pkg_rel)

log = log_init()
target_mch_compiler_type = None
get_target_machine_type()
if __name__ == '__main__':
    main()
