"""
A bunch of code blocks for testing indentation detection..
"""

blocks = []

def block(text, expect=None):
    blocks.append( (text.strip(), (expect or text).strip()) )

block("""
# comment
""")

block("""
call()
""")

block("""
\t\t\t# comment
""", """
# comment
""")

block("""
\t\t\tcall()
""", """
call()
""")

block("""
if test:
\tprint 'bla'
""")

block("""
if test:
\t\t\t\t\tprint 'bla'
""", """
if test:
\tprint 'bla'
""")

block("""
if test:
 print 'bla'
""", """
if test:
\tprint 'bla'
""")

block("""
if test:
     print 'bla'
""", """
if test:
\tprint 'bla'
""")

block("""
if test:
\t\t\t\t\tprint 'for'
else:
\t\t\t\t\tprint 'bla'
""")

block("""
if test:
print 'bla'
""", """
if test:
\tprint 'bla'""")

block("""
if x:
    print 'foo'
  else:
    print 'bar'
""","""
if x:
  print 'foo'
else:
  print 'bar'
""")

block("""
if x:
\tprint 'foo'
\tif y:
\t\tprint 'bar'
""")

block("""
import bla
bla.bla()
""")

block("""
# if x:
print 'bla'
""")

block("""
if f(x,
\ty) == 1:
\tprint 'bla'
""")

block("""
if f(x,
y) == 1:
print 'bla'
""", """
if f(x,
\ty) == 1:
\tprint 'bla'
""")

block("""
if f(x) == 1: # bla
print 'bla'
""", """
if f(x) == 1: # bla
\tprint 'bla'
""")

block("""
if "#":
pass""", """
if "#":
\tpass
""")

block("""
try:
\ttest()
except:
\toops()
""")

block("""
class x:
pass""", """
class x:
\tpass
""")

block("""
# ignore comments
\t\twhile something:
\t\t\tdo(something)
\t\telse:
\t\t\tsleep()
""", """
# ignore comments
while something:
\tdo(something)
else:
\tsleep()
""")

block("""
\t\t\t\t\t
\tclass x:
\t\t\t\t\t
\t\tpass
\t\t\t\t\t
\texit(1)
""", """

class x:

\tpass

exit(1)
""")

block("""
\tif this:
\t\tdo(that)
\telse # oops
\t\tforgot(something)
""", """
if this:
\tdo(that)
else # oops
\tforgot(something)
""")

block("""
elif something:
pass""")

block("""
else:
pass""")

block("""
except:
pass""")

block("""
if1:
pass""")
