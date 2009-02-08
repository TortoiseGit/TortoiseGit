<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

<xsl:import href="../htmlhelp.xsl"/>

<xsl:param name="chunker.output.encoding" select="'GB18030'"/>
<xsl:param name="htmlhelp.encoding" select="'GB18030'"/>

<xsl:template name="person.name.family-given">
  <xsl:param name="node" select="."/>

  <xsl:if test="$node//honorific">
    <xsl:apply-templates select="$node//honorific[1]"/>
    <xsl:value-of select="$punct.honorific"/>
  </xsl:if>

  <xsl:if test="$node//firstname">
    <xsl:if test="$node//honorific">
      <xsl:text> </xsl:text>
    </xsl:if>
    <xsl:apply-templates select="$node//firstname[1]"/>
  </xsl:if>

  <xsl:if test="$node//othername and $author.othername.in.middle != 0">
    <xsl:if test="$node//honorific or $node//firstname">
      <xsl:text> </xsl:text>
    </xsl:if>
    <xsl:apply-templates select="$node//othername[1]"/>
  </xsl:if>

  <xsl:if test="$node//surname">
    <xsl:if test="$node//honorific or $node//firstname
                  or ($node//othername and $author.othername.in.middle != 0)">
      <xsl:text> </xsl:text>
    </xsl:if>
    <xsl:apply-templates select="$node//surname[1]"/>
  </xsl:if>

  <xsl:if test="$node//lineage">
    <xsl:text>, </xsl:text>
    <xsl:apply-templates select="$node//lineage[1]"/>
  </xsl:if>
</xsl:template>

</xsl:stylesheet>
