<?xml version="1.0" encoding="utf-8"?>
<?python #
title = "A document that loads itself"
?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN"
                      "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">
<html xmlns="http://www.w3.org/1999/xhtml"
      xmlns:py="http://purl.org/kid/ns#">
  <head>
    <title py:content="title">Page Title</title>
  </head>
  <body>
    <h1 py:content="title">Page Title</h1>
    <pre py:content="open(__file__).read()">
      File contents will appear here.
    </pre>
  </body>
</html>
