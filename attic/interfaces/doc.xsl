<!-- Copyright (C) 2011-2016 GSI Helmholtz Centre for Heavy Ion Research GmbH 
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
