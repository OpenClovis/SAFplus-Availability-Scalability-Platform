#!/usr/bin/env python
################################################################################
#
#   Copyright (C) 2002-2010 by OpenClovis Inc. All Rights  Reserved.
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
################################################################################
#
# Build: 5.0.0
#

# This script converts target.conf into xml.
# It dumps to stdout, so redirect output to myfile.xml

import sys, os
from xml.sax.saxutils import escape


# returns a substring between two characters
# no error checking
def between(string, first, last):
    return string[string.find(first)+1:string.find(last)]


# prints instr to stdout with a given indentation, 
# and an optional trailing newline character
def iprint(instr, depth=0, newline=True):
    print ' '*(depth*4) + instr.rstrip() + ('\n' if newline else ''),


# a slot object
# link defaults to eth0
class Slot:
    def __init__(self, name, addr, link='eth0', arch=''):
        self.name = name
        self.addr = addr
        self.link = link
        self.arch = arch
    
    def set_link(self, link):
        self.link = link

    def set_arch(self, arch):
        self.arch = arch

    
    def print_xml(self, depth):
        iprint('<slot>', depth)
        depth += 1
        iprint('<NAME>%s</NAME>' % self.name, depth)
        iprint('<ADDR>%s</ADDR>' % self.addr, depth)
        iprint('<LINK>%s</LINK>' % self.link, depth)
        iprint('<ARCH>%s</ARCH>' % self.arch, depth)
        depth -= 1
        iprint('</slot>', depth)


def main():

    if len(sys.argv) < 2:
        print 'Error: Not enough arguments'
        print 'Usage: %s <path-to-target.conf>' % sys.argv[0]
        sys.exit(1)

    if not os.path.isfile(sys.argv[1]):
        print 'Error: File not found'
        print 'Usage: %s <path-to-target.conf>' % sys.argv[0]
        sys.exit(1)

    if not os.path.basename(sys.argv[1]).startswith('target.conf'):
        print 'Error: Invalid File Type'
        print 'Usage: %s <path-to-target.conf>' % sys.argv[0]
        sys.exit(1)
    
    with open(sys.argv[1], 'r') as fh:
        tconf_array = fh.readlines()

    depth = 0
    iprint('<?xml version="1.0" encoding="UTF-8"?>', depth)
    iprint('<version v0="5.0.0">', depth)

    depth += 1    
    iprint('<target_conf>', depth)

    depth += 1

    slots = {} # dictionary to hold all slots indexed by name
    default_arch = ''

    for line in tconf_array:
    
        line = line.strip()
    
        # ignore commented and blank lines
        if line.startswith('#') or line == '':
            continue

        line_array = line.split('=')

        # if this is a slot line
        if line.lower().startswith('slot'):

            name = between(line, '_', '=')
            addr = line_array[1]

            # create new slot object with default link              
            # put it into dictionary
            slots[name] = Slot(name, addr)
            
        elif line.lower().startswith('link'):
        
            # we've got a link line
            name = between(line, '_', '=')
            
            s = slots[name]
            
            if not s:
                # error
                continue
                
            s.set_link(line_array[1])
        
        elif line.lower().startswith('arch'):
        
            # arch line, but there are two types
            
            if not '_' in line_array[0]:
                # 'arch=XYZ' line
                default_arch = line_array[1]
            else:
                # 'arch_name=XYZ' line
                s = slots[name]
            
                if not s:
                    # error
                    continue
                
                s.set_arch(line_array[1])            
        
        else:
            # a non-slot and non-link line
            
            # escape() is from xml.sax.saxutils, and automatically handles  
            # otherwise invalid characters in an xml string, such as '<'
            xml_line = '<%s>%s</%s>\n' % \
                (line_array[0], escape(line_array[1]), line_array[0])
            
            iprint(xml_line, depth)


    iprint('<slots count="%d">' % len(slots), depth)

    depth += 1

    if not default_arch:
        # error
        pass

    # set blank arch's to default arch
    # print all slots
    for s in slots.keys():
        if slots[s].arch == '':
            slots[s].set_arch(default_arch)
        slots[s].print_xml(depth)

    depth -= 1

    iprint('</slots>', depth)

    depth -= 1
    iprint('</target_conf>', depth)

    depth -= 1
    iprint('</version>', depth)



if __name__ == '__main__':
    main()


