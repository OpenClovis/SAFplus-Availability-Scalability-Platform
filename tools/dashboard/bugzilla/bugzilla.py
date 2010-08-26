#!/usr/bin/python
"""
Common definitions for bugzilla mining
"""

import sys, pdb
import MySQLdb as db
import string
import datetime

HOST = '192.168.0.8'
USER = 'c3po'
PASSWD = 'poc3secret'
DB = 'bugs'
PORT = 3306

_con = None
_cursor = None

#
# Open connection to MySQL database and create dictionary based cursor
#
def db_connect(host=HOST, user=USER, passwd=PASSWD, dbname=DB, port=PORT):
    """Open connection to MySQL database and create dictionary based cursor"""
    global _con, _cursor
    try:
        _con = db.connect (host   = host,
                          user   = user,
                          passwd = passwd,
                          db     = dbname,
                          port   = port)
    except db.Error, e:
        print 'Error %d: %s' % (e.args[0], e.args[1])
        sys.exit (1)
        
    _cursor = _con.cursor(db.cursors.DictCursor) # Using dictionary style results

#
# Close database cursor and database connection
#
def db_close():
    """Close database cursor and database connection"""
    global _cursor, _con
    if _cursor != None:
        _cursor.close ()
        _cursor = None
    if _con != None:
        _con.close ()
        _con = None

#
# debug message
#
def db_msg(msg):
    print >> sys.stderr, msg
    
def grab_bug_table():
    """
    Grabs the entire bug table, and returns an iterable object where each
    entry is a bug, represented by a dictionary of the following string keys:
    'id', 'assigned', etc. (see AS column of SELECT statement below).
    """
    if not _con or not _cursor:
        db_connect()
    cursor = _cursor
    
    sql_request = """
        SELECT      b.bug_id            AS id,
                    p.login_name        AS assigned,
                    b.bug_severity      AS severity,
                    b.bug_status        AS status,
                    b.short_desc        AS synopsis,
                    b.priority          AS priority,
                    b.resolution        AS resolution,
                    b.creation_ts       AS creation_ts,
                    b.version           AS version,
                    b.target_milestone  AS target,
                    c.name              AS component
        FROM        bugs                AS b,
                    profiles            AS p,
                    components          AS c
        WHERE       b.assigned_to = p.userid AND
                    b.component_id = c.id
        ORDER BY    b.bug_id
    """
    
    # db_msg('mysql request: %s' % sql_request)
    cursor.execute(sql_request)
    bugs = cursor.fetchall()
    
    # db_msg('Number of entries: %d' % len(bugs))
    return bugs

def grab_bug_statuschange_table(from_date=None, to_date=None):
    """
    Grabs the table representing bug status changes.
    """
    if not _con or not _cursor:
        db_connect()
    cursor = _cursor
        
    sql_request = """
        SELECT      f.fieldid           AS id
        FROM        fielddefs           AS f
        WHERE       f.name = 'bug_status'
    """
    cursor.execute(sql_request)
    fields = cursor.fetchall()
    assert(len(fields)==1)
    fieldid_status = int(fields[0]['id'])
    # db_msg('field id of bug_status is %d' % fieldid_status)
    
    date_test = ''
    if from_date:
        date_test += """ AND
                    a.bug_when > '%d-%02d-%02d %02d:%02d:%02d'
        """ % (from_date.year, from_date.month, from_date.day,
               from_date.hour, from_date.minute, from_date.second)
    if to_date:
        date_test += """ AND
                    a.bug_when < '%04d-%02d-%02d %02d:%02d:%02d'
        """ % (to_date.year, to_date.month, to_date.day,
               to_date.hour, to_date.minute, to_date.second)
        
    sql_request = """
        SELECT      b.bug_id            AS id,
                    p.login_name        AS who,
                    a.bug_when          AS date,
                    a.removed           AS from_state,
                    a.added             AS to_state,
                    b.short_desc        AS synopsis
        FROM        bugs_activity       AS a,
                    bugs                AS b,
                    profiles            AS p
        WHERE       a.bug_id = b.bug_id AND
                    a.who = p.userid AND
                    a.fieldid = '%d' %s
        ORDER BY    a.bug_when
        """ % (fieldid_status, date_test)
    # db_msg('mysql request: %s' % sql_request)
    cursor.execute(sql_request)
    changes = cursor.fetchall()
    
    # db_msg('Number of entries: %d' % len(changes))
    return changes

FIXED_STATES = ['RESOLVED', 'VERIFIED']
CLOSED_STATES = ['CLOSED']
OPEN_STATES = ['NEW', 'ASSIGNED', 'REOPENED']

OPEN_STATE = 0
FIXED_STATE = 1
CLOSED_STATE = 2

state_map = { 'ASSIGNED': OPEN_STATE,
              'CLOSED':   CLOSED_STATE,
              'NEW':      OPEN_STATE,
              'REOPENED': OPEN_STATE,
              'RESOLVED': FIXED_STATE,
              'VERIFIED': FIXED_STATE }
              
def analyze_changes(current_status, creation_date,
                    from_date, to_date, changelist):

    exists = opened = closed = fixed = 0
    # pdb.set_trace()
    
    if creation_date > to_date:
        # it has not been created yet
        return (exists, opened, fixed, closed, '-')
    else:
        exists = 1
    
    if creation_date >= from_date:
        opened = 1

    if changelist:
        # print current_status, changelist
        # assert(current_status == changelist[-1]['to_state'])
        changelist1 = [change
                       for change 
                       in changelist
                       if change['date'] <= to_date]
        changelist2 = [change
                       for change 
                       in changelist
                       if change['date'] > to_date]
        if changelist1 and changelist2:
            assert(changelist1[-1]['to_state'] ==
                   changelist2[0]['from_state'])

        if changelist2: # some change happened since closing date
            closing_state = changelist2[0]['from_state']
        else:
            closing_state = current_status
            
        if changelist1: # some changes happened during the specified window
            for change in changelist1:
                from_state = state_map[change['from_state']]
                to_state = state_map[change['to_state']]
                if from_state == to_state:
                    continue
                # the following transients should never happen
                assert(not (from_state == OPEN_STATE and to_state == CLOSED_STATE))
                #assert(not (from_state == CLOSED_STATE and to_state == FIXED_STATE))
                if to_state == FIXED_STATE:
                    fixed += 1
                elif to_state == CLOSED_STATE:
                    closed += 1
                else:
                    opened += 1
        
    else:
        closing_state = current_status
        
    return (exists, opened, fixed, closed, closing_state)

def grab_bug_table_with_change(from_date, to_date=None):
    """ Same as grab_bug_table, but also incorporates status change info """
    
    bugs = grab_bug_table()
    
    changes = grab_bug_statuschange_table(
            from_date=from_date,
            to_date=to_date)
    
    for bug in bugs:
        #pdb.set_trace()
        (exists, opened, resolved, closed, status_at_to_date) = \
            analyze_changes(bug['status'],
                    bug['creation_ts'],
                    from_date, to_date,
                    [change for change in changes if change['id'] == bug['id']])
                
        bug['exists'] = exists
        bug['opened'] = opened
        bug['resolved'] = resolved
        bug['closed'] = closed
        bug['status_at_to_date'] = status_at_to_date

    return bugs        

def grab_component_list():
    """ Returns with a list of component names """
    if not _con or not _cursor:
        db_connect()
    cursor = _cursor
        
    sql_request = """
        SELECT      name
        FROM        components
    """
    cursor.execute(sql_request)
    comps = cursor.fetchall()
    
    return map(lambda c:c['name'], comps)
