<?xml version="1.0" encoding="iso-8859-2"?>
<testdoc xmlns:py="http://purl.org/kid/ns#">

<?python
    encoding = 'iso-8859-2'
    a = '\xb5\xb9\xe8\xbb\xbe'.decode(encoding)
    e = '\xb5\xb9\xe8\xbb\xbe\xfd\xe1\xed'.decode(encoding)
?>

<test>
    <attempt>
        <e a="ľščťž">ľščťžýáí</e>
    </attempt>
    <expect>
        <e a="$a">$e</e>
    </expect>
</test>

</testdoc>
