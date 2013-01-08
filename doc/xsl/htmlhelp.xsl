<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0"> 

<xsl:import href="./db_htmlhelp.xsl"/> 
<xsl:import href="./defaults.xsl"/> 
 <xsl:param name="keep.relative.image.uris" select="0"/>

<xsl:param name="suppress.navigation" select="0"/> 
<xsl:param name="toc.section.depth" select="4"/> 
<xsl:param name="htmlhelp.force.map.and.alias" select="1"/> 
<xsl:param name="htmlhelp.show.menu" select="1"/> 
<xsl:param name="htmlhelp.show.advanced.search" select="1"/> 
<xsl:param name="htmlhelp.hhc.folders.instead.books" select="1"></xsl:param>
<xsl:param name="htmlhelp.hhc.binary" select="0"></xsl:param>
<xsl:param name="htmlhelp.use.hhk" select="1"></xsl:param>

</xsl:stylesheet> 
