#!/usr/bin/env python
###############################################################################
#
# Copyright (C) 2002-2009 by OpenClovis Inc. All  Rights Reserved.
# 
# The source code for  this program is not published  or otherwise 
# divested of  its trade secrets, irrespective  of  what  has been 
# deposited with the U.S. Copyright office.
# 
# This program is  free software; you can redistribute it and / or
# modify  it under  the  terms  of  the GNU General Public License
# version 2 as published by the Free Software Foundation.
# 
# This program is distributed in the  hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied  warranty  of 
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
# General Public License for more details.
# 
# You  should  have  received  a  copy of  the  GNU General Public
# License along  with  this program. If  not,  write  to  the 
# Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
#
###############################################################################
# 
# Extract host specific information from target.conf file
# extract_nodeinfo.py pulls information out of the clAmfConfig.xml
# file.  We pull it out of the target.conf file.

from optparse import OptionParser, make_option
import sys
import re

class ConfigVarFetcher:
    """Class that will fetch variables and their values from the target.conf file for a
    specified host (or target).  The variable names and values can be extracted
    either one by one, or in a batch.
    """
    def __init__(self, file, hosts):
        self.dbinfo = {}
        self.hoinfo = {}
        self.defaults = {}
        self.conf_file = file
        self.var_name = None
        self.hosts = hosts
  
    def parse_db(self, host, db_entry):
        local_dict = {}
        exec "x=%s" % db_entry in local_dict
        self.dbinfo[host] = local_dict["x"]
        
    def parse_hvo(self, host, var, value):
        try:
            self.hoinfo[host][var] = value
        except KeyError:
            self.hoinfo[host] = {}
            self.hoinfo[host][var] = value

    def parse_defaults(self, var, value):
        self.defaults[var] = value
        
    def  report(self, host, varname):
        """Report on variables derived from the target configuration file for a specific
        hostname (or hostname, varname combination)."""
        if varname:
            val = None
            try:
                val = self.dbinfo[host][varname]
            except:
                try:
                    val = self.hoinfo[host][varname]
                except:
                    try:
                        val = self.defaults[varname]
                    except:
                        pass
        else:
            print "We don't support the full 'report on host' mode"

        if val:
            print "%s=%s" % (varname, val)
            
    def  parse_file(self):
        """Parse the target configuration file specified at instantiation.  Read variable definitions
        and load them locally for reporting later.
        There are three kinds of setting that we want to grab.
        1) There are default settings such as PERSISTENT=/root/asp/persistent
        2) There are host/variable specific overrides such as SLOT_SCNodeI0=1
        3) There are host specific settings dbs such as
            HOST_DB_SCNodeI0={'SLOT':1, 'IP':'192.168.42.33', 'PERSISTENT':'/root/asp/persistent'}
        Default settings are overridden by settings in host specific db.  Host specific db settings
        are overridden by host/variable specific overrides.
        """
        # Build regular expression to match any of the hosts in a host/variable specific override
        ex_str = "(" + "|".join(self.hosts) + ")"
        hvo_re = re.compile("([^=]+)_" + ex_str + "=(.*)$")
        db_re = re.compile('HOST_DB_([^=]*)=[\'\"](.*)[\'\"]$')
        def_re = re.compile('([^=]*)=(.*)$')
        f = open(self.conf_file)
        for line in f:
            # Ignore comments and blank lines
            if line[0] == "#":
                continue
            if line == "":
                continue
            m = db_re.match(line)
            if m:
                self.parse_db(m.group(1), m.group(2))
                continue
            m = hvo_re.match(line)
            if m:
                self.parse_hvo(m.group(2), m.group(1), m.group(3))
                continue
            m = def_re.match(line)
            if m:
                self.parse_defaults(m.group(1), m.group(2))
            
    
def main():
    """Parse command line options, create the Extractor object, load the target.conf
    file with the Extractor object.  Then, spit out the requested information."""
    opt_list = [
        make_option("-t", "--target-conf", action="store", type="string",
            dest="target_conf", default="target.conf"),
        make_option("-v", "--variable", action="store", type="string",
            dest="var_name", default=None)
    ]
    parser = OptionParser("[-t conf] -v var hostname...")
    for opt in opt_list:
        parser.add_option(opt)
    (opts, parsed_args) = parser.parse_args()
    if len(parsed_args) == 0:
        parser.error("No hostname specified")
    if opts.var_name == None:
        parser.error("No variable name specified")
    
    fetcher = ConfigVarFetcher(opts.target_conf, parsed_args)
    fetcher.parse_file()
    for host in parsed_args:
        fetcher.report(host, opts.var_name)
    return 0

#my_dict = {}
#exec line in my_dict

if __name__ == "__main__":
    sys.exit( main())
