<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0"> 

<xsl:import href="./db_htmlchunk.xsl"/> 
<xsl:import href="./tgit.xsl"/>
<xsl:param name="ulink.target">_blank</xsl:param>
<xsl:param name="keep.relative.image.uris" select="0"/>
<xsl:param name="chunker.output.encoding" select="'UTF-8'"/>
<xsl:param name="callout.graphics.extension">.svg</xsl:param>

<xsl:param name="generate.help.mapping" select="1"/>
<xsl:template match="/">
  <xsl:apply-templates/>
  <xsl:choose>
    <xsl:when test="$generate.help.mapping != '0'">
      <xsl:call-template name="hh-alias"/>
    </xsl:when>
  </xsl:choose>
</xsl:template>
<xsl:template name="hh-alias">
  <xsl:call-template name="write.text.chunk">
    <xsl:with-param name="filename">
      <xsl:if test="$manifest.in.base.dir != 0">
        <xsl:value-of select="$chunk.base.dir"/>
      </xsl:if>
      <xsl:value-of select="$htmlhelp.alias.file"/>
    </xsl:with-param>
    <xsl:with-param name="method" select="'text'"/>
    <xsl:with-param name="content">
     <xsl:choose>
       <xsl:when test="$rootid != ''">
         <xsl:apply-templates select="key('id',$rootid)" mode="hh-alias"/>
       </xsl:when>
       <xsl:otherwise>
         <xsl:apply-templates select="/" mode="hh-alias"/>
       </xsl:otherwise>
     </xsl:choose>
    </xsl:with-param>
    <xsl:with-param name="encoding" select="$htmlhelp.encoding"/>
    <xsl:with-param name="quiet" select="$chunk.quietly"/>
  </xsl:call-template>
</xsl:template>
<xsl:template match="processing-instruction('dbhh')" mode="hh-alias">
  <xsl:variable name="topicname">
    <xsl:call-template name="pi-attribute">
      <xsl:with-param name="pis"
                      select="."/>
      <xsl:with-param name="attribute" select="'topicname'"/>
    </xsl:call-template>
  </xsl:variable>
  <xsl:variable name="href">
    <xsl:call-template name="href.target.with.base.dir">
      <xsl:with-param name="object" select=".."/>
    </xsl:call-template>
  </xsl:variable>
  <xsl:value-of select="$topicname"/>
  <xsl:text>=</xsl:text>
  <!-- Some versions of HH doesn't like fragment identifires, but some does. -->
  <!-- <xsl:value-of select="substring-before(concat($href, '#'), '#')"/> -->
  <xsl:value-of select="$href"/>
  <xsl:text>&#xA;</xsl:text>
</xsl:template>
<xsl:template match="text()" mode="hh-alias"/>
</xsl:stylesheet> 
