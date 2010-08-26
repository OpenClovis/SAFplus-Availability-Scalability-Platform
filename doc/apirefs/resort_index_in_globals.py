#!/usr/bin/python

import sys, re, string, pdb

PREFIX=r'(Cl|cl|CL_)'

class NotFound(Exception): pass

def find_in_list(lines, pattern, occurrance):
    found = 0
    for i, line in enumerate(lines):
        if re.match(pattern, line):
            found += 1
            if found == occurrance:
                return i
    raise NotFound('Cound not locate %d(st/nd/th) occurrance of %s' % (occurrance, pattern))

# read in all lines from file
lines = sys.stdin.readlines()

# find line of 2nd div which marks first line after haeader block
# beginning of 2nd <div>
div2_start = find_in_list(lines, r'<div class="tabs">\n', 2)
o_header_lines = lines[:div2_start]

#find trailer lines
start_of_trailer = find_in_list(lines, r'<small>Generated on', 1)-1
end_of_trailer = find_in_list(lines, r'</html>', 1)
o_trailer_lines = lines[start_of_trailer:end_of_trailer+1]


o_list_lines = []
line_marker = find_in_list(lines, r'.*</div>\n', 2)

# Start processing anchor lists
while True:
    lines = lines[line_marker:]
    try:
        anchor_1st = find_in_list(lines, r'.*a class="anchor" name="index_.*\n', 1)
    except NotFound:
        break
    li_first = anchor_1st+1
    li_last = li_first + find_in_list(lines[li_first:], r'.*</ul>\n', 1)

    # Rearrange the index <li> lines to have proper line breaking
    o_list_lines.append(lines[li_first][:-1] + \
                        lines[li_first+1].split('</a>')[0] + \
                        '</a>\n')
    for i in range(li_first+1, li_last):
        o_list_lines.append(lines[i].split('</a>')[1][:-1] + \
                            lines[i+1].split('</a>')[0] + \
                            '</a>\n')
    li_last-=1
    
    line_marker = li_last

# Now each <li> line looks like this (example):
# <li>clQueueNodeDelete(): <a class="el" href="a00352.html#g0d8c74eaab2668d6bacd99cc43653d8e">clQueueApi.h</a>

# Generate an index key for each <li> line, removing a predefined 'cl' prefix
tab = [(re.sub(r'^%s' % PREFIX, '', # will remove PREFIX
               li.split(':')[0]     # of line content up to ':' 
                            [4:])   #   but without the starting <li> tag
       , li)              # add original line as 2nd item in tuple
       for li in o_list_lines]

# Now create a dictionary per english letter, containing the index entries
# that start with that letter (case insensitive)
indices = {}
letters = '_'+string.ascii_uppercase
for letter in letters:
    entries = [e for e in tab if e[0][0].upper()==letter]
    if len(entries):
        sys.stderr.write('found %d entries for letter %s\n' % 
                         (len(entries), letter))
        # sort the entries right away
        entries.sort(key=lambda e: e[0])
        indices[letter] = entries
        
o_list_lines = [] # reuse it
# ... and then appending the new content lines, per alphabet
for letter in letters:
    if not indices.has_key(letter):
        continue
    o_list_lines.append('<h3><a class="anchor" name="index_%s">- %s -</a></h3><ul>\n' %
                     (letter, letter))
    for entry in indices[letter]:
        o_list_lines.append(entry[1])
    o_list_lines.append('</ul>\n')

# Create the tab references for each letter needed
o_tab_lines = [
    '<div class="tabs">\n',
    '  <ul>\n'
]
for letter in letters:
    if not indices.has_key(letter):
        continue
    o_tab_lines.append('    <li><a href="#index_%s"><span>%s</span></a></li>\n' %
                       (letter, letter))
o_tab_lines += [
    '  </ul>\n',
    '</div>\n',
    '\n',
    '<p>\n',
    '&nbsp;\n',
    '<p>\n'
]


# Time to write out the various blocks to the standard output
for line in o_header_lines: print line,
for line in o_tab_lines: print line,
for line in o_list_lines: print line,
for line in o_trailer_lines: print line,
