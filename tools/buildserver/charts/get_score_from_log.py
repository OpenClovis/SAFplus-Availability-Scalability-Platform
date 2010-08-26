#!/bin/env python

import re, sys, os, pdb

n_tc = 0 # # of testcases
n_f  = 0 # # of fails
n_e  = 0 # # of errors

rex_rev     = re.compile(r'Build number \(rev\): (?P<rev>\d+)')
rex_run     = re.compile(r'TAE test results for \[(?P<run>[\w-]+)\]')
rex_total   = re.compile(r'Total testcases run: (?P<n>\d+)')
rex_result  = re.compile(r'Result summary:\s+(?P<res>.*)')
rex_fe      = re.compile(r'FAILED \(failures=(?P<f>\d+), errors=(?P<e>\d+)\)')
rex_f       = re.compile(r'FAILED \(failures=(?P<f>\d+)\)')
rex_e       = re.compile(r'FAILED \(errors=(?P<e>\d+)\)')

rev = '0'
run = None
runs = {}
bad_apple = False

if len(sys.argv) == 1: fin = sys.stdin
else: fin = file(sys.argv[1], 'r')

while True:

    line = fin.readline()
    if not line:
        break

    m = rex_rev.match(line)
    if m:
        rev = m.groupdict()['rev']
        continue
    
    m = rex_run.match(line)
    if m:
        if run: # this means previous run was not completed
            runs[run].append('TAE RUN COULD NOT COMPLETE!')
            bad_apple = True
        run = m.groupdict()['run']
        runs[run] = [len(runs)+1,]
        continue
        
    m = rex_total.match(line)    
    if m:
        tc = int(m.groupdict()['n'])
        n_tc += tc
        continue
    
    m = rex_result.match(line)
    if m:
        res = m.groupdict()['res']
        m = rex_fe.match(res)
	# pdb.set_trace()
	f = e = 0
        if m:
            f = int(m.groupdict()['f'])
            n_f += f
            e = int(m.groupdict()['e'])
            n_e += e
        m = rex_f.match(res)
        if m:
            f = int(m.groupdict()['f'])
            n_f += f
        m = rex_e.match(res)
        if m:
            e = int(m.groupdict()['e'])
            n_e += e
        runs[run].append('(%d/%d/%d/%d)' % (tc, tc-f-e, f, e))
        run = None


#pdb.set_trace()
if run: # this means last run was not completed
    runs[run].append('TAE RUN COULD NOT COMPLETE!')
    bad_apple = True
    
print 'Test run summary (details further below):'
print '========================================='
print '  Run                     (Total/Pass/Fail/Error)'
print '  -----------------------------------------------'
for key, data in sorted(runs.items(), key=lambda s: s[1][0]):
    try:
        print '  %-23s %s' % (key, data[1])
    except:
        print '  %-23s TAE RUN COULD NOT COMPLETE!' % key
        bad_apple = True
print rev, n_tc, n_f, n_e, bad_apple and '*' or '+'
