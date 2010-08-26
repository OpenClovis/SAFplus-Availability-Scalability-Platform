#!/usr/bin/python
"""
Extract info from bugzilla mysql database

This is a prototype for now.
"""

import sys
import MySQLdb as db

from bugzilla_miner import *

MAX_DESCRIPTION_LEN = 60 # Only this much will be shown from the bug desc field

#
# Main function
#
def main():
    
    # Open database and DOM skeleton
    bugs = grab_bug_table()
    max_severity_length = max([len(b['severity']) for b in bugs])
    max_status_length = max([len(b['status']) for b in bugs])
    max_resolution_length = max([len(b['resolution']) for b in bugs])
    max_component_length = max([len(b['component']) for b in bugs])
    max_assigned_length = max([len(b['assigned']) for b in bugs])
    max_version_length = max([len(b['version']) for b in bugs])
    for bug in bugs:
        fmt = '%%-5s %%-%ds %%-2s %%-%ds %%-%ds %%-%ds %%-%ds %%-%ds %%s' % (
            max_severity_length,
            max_status_length,
            max_resolution_length,
            max_version_length,
            max_component_length,
            max_assigned_length)
        
        if len(bug['synopsis']) > MAX_DESCRIPTION_LEN-3:
            synopsis = bug['synopsis'][:MAX_DESCRIPTION_LEN]+'...'
        else:
            synopsis = bug['synopsis']
            
        print fmt % (
            bug['id'],
            bug['severity'],
            bug['priority'],
            bug['status'],
            bug['resolution'],
            bug['version'],
            bug['component'],
            bug['assigned'],
            synopsis)
                
if __name__ == '__main__':
    main()

