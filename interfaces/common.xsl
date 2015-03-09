<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

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

<xsl:template name="iface-type">
  <xsl:param name="input"/>
  <xsl:choose>
    <xsl:when test="$input='y'">unsigned char</xsl:when>
    <xsl:when test="$input='b'">bool</xsl:when>
    <xsl:when test="$input='n'">gint16</xsl:when>
    <xsl:when test="$input='q'">guint16</xsl:when>
    <xsl:when test="$input='i'">gint32</xsl:when>
    <xsl:when test="$input='u'">guint32</xsl:when>
    <xsl:when test="$input='x'">gint64</xsl:when>
    <xsl:when test="$input='t'">guint64</xsl:when>
    <xsl:when test="$input='d'">double</xsl:when>
    <xsl:when test="$input='h'">int</xsl:when>
    <xsl:when test="$input='s'">Glib::ustring</xsl:when>
    <xsl:when test="$input='v'">Glib::VariantBase</xsl:when>
    <xsl:when test="substring($input,1,1)='(' and substring($input,string-length($input),1)=')'">
      <xsl:message terminate="yes">
        Error: Anonymous tuples currently unsupported.
      </xsl:message>
    </xsl:when>
    <!-- a{Bt} . B always basic type -->
    <xsl:when test="substring($input,1,2)='a{' and substring($input,string-length($input),1)='}' and string-length($input) >= 5">
      <xsl:text>std::map&lt; </xsl:text>
      <xsl:call-template name="iface-type">
        <xsl:with-param name="input" select="substring($input,3,1)"/>
      </xsl:call-template>
      <xsl:text>, </xsl:text>
      <xsl:call-template name="iface-type">
        <xsl:with-param name="input" select="substring($input,4,string-length($input)-4)"/>
      </xsl:call-template>
      <xsl:text> &gt;</xsl:text>
    </xsl:when>
    <xsl:when test="substring($input,1,1)='a'">
      <xsl:text>std::vector&lt; </xsl:text>
      <xsl:call-template name="iface-type">
        <xsl:with-param name="input" select="substring($input,2)"/>
      </xsl:call-template>
      <xsl:text> &gt;</xsl:text>
    </xsl:when>
    <xsl:otherwise>
      <xsl:message terminate="yes">
        Error: Unknown type string '<xsl:value-of select="$input"/>'
      </xsl:message>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template name="simple-type">
  <xsl:variable name="input" select="@type"/>
  <xsl:choose>
    <xsl:when test="$input='y'">true</xsl:when>
    <xsl:when test="$input='b'">true</xsl:when>
    <xsl:when test="$input='n'">true</xsl:when>
    <xsl:when test="$input='q'">true</xsl:when>
    <xsl:when test="$input='i'">true</xsl:when>
    <xsl:when test="$input='u'">true</xsl:when>
    <xsl:when test="$input='x'">true</xsl:when>
    <xsl:when test="$input='t'">true</xsl:when>
    <xsl:when test="$input='d'">true</xsl:when>
    <xsl:when test="$input='h'">true</xsl:when>
    <xsl:when test="$input='s'">false</xsl:when>
    <xsl:when test="$input='v'">false</xsl:when>
    <xsl:when test="substring($input,1,1)='('">false</xsl:when>
    <xsl:when test="substring($input,1,1)='a'">false</xsl:when>
    <xsl:otherwise>
      <xsl:message terminate="yes">
        Error: Unknown type string '<xsl:value-of select="$input"/>'
      </xsl:message>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template name="raw-type">
  <xsl:call-template name="iface-type">
    <xsl:with-param name="input" select="@type"/>
  </xsl:call-template>
</xsl:template>

<xsl:template name="variant-type">
  <xsl:text>Glib::Variant&lt; </xsl:text>
  <xsl:call-template name="raw-type"/>
  <xsl:text> &gt;</xsl:text>
</xsl:template>

<xsl:template name="input-type">
  <xsl:variable name="simple">
   <xsl:call-template name="simple-type"/>
  </xsl:variable>
  <xsl:variable name="use_ref" select="$simple!='true'"/>
  <xsl:if test="$use_ref">const </xsl:if>
  <xsl:call-template name="raw-type"/>
  <xsl:if test="$use_ref">&amp;</xsl:if>
</xsl:template>

<xsl:template name="output-type">
  <xsl:call-template name="raw-type"/>
  <xsl:text>&amp;</xsl:text>
</xsl:template>

<xsl:template name="inout-type">
  <xsl:choose>
    <xsl:when test="@direction='in'"><xsl:call-template name="input-type"/></xsl:when>
    <xsl:when test="@direction='out'"><xsl:call-template name="output-type"/></xsl:when>
    <xsl:otherwise><xsl:message terminate="yes">Error: no direction</xsl:message></xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template name="method-service-type">
  <xsl:param name="namespace"/>
  <xsl:text>void</xsl:text>
  <xsl:text> </xsl:text>
  <xsl:value-of select="$namespace"/>
  <xsl:value-of select="@name"/>
  <xsl:text>(</xsl:text>
  <xsl:for-each select="arg">
    <xsl:if test="position()>1">, </xsl:if>
    <xsl:call-template name="inout-type"/>
    <xsl:text> </xsl:text>
    <xsl:value-of select="@name"/>
  </xsl:for-each>
  <xsl:text>)</xsl:text>
</xsl:template>

