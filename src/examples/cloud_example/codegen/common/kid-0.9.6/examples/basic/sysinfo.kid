<?xml version='1.0' encoding='utf-8'?>
<?python #
import os
import sys
import time
uname = os.uname()

try:
  getloadavg = os.getloadavg
except AttributeError:
  getloadavg = lambda : "(not available)"

?>
<html xmlns:py="http://purl.org/kid/ns#">
  <head>
    <title>System Information</title>
  </head>
  <body>
    <h1>Kid System Info Demo</h1>
    <p>
      This file: ${os.path.abspath(__file__)}<br/>
      Operating System: <span py:content="os.name"/><br/>
      Load: ${repr(getloadavg())}<br/>
      Time: <span py:replace="time.strftime('%C %c')"/>
    </p>

    <h2>Environment</h2>
    <table>
      <tr py:for="k,v in os.environ.items()">
        <td>$k</td>
        <td>$v</td>
      </tr>
    </table>
  </body>
</html>
