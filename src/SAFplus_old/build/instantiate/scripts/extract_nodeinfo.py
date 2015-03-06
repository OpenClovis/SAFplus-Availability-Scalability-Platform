#!/usr/bin/env python
# Copyright (C) 2002-2012 OpenClovis Solutions Inc.  All Rights Reserved.
# This file is available  under  a  commercial  license  from  the
# copyright  holder or the GNU General Public License Version 2.0.
#
# The source code for  this program is not published  or otherwise 
# divested of  its trade secrets, irrespective  of  what  has been 
# deposited with the U.S. Copyright office.
# 
# This program is distributed in the  hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied  warranty  of 
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
# General Public License for more details.
# 
# For more  information,  see the  file COPYING provided with this
# material.

import sys
from xml.dom import minidom

def is_sc(n):
    """Check whether a given dom node is for a system controller node type.
       Find this by checking the cpm type to be either GLOBAL or LOCAL.
    """

    if n.getAttribute('cpmType') == 'GLOBAL':
       return 1
    else:
       return 0


if len(sys.argv) != 2:
    print "Usage:"
    print "  extract_nodeinfo.py <full-path-to-clAmfConfig.xml>"
    print ""
    sys.exit(1)
    
in_file = sys.argv[1]


dom = minidom.parse(in_file)

# Extracting node types:

cpmconf_nodes = dom.getElementsByTagName('cpmConfig')

nodetypes = [n.getAttribute('nodeType') for n in cpmconf_nodes]

sc_types = [n.getAttribute('nodeType') for n in cpmconf_nodes if is_sc(n) != 0]

# Extracting bootconfig name per node type (into a dictionary):

bootconfig = dict()
for n in cpmconf_nodes:
    bootconfig_node = n.getElementsByTagName('bootConfig')[0]
    bootconfig[n.getAttribute('nodeType')] = \
        bootconfig_node.getAttribute('name')

# Extracting node instance names

nodeinstance_nodes = dom.getElementsByTagName('nodeInstance')

nodenames = [n.getAttribute('name') for n in nodeinstance_nodes]

# Extracting node types per node instance

system_controller_list = []
nodeinstancetypes = dict()
for n in nodeinstance_nodes:
    t = n.getAttribute('type')
    name = n.getAttribute('name')
    nodeinstancetypes[name] = t
    if t in sc_types:
        system_controller_list.append(name)
    
print """#
# This file describes crucial node information, automatically derived from
# your model configuration (xml) files.  Edit it only if you know what you
# are doing.
#
##############################################################################

#
# NODE_TYPES (Mandatory)
#
NODE_TYPES=(""",
print ' '.join(nodetypes),
print """)

#
# BOOTCONFIG_<NODE_TYPE> (Mandatory)
#"""
for t in nodetypes:
    print 'BOOTCONFIG_%s=%s' % (t, bootconfig[t])
print """
#
# NODE_INSTANCES (Mandatory)
#
NODE_INSTANCES=(""",
print ' '.join(nodenames),
print """)

#
# NODETYPE_<NODE_INSTANCE> (Mandatory)
#"""
for n in nodenames:
    print 'NODETYPE_%s=%s' % (n, nodeinstancetypes[n])

print """
#
# SYSTEM_CONTROLLERS (Mandatory)
#
SYSTEM_CONTROLLERS=(""",
print ' '.join(system_controller_list),
print ")"
print
