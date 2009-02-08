<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0"> 
 
<!--
  PDF help creation
  Include the default settings for all languages
  Define xsl parameters here
  Define fop parameters (fonts) in userconfig.template.xml
-->

<xsl:import href="../pdfdoc.xsl"/> 

<xsl:param name="body.font.family" select="'Times New Roman'"/> 
<xsl:param name="title.font.family" select="'Arial'"/> 
<xsl:param name="sans.font.family" select="'Arial'"/> 
<xsl:param name="monospace.font.family" select="'Courier New'"/> 


</xsl:stylesheet> 
