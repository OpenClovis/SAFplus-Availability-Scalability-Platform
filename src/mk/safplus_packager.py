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
import subprocess
import pdb


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
    log.info("{}".format(errmsg))
    sys.exit(-1)


def check_dir_exists(dir_name):
    """ Return true if the given dir_name existed in the file system otherwise it will return false
    """
    return os.path.exists(dir_name)


def unusedcreatedir(dir_name):
    try:
        os.mkdir(dir_name, 0o755)
    except OSError, e:
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
def create_archive(tar_name, tar_dir, arch_format='gztar'):
    """ Create an archive with a given archive name and archive format for the provided directory
    """
    if check_dir_exists(tar_dir):
        log.info("Archive name is {}".format(tar_name))
        tar_name = shutil.make_archive(tar_name, arch_format, tar_dir)
        log.info("Archive {} generated successfully".format(tar_name))
    else:
        fail_and_exit("Archive directory {} not exists".format(tar_dir))


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
    log.debug(recursion*" " + src)
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
                    log.debug((recursion+1)*" " + s + " -> " + d)
                    shutil.copy(s, d)
                    # mark any python or shell scripts as executable
                    if re.search(r'(\w+).(py|sh)', d):
                        os.chmod(d, 0o755)


def get_compression_format(archive_name):
    """ Returns archive_name without extension and compression method from the archive_name
    """
    default_compress_format = {'tar': ('.tar', 'tar'), 'gz': ('.tar.gz', 'gztar'),
                               'bz2': ('.tar.bz2', 'bztar'), 'tgz': ('.tgz', 'gztar'),
                               'zip': ('.zip', 'zip')}
    compress_format = "gztar"
    compress_extension = archive_name.split('.')[-1]
    if compress_extension in default_compress_format:
        extension, compress_format = default_compress_format.get(compress_extension)
        archive_name = archive_name.rstrip(extension)

    return archive_name, compress_format


def package_dirs(target_dir, tar_dir):
    """ package_dirs is a helper function which will copy the directories/files from source location to destination
        location based on the directories name. This function will identify the directory names like (lib or plugin,
        bin or sbin, test and share).
    """
    if check_dir_exists(target_dir):
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
    log.debug("Image File Name is {}".format(image_file_name))
    return image_file_name


def package(base_dir, tar_name, machine=None, kernel_version=None, pre_build_dir=None,execute=None):
    """ This function packages the model related binaries, libraries, test examples and 3rd party utilities
       into an archive to the given target platform.
    """
    if base_dir:
        log.info("SAFPlus Model directory is {}".format(base_dir))

    # Break the output file name into its components so we can ...
    image_dir_path = get_image_dir_path(tar_name)
    image_dir = "{}/images".format(image_dir_path)
    image_stage_dir = image_dir + os.sep + os.path.splitext(get_image_file_name(tar_name))[0]

    # Blow away the old staging directory if it exists and recreate it
    if check_dir_exists(image_dir):
        shutil.rmtree(image_dir)
        # There's no need to copy it somewhere else... nobody needs it.
        #log.info("Backing up the already existed image directory")
        #image_backup_dir = "{}/images_backup".format(image_dir_path)
        #if check_dir_exists(image_backup_dir):
        #    shutil.rmtree(image_backup_dir)
        #shutil.move(image_dir, image_backup_dir)

    os.makedirs(image_stage_dir)

    # If the user does not supply the crossbuild into, assume the local machine -- so get the local machine's data
    if not machine:
        machine = platform.machine()  # architecture, e.g. x86_64

    if not kernel_version:
        kernel_version = platform.release() # e.g 3.13.0-32-generic

    # Print out what was discovered so user can visually verify.
    log.info("Target platform machine type:{}".format(machine))
    log.info("Target platform kernel version:{}".format(kernel_version))
    log.info("Target platform image directory is {}".format(image_dir))
    tar_name, compress_format = get_compression_format(tar_name)
    #tar_dir = "{}/{}".format(image_dir, tar_name)
    #check_and_createdir(tar_dir)
    log.info("Packaging files from {0} and {1}".format(pre_build_dir,base_dir));

    if pre_build_dir:  # Create the actual prebuilt dir by combining what the user passed with the arch and kernel.
        target_dir = "{0}/target/{1}/{2}".format(pre_build_dir, machine, kernel_version)
        log.info("Prebuilt target platform related SAFPlus binaries, libraries, and third party utilities are present in {}".format(target_dir))
        if check_dir_exists(pre_build_dir):
            pre_build_dir = "{}/target/{}/{}".format(pre_build_dir, machine, kernel_version)
            package_dirs(pre_build_dir, image_stage_dir)

    if base_dir:
        target_dir = "{0}/target/{1}/{2}".format(base_dir, machine, kernel_version)
        log.info("SAFPlus binaries, libraries and third party utilities related to target platform are present in {}".format(target_dir))
        package_dirs(target_dir, image_stage_dir)
    log.info("Packaging complete")

    if execute:
      tmp = execute.format(image_dir=image_stage_dir)
      log.info("Executing user supplied script: %s" % tmp)
      try:
        ret = subprocess.call(tmp, shell=True)
      except Exception,e:
        pdb.set_trace()
      log.info("script execution complete, returned %d", ret)

    log.info("Archive name is {0} Archive compression format is {1}".format(tar_name, compress_format))
    # put the tarball exactly where the requested on the command line: tar_name = os.path.join(image_dir, tar_name)
    create_archive(tar_name, image_dir, compress_format)
    pass


