<?xml version='1.0' encoding='utf-8'?>
<?python 
# some fruits
fruits = ["apple", "orange", "kiwi", "M&M"]
?>
<testdoc xmlns:py="http://purl.org/kid/ns#">

  <test>
    <attempt>
      <ul>
        <li py:for="fruit in fruits" py:content="fruit">A fruit</li>
      </ul>
    </attempt>
    <expect>
      <ul>
        <li>apple</li><li>orange</li><li>kiwi</li><li>M&amp;M</li>
      </ul>
    </expect>
  </test>
    
  <test>
    <attempt>
      <ul>
        <li py:for="num1 in range(1,3)">
          <p py:content="num1"/>
          <ul>
            <li py:for="num2 in range(1,3)" 
                py:content="str(num1) + str(num2)"/>
          </ul>
        </li>
      </ul>
    </attempt>
    <expect>
      <ul>
        <li>
          <p>1</p>
          <ul>
            <li>11</li><li>12</li>
          </ul>
        </li><li>
          <p>2</p>
          <ul>
            <li>21</li><li>22</li>
          </ul>
        </li>
      </ul>
    </expect>
  </test>

</testdoc>
