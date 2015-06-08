<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

<xsl:output method="text" encoding="utf-8" indent="no"/>
<xsl:include href="./common.xsl"/>

<xsl:template match="/node">

  <xsl:for-each select="interface">
    <xsl:variable name="iface">
      <xsl:apply-templates mode="iface-name" select="."/>
    </xsl:variable>

    <xsl:document href="{$iface}.doc-xml" method="xml" encoding="utf-8" indent="yes">
      <node>
        <xsl:copy-of select="preceding-sibling::comment()[1]"/>
        <xsl:copy-of select="."/>
      </node>
    </xsl:document>
  </xsl:for-each>
</xsl:template>

</xsl:stylesheet>
