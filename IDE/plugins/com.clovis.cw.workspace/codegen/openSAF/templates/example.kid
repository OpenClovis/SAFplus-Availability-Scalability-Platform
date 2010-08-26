<?xml version='1.0' encoding='utf-8'?>
<root xmlns="http://www.w3.org/1999/xhtml" xmlns:py="http://purl.org/kid/ns#">
/* See for kid template language doc: http://www.kid-templating.org/language.html */

/* Simple Substitution ${title} */


/* Note the xml tags are completely arbitrary they will be removed */
/* IF statement: */
<main py:if="addmain is True">
void main()
  {
    printf("${startupString}");
  }
</main>

/* FOR statement */
<z py:if="defined('globals')" py:for="v in globals">
${v.mod} ${v.type} ${v.name};  
</z>

/* Jam in Python code -- Please do NOT use for nontrivial functions */
<?python
import time
?>
/* Evaluation replacement */
/* This file generated at <p py:content="time.strftime('%C %c')">The Time</p> */

/* Define a named sub-template (i.e. a function) */
<t py:def="list2typedef(name,lst)">
typedef struct $name
  {
<z py:for="v in lst">  ${v.mod} ${v.type} ${v.name}; 
</z>
  } ${name}T;
</t>

/* Now call that function */
<z py:for="name,val in typedefs.items()">
${list2typedef(name,val)}
</z>


</root>

