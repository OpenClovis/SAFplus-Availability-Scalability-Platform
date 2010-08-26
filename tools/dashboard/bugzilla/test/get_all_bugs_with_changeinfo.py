#!/usr/bin/python
"""
Extract info from bugzilla mysql database

This is a prototype for now.
"""

import sys, os
import MySQLdb as db

from bugzilla_miner import *

MAX_DESCRIPTION_LEN = 60 # Only this much will be shown from the bug desc field

appname = os.path.basename(sys.argv[0])
usage_msg = """
%(appname)s - print bug database, with change counts between dates

Usage:
    %(appname)s <start-date> [<end-date>]

Examplea:
    Calculate changes since beginning of year till today:
    %(appname)s 2007-01-01           
    
    Calculate changes for April:
    %(appname)s 2007-04-01 2007-04-30

""" % globals()

def print_usage_and_exit():
    print usage_msg
    sys.exit(1)
#
# Main function
#
def main():
    
    if not len(sys.argv) in [2, 3]:
        print 'Error: Wrong number of args'
        print_usage_and_exit()
    
    nums = sys.argv[1].split('-')
    if not len(nums) == 3:
        print 'Error: wrong from date %s' % sys.argv[1]
        print_usage_and_exit()
    else:
        from_date = datetime.datetime(int(nums[0]),
                                      int(nums[1]),
                                      int(nums[2]), 0, 0, 0)

    if len(sys.argv) == 3:
        nums = sys.argv[2].split('-')
        if not len(nums) == 3:
            print 'Error: wrong to date %s' % sys.argv[2]
            print_usage_and_exit()
        else:
            to_date = datetime.datetime(int(nums[0]),
                                        int(nums[1]),
                                        int(nums[2]), 23, 59, 59)
    else:
        to_date = datetime.datetime.now()
        
    bugs = grab_bug_table_with_change(from_date, to_date)
    
    max_severity_length = max([len(b['severity']) for b in bugs])
    max_status_length = max([len(b['status']) for b in bugs])
    max_resolution_length = max([len(b['resolution']) for b in bugs])
    max_component_length = max([len(b['component']) for b in bugs])
    max_assigned_length = max([len(b['assigned'].split('@')[0]) for b in bugs])
    max_version_length = max([len(b['version']) for b in bugs])

    for bug in bugs:
        fmt = '%%-5s %%-%ds %%-2s %%-%ds %%-%ds %%-2d %%-2d %%-2d %%-2d %%-%ds %%-%ds %%-%ds %%-%ds %%-%ds' % (
            max_severity_length,
            max_status_length,
            max_resolution_length,
            max_status_length,
            max_version_length,
            max_component_length,
            max_assigned_length,
            MAX_DESCRIPTION_LEN,)
        
        if len(bug['synopsis']) > MAX_DESCRIPTION_LEN-3:
            synopsis = bug['synopsis'][:MAX_DESCRIPTION_LEN-3]+'...'
        else:
            synopsis = bug['synopsis']
        
        print fmt % (
            bug['id'],
            bug['severity'],
            bug['priority'],
            bug['status'],
            [bug['resolution'], '-'][len(bug['resolution'])==0],
            bug['exists'],
            bug['opened'],
            bug['resolved'],
            bug['closed'],
            bug['status_at_to_date'],
            bug['version'],
            bug['component'],
            bug['assigned'].split('@')[0],
            synopsis)
    
    
if __name__ == '__main__':
    main()

