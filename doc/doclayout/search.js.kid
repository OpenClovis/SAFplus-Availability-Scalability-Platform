<script>
 //alert("search JS");

  searchDict = ${me.searchDict}


  function hitSort(a,b)
  {
    return a.cnt - b.cnt;
  }

  function doSearch(textBoxId,here)
  {
    //alert(textBoxId);
    var terms = document.getElementById(textBoxId);
    //alert(terms.value);
    var words = new Array();
    words = terms.value.toLowerCase().split(" ");
    var wordLup = new Array(words.length);
    var refLup = {};
    for (var i=0;i != words.length ; i++)
      {
        wordLup[i] = new Object;
        if (words[i] in searchDict)
          {
            wordLup[i].word = words[i];
            wordLup[i].refs = searchDict[words[i]];
            //alert(words[i]);
            // Count how many in each reference
            for (var k in wordLup[i].refs)
              {
                if (k in refLup) refLup[k] += wordLup[i].refs[k];
                else refLup[k] = wordLup[i].refs[k];
              }
          }
      }
    //alert(refLup.toString());
    
    var sorted = new Array();
    for (var k in refLup)
      {
        var t = new Object;
        t.ref = k;
        t.cnt  = refLup[k];
        //alert(t.ref);
        sorted.push(t);
      }

    sorted.sort(hitSort);

    var tgt  = document.getElementById("srchResults");
    //alert(tgt);
    while( tgt.hasChildNodes() ) { tgt.removeChild( tgt.lastChild ); }

    // Now display them in sorted order
    for (var i=0; i != sorted.length;i++)
      {
        var s = "e" + sorted[i].ref.toString();
        //alert(s);
        var ctnt = document.getElementById(s);
        //alert(ctnt);
        CopyContent(s,"srchResults",0);
      }
  }

</script>
