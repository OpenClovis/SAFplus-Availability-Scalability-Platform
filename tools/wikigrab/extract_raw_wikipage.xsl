<!-- XSLT script to extract text content from Wiki XML page. Usage:
     xsltproc extract_raw_wikipage.xsl input.xml > raw_wiki_text.txt
-->


<xsl:stylesheet
  xmlns:w="http://www.mediawiki.org/xml/export-0.3/"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
  <xsl:output method="text"/>

  <xsl:template match="/">
    <xsl:apply-templates select="//w:text"/>
  </xsl:template>

  <xsl:template match="text">
    <xsl:value-of select="."/>
  </xsl:template>

</xsl:stylesheet>
