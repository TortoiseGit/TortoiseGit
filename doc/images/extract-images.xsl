<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:html="http://www.w3.org/1999/xhtml" xmlns="http://www.w3.org/1999/xhtml" exclude-result-prefixes="html">
	<xsl:output method="text" />
	<xsl:template match="//graphic">
		<xsl:value-of select="substring-after(@fileref, 'images/')"/><xsl:text>&#xA;</xsl:text>
	</xsl:template>
	<xsl:template match="node()">
		<xsl:apply-templates select="node()"/>
	</xsl:template>
</xsl:stylesheet>
