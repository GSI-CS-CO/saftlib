<!-- Copyright (C) 2011-2012 GSI Helmholtz Centre for Heavy Ion Research GmbH 
  **
  ** @author Wesley W. Terpstra <w.terpstra@gsi.de>
  **
  ********************************************************************************
  **  This library is free software; you can redistribute it and/or
  **  modify it under the terms of the GNU Lesser General Public
  **  License as published by the Free Software Foundation; either
  **  version 3 of the License, or (at your option) any later version.
  **
  **  This library is distributed in the hope that it will be useful,
  **  but WITHOUT ANY WARRANTY; without even the implied warranty of
  **  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  **  Lesser General Public License for more details.
  **  
  **  You should have received a copy of the GNU Lesser General Public
  **  License along with this library. If not, see <http://www.gnu.org/licenses/>.
  ********************************************************************************
  -->
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

<xsl:output 
  doctype-public="-//OASIS//DTD DocBook XML V4.1.2//EN"
  doctype-system="http://www.oasis-open.org/docbook/xml/4.1.2/docbookx.dtd"/>

<!-- Copy everything not otherwise patched below -->
<xsl:template match="@*|node()">
  <xsl:copy>
    <xsl:apply-templates select="@*|node()"/>
  </xsl:copy>
</xsl:template>

<!-- Buggy gdbus-codegen can nest para inside para -->
<xsl:template match="para[para]">
  <xsl:for-each select="para">
    <xsl:apply-templates select="."/>
  </xsl:for-each>
</xsl:template>

<!-- Buggy gdbus-codegen can write empty variablelists -->
<xsl:template match="variablelist[not(varlistentry)]"/>

</xsl:stylesheet>
