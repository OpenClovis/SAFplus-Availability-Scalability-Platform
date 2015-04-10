#!/usr/bin/env python
import os
import sys
import shutil
import platform
import glob
import logging
import re
import getopt
import errno


def file_list(dir_path, pattern='*'):
    # Returns a list of file names having same extension present in the directory based on the pattern.
    # Default this function returns a list of files present in the directory.
    file_names_list = None
    if checkDirExists(dir_path):
        pattern = "{}/{}".format(dir_path, pattern)
        file_names_list = glob.glob(pattern)
    return file_names_list


def fail_and_exit(errmsg):
    log.info("{}".format(errmsg))
    sys.exit(-1)


def checkDirExists(dirname):
    return os.path.exists(dirname)


def createDir(dir_name):
    try:
        os.mkdir(dir_name, 0o755)
    except OSError, e:
        if e.errno != errno.EEXIST:
            fail_and_exit(" Failed to create the dir {}".format(dir_name))


def checkAndCreateDir(dir_path):
    if not checkDirExists(dir_path):
        createDir(dir_path)
    return True


def log_init():
    logging.basicConfig(filemode='a', format='%(asctime)s:%(msecs)d %(levelname)s: %(message)s',
                        datefmt='%H:%M:%S', level=logging.DEBUG)
    return logging.getLogger("UPDATE REPO")


# make_archive() is available from python 2.7 version onwards
def create_archive(tar_name, tar_dir, arch_format='gztar'):

    if checkDirExists(tar_dir):
        log.info("Archive name is {}".format(tar_name))
        tar_name = shutil.make_archive(tar_name, arch_format, tar_dir)
        log.info("Archive {} generated successfully".format(tar_name))
    else:
        fail_and_exit("Archive directory {} not exists".format(tar_dir))


def copy_dir(src, dst):
    # This function will recursively copy the files and sub-directories from src directory to destination directory
    if not checkDirExists(dst):
        # suppose the sub-directory in source directory contains only object and  header( like .h .hxx .hpp .ipp) files
        # below logic skips the creation of sub-directory  in the destination directory
        files_list = file_list(src)
        p = re.compile(r'(\w+)\.(h\w+|o|i\w+)')
        obj_header_files_count = len([e for e in files_list if p.search(e)])
        if len(files_list) == obj_header_files_count:
            return
        createDir(dst)
    for item in os.listdir(src):
        s = os.path.join(src, item)
        d = os.path.join(dst, item)
        if os.path.isdir(s):
            copy_dir(s, d)
        else:
            if not os.path.exists(d) or os.stat(src).st_mtime - os.stat(dst).st_mtime > 1:
                # Below regular expression skips the copying of object files and header files
                # into the destination directory
                if not re.search(r'(\w+)\.(h\w+|o)', s):
                    shutil.copy(s, d)
                    if re.search(r'(\w+).(py)', d):
                        os.chmod(d, 0o755)


def get_target_os_release():
    return platform.release()


def get_target_machine():
    return platform.machine()


def get_compression_format(archive_name):
    default_compress_format = {'tar': ('.tar', 'tar'), 'gz': ('.tar.gz', 'gztar'),
                               'bz2': ('.tar.bz2', 'bztar'), 'zip': ('.zip', 'zip')}
    compress_format = "gztar"
    compress_extension = archive_name.split('.')[-1]
    if compress_extension in default_compress_format:
        extension, compress_format = default_compress_format.get(compress_extension)
        archive_name = archive_name.rstrip(extension)

    return archive_name, compress_format


def package_dirs(target_dir, tar_dir):
    if checkDirExists(target_dir):
        for dir_name, subdir_list, filename_list in os.walk(target_dir):
            if os.path.basename(dir_name) == 'lib' or os.path.basename(dir_name) == 'plugin':
                tar_lib_dir = tar_dir+"/lib"
                copy_dir(dir_name, tar_lib_dir)
            elif os.path.basename(dir_name) == 'bin' or os.path.basename(dir_name) == 'sbin':
                tar_bin_dir = tar_dir+"/bin"
                copy_dir(dir_name, tar_bin_dir)
            elif os.path.basename(dir_name) == 'test':
                tar_test_dir = tar_dir + "/test"
                copy_dir(dir_name, tar_test_dir)
            elif os.path.basename(dir_name) == "share":
                tar_share_dir = tar_dir+'/share'
                copy_dir(dir_name, tar_share_dir)
            else:
                pass
    else:
        log.info(" {} does not exists".format(target_dir))


