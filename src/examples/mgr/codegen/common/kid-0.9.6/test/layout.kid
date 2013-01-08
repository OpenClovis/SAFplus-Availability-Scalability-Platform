<?xml version='1.0' encoding='utf-8'?>
<?python
title = "layout title"
?>
<testdoc xmlns:py="http://purl.org/kid/ns#">

  <div py:match="item.tag == 'content'">Incorrect content...</div>

  <input py:def="xinput(name)" name="eggs are not what we want" />

  <test>
    <attempt><title>This is the ${title}</title></attempt>
    <expect><title>This is the correct title</title></expect>
  </test>

  <test>
    <attempt><content>This body content should not be displayed</content></attempt>
    <expect><div>This is the correct content!</div></expect>
  </test>

  <test>
    <attempt>${xinput("toast")}</attempt>
    <expect><input name="toast" /></expect>
  </test>

</testdoc>