<?xml version='1.0' encoding='utf-8'?>
<?python #
media_type = 'text/html'

title = "Hello World"
def say_hello():
  return "Hello world!"
fruits = ["apple", "orange", "kiwi", "M&M"]

some_links = [('http://python.org', 'Python'),
              ('http://www.planetpython.org/', 'Planet Python')]

import time
?>
<html xmlns:py="http://purl.org/kid/ns#">
  <head>
    <title py:content="title">This is replaced.</title>
  </head>
  <body>
    <h3>py:content</h3>
    <p py:content="say_hello()"/>

    <h3>py:repeat</h3>
    <p>These are some things I like to eat:</p>
    <ul>
      <li py:repeat="fruit in fruits" 
          py:content="fruit">A fruit</li>
    </ul>
    
    <h3>py:if (False value)</h3>
    <p>You shouldn't see anything after this paragraph..</p>
    <p py:if="False">This should not be output or put out.</p>
    
    <h3>py:if (True value)</h3>
    <p>You <em>should</em> see something after this paragraph..</p>
    <p py:if="True">This should be output.</p>

    <h3>attribute interpolation</h3>
    <ul>
      <li py:repeat="link in some_links">
        <a href="{link[0]}" py:content="link[1]">Link Text</a>
      </li>
    </ul>
    <p>
      <a title="This {'should'} read {'like'} a {'normal'} sentence."
       >The title attribute should read: "This should read like a normal sentence."</a>
    </p>
    <p>
      <a title="{None}{None}"
       >The title attribute of this element shouldn't be output at all</a>
    </p>

    <h3>py:strip</h3>
    <p>
      <div py:strip="">
        The containing &lt;div&gt; should be omitted.
      </div>
      <div py:strip="False">
        The containing &lt;div&gt; should not be omitted.
      </div>
      <div py:strip="True">
        The containing &lt;div&gt; should be omitted.
      </div>
    </p>
    
    <h3>py:repeat (with nesting)</h3>
    <ul>
      <li py:repeat="num1 in range(1,6)">
        <p py:content="num1"/>
        <ul>
          <li py:repeat="num2 in range(1,6)" 
              py:content="str(num1) + str(num2)"/>
        </ul>
      </li>
    </ul>
  </body>
</html>
