<?xml version="1.0" encoding="utf-8"?>
<testdoc xmlns:py="http://purl.org/kid/ns#">

<?python
    var = u'ca\xe7\xe3o'
?>
    <span py:content="var" />
    <span title="${var*2}">${var}</span>
    <span py:replace="var" />

<?python
    var = u'ca\xe7\xe3o'.encode("utf-8")
?>
    <span py:content="var" />
    <span title="${var*2}">${var}</span>
    <span py:replace="var" />
</testdoc>
