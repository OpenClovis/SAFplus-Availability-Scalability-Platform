<?xml version='1.0' encoding='utf-8'?>
<?xml-stylesheet some stuff?>
<testdoc xmlns:py="http://purl.org/kid/ns#">
  <test>
    <attempt><div><!-- check that comments pass through --></div></attempt>
    <expect type="text">&lt;div>&lt;!-- check that comments pass through -->&lt;/div></expect>
  </test>

  <test>
    <attempt><?test-pi check that processing instructions pass through ?></attempt>
    <expect><?test-pi check that processing instructions pass through ?></expect>
  </test>

  <?python 
  x = 42
  y = 'string of text'
  ?>
  <test>
    <attempt><!-- <span value="${x}" py:content="y"/> --></attempt>
    <expect><!-- <span value="42" py:content="y"/> --></expect>
  </test>
</testdoc>
