<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0"> 

<xsl:import href="../htmlhelp.xsl"/> 
 
<xsl:param name="htmlhelp.encoding" select="'windows-1251'"/> 
<xsl:param name="chunker.output.encoding" select="'windows-1251'"/>

<!-- Saxon-specific parameters
<xsl:param name="saxon.character.representation" select="'native'"/>
<xsl:param name="manifest.in.base.dir" select="1"/>
-->

<!-- produce correct back-of-the-book index for non-English-alphabet languages, 
     don't work with xsltproc (tested up to v.2.06.19)
<xsl:include href="file:///C:/works/TSVN/Tools/xsl/html/autoidx-ng.xsl"/> 
-->

<!-- to use with xml catalog 
<xsl:include href="http://docbook.sourceforge.net/release/xsl/current/html/autoidx-ng.xsl"/> 
-->

</xsl:stylesheet> 
