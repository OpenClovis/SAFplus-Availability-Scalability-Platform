<?xml version='1.0' encoding='utf-8'?>
<?python

include_common = "<x xmlns='urn:test2' xmlns:t='urn:test2'><foo t:x='y'>bar</foo></x>"

?>
<testdoc xmlns:py="http://purl.org/kid/ns#"
         xmlns:xhtml="http://www.w3.org/1999/xhtml"
         xmlns:test="urn:test"
         lang="en" xml:lang="en">

  <test>
    <attempt><test:elem x="10" test:x="20">some text</test:elem></attempt>
    <expect><test:elem x="10" test:x="20">some text</test:elem></expect>
  </test>

  <test>
    <attempt><elem xmlns="urn:test" x="10" test:x="20">some text</elem></attempt>
    <expect><test:elem x="10" test:x="20">some text</test:elem></expect>
  </test>

  <test>
    <attempt><elem xmlns="urn:test" x="10" test:x="20"><test/></elem></attempt>
    <expect><test:elem x="10" test:x="20"><test:test/></test:elem></expect>
  </test>

  <test>
    <attempt>${XML(include_common)}</attempt>
    <expect><x xmlns="urn:test2"><foo ns1:x='y' xmlns:ns1='urn:test2'>bar</foo></x></expect>
  </test>

</testdoc>
