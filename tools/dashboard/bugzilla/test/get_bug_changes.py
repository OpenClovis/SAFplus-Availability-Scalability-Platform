#!/usr/bin/python
"""
Extract info from bugzilla mysql database

This is a prototype for now.
"""

import sys
import MySQLdb as db

from bugzilla_miner import *

MAX_DESCRIPTION_LEN = 70 # Only this much will be shown from the bug desc field

def print_changes():
    
    changes = grab_bug_statuschange_table(
            from_date=datetime.date(2007, 1, 1),
            to_date=datetime.date(2007, 2, 1))
    
    for change in changes:
        print change
    
    
#
# Main function
#
def main():
    print_changes()
    
if __name__ == '__main__':
    main()

