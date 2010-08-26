#!/bin/env python

import sys, pdb, datetime, time
from pychart import *
from math import sqrt

######################################################
#
# Setting up pychart allowing command line options
# to alter default behavior
#
######################################################

#can = canvas.init('bug_snapshot.png')
theme.get_options()
theme.scale_factor = 1.9
theme.use_color = True
theme.output_format = 'ps'
theme.default_font_size = 6
theme.delta_bounding_box = (-10, -5, +10, +5)
remaining_args = theme.get_options()
theme.reinitialize()

######################################################
#
# Get all data in from argument history files
#
######################################################

history = {}
def load_file(f, history):
    for line in f.readlines():
        (date, label, all, future) = line.split()
        if history.has_key(date):
            history[date][label] = (all, future)
        else:
            history[date] = { label: (all, future) }
            
        
for fname in remaining_args:
    f = file(fname, 'r')
    load_file(f, history)
    f.close()

h = history

today_str = sorted(h.keys())[-1]

######################################################
#
# Create data for total history, in the format:
# (date-cardinal, total, open, fixed, future)
# and plot.
#
######################################################

data = [ (datetime.date(*map(int, d[0].split('-'))).toordinal(),
          int(d[1]['total'][0]),
          int(d[1]['open'][0])-int(d[1]['open'][1]),
          int(d[1]['fixed'][0])+int(d[1]['closed'][0]),
          int(d[1]['open'][1])
         ) for d in sorted(h.items())]

def format_date(ordinal):
    d = datetime.date.fromordinal(int(ordinal))
    return '/6/a90{}' + d.isoformat()

def month_end(d1, d2):
    #print d1, d2
    res = filter(lambda d: datetime.date.fromordinal(d).day==1,
                  range(d1, d2+1))
    #print res
    return res

minx = min(d[0] for d in data)
maxx = max(d[0] for d in data)
ar = area.T(size = (260, 200),
            x_range = (minx, maxx),
            x_axis=axis.X(label='Date',
                          format=format_date,
                          tic_interval=7,
                          tic_len=3,
                          tic_label_offset=(2,-1)),
            x_grid_style=line_style.gray70_dash1,
            x_grid_interval = month_end,
            y_range = (0, max([d[1] for d in data])*1.1),
            y_axis=axis.Y(label="# of bugs",
                          format='/6 %s',
                          tic_len=3,
                          tic_interval=50,
                          tic_label_offset=(-1,1)),
            y_grid_style=line_style.gray70_dash1,
            y_grid_interval = 100,
            legend = legend.T(loc=(10,167)),
            # bg_style = fill_style.gray90,
            border_line_style = line_style.default)

plot_total  = line_plot.T(label="total",  data=data, xcol=0, ycol=1)
plot_open   = line_plot.T(label="open",   data=data, xcol=0, ycol=2)
plot_fixed  = line_plot.T(label="fixed",  data=data, xcol=0, ycol=3)
plot_future = line_plot.T(label="future", data=data, xcol=0, ycol=4)
ar.add_plot(plot_total, plot_open, plot_fixed, plot_future)
ar.draw()
can = canvas.default_canvas()
can.show(200, 210,
         '/9/hC/H/b Release 3.0 bug history and snapshot by day end %s' %
         today_str)

######################################################
#
# Create data for severity piechart for latest open
# bugs and plot it as a piechart
#
######################################################

y_pos_last_open = ar.y_pos(data[-1][2])

# Connect last point in 'open' curve to piechart center
can.line(line_style.black_dash1, 260, y_pos_last_open, 340, 160)

data = filter(lambda d: d[1]>0,
        [('%s(%d)' % (s[0], s[1]), s[1], s[1] and int(20/sqrt(s[1])) or 0)
        for s in [(s[0], int(s[1][0])-int(s[1][1]))
        for s in [(s,
                   sorted(h.items())[-1][1]['open.'+s])
                   for s in ['blocker',
                             'critical',
                             'major',
                             'normal',
                             'minor',
                             'trivial',
                             'enhancement']]]])

# can = canvas.init('totals.eps')
ar = area.T(size=(200,160),
            loc=(240,80),
            legend=None,
            x_grid_style = None, y_grid_style = None)

plot = pie_plot.T(data=data,
                  radius=30,
                  # arc_offsets=([10,5,0,0,10,10,10]),
                  arc_offsets=([d[2] for d in data]),
                  #label_format='/5/hL %s',
                  label_format='/6/hR %s',
                  start_angle=0,
                  #center = (350, 100),
                  # shadow = (2, -2, fill_style.gray50),
                  label_offset = 15,
                  label_col=0,
                  label_fill_style=None,
                  arrow_style = arrow.T(head_len=0,
                                        thickness=0,
                                        line_style=line_style.T(width=0.1)))
ar.add_plot(plot)
ar.draw()

######################################################
#
# Create data for component category piechart for latest open
# bugs and plot it as a piechart
#
######################################################

# Connect last point in 'open' curve to piechart center

# Bug categories for piechart:
categories = {
    'unassigned': ['any.unassigned'],
    'arch':       ['arch'],
    'bic':        ['asp.buffer', 'asp.ckpt',  'asp.cnt',   'asp.dbal',
                   'asp.debug',  'asp.eo',    'asp.hal',   'asp.log',
                   'asp.osal',   'asp.timer', 'asp.utils'],
    'comm':       ['asp.event',  'asp.ioc',   'asp.name',  'asp.rmd'],
    'mgm':        ['asp.alarm',  'asp.cor',   'asp.fault', 'asp.med',
                   'asp.om',     'asp.prov',  'asp.snmp',  'asp.txn'],
    'ha':         ['asp.ams',    'asp.cpm',   'asp.gms'],
    'platform':   ['asp.cm'],
    'tools':      ['tools.build', 'tools.packaging'],
    'ide':        ['ide.codegen', 'ide.idl', 'ide.ui',
                   'ide.logviewer', 'ide.cmdline'],
    'doc':        ['doc.apirefs', 'doc.general', 'doc.ideguide',
                   'doc.installguide', 'doc.relnotes', 'doc.sdkguide',
                   'doc.tutorial'],
    'misc':       ['misc.evalkit']
}

can.line(line_style.black_dash1, 260, y_pos_last_open, 340, 20)

data = filter(lambda d: d[1]>0,
       [( '%s(%d)' % (s[0],
          s[1]),
          s[1], s[1] and int(20/sqrt(s[1])) or 0
        ) for s in [
            ( s[0],
              sum(map(lambda d: int(d[1][0])-int(d[1][1]), s[1]))
            ) for s in [
                ( s,
                  filter(lambda d: d[0][len('open.'):] in categories[s],
                         sorted(h.items())[-1][1].items())
                ) for s in categories]]])

data.sort()
              
#data = for c in categories.items()

# can = canvas.init('totals.eps')
ar = area.T(size=(200,160),
            loc=(240,-60),
            legend=None,
            x_grid_style = None, y_grid_style = None)

plot = pie_plot.T(data=data,
                  radius=30,
                  # arc_offsets=([10,5,0,0,10,10,10]),
                  arc_offsets=([d[2] for d in data]),
                  #label_format='/5/hL %s',
                  label_format='/6/hR %s',
                  start_angle=0,
                  #center = (350, 100),
                  # shadow = (2, -2, fill_style.gray50),
                  label_offset = 15,
                  label_col=0,
                  label_fill_style=None,
                  arrow_style = arrow.T(head_len=0,
                                        thickness=0,
                                        line_style=line_style.T(width=0.1)))
ar.add_plot(plot)
ar.draw()


#pdb.set_trace()
