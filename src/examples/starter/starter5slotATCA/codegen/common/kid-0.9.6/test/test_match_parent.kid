<?xml version='1.0' encoding='utf-8'?>
<?python

from kid.namespace import Namespace
test = Namespace('urn:test')

?>
<testdoc xmlns:py="http://purl.org/kid/ns#">
  

  <div py:match="item.tag == 'foobar'" class="foobar">second parent - ${[item.text, item.getchildren()]}
  </div>
</testdoc>