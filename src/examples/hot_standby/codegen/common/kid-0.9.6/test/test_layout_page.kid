<?xml version='1.0' encoding='utf-8'?>
<?python
layout_params["title"] = "correct title"
?>
<html xmlns:py="http://purl.org/kid/ns#" py:layout="'layout.kid'">

  <div py:match="item.tag == 'content'">This is the correct content!</div>

  <input py:def="xinput(name)" name="${name}" />

</html>