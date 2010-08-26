#!/usr/bin/env python

import shutil
import os
from sys import argv
from optparse import OptionParser
from optparse import make_option

FILE_TEMPLATE = """#!/bin/bash
PROG=`basename $0`
DIR=`dirname $0`
BIN=harnessed_binary_${PROG}
OUTPUT=${PROG}.output.$$
export > ${OUTPUT}
echo "ARGS" >> ${OUTPUT}
echo $0 >> ${OUTPUT}
for i in "$@"
do
	echo $i >> ${OUTPUT}
done
echo "OUTPUT" >> ${OUTPUT}

exec %s ${DIR}/${BIN} "$@" >> ${OUTPUT} 2>&1
"""

class DebugHarness:
    """A Harness object is used to create a debugging wrapper around an
ASP binary.  The wrapper is a shell script that dumps some debug
information and then invokes the real binary forwarding all command
line arguments."""
    def __init__(self, args):
        """Initialize a Harness object.  Extract behavior invormation from
command line arguments and initialize the Harness obect appropriately
for the arguments specified.
It is perhaps somewhat retarded having the constructor parse the command line
options.  One down side of that is that a --help in the command line arguments
terminate the program with a usage message.  For that matter, so will a usage
error.  Perhaps a try/catch around the parsing code?  But since this script is the
only client of this class, that is perhaps not necessary.  Maybe that makes it
stupid to separate this out as a class?"""
# Let's have some fun learing how to use OptionParser.
        parser = OptionParser(usage="%prog [-d directory] [-t program_name] [-s path_to_strace]")
        optionlist = [
            make_option("-d", "--directory", action="store", type="string",
                dest="directory", default="/root/asp/bin",
                help="directory where binaries reside, default=/root/asp/bin"),
            make_option("-t", "--trace", action="store_true", dest="trace",  default=False,
                help="Run the harnessed binary with strace."),
            make_option("-u", "--unharness", action="store_true", dest="unharness",
                default=False, help="Uninstall the debugging harness from named binaries."),
            make_option("-s", "--straceprog", action="store", type="string",
                dest="strace_prog", default="/usr/bin/strace",
                help="Specify full path name of strace (or equivalent) program.")
            ]
        for opt in optionlist:
            parser.add_option(opt)

        (self.opts, self.parsed_args) = parser.parse_args(args)
        if len(self.parsed_args) < 1:
            parser.error("No program name specified.")
    
    def install(self, dir, p):
        """Install a debugging harness for a specific program"""
        program = os.path.join(dir, p)
        harnessed_program = os.path.join(dir, "harnessed_binary_" + p)
        if os.path.exists(harnessed_program):
            print "%s is already harnessed.  Skipping..." % p
            return
        print "Install %s as %s" % (p, harnessed_program)
        shutil.move(program, harnessed_program)
        f = open(program, "w", 0555)
        if self.opts.trace:
            trace_str = "/usr/bin/strace"
        else:
            trace_str = ""
        f.write(FILE_TEMPLATE % trace_str)
        f.close()
        shutil.copymode(harnessed_program, program)

    def uninstall(self, dir, p):
        """Uninstall a debugging harness for a specific program"""
        program = os.path.join(dir, p)
        harnessed_program = os.path.join(dir, "harnessed_binary_" + p)
        if os.path.exists(harnessed_program) == False:
            print "%s is not yet harnessed.  Skipping..." % p
            return
        print "Uninstall harness %s from %s" % (harnessed_program, p)
        os.remove(program)
        shutil.move(harnessed_program, program)

    def execute(self):
        """Install the debugging harness for the program identified at initialization time.  In the
        binary directory, change the name of the binary "prog" to "harnessed_binary_prog"
        and create a harness script in the specified directory.  If requested then this will
        unharness any previously harnessed named programs"""
        
        for p in self.parsed_args:
            dir = self.opts.directory
            if self.opts.unharness == False:
                self.install(dir, p)
            else:
                self.uninstall(dir, p)
         

if __name__ == "__main__":
    harness = DebugHarness(argv[1:])
    harness.execute()
