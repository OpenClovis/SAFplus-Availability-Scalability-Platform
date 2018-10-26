<?xml version="1.0" encoding="utf-8"?>
<doesnt-event-matter-i-guess xmlns:py="http://purl.org/kid/ns#">
  
  <test1 py:def="template1()"><p>This is a test</p></test1>

  <test2 py:def="template2(text)"><p py:content="text"/></test2>
  
  <test3 py:def="template3(header, mapping)">
    <h1 py:content="header"/>
    <dl>
      <omit py:for="k,v in mapping.items()" py:strip="">
      <dt py:content="k"/>
      <dd py:content="v"/>
      </omit>
    </dl>
  </test3>
  
</doesnt-event-matter-i-guess>
