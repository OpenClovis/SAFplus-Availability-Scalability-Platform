<?xml version='1.0' encoding='utf-8'?>
<?python 
# import some modules
import os
import sys
import time
from os.path import dirname, basename, abspath, join as joinpath

# these are used by our cgi handler
media_type = 'text/html'
encoding = 'utf-8'

try:
  getloadavg = os.getloadavg
except AttributeError:
  getloadavg = lambda : "(not available)"

context = {}

?>
<html xmlns:py="http://purl.org/kid/ns#">
  <?python
  if context.has_key("page"):
    page = context["page"].value
  else:
    page = 'sys'
  ?>
  <head>
    <title>System Information</title>
  </head>
  <body>
    <h1>Kid System Info Demo</h1>
    
    <a href="?page=sys">sys</a> | 
    <a href="?page=env">env</a> |
    <a href="?page=dir">dir</a>
    
    <!-- System Information -->
    <div py:if="page == 'sys'">
      <h2>General Info</h2>
      <p>
      This file: <span py:content="abspath(__file__)"/><br/>
      Operating System: <span py:content="os.name"/><br/>
      Load: <span py:content="repr(getloadavg())"/><br/>
      Time: <span py:content="time.strftime('%C %c')"/>
      </p>
    </div>

    <!-- Environment -->
    <div py:if="page == 'env'">
      <h2>Environment</h2>
      <table>
        <tr py:for="k,v in os.environ.items()">
          <td py:content="k"/>
          <td py:content="v"/>
        </tr>
      </table>
    </div>
    
    <!-- Directory list -->
    <div py:if="page == 'dir'">
      <ul>
        <?python
        from glob import glob
        ?>
        <li py:for="file in glob(joinpath(dirname(__file__), '*'))"
            py:if="not os.path.isdir(file)"
            ><a href="?page=file&amp;file={file}" py:content="basename(file)">
             The file name will go here</a>
        </li>
      </ul>
    </div>

    <!-- View a File -->
    <div py:if="page == 'file'">
      <?python
      filename = context.has_key('file') and context["file"].value
      if not filename:
        raise "No filename provided.."
      ?>
      <pre py:content="open(filename).read()"/>
    </div>
  </body>
</html>
