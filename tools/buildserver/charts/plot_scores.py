#!/bin/env python

import sys
from pychart import *
import pdb
import datetime
import time

#pdb.set_trace()

#
# read the data from stdin or from file specified as first argument
#
if len(sys.argv) == 1:
    f_in = sys.stdin
else:
    f_in = file(sys.argv[1], 'r')

#
# Massage it to right format for pychart
#
lines = f_in.readlines()
data = [(time.mktime(map(int, (l[0]+'-0-0-0-0').split('-'))),
         int(l[1]),
         int(l[2]),
         int(l[3]),
         int(l[4]),
         int(l[2])-int(l[3])-int(l[4]),
         l[5]) 
        for l in [l.split() for l in lines]]


#
# Setup pychart
#
theme.get_options()
theme.scale_factor = 2.0
theme.default_font_size = 7
theme.use_color = True
theme.output_format = 'png'
theme.reinitialize()

def format_date(utc):
    #d = datetime.date.fromordinal(int(ordinal))
    t = time.localtime(utc)
    return '/6/a90{}' + time.strftime('%Y-%m-%d (%H:%M)', t)
    
ar = area.T(size = (300, 200),
            y_range = (0, max([d[2] for d in data])*1.3),
            x_coord = category_coord.T(data, 0),
            legend = legend.T(loc=(310,3)),
            x_axis=axis.X(label='Date',
                          format=format_date,
                          tic_interval=1),
            y_axis=axis.Y(label="# of testcases",
                          tic_interval=50),
            #bg_style = fill_style.gray90,
            border_line_style = line_style.default)
            
bar_plot.fill_styles.reset();

#pdb.set_trace()
#
# Create the plots
#
plot_eb = bar_plot.T(label="error", data=data, bcol=0, hcol=3)
plot_fb = bar_plot.T(label="fail",  data=data, bcol=0, hcol=4, stack_on = plot_eb)
plot_pb = bar_plot.T(label="pass",  data=data, bcol=0, hcol=5, stack_on = plot_fb)

plot_tl = line_plot.T(label="total", data=data, xcol=0, ycol=2) #, data_label_format="/o/5{}%d")
plot_fl = line_plot.T(label="fail",  data=data, xcol=0, ycol=4)
plot_el = line_plot.T(label="error", data=data, xcol=0, ycol=3)

    
ar.add_plot(plot_tl, plot_el, plot_fl)
ar.add_plot(plot_eb, plot_fb, plot_pb)

# print >> sys.stderr, 200, ar.y_pos(200)
can = canvas.default_canvas()

ar.draw()
for d in data:
    can.show(ar.x_pos(d[0])-1,
             ar.y_pos(d[2])+2,
             '/6/a90/hL/vM/H - rev %s %s' % (d[1], d[6]=='*' and '/6/b(/C*/H)' or ''))
can.show(5, 190, '/7/hL/H/b(/C*/H) indicates incomplete TAE runs')
