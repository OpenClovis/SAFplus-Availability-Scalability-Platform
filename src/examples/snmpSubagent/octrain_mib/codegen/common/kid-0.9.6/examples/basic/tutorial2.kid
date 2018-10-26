<?xml version="1.0" encoding="utf-8"?>
<?python 
# you can treat templates just like python modules
import kid ; kid.enable_import()
from tutorial import display_dict

lil_bears = { 'PHP' : 'Too soft',
              'XSLT' : 'Too hard',
              'Kid' : 'Just right!' }

?>
<html xmlns="http://www.w3.org/1999/xhtml"
      xmlns:py="http://purl.org/kid/ns#">
  <head>
    <title>Let's use that template function</title>
  </head>
  <body>
    <h1>Here's a table of stuff</h1>

    ${display_dict('Template Language', 'Rating', lil_bears)}

  </body>
</html>
