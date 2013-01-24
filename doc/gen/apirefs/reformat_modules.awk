#!/usr/bin/awk -f
#
# Short script to remove sub-module lists from the module.html page
#
# <ul> list nested deeper than MAX_LEVEL will be removed from the output
#

BEGIN               { MAX_LEVEL=1; level=0 }

# Find <ul> tags starting at the first position of a line:
/<ul>/              { level++ }

# Print unless we are in a <ul> ... </ul> block
level<=MAX_LEVEL    { print }

# Find end of <ul> ... </ul> block
/<\/ul>/            { level-- }
