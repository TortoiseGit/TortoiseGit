<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:exsl="http://exslt.org/common" extension-element-prefixes="exsl" version="1.0">

  <xsl:param name="gitdoc.external" select="0"/>

  <xsl:template match="gitdoclink" name="gitdoclink">
    <xsl:variable name="section">
      <xsl:choose>
        <xsl:when test="@section">(<xsl:value-of select="@section"/>)</xsl:when>
      </xsl:choose>
    </xsl:variable>
    <xsl:choose>
      <xsl:when test="$gitdoc.external != '1'">
        <xsl:variable name="cmd">
          <xsl:choose>
            <xsl:when test="@cmd ='user-manual' and @anchor">Git User Manual</xsl:when>
            <xsl:otherwise><xsl:value-of select="@cmd"/></xsl:otherwise>
          </xsl:choose>
        </xsl:variable>
        <xsl:variable name="anchor">
          <xsl:choose>
            <xsl:when test="@anchor">_<xsl:value-of select="@anchor"/></xsl:when>
          </xsl:choose>
        </xsl:variable>
        <xsl:variable name="temp">
          <xref>
            <xsl:attribute name="linkend"><xsl:value-of select="$cmd"/><xsl:value-of select="$section"/><xsl:value-of select="$anchor"/></xsl:attribute>
          </xref>
          <xsl:copy-of select="/book"/>
        </xsl:variable>
        <xsl:apply-templates select="exsl:node-set($temp)/xref[1]"/>
      </xsl:when>
      <xsl:otherwise>
         <xsl:variable name="tempA">
           <ulink>
             <xsl:attribute name="url">https://git-scm.com/docs/<xsl:value-of select="@cmd"/><xsl:if test="@anchor">#<xsl:value-of select="@anchor"/></xsl:if></xsl:attribute>
             <xsl:value-of select="@cmd"/><xsl:value-of select="$section"/>
             <xsl:text> man-page</xsl:text>
           </ulink>
        </xsl:variable>
        <xsl:apply-templates select="exsl:node-set($tempA)/ulink[1]"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>
</xsl:stylesheet>