<xsl:template name="method-proxy-type">
  <xsl:param name="namespace"/>
  <xsl:text>void</xsl:text>
  <xsl:text> </xsl:text>
  <xsl:value-of select="$namespace"/>
  <xsl:value-of select="@name"/>
  <xsl:text>(</xsl:text>
  <xsl:for-each select="arg">
    <xsl:if test="position()>1">, </xsl:if>
    <xsl:call-template name="inout-type"/>
    <xsl:text> </xsl:text>
    <xsl:value-of select="@name"/>
  </xsl:for-each>
  <xsl:text>)</xsl:text>
</xsl:template>

<xsl:template name="signal-service-type">
  <xsl:param name="namespace"/>
  <xsl:text>void </xsl:text>
  <xsl:value-of select="$namespace"/>
  <xsl:value-of select="@name"/>
  <xsl:text>(</xsl:text>
  <xsl:for-each select="arg">
    <xsl:if test="position()>1">, </xsl:if>
    <xsl:call-template name="input-type"/>
    <xsl:text> </xsl:text>
    <xsl:value-of select="@name"/>
  </xsl:for-each>
  <xsl:text>)</xsl:text>
</xsl:template>

<xsl:template name="signal-proxy-type">
  <xsl:param name="namespace"/>
  <xsl:text>sigc::signal&lt; void </xsl:text>
  <xsl:for-each select="arg">
    <xsl:text>, </xsl:text>
    <xsl:call-template name="input-type"/>
    <xsl:text> </xsl:text>
  </xsl:for-each>
  <xsl:text>&gt; </xsl:text>
  <xsl:value-of select="$namespace"/>
  <xsl:value-of select="@name"/>
</xsl:template>

<xsl:template name="prop-service-gettype">
  <xsl:param name="namespace"/>
  <xsl:call-template name="input-type"/>
  <xsl:text> </xsl:text>
  <xsl:value-of select="$namespace"/>
  <xsl:text>get</xsl:text>
  <xsl:value-of select="@name"/>
  <xsl:text>() const</xsl:text>
</xsl:template>

<xsl:template name="prop-proxy-gettype">
  <xsl:param name="namespace"/>
  <xsl:call-template name="raw-type"/>
  <xsl:text> </xsl:text>
  <xsl:value-of select="$namespace"/>
  <xsl:text>get</xsl:text>
  <xsl:value-of select="@name"/>
  <xsl:text>() const</xsl:text>
</xsl:template>

<xsl:template name="prop-service-settype">
  <xsl:param name="namespace"/>
  <xsl:text>void </xsl:text>
  <xsl:value-of select="$namespace"/>
  <xsl:text>set</xsl:text>
  <xsl:value-of select="@name"/>
  <xsl:text>(</xsl:text>
  <xsl:call-template name="input-type"/>
  <xsl:text> val)</xsl:text>
</xsl:template>

<xsl:template name="prop-proxy-settype">
  <xsl:param name="namespace"/>
  <xsl:text>void </xsl:text>
  <xsl:value-of select="$namespace"/>
  <xsl:text>set</xsl:text>
  <xsl:value-of select="@name"/>
  <xsl:text>(</xsl:text>
  <xsl:call-template name="input-type"/>
  <xsl:text> val)</xsl:text>
</xsl:template>

<xsl:template name="prop-proxy-sigtype">
  <xsl:param name="namespace"/>
  <xsl:text>sigc::signal&lt; void, </xsl:text>
  <xsl:call-template name="input-type"/>
  <xsl:text> &gt; </xsl:text>
  <xsl:value-of select="$namespace"/>
  <xsl:value-of select="@name"/>
</xsl:template>

<xsl:template match="*" mode="escape">
    <!-- Begin opening tag -->
    <xsl:text>&lt;</xsl:text>
    <xsl:value-of select="name()"/>

    <!-- Attributes -->
    <xsl:for-each select="@*">
        <xsl:text> </xsl:text>
        <xsl:value-of select="name()"/>
        <xsl:text>='</xsl:text>
        <xsl:call-template name="escape-xml">
            <xsl:with-param name="text" select="."/>
        </xsl:call-template>
        <xsl:text>'</xsl:text>
    </xsl:for-each>

    <!-- End opening tag -->
    <xsl:if test="not(node())">/</xsl:if>
    <xsl:text>&gt;</xsl:text>

    <!-- Content (child elements, text nodes, and PIs) -->
    <xsl:apply-templates select="node()" mode="escape" />

    <!-- Closing tag -->
    <xsl:if test="node()">
      <xsl:text>&lt;/</xsl:text>
      <xsl:value-of select="name()"/>
    <xsl:text>&gt;</xsl:text>
</xsl:if>
</xsl:template>

<xsl:template match="text()" mode="escape">
<!--
    <xsl:call-template name="escape-xml">
        <xsl:with-param name="text" select="."/>
    </xsl:call-template>
-->
</xsl:template>

<xsl:template name="escape-xml">
    <xsl:param name="text"/>
    <xsl:if test="$text != ''">
        <xsl:variable name="head" select="substring($text, 1, 1)"/>
        <xsl:variable name="tail" select="substring($text, 2)"/>
        <xsl:choose>
            <xsl:when test="$head = '&amp;'">&amp;amp;</xsl:when>
            <xsl:when test="$head = '&lt;'">&amp;lt;</xsl:when>
            <xsl:when test="$head = '&gt;'">&amp;gt;</xsl:when>
            <xsl:when test="$head = '&quot;'">&amp;quot;</xsl:when>
            <xsl:when test="$head = &quot;&apos;&quot;">&amp;apos;</xsl:when>
            <xsl:otherwise><xsl:value-of select="$head"/></xsl:otherwise>
        </xsl:choose>
        <xsl:call-template name="escape-xml">
            <xsl:with-param name="text" select="$tail"/>
        </xsl:call-template>
    </xsl:if>
</xsl:template>

</xsl:stylesheet>
