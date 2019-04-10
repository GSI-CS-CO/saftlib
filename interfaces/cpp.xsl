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
  <xsl:variable name="name" select="@name"/>

  <xsl:document href="{$name}.cpp" method="text" encoding="utf-8" indent="no">
    <xsl:text>// This is a generated file. Do not modify.&#10;&#10;</xsl:text>
    <xsl:text>#include &lt;sigc++/sigc++.h&gt;&#10;</xsl:text>
    <xsl:text>#include "</xsl:text>
    <xsl:value-of select="$name"/>
    <xsl:text>.h"&#10;</xsl:text>
    <xsl:text>&#10;</xsl:text>
    <xsl:text>namespace saftlib {&#10;&#10;</xsl:text>

    <xsl:text>std::shared_ptr&lt;</xsl:text>
    <xsl:value-of select="$name"/>
    <xsl:text>_Proxy&gt; </xsl:text>
    <xsl:value-of select="$name"/>
    <xsl:text>_Proxy::create(&#10;</xsl:text>
    <xsl:text>  const std::string&amp; object_path,&#10;</xsl:text>
    <xsl:text>   saftlib::SignalGroup &amp;signalGroup)&#10;{&#10;</xsl:text>
    <xsl:text>  return std::shared_ptr&lt;</xsl:text>
    <xsl:value-of select="$name"/>
    <xsl:text>_Proxy&gt;(new </xsl:text>
    <xsl:value-of select="$name"/>
    <xsl:text>_Proxy(object_path, "</xsl:text>
    <xsl:value-of select="annotation[@name='de.gsi.saftlib.name']/@value"/>
    <xsl:text>", </xsl:text>
    <xsl:text> saftbus::BUS_TYPE_SYSTEM, </xsl:text>
    <xsl:text> signalGroup));&#10;}&#10;&#10;</xsl:text>

    <!-- Proxy Constructor -->
    <xsl:value-of select="$name"/>
    <xsl:text>_Proxy::</xsl:text>
    <xsl:value-of select="$name"/>
    <xsl:text>_Proxy(&#10;</xsl:text>
    <xsl:text>  const std::string&amp; object_path,&#10;</xsl:text>
    <xsl:text>  const std::string&amp; name,&#10;</xsl:text>
    <xsl:text>  saftbus::BusType bus_type,&#10;</xsl:text>
    <xsl:text>  saftlib::SignalGroup &amp;signalGroup)&#10;</xsl:text>
    <xsl:text>: </xsl:text>
    <xsl:for-each select="interface">
      <xsl:if test="position()>1">,&#10;  </xsl:if>
      <xsl:apply-templates mode="iface-name" select="."/>
      <xsl:text>(i</xsl:text>
      <xsl:apply-templates mode="iface-name" select="."/>
      <xsl:text>_Proxy::create(object_path, name, bus_type, signalGroup))</xsl:text>
    </xsl:for-each>
    <xsl:for-each select="interface">
      <xsl:variable name="iface"><xsl:apply-templates mode="iface-name" select="."/></xsl:variable>
    </xsl:for-each>
    <xsl:for-each select="interface">
      <xsl:variable name="iface"><xsl:apply-templates mode="iface-name" select="."/></xsl:variable>
      <xsl:for-each select="signal">
        <xsl:text>,&#10;  con_sig</xsl:text>
        <xsl:value-of select="@name"/>
        <xsl:text>(</xsl:text>
        <xsl:value-of select="$iface"/>
        <xsl:text>-&gt;</xsl:text>
        <xsl:value-of select="@name"/>
        <xsl:text>.connect(</xsl:text>
        <xsl:value-of select="@name"/>
        <xsl:text>.make_slot()))</xsl:text>
      </xsl:for-each>
    </xsl:for-each>
    <xsl:text>&#10;{&#10;}&#10;&#10;</xsl:text>

    <!-- Proxy Destructor -->
    <xsl:value-of select="$name"/>
    <xsl:text>_Proxy::~</xsl:text>
    <xsl:value-of select="$name"/>
    <xsl:text>_Proxy()&#10;{&#10;</xsl:text>
    <xsl:for-each select="interface">
      <xsl:variable name="iface"><xsl:apply-templates mode="iface-name" select="."/></xsl:variable>
    </xsl:for-each>
    <xsl:for-each select="interface">
      <xsl:variable name="iface"><xsl:apply-templates mode="iface-name" select="."/></xsl:variable>
      <xsl:for-each select="signal">
        <xsl:text>  con_sig</xsl:text>
        <xsl:value-of select="@name"/>
        <xsl:text>.disconnect();&#10;</xsl:text>
      </xsl:for-each>
    </xsl:for-each>
    <xsl:text>}&#10;&#10;</xsl:text>

    <!-- Register all interfaces -->
    <xsl:text>void </xsl:text>
    <xsl:value-of select="$name"/>
    <xsl:text>_Service::register_self(const std::shared_ptr&lt;saftbus::Connection&gt;&amp; con, const std::string&amp; path)&#10;{&#10;</xsl:text>
    <xsl:for-each select="interface">
      <xsl:text>  </xsl:text>
      <xsl:apply-templates mode="iface-name" select="."/>
      <xsl:text>.register_self(con, path);&#10;</xsl:text>
    </xsl:for-each>
    <xsl:text>}&#10;&#10;</xsl:text>

    <!-- Unegister all interfaces -->
    <xsl:text>void </xsl:text>
    <xsl:value-of select="$name"/>
    <xsl:text>_Service::unregister_self()&#10;{&#10;</xsl:text>
    <xsl:for-each select="interface">
      <xsl:text>  </xsl:text>
      <xsl:apply-templates mode="iface-name" select="."/>
      <xsl:text>.unregister_self();&#10;</xsl:text>
    </xsl:for-each>
    <xsl:text>}&#10;&#10;</xsl:text>

    <!-- isActive method -->
    <xsl:text>bool </xsl:text>
    <xsl:value-of select="$name"/>
    <xsl:text>_Service::isActive() const&#10;{&#10;</xsl:text>
    <xsl:for-each select="interface">
      <xsl:text>  if (</xsl:text>
      <xsl:apply-templates mode="iface-name" select="."/>
      <xsl:text>.isActive()) return true;&#10;</xsl:text>
    </xsl:for-each>
    <xsl:text>  return false;&#10;</xsl:text>
    <xsl:text>}&#10;&#10;</xsl:text>

    <!-- getSender method -->
    <xsl:text>const std::string&amp; </xsl:text>
    <xsl:value-of select="$name"/>
    <xsl:text>_Service::getSender() const&#10;{&#10;</xsl:text>
    <xsl:for-each select="interface">
      <xsl:text>  if (</xsl:text>
      <xsl:apply-templates mode="iface-name" select="."/>
      <xsl:text>.isActive()) return </xsl:text>
      <xsl:apply-templates mode="iface-name" select="."/>
      <xsl:text>.getSender();&#10;</xsl:text>
    </xsl:for-each>
    <xsl:text>  throw saftbus::Error(saftbus::Error::INVALID_ARGS, "Not inside DBus callback on this object");&#10;</xsl:text>
    <xsl:text>}&#10;&#10;</xsl:text>

    <!-- getObjectPath method -->
    <xsl:text>const std::string&amp; </xsl:text>
    <xsl:value-of select="$name"/>
    <xsl:text>_Service::getObjectPath() const&#10;{&#10;</xsl:text>
    <xsl:for-each select="interface">
      <xsl:text>  if (</xsl:text>
      <xsl:apply-templates mode="iface-name" select="."/>
      <xsl:text>.isActive()) return </xsl:text>
      <xsl:apply-templates mode="iface-name" select="."/>
      <xsl:text>.getObjectPath();&#10;</xsl:text>
    </xsl:for-each>
    <xsl:text>  throw saftbus::Error(saftbus::Error::INVALID_ARGS, "Not inside DBus callback on this object");&#10;</xsl:text>
    <xsl:text>}&#10;&#10;</xsl:text>

    <!-- getConnection method -->
    <xsl:text>const std::shared_ptr&lt;saftbus::Connection&gt;&amp; </xsl:text>
    <xsl:value-of select="$name"/>
    <xsl:text>_Service::getConnection() const&#10;{&#10;</xsl:text>
    <xsl:for-each select="interface">
      <xsl:text>  if (</xsl:text>
      <xsl:apply-templates mode="iface-name" select="."/>
      <xsl:text>.isActive()) return </xsl:text>
      <xsl:apply-templates mode="iface-name" select="."/>
      <xsl:text>.getConnection();&#10;</xsl:text>
    </xsl:for-each>
    <xsl:text>  throw saftbus::Error(saftbus::Error::INVALID_ARGS, "Not inside DBus callback on this object");&#10;</xsl:text>
    <xsl:text>}&#10;&#10;</xsl:text>

    <xsl:text>}&#10;</xsl:text>
  </xsl:document>

  <xsl:for-each select="interface">
    <xsl:variable name="iface_full" select="@name"/>
    <xsl:variable name="iface">
      <xsl:apply-templates mode="iface-name" select="."/>
    </xsl:variable>
    <xsl:document href="i{$iface}.cpp" method="text" encoding="utf-8" indent="no">

      <!-- C++ boilerplate -->
      <xsl:text>// This is a generated file. Do not modify.&#10;&#10;</xsl:text>
      <xsl:text>#include &lt;sigc++/sigc++.h&gt;&#10;</xsl:text>
      <xsl:text>#include &lt;cstdint&gt;&#10;</xsl:text>
      <xsl:text>#include "i</xsl:text>
      <xsl:value-of select="$iface"/>
      <xsl:text>.h"&#10;&#10;</xsl:text>
      <xsl:text>namespace saftlib {&#10;&#10;</xsl:text>

      <!-- XML interface file -->
      <xsl:text>const std::string i</xsl:text>
      <xsl:value-of select="$iface"/>
      <xsl:text>_Service::xml =&#10;"&lt;node&gt;</xsl:text>
      <!-- unfortunately, the following line needs to be replaced by a bunch of code to copy all but the 'Ax' args; is there a simpler way of achieving the same ? -->
      <xsl:apply-templates select="." mode="escape"/>
      <xsl:text>"&#10;</xsl:text>
      <xsl:text>"&lt;/node&gt;";&#10;&#10;</xsl:text>

      <!-- Interface vtable -->
      <xsl:text>i</xsl:text>
      <xsl:value-of select="$iface"/>
      <xsl:text>::~i</xsl:text>
      <xsl:value-of select="$iface"/>
      <xsl:text>() { }&#10;&#10;</xsl:text>

      <!-- Proxy methods -->
      <xsl:for-each select="method">
        <xsl:call-template name="method-type">
          <xsl:with-param name="namespace">i<xsl:value-of select="$iface"/>_Proxy::</xsl:with-param>
        </xsl:call-template>
        <xsl:text>&#10;{&#10;</xsl:text>
        <xsl:text>  saftbus::Serial query;&#10;</xsl:text>
        <xsl:for-each select="arg[@direction='in']">
          <xsl:text>  query.put(</xsl:text>
          <xsl:value-of select="@name"/>
          <xsl:text>);&#10;</xsl:text>
        </xsl:for-each>
        <xsl:text>  </xsl:text>
        <xsl:if test="arg[@direction='out']">saftbus::Serial response = </xsl:if>
        <xsl:text>call_sync("</xsl:text>
        <xsl:value-of select="@name"/>
        <xsl:text>", query);&#10;</xsl:text>
        <xsl:if test="arg[@direction='out']">  response.get_init();&#10;</xsl:if>
        <xsl:for-each select="arg[@direction='out']">
          <xsl:text>  </xsl:text>
          <xsl:call-template name="raw-type"/> ov_<xsl:value-of select="@name"/>
          <xsl:text>;&#10;</xsl:text>
        </xsl:for-each>
        <xsl:for-each select="arg[@direction='out']">
          <xsl:text>  response.get(ov_</xsl:text>
          <xsl:value-of select="@name"/>
          <xsl:text>);&#10;</xsl:text>
        </xsl:for-each>
        <xsl:choose>
          <xsl:when test="count(arg[@direction='out']) = 1">
            <xsl:text>  return ov_</xsl:text>
            <xsl:value-of select="arg[@direction='out']/@name"/>
            <xsl:text>;&#10;</xsl:text>
          </xsl:when>
          <xsl:otherwise>
            <xsl:for-each select="arg[@direction='out']">
              <xsl:text>  </xsl:text>
              <xsl:value-of select="@name"/>
              <xsl:text> = ov_</xsl:text>
              <xsl:value-of select="@name"/>
              <xsl:text>.get();&#10;</xsl:text>
            </xsl:for-each>
          </xsl:otherwise>
        </xsl:choose>
        <xsl:text>}&#10;&#10;</xsl:text>
      </xsl:for-each>


      <!-- Boiler-plate to retrieve a property -->
      <xsl:text>void i</xsl:text>
      <xsl:value-of select="$iface"/>
      <xsl:text>_Proxy::fetch_property(const char* name, saftbus::Serial&amp; val) const&#10;</xsl:text>
      <xsl:text>{&#10;</xsl:text>
      <xsl:text>  saftbus::Serial params;&#10;</xsl:text>
      <xsl:text>  params.put(std::string("</xsl:text>
      <xsl:value-of select="$iface_full"/>
      <xsl:text>"));&#10;</xsl:text>
      <xsl:text>  params.put(std::string(name));&#10;</xsl:text>
      <xsl:text>  std::shared_ptr&lt;saftbus::ProxyConnection&gt; connection =&#10;</xsl:text>
      <xsl:text>    std::const_pointer_cast&lt;saftbus::ProxyConnection&gt;(get_connection());&#10;</xsl:text>
      <xsl:text>  val =&#10;</xsl:text>
      <xsl:text>    connection->call_sync(get_object_path(), "org.freedesktop.DBus.Properties", "Get", &#10;</xsl:text>
      <xsl:text>      params, get_name());&#10;</xsl:text>
      <xsl:text>}&#10;&#10;</xsl:text>

      <!-- Property getters -->
      <xsl:for-each select="property[@access='read' or @access='readwrite']">
        <xsl:call-template name="prop-gettype">
          <xsl:with-param name="namespace">i<xsl:value-of select="$iface"/>_Proxy::</xsl:with-param>
        </xsl:call-template>
        <xsl:text>&#10;{&#10;  </xsl:text>
        <xsl:call-template name="raw-type"/>
        <xsl:text> value;&#10;</xsl:text>
        <xsl:text>  saftbus::Serial response;&#10;</xsl:text>
        <xsl:text>  fetch_property("</xsl:text>
        <xsl:value-of select="@name"/>
        <xsl:text>", response);&#10;</xsl:text>
        <xsl:text>  response.get_init();&#10;</xsl:text>
        <xsl:text>  response.get(value);&#10;</xsl:text>
        <xsl:text>  return value;&#10;}&#10;&#10;</xsl:text>
      </xsl:for-each>

      <!-- Boiler-plate to set a property -->
      <xsl:text>void i</xsl:text>
      <xsl:value-of select="$iface"/>
      <xsl:text>_Proxy::update_property(const char* name, const saftbus::Serial&amp; val)&#10;{&#10;</xsl:text>
      <xsl:text>  saftbus::Serial params;&#10;</xsl:text>
      <xsl:text>  params.put(std::string("</xsl:text>
      <xsl:value-of select="$iface_full"/>
      <xsl:text>"));&#10;</xsl:text>
      <xsl:text>  params.put(std::string(name));&#10;</xsl:text>
      <xsl:text>  params.put(val);&#10;</xsl:text>
      <xsl:text>  std::shared_ptr&lt;saftbus::ProxyConnection&gt; connection = get_connection();&#10;</xsl:text>
      <xsl:text>  connection->call_sync(get_object_path(), "org.freedesktop.DBus.Properties", "Set",&#10;</xsl:text>
      <xsl:text>    params, get_name());&#10;}&#10;&#10;</xsl:text>

      <!-- Property setters -->
      <xsl:for-each select="property[@access='write' or @access='readwrite']">
        <xsl:call-template name="prop-settype">
          <xsl:with-param name="namespace">i<xsl:value-of select="$iface"/>_Proxy::</xsl:with-param>
        </xsl:call-template>
        <xsl:text>&#10;{&#10;</xsl:text>
        <xsl:text>  saftbus::Serial parameter;&#10;</xsl:text>
        <xsl:text>  parameter.put(val);&#10;</xsl:text>
        <xsl:text>  update_property("</xsl:text>
        <xsl:value-of select="@name"/>
        <xsl:text>", </xsl:text>
        <xsl:text>parameter);&#10;}&#10;&#10;</xsl:text>
      </xsl:for-each>

      <!-- Receive changed properties -->
      <xsl:text>void i</xsl:text>
      <xsl:value-of select="$iface"/>
      <xsl:text>_Proxy::on_properties_changed(&#10;</xsl:text>
      <xsl:text>  const MapChangedProperties&amp; changed_properties,&#10;</xsl:text>
      <xsl:text>  const std::vector&lt; std::string &gt;&amp; invalidated_properties)&#10;</xsl:text>
      <xsl:text>{&#10;</xsl:text>
      <xsl:text>  saftbus::Proxy::on_properties_changed(changed_properties, invalidated_properties);&#10;</xsl:text>
      <xsl:text>  for (MapChangedProperties::const_iterator i = changed_properties.begin(); i != changed_properties.end(); ++i) {&#10;</xsl:text>
      <xsl:text>    </xsl:text>
      <xsl:for-each select="property[@access='read' or @access='readwrite']">
        <xsl:text>if (i->first == "</xsl:text>
        <xsl:value-of select="@name"/>
        <xsl:text>") {&#10;</xsl:text>
        <xsl:choose>
          <xsl:when test="annotation[@name = 'org.freedesktop.DBus.Property.EmitsChangedSignal' and @value != 'true']">
            <xsl:text>      // EmitsChangedSignal != true&#10;</xsl:text>
          </xsl:when>
          <xsl:otherwise>
            <xsl:text>      </xsl:text>
            <xsl:call-template name="raw-type"/>
            <xsl:text> property_value;&#10;</xsl:text>
            <xsl:text>      i->second.get_init();&#10;</xsl:text>
            <xsl:text>      i->second.get(property_value);&#10;</xsl:text>
            <xsl:text>      </xsl:text>
            <xsl:value-of select="@name"/>
            <xsl:text>(property_value);&#10;</xsl:text>
          </xsl:otherwise>
        </xsl:choose>
        <xsl:text>    } else </xsl:text>
      </xsl:for-each>
      <xsl:text> {&#10;      // noop&#10;    }&#10;  }&#10;}&#10;&#10;</xsl:text>

      <!-- Receive signals -->
      <xsl:text>void i</xsl:text>
      <xsl:value-of select="$iface"/>
      <xsl:text>_Proxy::on_signal(&#10;</xsl:text>
      <xsl:text>  const std::string&amp; sender_name,&#10;</xsl:text>
      <xsl:text>  const std::string&amp; signal_name,&#10;</xsl:text>
      <xsl:text>  const saftbus::Serial&amp; parameters)&#10;</xsl:text>
      <xsl:text>{&#10;</xsl:text>
      <xsl:text>  saftbus::Proxy::on_signal(sender_name, signal_name, parameters);&#10;</xsl:text>
      <xsl:text>  </xsl:text>
      <xsl:for-each select="signal">
        <xsl:text>if (signal_name == "</xsl:text>
        <xsl:value-of select="@name"/>
        <xsl:text>") {&#10;</xsl:text>
        <xsl:for-each select="arg">
          <xsl:text>    </xsl:text>
          <xsl:call-template name="raw-type"/>
          <xsl:text> </xsl:text>
          <xsl:value-of select="@name"/>
          <xsl:text>;&#10;</xsl:text>
        </xsl:for-each>
        <xsl:text>    parameters.get_init();&#10;</xsl:text>
        <xsl:for-each select="arg">
          <xsl:text>    parameters.get(</xsl:text>
          <xsl:value-of select="@name"/>
          <xsl:text>);&#10;</xsl:text>
        </xsl:for-each>
        <xsl:text>    </xsl:text>
        <xsl:value-of select="@name"/>
        <xsl:text>(</xsl:text>
        <xsl:for-each select="arg">
          <xsl:if test="position()>1">, </xsl:if>
          <xsl:value-of select="@name"/>
        </xsl:for-each>
        <xsl:text>);&#10;  } else </xsl:text>
      </xsl:for-each>
      <xsl:text>{&#10;    // noop&#10;  }&#10;}&#10;&#10;</xsl:text>

      <!-- Constructor -->
      <xsl:text>i</xsl:text>
      <xsl:value-of select="$iface"/>
      <xsl:text>_Proxy::i</xsl:text>
      <xsl:value-of select="$iface"/>
      <xsl:text>_Proxy(&#10;</xsl:text>
      <xsl:text>  saftbus::BusType bus_type,&#10;</xsl:text>
      <xsl:text>  const std::string&amp; name,&#10;</xsl:text>
      <xsl:text>  const std::string&amp; object_path,&#10;</xsl:text>
      <xsl:text>  const std::string&amp; interface_name,&#10;</xsl:text>
      <xsl:text>  saftlib::SignalGroup &amp;signalGroup)&#10;</xsl:text>
      <xsl:text>: Proxy(bus_type, name, object_path, interface_name, std::shared_ptr&lt;saftbus::InterfaceInfo&gt;(), signalGroup)&#10;</xsl:text>
      <xsl:text>{&#10;}&#10;&#10;</xsl:text>

      <!-- Create -->
      <xsl:text>std::shared_ptr&lt;i</xsl:text>
      <xsl:value-of select="$iface"/>
      <xsl:text>_Proxy&gt; i</xsl:text>
      <xsl:value-of select="$iface"/>
      <xsl:text>_Proxy::create(&#10;</xsl:text>
      <xsl:text>  const std::string&amp; object_path,&#10;</xsl:text>
      <xsl:text>  const std::string&amp; name,&#10;</xsl:text>
      <xsl:text>  saftbus::BusType bus_type,&#10;</xsl:text>
      <xsl:text>  saftlib::SignalGroup &amp;signalGroup)&#10;{&#10;</xsl:text>
      <xsl:text>  return std::shared_ptr&lt;i</xsl:text>
      <xsl:value-of select="$iface"/>
      <xsl:text>_Proxy&gt;(new i</xsl:text>
      <xsl:value-of select="$iface"/>
      <xsl:text>_Proxy(bus_type, name,&#10;</xsl:text>
      <xsl:text>    object_path, "</xsl:text>
      <xsl:value-of select="$iface_full"/>
      <xsl:text>", signalGroup));&#10;}&#10;&#10;</xsl:text>

      <!-- Register method -->
      <xsl:text>void i</xsl:text>
      <xsl:value-of select="$iface"/>
      <xsl:text>_Service::register_self(const std::shared_ptr&lt;saftbus::Connection&gt;&amp; connection, const std::string&amp; object_path)&#10;{&#10;</xsl:text>
      <xsl:text>  static std::shared_ptr&lt;saftbus::NodeInfo&gt; introspection;&#10;</xsl:text>
      <xsl:text>  if (!introspection)&#10;</xsl:text>
      <xsl:text>    introspection = saftbus::NodeInfo::create_for_xml(xml);&#10;</xsl:text>
      <xsl:text>  unsigned id = connection->register_object(&#10;</xsl:text>
      <xsl:text>    object_path, introspection->lookup_interface(), interface_vtable);&#10;</xsl:text>
      <xsl:text>  exports.push_back(Export(connection, object_path, id));&#10;</xsl:text>
      <xsl:text>}&#10;&#10;</xsl:text>

      <!-- Unregister method -->
      <xsl:text>void i</xsl:text>
      <xsl:value-of select="$iface"/>
      <xsl:text>_Service::unregister_self()&#10;{&#10;</xsl:text>
      <xsl:text>  for (unsigned i = 0; i &lt; exports.size(); ++i)&#10;</xsl:text>
      <xsl:text>    exports[i].connection->unregister_object(exports[i].id);&#10;</xsl:text>
      <xsl:text>  exports.clear();&#10;</xsl:text>
      <xsl:text>}&#10;&#10;</xsl:text>

      <!-- isActive method -->
      <xsl:text>bool i</xsl:text>
      <xsl:value-of select="$iface"/>
      <xsl:text>_Service::isActive() const&#10;{&#10;</xsl:text>
      <xsl:text>  // implicit conversion from std::shared_ptr to bool is not possible in later versions (&gt;=2.49.1) of Glibmm&#10;</xsl:text>
      <xsl:text>  return static_cast&lt;bool&gt;(connection);&#10;</xsl:text>
      <xsl:text>}&#10;&#10;</xsl:text>

      <!-- getSender method -->
      <xsl:text>const std::string&amp; i</xsl:text>
      <xsl:value-of select="$iface"/>
      <xsl:text>_Service::getSender() const&#10;{&#10;</xsl:text>
      <xsl:text>  if (!isActive()) throw saftbus::Error(saftbus::Error::INVALID_ARGS, "Not inside DBus callback on this object");&#10;</xsl:text>
      <xsl:text>  return *sender;&#10;</xsl:text>
      <xsl:text>}&#10;&#10;</xsl:text>

      <!-- getObjectPath method -->
      <xsl:text>const std::string&amp; i</xsl:text>
      <xsl:value-of select="$iface"/>
      <xsl:text>_Service::getObjectPath() const&#10;{&#10;</xsl:text>
      <xsl:text>  if (!isActive()) throw saftbus::Error(saftbus::Error::INVALID_ARGS, "Not inside DBus callback on this object");&#10;</xsl:text>
      <xsl:text>  return *objectPath;&#10;</xsl:text>
      <xsl:text>}&#10;&#10;</xsl:text>

      <!-- getConnection method -->
      <xsl:text>const std::shared_ptr&lt;saftbus::Connection&gt;&amp; i</xsl:text>
      <xsl:value-of select="$iface"/>
      <xsl:text>_Service::getConnection() const&#10;{&#10;</xsl:text>
      <xsl:text>  if (!isActive()) throw saftbus::Error(saftbus::Error::INVALID_ARGS, "Not inside DBus callback on this object");&#10;</xsl:text>
      <xsl:text>  return connection;&#10;</xsl:text>
      <xsl:text>}&#10;&#10;</xsl:text>

      <!-- Service methods -->
      <xsl:text>void i</xsl:text>
      <xsl:value-of select="$iface"/>
      <xsl:text>_Service::on_method_call(&#10;</xsl:text>
      <xsl:text>  const std::shared_ptr&lt;saftbus::Connection&gt;&amp; connection_,&#10;</xsl:text>
      <xsl:text>  const std::string&amp;  sender_, const std::string&amp; object_path,&#10;</xsl:text>
      <xsl:text>  const std::string&amp; /* interface_name */, const std::string&amp; method_name,&#10;</xsl:text>
      <xsl:text>  const saftbus::Serial&amp; parameters,&#10;</xsl:text>
      <xsl:text>  const std::shared_ptr&lt;saftbus::MethodInvocation&gt;&amp; invocation)&#10;{&#10;</xsl:text>
      <xsl:text>  sender = &amp;sender_;&#10;</xsl:text>
      <xsl:text>  objectPath = &amp;object_path;&#10;</xsl:text>
      <xsl:text>  connection = connection_;&#10;</xsl:text>
      <xsl:text>  </xsl:text>
      <xsl:for-each select="method">
        <xsl:text>if (method_name == "</xsl:text>
        <xsl:value-of select="@name"/>
        <xsl:text>") {&#10;</xsl:text>
        <xsl:text>    try {&#10;</xsl:text>
        <xsl:for-each select="arg[@direction='in']">
          <xsl:text>      </xsl:text>
            <xsl:call-template name="raw-type"/> 
          <xsl:text> </xsl:text>
          <xsl:value-of select="@name"/>
          <xsl:text>;&#10;</xsl:text>
        </xsl:for-each>
        <xsl:for-each select="arg[@direction='out']">
          <xsl:text>      </xsl:text>
          <xsl:call-template name="raw-type"/>
          <xsl:text> </xsl:text>
          <xsl:value-of select="@name"/>
          <xsl:text>;&#10;</xsl:text>
        </xsl:for-each>
        <!-- get parameter values -->
        <xsl:text>      parameters.get_init();&#10;</xsl:text>
        <xsl:for-each select="arg[@direction='in']">
          <xsl:text>      parameters.get(</xsl:text>
          <xsl:value-of select="@name"/>
          <xsl:text>);&#10;</xsl:text>
        </xsl:for-each>
        <xsl:text>      try {&#10;</xsl:text>
        <xsl:text>        </xsl:text>
        <xsl:variable name="void" select="count(arg[@direction='out']) != 1"/>
        <xsl:if test="not($void)">
          <xsl:value-of select="arg[@direction='out']/@name"/>
          <xsl:text> = </xsl:text>
        </xsl:if>
        <xsl:text>impl-&gt;</xsl:text>
        <xsl:value-of select="@name"/>
        <xsl:text>(</xsl:text>
        <xsl:for-each select="arg[$void or @direction='in']">
          <xsl:if test="position()>1">, </xsl:if>
          <xsl:value-of select="@name"/>
        </xsl:for-each>
        <xsl:text>);&#10;</xsl:text>
        <xsl:text>      } catch (...) {&#10;</xsl:text>
        <xsl:text>        rethrow("</xsl:text>
        <xsl:value-of select="@name"/>
        <xsl:text>");&#10;</xsl:text>
        <xsl:text>        throw;&#10;</xsl:text>
        <xsl:text>      }&#10;</xsl:text>
        <xsl:text>      saftbus::Serial &amp;response = invocation->get_return_value();&#10;</xsl:text>
        <xsl:text>      response.put_init();&#10;</xsl:text>
        <xsl:for-each select="arg[@direction='out']">
          <xsl:text>      response.put(</xsl:text>
          <xsl:value-of select="@name"/>
          <xsl:text>);&#10;</xsl:text>
        </xsl:for-each>

        <xsl:text>    } catch (const saftbus::Error&amp; error) {&#10;</xsl:text>
        <xsl:text>      invocation->return_error(error);&#10;</xsl:text>
        <xsl:text>    }&#10;</xsl:text>
        <xsl:text>  } else </xsl:text>
      </xsl:for-each>
      <xsl:text>{&#10;</xsl:text>
      <!-- <xsl:text>    connection.reset();&#10;</xsl:text> -->
      <xsl:text>    saftbus::Error error(saftbus::Error::UNKNOWN_METHOD, "No such method.");&#10;</xsl:text>
      <xsl:text>    invocation->return_error(error);&#10;</xsl:text>
      <xsl:text>  }&#10;}&#10;&#10;</xsl:text>

      <!-- Property getters -->
      <xsl:text>void i</xsl:text>
      <xsl:value-of select="$iface"/>
      <xsl:text>_Service::on_get_property(&#10;</xsl:text>
      <xsl:text>  saftbus::Serial&amp; property,&#10;</xsl:text>
      <xsl:text>  const std::shared_ptr&lt;saftbus::Connection&gt;&amp; connection_,&#10;</xsl:text>
      <xsl:text>  const std::string&amp; sender_, const std::string&amp; object_path,&#10;</xsl:text>
      <xsl:text>  const std::string&amp; /*interface_name */, const std::string&amp; property_name)&#10;{&#10;</xsl:text>
      <xsl:text>  property.put_init();&#10;</xsl:text>
<!--       <xsl:text>  sender = &amp;sender_;&#10;</xsl:text>
      <xsl:text>  objectPath = &amp;object_path;&#10;</xsl:text>
      <xsl:text>  connection = connection_;&#10;</xsl:text> -->
      <xsl:text>  </xsl:text>
      <xsl:for-each select="property[@access='read' or @access='readwrite']">
        <xsl:text>if (property_name == "</xsl:text>
        <xsl:value-of select="@name"/>
        <xsl:text>") {&#10;</xsl:text>
        <xsl:text>    try {&#10;</xsl:text>
        <xsl:text>      property.put(impl-&gt;get</xsl:text>
        <xsl:value-of select="@name"/>
        <xsl:text>());&#10;</xsl:text>
        <!-- <xsl:text>      connection.reset();&#10;</xsl:text> -->
        <xsl:text>    } catch (...) {&#10;</xsl:text>
        <!-- <xsl:text>      connection.reset();&#10;</xsl:text> -->
        <xsl:text>      rethrow("</xsl:text>
        <xsl:value-of select="@name"/>
        <xsl:text>");&#10;</xsl:text>
        <xsl:text>      throw;&#10;</xsl:text>
        <xsl:text>    }&#10;</xsl:text>
        <xsl:text>  } else </xsl:text>
      </xsl:for-each>
      <xsl:text>{&#10;    // no property found&#10;    }&#10;}&#10;&#10;</xsl:text>

      <!-- Property setters -->
      <xsl:text>bool i</xsl:text>
      <xsl:value-of select="$iface"/>
      <xsl:text>_Service::on_set_property(&#10;</xsl:text>
      <xsl:text>  const std::shared_ptr&lt;saftbus::Connection&gt;&amp; connection_,&#10;</xsl:text>
      <xsl:text>  const std::string&amp; sender_, const std::string&amp; object_path,&#10;</xsl:text>
      <xsl:text>  const std::string&amp; /* interface_name */, const std::string&amp; property_name,&#10;</xsl:text>
      <xsl:text>  const saftbus::Serial&amp; value)&#10;{&#10;</xsl:text>
      <xsl:text>  sender = &amp;sender_;&#10;</xsl:text>
      <xsl:text>  objectPath = &amp;object_path;&#10;</xsl:text>
      <xsl:text>  connection = connection_;&#10;</xsl:text>
      <xsl:text>  </xsl:text>
      <xsl:for-each select="property[@access='write' or @access='readwrite']">
        <xsl:text>if (property_name == "</xsl:text>
        <xsl:value-of select="@name"/>
        <xsl:text>") {&#10;</xsl:text>
        <xsl:text>    try {&#10;</xsl:text>
        <xsl:text>      </xsl:text>
        <xsl:call-template name="raw-type"/>
        <xsl:text> val;&#10;</xsl:text>
        <xsl:text>      value.get_init();&#10;</xsl:text>
        <xsl:text>      value.get(val);&#10;</xsl:text>
        <xsl:text>      impl-&gt;set</xsl:text>
        <xsl:value-of select="@name"/>
        <xsl:text>(val);&#10;</xsl:text>
        <!-- <xsl:text>      connection.reset();&#10;</xsl:text> -->
        <xsl:text>      return true;&#10;</xsl:text>
        <xsl:text>    } catch (...) {&#10;</xsl:text>
        <!-- <xsl:text>      connection.reset();&#10;</xsl:text> -->
        <xsl:text>      rethrow("</xsl:text>
        <xsl:value-of select="@name"/>
        <xsl:text>");&#10;</xsl:text>
        <xsl:text>      throw;&#10;</xsl:text>
        <xsl:text>    }&#10;</xsl:text>
        <xsl:text>  } else </xsl:text>
      </xsl:for-each>
      <xsl:text>{&#10;    // no property found&#10;    return false;&#10;  }&#10;}&#10;&#10;</xsl:text>

      <!-- Forward signals -->
      <xsl:for-each select="signal">
        <xsl:call-template name="signal-mtype">
         <xsl:with-param name="namespace">i<xsl:value-of select="$iface"/>_Service::on_sig</xsl:with-param>
        </xsl:call-template>
        <xsl:text>&#10;{&#10;</xsl:text>
        <xsl:text>  saftbus::Serial data_vector;&#10;</xsl:text>
        <xsl:for-each select="arg">
         <xsl:text>  data_vector.put(</xsl:text>
<!--          <xsl:call-template name="raw-type"/>
         <xsl:text>::create(</xsl:text> -->
         <xsl:value-of select="@name"/>
         <xsl:text>);&#10;</xsl:text>
        </xsl:for-each>
        <xsl:text>  for (unsigned i = 0; i &lt; exports.size(); ++i) {&#10;</xsl:text>
        <xsl:text>    exports[i].connection->emit_signal(exports[i].object_path,&#10;</xsl:text>
        <xsl:text>      "</xsl:text>
        <xsl:value-of select="$iface_full"/>
        <xsl:text>", "</xsl:text>
        <xsl:value-of select="@name"/>
        <xsl:text>", "", &#10;</xsl:text>
        <xsl:text>      data_vector);&#10;</xsl:text>
        <xsl:text>  }&#10;}&#10;&#10;</xsl:text>
      </xsl:for-each>

      <!-- Constructor -->
      <xsl:text>i</xsl:text>
      <xsl:value-of select="$iface"/>
      <xsl:text>_Service::i</xsl:text>
      <xsl:value-of select="$iface"/>
      <xsl:text>_Service(i</xsl:text>
      <xsl:value-of select="$iface"/>
      <xsl:text>* impl_, sigc::slot&lt;void, const char*&gt; rethrow_)&#10;</xsl:text>
      <xsl:text>: impl(impl_), rethrow(rethrow_),&#10;</xsl:text>
      <xsl:for-each select="signal">
        <xsl:text>  con_sig</xsl:text>
        <xsl:value-of select="@name"/>
        <xsl:text>(impl_-></xsl:text>
        <xsl:value-of select="@name"/>
        <xsl:text>.connect(sigc::mem_fun(this, &amp;i</xsl:text>
        <xsl:value-of select="$iface"/>
        <xsl:text>_Service::on_sig</xsl:text>
        <xsl:value-of select="@name"/>
        <xsl:text>))),&#10;</xsl:text>
      </xsl:for-each>
      <xsl:text>  interface_vtable(&#10;</xsl:text>
      <xsl:text>    sigc::mem_fun(this, &amp;i</xsl:text>
      <xsl:value-of select="$iface"/>
      <xsl:text>_Service::on_method_call),&#10;</xsl:text>
      <xsl:text>    sigc::mem_fun(this, &amp;i</xsl:text>
      <xsl:value-of select="$iface"/>
      <xsl:text>_Service::on_get_property),&#10;</xsl:text>
      <xsl:text>    sigc::mem_fun(this, &amp;i</xsl:text>
      <xsl:value-of select="$iface"/>
      <xsl:text>_Service::on_set_property))&#10;</xsl:text>
      <xsl:text>{&#10;}&#10;&#10;</xsl:text>

      <!-- Deconstructor -->
      <xsl:text>i</xsl:text>
      <xsl:value-of select="$iface"/>
      <xsl:text>_Service::~i</xsl:text>
      <xsl:value-of select="$iface"/>
      <xsl:text>_Service()&#10;{&#10;</xsl:text>
      <xsl:text>  unregister_self();&#10;</xsl:text>
      <xsl:for-each select="signal">
        <xsl:text>  con_sig</xsl:text>
        <xsl:value-of select="@name"/>
        <xsl:text>.disconnect();&#10;</xsl:text>
      </xsl:for-each>
      <xsl:text>}&#10;&#10;</xsl:text>

      <xsl:text>}&#10;</xsl:text>

    </xsl:document>
  </xsl:for-each>
</xsl:template>

</xsl:stylesheet>
