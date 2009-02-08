<xsl:stylesheet
 xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
 xmlns:fo="http://www.w3.org/1999/XSL/Format"
 version="1.0">

  <xsl:import href="../pdfdoc.xsl"/>

  <xsl:param name="l10n.gentext.language" select="'ja'"/>

  <xsl:param name="body.font.family" select="'Mincho'"></xsl:param>
  <xsl:param name="monospace.font.family" select="'Mincho'"></xsl:param>
  <xsl:param name="title.font.family" select="'Gothic'"></xsl:param>

  <xsl:param name="hyphenate">false</xsl:param>

  <xsl:template match="menuchoice">
    <fo:inline font-family="Gothic">
      <xsl:call-template name="process.menuchoice"/>
    </fo:inline>
  </xsl:template>

  <xsl:template match="guilabel">
    <fo:inline font-family="Gothic">
      <xsl:call-template name="inline.charseq"/>
    </fo:inline>
  </xsl:template>

  <xsl:template match="guibutton">
    <fo:inline font-family="Gothic">
      <xsl:call-template name="inline.charseq"/>
    </fo:inline>
  </xsl:template>

  <xsl:template match="keysym">
    <fo:inline font-family="Symbol">
      <xsl:call-template name="inline.charseq"/>
    </fo:inline>
  </xsl:template>

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
