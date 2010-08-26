#!/bin/env python
#
# run it as 'daily.py'
# or, e.g., 'daily.py 2007-07-12'
#

import sys, pdb, re
import MySQLdb as db

from bugzilla import *

############################################################################
#
# Filter for bugs we are interested in:
#
# Version filter - we will grab only bugs whose version field matches:
#
rex_version = re.compile(r'(2\.3)|(3\.0)') # "has 2.3 or 3.0 in the version field
#
# We will ignore the following components (their bugs, that is):
#
obsolete_comps = ('cw.model',
                  'cw.scripts',
                  'cw.workspace',
                  'deprecated.bic',
                  'deprecated.cbd',
                  'deprecated.supercomm',
                  'deprecated.training')
                  
############################################################################

def bug_filter(bug):
    return bool(rex_version.search(bug['version']))

def err(msg, code):
    print >> sys.stderr, 'Error: %s' % msg
    sys.exit(code)

_today_str = None

def put(label, val, val2=0):
    print '%s    %-39s %4d %4d' % (_today_str, label, val, val2)


def main():

    global _today_str
    
    ###############################################
    #
    # Data collection
    #
    ###############################################
    
    # Get today
    if len(sys.argv) == 1:
        # If no date is specified, we collect on the previous full
        # day (through today 00:00)
        today = datetime.date.today()
        tomorrow = today
        today = datetime.date.fromordinal(today.toordinal()-1)
    else:
        # If date is specified on command line, we do bugs status at the
        # end of that day (by 00:00 of the next day)
        date = sys.argv[1]
        d = date.split('-')
        if not len(d) == 3:
            err('Wrong date format [%s]. Expected format: YYYY-MM-DD' % date, 1)
        today = datetime.date(*map(int, d))
        tomorrow = datetime.date.fromordinal(today.toordinal()+1)
    _today_str = today.isoformat()
    
    # Make today and yeterday datetime objects:
    today = datetime.datetime(*(today.timetuple()[:6]))
    tomorrow = datetime.datetime(*(tomorrow.timetuple()[:6]))
    
    # Get all bugs, with change info from yesterday to today
    bugs = grab_bug_table_with_change(from_date=today,
                                      to_date=tomorrow)

    # Narrow down to bugs we are interested in based on product version
    bugs = filter(bug_filter, bugs)
        
    # filter out bugs that did not exist "today"
    bugs = filter(lambda b: b['exists'], bugs)
    
    ###############################################
    #
    # Data processign and output
    #
    ###############################################

    # Total numbers
    def future(bug): return bug['target'] == 'Future'
    def put_total(label, filter_fnc):
        put(label, len(filter(filter_fnc, bugs)),
                   len(filter(future, filter(filter_fnc, bugs))))
    put_total('total',  lambda b: True)
    put_total('open',   lambda b: b['status'] in OPEN_STATES)
    put_total('fixed',  lambda b: b['status'] in FIXED_STATES)
    put_total('closed', lambda b: b['status'] in CLOSED_STATES)

    # Opened bugs per severity level
    severities = ('blocker','critical','major','normal','minor','trivial','enhancement')
    for s in severities:
        put_total('open.%s' % s,
                  lambda b: b['status'] in OPEN_STATES and b['severity'] == s)
    
    # Opened bugs per components
    comps = filter(lambda c: not c in obsolete_comps, grab_component_list())
    comps.sort()
    for c in comps:
        put_total('open.%s' % c,
                  lambda b: b['status'] in OPEN_STATES and b['component'] == c)
    
    
    # Changes since yesterday
    put('got-opened', sum([b['opened'] for b in bugs]))
    put('got-fixed',  sum([b['resolved'] for b in bugs]))
    put('got-closed', sum([b['closed'] for b in bugs]))
        
    # pdb.set_trace()
    
if __name__ == '__main__':
    main()

