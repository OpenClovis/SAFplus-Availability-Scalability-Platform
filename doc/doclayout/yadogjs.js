
function FindByAttributeValue(attribute, value)    {
  var All = document.getElementsByTagName('*');
  for (var i = 0; i < All.length; i++)       {
    if (All[i].getAttribute(attribute) == value) { return All[i]; }
  }
}

function onHashChange() {
    var hsh=location.hash.replace('#','');
    if (hsh=='') hsh='Home';
    var hshArr = hsh.split('__');
    var childPage = hshArr[0];
    var anchorLoc = '';
    if (hshArr.length > 1)
    {
        anchorLoc = hshArr[1];
    }
    ReplaceChildrenWithUri('center',childPage + '.html'); 
    console.log('loaded ' + childPage + '.html');
    LoadScript('centerscript',childPage + '.js');
    if (anchorLoc)
    {
      console.log("looking for " + anchorLoc);
      var elem = FindByAttributeValue("hash",anchorLoc);
      if (elem)
        {
        console.log("found " + anchorLoc);
        //console.log(elem.parentNode.nextSibling.nodeValue);
        setTimeout(function() {
          console.log("show " + anchorLoc);
          swapAttr(elem,'class','oclass');
/*
          var n = document.createTextNode('DDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDD');
          elem.parentNode.appendChild(n);
          elem.className = "varsTableDescRow";
          var tmp = elem.parentNode.style.display;
          elem.parentNode.style.display = 'none';
          elem.parentNode.style.display = 'block';   */
}, 500);
        }
    }
}


function getUrlParameters(parameter, staticURL, decode){
   /*
    Function: getUrlParameters
    Description: Get the value of URL parameters either from 
                 current URL or static URL
    Author: Tirumal
    URL: www.code-tricks.com
   */
     
   var currLocation = (staticURL.length)? staticURL : window.location.search,
       parArr = "",
       returnBool = true;

   try
   {
     parArr = currLocation.split("?")[1].split("&");
   } catch(e)
   {
     return "";
   } 
  
   for(var i = 0; i < parArr.length; i++){
        parr = parArr[i].split("=");
        if(parr[0] == parameter){
            return (decode) ? decodeURIComponent(parr[1]) : parr[1];
            returnBool = true;
        }else{
            returnBool = false;            
        }
   }
   
   if(!returnBool) return false;  
}

function toggleShow(itemId) {
  var ctnt = document.getElementById(itemId);

  if ((ctnt.style.display == "none") || (ctnt.style.display == "")) {
    if (ctnt.getAttribute("actualdisplay"))
      ctnt.style.display = ctnt.getAttribute("actualdisplay");
    else
      ctnt.style.display = "block";
  }
  else {
    ctnt.setAttribute("actualdisplay",ctnt.style.display);
    ctnt.style.display = "none";
  }

  return 1;
} 

function setClassTo(ctnt,newclass) {
  //var ctnt = document.getElementById(itemId);

  if (ctnt.getAttribute("oclass") == NULL)
    {
      ctnt.setAttribute("oclass",ctnt.style.class);
    }
  ctnt.style.class = newclass;
  return 1;
} 

function restoreClass(ctnt) {
  ctnt.style.class = ctnt.style.oclass
  return 1;
}


function swapAttr(node,attr1,attr2)
{
  //alert(node.toString() + " " + node.nodeName + " " + node.nodeValue );

  var tmp = node.getAttribute(attr2);
  node.setAttribute(attr2,node.getAttribute(attr1));
  node.setAttribute(attr1,tmp);
  return 1;
}


function SwapContent(contentId, toId, hideId){
  var ctnt = document.getElementById(contentId);
  var hide = document.getElementById(hideId) ;
  var tgt  = document.getElementById(toId);

  kids = tgt.childNodes;
 
  for (var i = 0; i < kids.length; i++) {
    hide.appendChild(kids[i]);
  }
  tgt.appendChild(ctnt);
}

function MoveContent(contentId,toId){
  var ctnt = document.getElementById(contentId);
  var tgt  = document.getElementById(toId);
  tgt.appendChild(ctnt);
}

function CopyContent(contentId,toId,remExisting){
  var ctnt = document.getElementById(contentId);
  var tgt  = document.getElementById(toId);
  var copy = ctnt.cloneNode(true);
  copy.removeAttribute('id');
  if (remExisting) while( tgt.hasChildNodes() ) { tgt.removeChild( tgt.lastChild ); }
  tgt.appendChild(copy);
}

function LoadScript(id,src) {
  var newScript = null;
/* Does not allow you to overwrite a script
  if (id != null) 
    {
      newScript = document.getElementById(id);
      if (newScript) return 1;
    }
*/
  var headID = document.getElementsByTagName("head")[0];      
  newScript = document.createElement('script'); 
  newScript.id = id;
  newScript.type = 'text/javascript';
  newScript.src = src;
  headID.appendChild(newScript);
  return 1;
}
