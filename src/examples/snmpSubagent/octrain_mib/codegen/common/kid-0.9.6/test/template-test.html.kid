<?xml version="1.0" encoding="utf-8"?>
<templates xmlns:py="http://naeblis.cx/ns/kid2#">
  <!-- create a binding for body elements -->
  <body py:template="x:body|body" py:strip="">
    <h1>Bla Bla</h1>
    
  </body>
  <q py:template="x:q|q" 
     py:strip=""
     py:attributes="kid.chomp(source, 1)">
    "<x py:contentX="stream" py:strip=""/>"
  </q>
</templates>
