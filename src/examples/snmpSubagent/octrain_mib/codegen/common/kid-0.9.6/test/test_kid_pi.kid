<?xml version='1.0' encoding='utf-8'?>
<?python global_level = 'global scope' ?>
<?py
if 1 == 1:
  indented_block = 1
scope_test = global_level
?>

<?python ?>
<?python

?>
<testdoc xmlns:py="http://purl.org/kid/ns#">
  <?python
    global scope_test
    local_level = 'local scope'
  ?>
  <test>
    <attempt py:content="global_level">some stuff</attempt>
    <expect>global scope</expect>
  </test>
  <test>
    <attempt py:content="scope_test">some stuff</attempt>
    <expect>global scope</expect>
  </test>
  <?python scope_test = local_level ?>
  <test>
    <attempt py:content="local_level">some stuff</attempt>
    <expect>local scope</expect>
  </test>
  <test>
    <attempt py:content="scope_test">some stuff</attempt>
    <expect>local scope</expect>
  </test>
  <?python indent_test_1 = "pass"  ?>
  <?python
indent_test_2 = indent_test_1  ?>
  <?python
  indent_test_3 = indent_test_2  ?>
  <?python indent_test_4 = indent_test_3
indent_test_5 = indent_test_4
  ?>
  <?python indent_test_6 = indent_test_5
  indent_test_7 = indent_test_6
  ?>
  <?python if indent_test_7 == "pass":
indent_test_8 = indent_test_7
  ?>
  <?python if indent_test_8 == "pass":
  indent_test_9 = indent_test_8
  ?>
  <?python if indent_test_9 == "pass":
  if indent_test_9 == "pass":
    if indent_test_9 == "pass":
      indent_test = "pass"
    else:
      indent_test = "failed"
  ?>
  <test>
    <attempt py:content="indent_test">some stuff</attempt>
    <expect>pass</expect>
  </test>
</testdoc>
