<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

  <xsl:param name="draft.mode" select="no"/>

  <xsl:param name="fop1.extensions" select="1" />
  <xsl:param name="section.autolabel" select="1" />
  <xsl:param name="section.label.includes.component.label" select="1" />
  <xsl:param name="generate.index" select="1" />
  <xsl:param name="glossary.sort" select="1" />

  <xsl:param name="body.start.indent">0pt</xsl:param>
  <xsl:param name="title.margin.left">-0.5in</xsl:param>
  
<xsl:attribute-set name="table.cell.padding">
  <xsl:attribute name="padding-left">4pt</xsl:attribute>
  <xsl:attribute name="padding-right">4pt</xsl:attribute>
  <xsl:attribute name="padding-top">2pt</xsl:attribute>
  <xsl:attribute name="padding-bottom">2pt</xsl:attribute>
</xsl:attribute-set>

<xsl:attribute-set name="section.title.level1.properties">
  <xsl:attribute name="font-size">
    <xsl:value-of select="$body.font.master * 1.44"/>
    <xsl:text>pt</xsl:text>
  </xsl:attribute>
</xsl:attribute-set>
<xsl:attribute-set name="section.title.level2.properties">
  <xsl:attribute name="font-size">
    <xsl:value-of select="$body.font.master * 1.2"/>
    <xsl:text>pt</xsl:text>
  </xsl:attribute>
</xsl:attribute-set>
<xsl:attribute-set name="section.title.level3.properties">
  <xsl:attribute name="font-size">
    <xsl:value-of select="$body.font.master * 1.1"/>
    <xsl:text>pt</xsl:text>
  </xsl:attribute>
</xsl:attribute-set>
<xsl:attribute-set name="section.title.level4.properties">
  <xsl:attribute name="font-size">
    <xsl:value-of select="$body.font.master * 1.05"/>
    <xsl:text>pt</xsl:text>
  </xsl:attribute>
</xsl:attribute-set>


  <xsl:param name="admon.graphics" select="1"></xsl:param>

  <xsl:attribute-set name="admonition.title.properties">
    <xsl:attribute name="font-size">14pt</xsl:attribute>
    <xsl:attribute name="font-weight">bold</xsl:attribute>
    <xsl:attribute name="hyphenate">false</xsl:attribute>
    <xsl:attribute name="keep-with-next.within-column">always</xsl:attribute>

    <xsl:attribute name="padding">.33em 0 0 5px</xsl:attribute>

    <xsl:attribute name="border-top">3px solid</xsl:attribute>
    <xsl:attribute name="border-left">1px solid</xsl:attribute>

    <xsl:attribute name="border-color">
      <xsl:param name="node" select="."/>
      <xsl:choose>
        <xsl:when test="name($node)='note'">#069</xsl:when>
        <xsl:when test="name($node)='warning'">#900</xsl:when>
        <xsl:when test="name($node)='caution'">#c60</xsl:when>
        <xsl:when test="name($node)='tip'">#090</xsl:when>
        <xsl:when test="name($node)='important'">#069</xsl:when>
        <xsl:otherwise>#069</xsl:otherwise>
      </xsl:choose>
    </xsl:attribute>

  </xsl:attribute-set>

  <xsl:attribute-set name="admonition.properties">
    <xsl:attribute name="padding">.33em 0 0 5px</xsl:attribute>

    <xsl:attribute name="border-left">1px solid</xsl:attribute>
    <xsl:attribute name="border-color">
      <xsl:param name="node" select="."/>
      <xsl:choose>
        <xsl:when test="name($node)='note'">#069</xsl:when>
        <xsl:when test="name($node)='warning'">#900</xsl:when>
        <xsl:when test="name($node)='caution'">#c60</xsl:when>
        <xsl:when test="name($node)='tip'">#090</xsl:when>
        <xsl:when test="name($node)='important'">#069</xsl:when>
        <xsl:otherwise>#069</xsl:otherwise>
      </xsl:choose>
    </xsl:attribute>

  </xsl:attribute-set>

</xsl:stylesheet>