def usage():
    """ print the usage and supported options of the script.
    """
    target_platform = platform.system()
    target_machine = platform.machine()
    target_kernel_version = platform.release()
    progName = os.path.split(sys.argv[0])[1]
    print """
The {0} script generates an archive for the given project/prebuild  directory
to a given machine type and kernel version.""".format(os.path.split(sys.argv[0])[1])
    print """
If you are producing a cross build, use the -m and -k flags to copy files from
the appropriate target/<-m flag>/<-k flag> directory to produce an archive for
your target architecture."""
    print """
Syntax {} [-p <pathToProject>] [-s <pathToSAFplus>] [-m <machineType>] [-k <kernelVersion>] [[-o] <outputFile>]""".format(sys.argv[0])
    print """
Options:
  -h or --help: This help
  -p or --project-dir=<PathtoProject/Applicationdir>
  -s or --safplus-dir=<PathtoSafplusLibraryDir>
  -m or --target-machine=<target architecture type>
     Ex: x86_64 x86 Default is {tgtMachine}
  -k or --target-os-kernel-version=<target operating system kernel version>
     Default is {kernelVersion}
  -x or --execute="string"
     Execute this string on the bash prompt after copying files but before creating the archive
  -o or --output=<archive name>
     Output file name. Can also be supplied as the first non-flag argument.  
     Extension selects the format (.tgz, .tar.gz, or .zip)
     default output file is {tp}_{tm}_{tk}
""".format(tgtMachine=target_machine,kernelVersion=target_kernel_version,tp=target_platform, tm=target_machine, tk=target_kernel_version)

    print """Example usage:

Package SAFplus into safplus.zip:
$ {sp} safplus.zip

Package SAFplus and a project together into myApp.tgz
$ {sp} [TODO]

Crossbuild packaging:

$ {sp} -s {s} -m {m} -k {k} crossPkg.zip""".\
        format(s=os.path.abspath(os.path.split(sys.argv[0])[0] +("/..")), m=target_machine,
               k=target_kernel_version, sp=progName)


def parser(args):
    """Read the command line arguments, parse them, and return a N-tuple of all the info"""
    model_dir = None
    tar_name = None
    execute = None
 
    target_platform = platform.system()
    target_machine = platform.machine()
    target_kernel_version = platform.release()
    pre_build_dir = None

    try:
        opts, args = getopt.getopt(args, "hm:k:s:o:p:x:", ["help", "project-dir=", "target-machine=",
                                                          "target-kernel=", "tar-name=", "safplus-dir=","execute="])
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
            log.info("Project dir is {}".format(model_dir))
        elif opt in ("-o", "--output"):
            tar_name = get_option_value(arg)
            log.debug("Archive name is {}".format(tar_name))
        elif opt in ("-a", "--target-machine"):
            target_machine = get_option_value(arg)
            log.info("target-machine is {}".format(target_machine))
        elif opt in ("-r", "--target-kernel"):
            target_kernel_version = get_option_value(arg)
            log.info("target-os-kernel-version is {}".format(target_kernel_version))
        elif opt in ("-s", "--safplus-dir"):
            pre_build_dir = get_option_value(arg)
            log.info("SAFplus dir {}".format(pre_build_dir))
        else:
            pass
    if len(args) >= 1:
      tar_name = args[0]


    if tar_name is None:
        tar_name = "safplus_{}_{}".format(target_machine, target_kernel_version)
    elif not re.search(r'(\w+).(tar|zip|tgz)', tar_name):
        tar_name = "{}_{}_{}".format(tar_name, target_machine, target_kernel_version)
    else:
        pass
    return model_dir, tar_name, target_machine, target_kernel_version, pre_build_dir,execute


def get_option_value(arg_val):

    if arg_val and arg_val.startswith("="):
        arg_val = arg_val[1:]
    return arg_val


def main():
    log.debug("Command line Arguments are {}".format(sys.argv))
    model_dir, tar_name, target_machine, target_kernel, pre_build_dir,execute = parser(sys.argv[1:])
    if pre_build_dir is None:
      pre_build_dir = os.path.abspath(os.path.dirname(__file__) + os.sep + '..')

    # log.info("%s, %s, %s, %s, %s, %s" % (model_dir, tar_name, target_machine, target_kernel, pre_build_dir,execute))
    package(model_dir, tar_name, target_machine, target_kernel, pre_build_dir,execute)


log = log_init()
if __name__ == '__main__':
    main()
