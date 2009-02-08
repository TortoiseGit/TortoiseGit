<xsl:stylesheet
 xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
 xmlns:fo="http://www.w3.org/1999/XSL/Format"
 version="1.0">

  <xsl:import href="../pdfdoc.xsl"/>

  <xsl:param name="l10n.gentext.language" select="'zh_cn'"/>

  <xsl:param name="body.font.family" select="'simsun'"></xsl:param>
  <xsl:param name="dingbat.font.family" select="'simhei'"></xsl:param>
  <xsl:param name="monospace.font.family" select="'simsun'"></xsl:param>
  <xsl:param name="title.font.family" select="'simhei'"></xsl:param>

  <xsl:param name="paper.type" select="'A4'"></xsl:param>
  <xsl:param name="hyphenate">false</xsl:param>

  <xsl:param name="draft.mode" select="no"/>

  <xsl:param name="variablelist.as.blocks" select="1" />
  <xsl:param name="admon.textlabel" select="0" />
  <xsl:param name="admon.graphics" select="1" />
  <xsl:param name="admon.graphics.path">images/</xsl:param>
  <xsl:param name="admon.graphics.extension">.png</xsl:param>
  <xsl:param name="section.autolabel" select="1" />
  <xsl:attribute-set name="sidebar.properties" use-attribute-sets="formal.object.properties">
    <xsl:attribute name="border-style">solid</xsl:attribute>
    <xsl:attribute name="border-width">.1mm</xsl:attribute>
    <xsl:attribute name="background-color">#EEEEEE</xsl:attribute>
  </xsl:attribute-set>

  <!-- Prevent blank pages in output -->
  <xsl:template name="book.titlepage.before.verso">
  </xsl:template>
  <xsl:template name="book.titlepage.verso">
  </xsl:template>
  <xsl:template name="book.titlepage.separator">
  </xsl:template>

  <xsl:template match="menuchoice">
    <fo:inline font-family="simsun">
      <xsl:call-template name="process.menuchoice"/>
    </fo:inline>
  </xsl:template>

  <xsl:template match="guilabel">
    <fo:inline font-family="simsun">
      <xsl:call-template name="inline.charseq"/>
    </fo:inline>
  </xsl:template>

  <xsl:template match="guibutton">
    <fo:inline font-family="simsun">
      <xsl:call-template name="inline.charseq"/>
    </fo:inline>
  </xsl:template>

  <xsl:template match="keysym">
    <fo:inline font-family="Symbol">
      <xsl:call-template name="inline.charseq"/>
    </fo:inline>
  </xsl:template>

</xsl:stylesheet>
