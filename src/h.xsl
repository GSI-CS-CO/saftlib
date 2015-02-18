<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

<xsl:output method="text" encoding="utf-8" indent="no"/>

<xsl:template match="/node">// This is a generated file. Do not modify.
<xsl:for-each select="interface">
  <xsl:variable name="iface">
    <xsl:apply-templates mode="iface-name" select="."/>
  </xsl:variable>

  <xsl:variable name="smallcase" select="'abcdefghijklmnopqrstuvwxyz'"/>
  <xsl:variable name="uppercase" select="'ABCDEFGHIJKLMNOPQRSTUVWXYZ'"/>
  <xsl:variable name="big_iface" select="translate($iface, $smallcase, $uppercase)"/>
#ifndef <xsl:value-of select="$big_iface"/>_H
#define <xsl:value-of select="$big_iface"/>_H

namespace saftlib {

class <xsl:value-of select="$iface"/>_Service : public Glib::Object {
  public:
  guint register_object(const Glib::RefPtr&lt;Gio::DBus::Connection&gt;&amp; connection, const Glib::ustring&amp; object_path);
  virtual ~<xsl:value-of select="$iface"/>_Service();<xsl:for-each select="method">
  virtual void <xsl:value-of select="@name"/>(<xsl:for-each select="arg">
    <xsl:if test="position()>1">, </xsl:if>
    <xsl:text>
    </xsl:text>
    <xsl:if test="@direction='in'">const </xsl:if>Glib::ustring&amp; <xsl:value-of select="@name"/>
  </xsl:for-each>);</xsl:for-each>
};

}

#endif

</xsl:for-each>

</xsl:template>

<xsl:template match="*" mode="iface-name">
  <xsl:call-template name="iface-name">
    <xsl:with-param name="input" select="@name"/>
  </xsl:call-template>
</xsl:template>

<xsl:template name="iface-name">
  <xsl:param name="input"/>
  <xsl:choose>
    <xsl:when test="contains($input,'.')">
      <xsl:call-template name="iface-name">
        <xsl:with-param name="input" select="substring-after($input,'.')" />
      </xsl:call-template>
    </xsl:when>
    <xsl:otherwise>
      <xsl:value-of select="$input" />
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

</xsl:stylesheet>