def package(base_dir, tar_name, machine=None, kernel_version=None, pre_build_dir=None):

    if base_dir and not checkDirExists(base_dir):
        fail_and_exit("Provide the Valid SAFplus Model directory path")

    log.info("SAFPlus Model directory is {}".format(base_dir))

    image_dir = "{}/target/images".format(base_dir)
    if checkDirExists(image_dir):
        image_backup_dir = "{}/target/images_backup".format(base_dir)
        if checkDirExists(image_backup_dir):
            shutil.rmtree(image_backup_dir)
        shutil.move(image_dir, image_backup_dir)
    createDir(image_dir)
    if not machine:
        machine = get_target_machine()

    if not kernel_version:
        kernel_version = get_target_os_release()

    target_dir = "{0}/target/{1}/{2}".format(base_dir, machine, kernel_version)

    log.info("Target platform Machine Type:{}".format(machine))
    log.info("Target platform Kernel Version:{}".format(kernel_version))
    log.info("SAFPlus binaries, libraries and third party utilities related to target platform are present in {} "
             .format(target_dir))
    log.info("Target platform Image directory is {}".format(image_dir))

    tar_name, compress_format = get_compression_format(tar_name)
    tar_dir = "{}/{}".format(image_dir, tar_name)
    checkAndCreateDir(tar_dir)
    log.info("Packaging the SAFPlus services binaries libraries tests and 3rd party utilities")
    if pre_build_dir:
        if checkDirExists(pre_build_dir):
            pre_build_dir = "{}/target/{}/{}".format(pre_build_dir, machine, kernel_version)
            package_dirs(pre_build_dir, tar_dir)

    if target_dir:
        package_dirs(target_dir, tar_dir)

    log.info("Archive Name is {0} Archive compression format is {1}".format(tar_name, compress_format))
    tar_name = os.path.join(image_dir, tar_name)
    create_archive(tar_name, image_dir, compress_format)
    pass


def usage():
    target_platform = platform.system()
    target_machine = get_target_machine()
    target_kernel_version = get_target_os_release()
    log.info("Usage is {} -s <pathtosafplusdir> -m <machineType> -t <tar_name> -k <kernel_version>".format(sys.argv[0]))
    log.info("Options:")
    log.info("-h or --help")
    log.info("-s or --safplus-model-dir=<PathtosafplusModeldir>")
    log.info("-m or --target-machine=<target architecture type> Ex: x86_64 x86 Default is {}".format(target_machine))
    log.info("-k or --target-os-kernel-version=<target operating system KernelVersion> Default is {}"
             .format(target_kernel_version))
    log.info("-t or --tar-name=<archive name>")
    log.info("  default tar_name is {0}_{1}_{2}".format(target_platform, target_machine, target_kernel_version))
    log.info("-p or --safplus-prebuild-dir=<PathtoSafplusPrebuildDir>")


def parser(args):
    model_dir = None
    tar_name = None
    target_machine = None
    target_kernel_version = None
    pre_build_dir = None
    try:
        opts, args = getopt.getopt(args, "hm:k:s:t:p:", ["help", "safplus-model-dir=", "target-machine=",
                                                         "target-kernel=", "tar-name=", "safplus-prebuild-dir="])
    except getopt.GetoptError as err:
        log.info("{}".format(err))
        usage()
        sys.exit(0)
    if not opts:
        log.info("Unrecognised options to the script")
        usage()
        sys.exit(0)

    for opt, arg in opts:
        if opt in ("-h", "--help"):
            usage()
            sys.exit()
        elif opt in ("-s", "--safplus-model-dir="):
            model_dir = get_parser_value(arg)
            log.info("Model dir is {}".format(model_dir))
        elif opt in ("-t", "--tar-name"):
            tar_name = get_parser_value(arg)
            log.info("Archive Name is {}".format(tar_name))
        elif opt in ("-a", "--target-machine"):
            target_machine = get_parser_value(arg)
            log.info("target-machine is {}".format(target_machine))
        elif opt in ("-r", "--target-kernel"):
            target_kernel_version = get_parser_value(arg)
            log.info("target-os-kernel-version is {}".format(target_kernel_version))
        elif opt in ("-p", "--safplus_prebuild_dir"):
            pre_build_dir = get_parser_value(arg)
            log.info("Pre-build dir {}".format(pre_build_dir))
        else:
            pass

    if tar_name is None:
        tar_name = "{}_{}_{}".format(platform.system(), get_target_machine(), get_target_os_release())
    elif not re.search(r'(\w+).(tar|zip)', tar_name):
        tar_name = "{}_{}_{}_{}".format(tar_name, platform.system(), get_target_machine(), get_target_os_release())
    else:
        pass
    return model_dir, tar_name, target_machine, target_kernel_version, pre_build_dir


def get_parser_value(arg_val):

    if arg_val and arg_val.startswith("="):
        arg_val = arg_val[1:].strip()

    return arg_val


def main():
    log.info("Command line Arguments are {}".format(sys.argv))
    model_dir, tar_name, target_machine, target_kernel, pre_build_dir = parser(sys.argv[1:])
    package(model_dir, tar_name, target_machine, target_kernel, pre_build_dir)


log = log_init()
if __name__ == '__main__':
    main()