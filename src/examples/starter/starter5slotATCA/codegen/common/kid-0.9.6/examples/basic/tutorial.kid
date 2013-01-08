<?xml version='1.0' encoding='utf-8'?>
<?python 
# python blocks can be embedded anywhere in a template 
# but cannot output anything directly..

# import some modules
import os, time

# define some functions, too
def percent_of(iz, ove):
    return (iz / ove)

# bind some names..
title = "Hello World"
fruits = ["apple", "orange", "kiwi", "M&M"]
ts = time.gmtime()

?>
<html xmlns="http://www.w3.org/1999/xhtml"
      xmlns:py="http://purl.org/kid/ns#">
  <head>
    <!-- 
    dollar-curly-braces can be used to interpolate python expressions in
    text items and attribute values. 
     -->
    <title>Site - $title</title>
    <meta name="generator" value="kid template v${kid.__version__}" />
  </head>
  <body>

    <!-- 
    you can also use design-tool friendly attributes 
    to evaluate expressions for element content. 
    -->
    <h1 py:content="title.upper()">
      This text is replaced with the uppercased title..
    </h1>
    
    <!-- 
    full python expressions let you express yourself fully. :) 
    -->
    <p>We're ${round(percent_of(ts.tm_yday, 365.0) * 100, 2)}% 
    through the year.</p>
    
    <!-- 
    iterate over sequences. the li element is repeated 
    for each item in the list of fruits 
    -->
    <ul>
      <li py:for="fruit in fruits">
        I like ${fruit}s..
      </li>
    </ul>
    
    
    <!-- control output conditionally -->
    <p py:if="ts.tm_wday == 0">
      Somebody's got the case of the <em>Muundays</em>.. :(
    </p>
    
    <!-- 
    define template functions that can be imported and 
    called from other templates
    -->
    <table py:def="display_dict(name_title, val_title, mapping)">
      <tr><th>$name_title</th><th>$val_title</th></tr>
      <tr py:for="name, val in mapping.items()">
        <td>$name</td>
        <td>$val</td>
      </tr>
    </table>
    
  </body>
</html>
