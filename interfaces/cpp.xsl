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
    <xsl:text>#include &lt;giomm.h&gt;&#10;</xsl:text>
    <xsl:text>#include &lt;glibmm.h&gt;&#10;</xsl:text>
    <xsl:text>#include "</xsl:text>
    <xsl:value-of select="$name"/>
    <xsl:text>.h"&#10;</xsl:text>
    <xsl:text>&#10;</xsl:text>
    <xsl:text>namespace saftlib {&#10;&#10;</xsl:text>

    <xsl:text>Glib::RefPtr&lt;</xsl:text>
    <xsl:value-of select="$name"/>
    <xsl:text>_Proxy&gt; </xsl:text>
    <xsl:value-of select="$name"/>
    <xsl:text>_Proxy::create(&#10;</xsl:text>
    <xsl:text>  const Glib::ustring&amp; object_path,&#10;</xsl:text>
    <xsl:text>  const Glib::ustring&amp; name,&#10;</xsl:text>
    <xsl:text>  Gio::DBus::BusType bus_type,&#10;</xsl:text>
    <xsl:text>  Gio::DBus::ProxyFlags flags)&#10;{&#10;</xsl:text>
    <xsl:text>  return Glib::RefPtr&lt;</xsl:text>
    <xsl:value-of select="$name"/>
    <xsl:text>_Proxy&gt;(new </xsl:text>
    <xsl:value-of select="$name"/>
    <xsl:text>_Proxy(object_path, name, bus_type, flags));&#10;}&#10;&#10;</xsl:text>

    <!-- Proxy Constructor -->
    <xsl:value-of select="$name"/>
    <xsl:text>_Proxy::</xsl:text>
    <xsl:value-of select="$name"/>
    <xsl:text>_Proxy(&#10;</xsl:text>
    <xsl:text>  const Glib::ustring&amp; object_path,&#10;</xsl:text>
    <xsl:text>  const Glib::ustring&amp; name,&#10;</xsl:text>
    <xsl:text>  Gio::DBus::BusType bus_type,&#10;</xsl:text>
    <xsl:text>  Gio::DBus::ProxyFlags flags)&#10;</xsl:text>
    <xsl:text>: </xsl:text>
    <xsl:for-each select="interface">
      <xsl:if test="position()>1">,&#10;  </xsl:if>
      <xsl:apply-templates mode="iface-name" select="."/>
      <xsl:text>(i</xsl:text>
      <xsl:apply-templates mode="iface-name" select="."/>
      <xsl:text>_Proxy::create(object_path, name, bus_type, flags))</xsl:text>
    </xsl:for-each>
    <xsl:for-each select="interface">
      <xsl:variable name="iface"><xsl:apply-templates mode="iface-name" select="."/></xsl:variable>
      <xsl:for-each select="property[@access='read' or @access='readwrite']">
        <xsl:if test="not(annotation[@name = 'org.freedesktop.DBus.Property.EmitsChangedSignal' and @value != 'true'])">
          <xsl:text>,&#10;  con_prop</xsl:text>
          <xsl:value-of select="@name"/>
          <xsl:text>(</xsl:text>
          <xsl:value-of select="$iface"/>
          <xsl:text>-&gt;</xsl:text>
          <xsl:value-of select="@name"/>
          <xsl:text>.connect(</xsl:text>
          <xsl:value-of select="@name"/>
          <xsl:text>.make_slot()))</xsl:text>
        </xsl:if>
      </xsl:for-each>
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
      <xsl:for-each select="property[@access='read' or @access='readwrite']">
        <xsl:if test="not(annotation[@name = 'org.freedesktop.DBus.Property.EmitsChangedSignal' and @value != 'true'])">
          <xsl:text>  con_prop</xsl:text>
          <xsl:value-of select="@name"/>
          <xsl:text>.disconnect();&#10;</xsl:text>
        </xsl:if>
      </xsl:for-each>
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
    <xsl:text>_Service::register_self(const Glib::RefPtr&lt;Gio::DBus::Connection&gt;&amp; con, const Glib::ustring&amp; path)&#10;{&#10;</xsl:text>
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
    <xsl:text>const Glib::ustring&amp; </xsl:text>
    <xsl:value-of select="$name"/>
    <xsl:text>_Service::getSender() const&#10;{&#10;</xsl:text>
    <xsl:for-each select="interface">
      <xsl:text>  if (</xsl:text>
      <xsl:apply-templates mode="iface-name" select="."/>
      <xsl:text>.isActive()) return </xsl:text>
      <xsl:apply-templates mode="iface-name" select="."/>
      <xsl:text>.getSender();&#10;</xsl:text>
    </xsl:for-each>
    <xsl:text>  throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "Not inside DBus callback on this object");&#10;</xsl:text>
    <xsl:text>}&#10;&#10;</xsl:text>

    <!-- getObjectPath method -->
    <xsl:text>const Glib::ustring&amp; </xsl:text>
    <xsl:value-of select="$name"/>
    <xsl:text>_Service::getObjectPath() const&#10;{&#10;</xsl:text>
    <xsl:for-each select="interface">
      <xsl:text>  if (</xsl:text>
      <xsl:apply-templates mode="iface-name" select="."/>
      <xsl:text>.isActive()) return </xsl:text>
      <xsl:apply-templates mode="iface-name" select="."/>
      <xsl:text>.getObjectPath();&#10;</xsl:text>
    </xsl:for-each>
    <xsl:text>  throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "Not inside DBus callback on this object");&#10;</xsl:text>
    <xsl:text>}&#10;&#10;</xsl:text>

    <!-- getConnection method -->
    <xsl:text>const Glib::RefPtr&lt;Gio::DBus::Connection&gt;&amp; </xsl:text>
    <xsl:value-of select="$name"/>
    <xsl:text>_Service::getConnection() const&#10;{&#10;</xsl:text>
    <xsl:for-each select="interface">
      <xsl:text>  if (</xsl:text>
      <xsl:apply-templates mode="iface-name" select="."/>
      <xsl:text>.isActive()) return </xsl:text>
      <xsl:apply-templates mode="iface-name" select="."/>
      <xsl:text>.getConnection();&#10;</xsl:text>
    </xsl:for-each>
    <xsl:text>  throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "Not inside DBus callback on this object");&#10;</xsl:text>
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
      <xsl:text>#include &lt;giomm.h&gt;&#10;</xsl:text>
      <xsl:text>#include &lt;glibmm.h&gt;&#10;</xsl:text>
      <xsl:text>#include &lt;gio/gunixfdlist.h&gt;&#10;</xsl:text>
      <xsl:text>#include "i</xsl:text>
      <xsl:value-of select="$iface"/>
      <xsl:text>.h"&#10;&#10;</xsl:text>
      <xsl:text>namespace saftlib {&#10;&#10;</xsl:text>

      <!-- XML interface file -->
      <xsl:text>const Glib::ustring i</xsl:text>
      <xsl:value-of select="$iface"/>
      <xsl:text>_Service::xml =&#10;"&lt;node&gt;</xsl:text>
      <!-- unfortunately, the following line needs to be replaced by a bunch of code to copy all but the 'Ax' args; is there a simpler way of achieving the same ? -->
<!--       <xsl:apply-templates select="." mode="escape"/> -->
      <xsl:text>"&#10;</xsl:text>
      <xsl:text>"  &lt;interface name='</xsl:text>
      <xsl:value-of select="@name"/>
      <xsl:text>'&gt;"&#10;</xsl:text>
      <!-- properties -->
      <xsl:for-each select="property">
        <xsl:text>"    &lt;property name='</xsl:text>
        <xsl:value-of select="@name"/>
        <xsl:text>' type='</xsl:text>
        <xsl:value-of select="@type"/>
        <xsl:text>' access='</xsl:text>
        <xsl:value-of select="@access"/>
        <xsl:text>'&gt;"&#10;</xsl:text>
          <xsl:for-each select="annotation">
            <xsl:text>"      &lt;annotation name='</xsl:text>
            <xsl:value-of select="@name"/>
            <xsl:text>' value='</xsl:text>
            <xsl:value-of select="@value"/>
            <xsl:text>'/&gt;"&#10;</xsl:text>
          </xsl:for-each>
        <xsl:text>"    &lt;/property&gt;"&#10;</xsl:text>
      </xsl:for-each>
      <!-- methods -->
      <xsl:for-each select="method">
        <xsl:text>"    &lt;method name='</xsl:text>
        <xsl:value-of select="@name"/>
        <xsl:text>'&gt;"&#10;</xsl:text>
          <xsl:for-each select="arg">
            <xsl:if test="not(substring(@type,1,1)='A' or @type='h')"> <!-- remove the array-through-pipe arguments from the d-bus interface -->
              <xsl:text>"      &lt;arg direction='</xsl:text>
              <xsl:value-of select="@direction"/>
              <xsl:text>' type='</xsl:text>
              <xsl:value-of select="@type"/>
              <xsl:text>' name='</xsl:text>
              <xsl:value-of select="@name"/>
              <xsl:text>'/&gt;"&#10;</xsl:text>
            </xsl:if>
          </xsl:for-each>
        <xsl:text>"    &lt;/method&gt;"&#10;</xsl:text>
      </xsl:for-each>
      <!-- signals -->
      <xsl:for-each select="signal">
        <xsl:text>"    &lt;signal name='</xsl:text>
        <xsl:value-of select="@name"/>
        <xsl:text>'&gt;"&#10;</xsl:text>
          <xsl:for-each select="arg">
            <xsl:if test="not(substring(@type,1,1)='A')"> <!-- remove the array-through-pipe arguments from the d-bus interface -->
              <xsl:text>"      &lt;arg name='</xsl:text>
              <xsl:value-of select="@name"/>
              <xsl:text>' type='</xsl:text>
              <xsl:value-of select="@type"/>
              <xsl:text>'/&gt;"&#10;</xsl:text>
            </xsl:if>
          </xsl:for-each>
        <xsl:text>"    &lt;/signal&gt;"&#10;</xsl:text>
      </xsl:for-each>
      <xsl:text>"  &lt;/interface&gt;"&#10;</xsl:text>
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
        <xsl:choose>
          <xsl:when test="count(arg[substring(@type,1,1)='A' or @type='h'])=0"> <!-- only if there are no vector-through-pipe requests ('A') -->
            <xsl:text>&#10;{&#10;</xsl:text>
            <xsl:text>  std::vector&lt;Glib::VariantBase&gt; query_vector;&#10;</xsl:text>
            <xsl:for-each select="arg[@direction='in']">
              <xsl:text>  query_vector.push_back(</xsl:text>
              <xsl:call-template name="variant-type"/>
              <xsl:text>::create(</xsl:text>
              <xsl:value-of select="@name"/>
              <xsl:text>));&#10;</xsl:text>
            </xsl:for-each>
            <xsl:text>  const Glib::VariantContainerBase&amp; query = Glib::VariantContainerBase::create_tuple(query_vector);&#10;</xsl:text>
            <xsl:text>  </xsl:text>
            <xsl:if test="arg[@direction='out']">const Glib::VariantContainerBase&amp; response = </xsl:if>
            <xsl:text>call_sync("</xsl:text>
            <xsl:value-of select="@name"/>
            <xsl:text>", query);&#10;</xsl:text>
            <xsl:for-each select="arg[@direction='out']">
              <xsl:text>  </xsl:text>
              <xsl:call-template name="variant-type"/> ov_<xsl:value-of select="@name"/>
              <xsl:text>;&#10;</xsl:text>
            </xsl:for-each>
            <xsl:for-each select="arg[@direction='out']">
              <xsl:text>  response.get_child(ov_</xsl:text>
              <xsl:value-of select="@name"/>
              <xsl:text>, </xsl:text>
              <xsl:value-of select="position()-1"/>
              <xsl:text>);&#10;</xsl:text>
            </xsl:for-each>
            <xsl:choose>
              <xsl:when test="count(arg[@direction='out']) = 1">
                <xsl:text>  return ov_</xsl:text>
                <xsl:value-of select="arg[@direction='out']/@name"/>
                <xsl:text>.get();&#10;</xsl:text>
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
          </xsl:when>
          <xsl:otherwise> <!-- there are 'A' or 'h' types -->
            <xsl:text>&#10;{&#10;</xsl:text>
            <xsl:text>  Glib::RefPtr&lt;Gio::DBus::Connection&gt; connection = get_connection();&#10;</xsl:text>
            <xsl:text>  connection-&gt;reference();&#10;</xsl:text>
            <xsl:text>  Glib::RefPtr&lt;Gio::Cancellable&gt; cancellable;&#10;</xsl:text>
            <xsl:text>  Glib::RefPtr&lt;Gio::UnixFDList&gt;  fd_list = Gio::UnixFDList::create();&#10;</xsl:text>
            <xsl:if test="not(count(arg[substring(@type,1,1)='A'])=0)"> <!-- in this case we only have 'h' and don't need to open a pipe -->
              <xsl:text>  gint _vector_pipe_fd[2];&#10;</xsl:text>
              <xsl:text>  /*int r =*/ pipe(_vector_pipe_fd);&#10;</xsl:text>
              <xsl:text>  fd_list-&gt;append(_vector_pipe_fd[0]);&#10;</xsl:text>
              <xsl:text>  fd_list-&gt;append(_vector_pipe_fd[1]);&#10;</xsl:text>
            </xsl:if>
            <xsl:text>  Glib::RefPtr&lt;Gio::UnixFDList&gt; out_fd_list = Gio::UnixFDList::create();&#10;</xsl:text>
            <xsl:text>  int timeout_msec = -1;&#10;</xsl:text> 
            <xsl:text>  Gio::DBus::CallFlags flags = Gio::DBus::CALL_FLAGS_NONE;&#10;</xsl:text>
            <xsl:text>  Glib::VariantType  reply_type;&#10;</xsl:text>
            <xsl:text>  Glib::RefPtr&lt;Glib::MainLoop&gt;    mainloop = Glib::MainLoop::create();&#10;</xsl:text>

            <!--  same as without vector-through-pipe request -->

            <xsl:text>  std::vector&lt;Glib::VariantBase&gt; query_vector;&#10;</xsl:text>
            <xsl:for-each select="arg[@direction='in' and not(substring(@type,1,1)='A')]"> <!-- for all non 'A' types -->
              <xsl:choose>
                <xsl:when test="@type='h'"> <!-- case of a file descriptor -->
                  <xsl:text>  fd_list-&gt;append(</xsl:text>
                  <xsl:value-of select="@name"/>
                  <xsl:text>);&#10;</xsl:text>
                </xsl:when>
                <xsl:otherwise>             <!-- all other argument types -->
                  <xsl:text>  query_vector.push_back(</xsl:text>
                  <xsl:call-template name="variant-type"/>
                  <xsl:text>::create(</xsl:text>
                  <xsl:value-of select="@name"/>
                  <xsl:text>));&#10;</xsl:text>
                </xsl:otherwise>
              </xsl:choose>
            </xsl:for-each>
            <xsl:text>  const Glib::VariantContainerBase&amp; query = Glib::VariantContainerBase::create_tuple(query_vector);&#10;&#10;</xsl:text>

            <xsl:text>  Glib::VariantContainerBase response;&#10;</xsl:text>

            <!-- make asynchronous call -->
            <xsl:text>  connection-&gt;call(&#10;</xsl:text>
            <xsl:text>      get_object_path(), &#10;</xsl:text>
            <xsl:text>      "de.gsi.saftlib.</xsl:text>
            <xsl:value-of select="$iface"/>
            <xsl:text>",&#10;</xsl:text> 
            <xsl:text>      "</xsl:text>
            <xsl:value-of select="@name"/>
            <xsl:text>",&#10;</xsl:text>
            <xsl:text>      query,&#10;</xsl:text>
            <xsl:text>      sigc::bind(sigc::bind(sigc::mem_fun(this, &amp;i</xsl:text> 
            <xsl:value-of select="$iface"/>
            <xsl:text>_Proxy::AsyncCallReady), &amp;response), mainloop),&#10;</xsl:text>
            <xsl:text>      cancellable,&#10;</xsl:text>
            <xsl:text>      fd_list,&#10;</xsl:text>
            <xsl:text>      "de.gsi.saftlib",&#10;</xsl:text>
            <xsl:text>      timeout_msec,&#10;</xsl:text>
            <xsl:text>      flags,&#10;</xsl:text>
            <xsl:text>      reply_type</xsl:text>
            <xsl:text>  );&#10;&#10;</xsl:text>  

            <!-- send vector data over pipe -->
            <xsl:for-each select="arg[@direction='in' and substring(@type,1,1)='A']">
              <xsl:text>  write_vector_to_pipe(_vector_pipe_fd[1], </xsl:text>
              <xsl:value-of select="@name"/>
              <xsl:text>);&#10;</xsl:text>
            </xsl:for-each>

            <xsl:text>&#10;</xsl:text>  

            <xsl:text>  mainloop-&gt;run(); // wait unitl the d-bus call was answered ("AsyncCallReady" was called)&#10;&#10;</xsl:text>

            <xsl:for-each select="arg[@direction='out' and substring(@type,1,1)='A']">
              <xsl:if test="count(../arg[@direction='out']) = 1">
                <xsl:text>  </xsl:text>
                <xsl:call-template name="raw-type"/> ov_<xsl:value-of select="@name"/>
                <xsl:text>;&#10;</xsl:text>
              </xsl:if>
              <xsl:text>  read_vector_from_pipe(_vector_pipe_fd[0], </xsl:text>
              <xsl:if test="count(../arg[@direction='out']) = 1">
                <xsl:text>ov_</xsl:text>
              </xsl:if>
              <xsl:value-of select="@name"/>
              <xsl:text>);&#10;</xsl:text>                  
            </xsl:for-each>
            <xsl:for-each select="arg[@direction='out' and not(substring(@type,1,1)='A' or @type='h')]">
              <xsl:text>  </xsl:text>
              <xsl:call-template name="variant-type"/> ov_<xsl:value-of select="@name"/>
              <xsl:text>;&#10;</xsl:text>
              <xsl:text>  response.get_child(ov_</xsl:text>
              <xsl:value-of select="@name"/>
              <xsl:text>, </xsl:text>
              <xsl:value-of select="position()-1"/>
              <xsl:text>);&#10;</xsl:text>
            </xsl:for-each>
            <xsl:if test="not(count(arg[@type='h' and @direction='out'])=0)">
              <!-- do not support output file descriptors for now -->
              <xsl:message terminate="yes">
                Error: File descriptors are only supported as direction="in" arguments
              </xsl:message>
            </xsl:if>

            <xsl:if test="not(count(arg[substring(@type,1,1)='A'])=0)">
              <xsl:text>  close(_vector_pipe_fd[0]);&#10;</xsl:text>
              <xsl:text>  close(_vector_pipe_fd[1]);&#10;</xsl:text>
            </xsl:if>

            <xsl:choose>
              <xsl:when test="count(arg[@direction='out']) = 1">
                <xsl:text>  return ov_</xsl:text>
                <xsl:value-of select="arg[@direction='out']/@name"/>
                <xsl:if test="not(substring(arg[@direction='out']/@type,1,1)='A')">
                  <xsl:text>.get()</xsl:text>
                </xsl:if>
                <xsl:text>;&#10;</xsl:text>
              </xsl:when>
              <xsl:otherwise>
                <xsl:for-each select="arg[@direction='out' and not(substring(@type,1,1)='A' or @type='h')]">
                  <xsl:text>  </xsl:text>
                  <xsl:value-of select="@name"/>
                  <xsl:text> = ov_</xsl:text>
                  <xsl:value-of select="@name"/>
                  <xsl:text>.get();&#10;</xsl:text>
                </xsl:for-each>
              </xsl:otherwise>
            </xsl:choose>

            <xsl:text>}&#10;&#10;</xsl:text>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:for-each>

      <!-- AsyncCallReady callback method implementation -->
      <!-- <xsl:variable name="void" select="count(arg[@direction='out']) != 1"/> -->
      <xsl:text>void i</xsl:text><xsl:value-of select="$iface"/><xsl:text>_Proxy::</xsl:text>
      <!-- <xsl:value-of select="@name"/> -->
      <xsl:text>AsyncCallReady(Glib::RefPtr&lt;Gio::AsyncResult&gt;&amp; async_result, Glib::RefPtr&lt;Glib::MainLoop&gt; mainloop, Glib::VariantContainerBase *result)</xsl:text>
      <xsl:text>&#10;</xsl:text>
      <xsl:text>{&#10;</xsl:text>
      <xsl:text>    auto connection = get_connection();&#10;</xsl:text>
      <xsl:text>    connection-&gt;reference();&#10;</xsl:text>
      <xsl:text>    *result = connection-&gt;call_finish(async_result);&#10;</xsl:text>
      <xsl:text>    mainloop-&gt;quit();&#10;</xsl:text>
      <xsl:text>}&#10;&#10;</xsl:text>

      <!-- Boiler-plate to retrieve a property -->
      <xsl:text>void i</xsl:text>
      <xsl:value-of select="$iface"/>
      <xsl:text>_Proxy::fetch_property(const char* name, Glib::VariantBase&amp; val) const&#10;</xsl:text>
      <xsl:text>{&#10;</xsl:text>
      <xsl:text>  std::vector&lt; Glib::VariantBase &gt; params;&#10;</xsl:text>
      <xsl:text>  params.push_back(Glib::Variant&lt; Glib::ustring &gt;::create("</xsl:text>
      <xsl:value-of select="$iface_full"/>
      <xsl:text>"));&#10;</xsl:text>
      <xsl:text>  params.push_back(Glib::Variant&lt; Glib::ustring &gt;::create(name));&#10;</xsl:text>
      <xsl:text>  Glib::RefPtr&lt;Gio::DBus::Connection&gt; connection =&#10;</xsl:text>
      <xsl:text>    Glib::RefPtr&lt;Gio::DBus::Connection&gt;::cast_const(get_connection());&#10;</xsl:text>
      <xsl:text>  connection->reference(); // work around get_connection does not increase reference bug&#10;</xsl:text>
      <xsl:text>  const Glib::VariantContainerBase&amp; result =&#10;</xsl:text>
      <xsl:text>    connection->call_sync(get_object_path(), "org.freedesktop.DBus.Properties", "Get", &#10;</xsl:text>
      <xsl:text>      Glib::VariantContainerBase::create_tuple(params), get_name());&#10;</xsl:text>
      <xsl:text>  Glib::Variant&lt;Glib::VariantBase&gt; variant;&#10;</xsl:text>
      <xsl:text>  result.get_child(variant, 0);&#10;</xsl:text>
      <xsl:text>  variant.get(val);&#10;</xsl:text>
      <xsl:text>}&#10;&#10;</xsl:text>

      <!-- Property getters -->
      <xsl:for-each select="property[@access='read' or @access='readwrite']">
        <xsl:call-template name="prop-gettype">
          <xsl:with-param name="namespace">i<xsl:value-of select="$iface"/>_Proxy::</xsl:with-param>
        </xsl:call-template>
        <xsl:text>&#10;{&#10;  </xsl:text>
        <xsl:call-template name="variant-type"/>
        <xsl:text> value;&#10;</xsl:text>
        <xsl:if test="not(annotation[@name = 'org.freedesktop.DBus.Property.EmitsChangedSignal' and @value = 'false'])">
          <xsl:text>  get_cached_property(value, "</xsl:text>
          <xsl:value-of select="@name"/>
          <xsl:text>");&#10;</xsl:text>
          <xsl:text>  if (value.gobj()) return value.get();&#10;</xsl:text>
        </xsl:if>
        <xsl:text>  fetch_property("</xsl:text>
        <xsl:value-of select="@name"/>
        <xsl:text>", value);&#10;</xsl:text>
        <xsl:text>  return value.get();&#10;}&#10;&#10;</xsl:text>
      </xsl:for-each>

      <!-- Boiler-plate to set a property -->
      <xsl:text>void i</xsl:text>
      <xsl:value-of select="$iface"/>
      <xsl:text>_Proxy::update_property(const char* name, const Glib::VariantBase&amp; val)&#10;{&#10;</xsl:text>
      <xsl:text>  std::vector&lt; Glib::VariantBase &gt; params;&#10;</xsl:text>
      <xsl:text>  params.push_back(Glib::Variant&lt; Glib::ustring &gt;::create("</xsl:text>
      <xsl:value-of select="$iface_full"/>
      <xsl:text>"));&#10;</xsl:text>
      <xsl:text>  params.push_back(Glib::Variant&lt; Glib::ustring &gt;::create(name));&#10;</xsl:text>
      <xsl:text>  params.push_back(Glib::Variant&lt; Glib::VariantBase &gt;::create(val));&#10;</xsl:text>
      <xsl:text>  Glib::RefPtr&lt;Gio::DBus::Connection&gt; connection = get_connection();&#10;</xsl:text>
      <xsl:text>  connection->reference(); // work around get_connection does not increase reference bug&#10;</xsl:text>
      <xsl:text>  connection->call_sync(get_object_path(), "org.freedesktop.DBus.Properties", "Set",&#10;</xsl:text>
      <xsl:text>    Glib::VariantContainerBase::create_tuple(params), get_name());&#10;}&#10;&#10;</xsl:text>

      <!-- Property setters -->
      <xsl:for-each select="property[@access='write' or @access='readwrite']">
        <xsl:call-template name="prop-settype">
          <xsl:with-param name="namespace">i<xsl:value-of select="$iface"/>_Proxy::</xsl:with-param>
        </xsl:call-template>
        <xsl:text>&#10;{&#10;</xsl:text>
        <xsl:text>  update_property("</xsl:text>
        <xsl:value-of select="@name"/>
        <xsl:text>", </xsl:text>
        <xsl:call-template name="variant-type"/>
        <xsl:text>::create(val));&#10;}&#10;&#10;</xsl:text>
      </xsl:for-each>

      <!-- Receive changed properties -->
      <xsl:text>void i</xsl:text>
      <xsl:value-of select="$iface"/>
      <xsl:text>_Proxy::on_properties_changed(&#10;</xsl:text>
      <xsl:text>  const MapChangedProperties&amp; changed_properties,&#10;</xsl:text>
      <xsl:text>  const std::vector&lt; Glib::ustring &gt;&amp; invalidated_properties)&#10;</xsl:text>
      <xsl:text>{&#10;</xsl:text>
      <xsl:text>  Gio::DBus::Proxy::on_properties_changed(changed_properties, invalidated_properties);&#10;</xsl:text>
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
            <xsl:value-of select="@name"/>
            <xsl:text>(Glib::VariantBase::cast_dynamic&lt; </xsl:text>
            <xsl:call-template name="variant-type"/>
            <xsl:text> &gt;(i->second).get());&#10;</xsl:text>
          </xsl:otherwise>
        </xsl:choose>
        <xsl:text>    } else </xsl:text>
      </xsl:for-each>
      <xsl:text> {&#10;      // noop&#10;    }&#10;  }&#10;}&#10;&#10;</xsl:text>

      <!-- Receive signals -->
      <xsl:text>void i</xsl:text>
      <xsl:value-of select="$iface"/>
      <xsl:text>_Proxy::on_signal(&#10;</xsl:text>
      <xsl:text>  const Glib::ustring&amp; sender_name,&#10;</xsl:text>
      <xsl:text>  const Glib::ustring&amp; signal_name,&#10;</xsl:text>
      <xsl:text>  const Glib::VariantContainerBase&amp; parameters)&#10;</xsl:text>
      <xsl:text>{&#10;</xsl:text>
      <xsl:text>  Gio::DBus::Proxy::on_signal(sender_name, signal_name, parameters);&#10;</xsl:text>
      <xsl:text>  </xsl:text>
      <xsl:for-each select="signal">
        <xsl:text>if (signal_name == "</xsl:text>
        <xsl:value-of select="@name"/>
        <xsl:text>") {&#10;</xsl:text>
        <xsl:for-each select="arg">
          <xsl:text>    </xsl:text>
          <xsl:call-template name="variant-type"/>
          <xsl:text> </xsl:text>
          <xsl:value-of select="@name"/>
          <xsl:text>;&#10;</xsl:text>
        </xsl:for-each>
        <xsl:for-each select="arg">
          <xsl:text>    parameters.get_child(</xsl:text>
          <xsl:value-of select="@name"/>
          <xsl:text>, </xsl:text>
          <xsl:value-of select="position()-1"/>
          <xsl:text>);&#10;</xsl:text>
        </xsl:for-each>
        <xsl:text>    </xsl:text>
        <xsl:value-of select="@name"/>
        <xsl:text>(</xsl:text>
        <xsl:for-each select="arg">
          <xsl:if test="position()>1">, </xsl:if>
          <xsl:value-of select="@name"/>
          <xsl:text>.get()</xsl:text>
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
      <xsl:text>  Gio::DBus::BusType bus_type,&#10;</xsl:text>
      <xsl:text>  const Glib::ustring&amp; name,&#10;</xsl:text>
      <xsl:text>  const Glib::ustring&amp; object_path,&#10;</xsl:text>
      <xsl:text>  const Glib::ustring&amp; interface_name,&#10;</xsl:text>
      <xsl:text>  Gio::DBus::ProxyFlags flags)&#10;</xsl:text>
      <xsl:text>: Proxy(bus_type, name, object_path, interface_name, Glib::RefPtr&lt;Gio::DBus::InterfaceInfo&gt;(), flags)&#10;</xsl:text>
      <xsl:text>{&#10;}&#10;&#10;</xsl:text>

      <!-- Create -->
      <xsl:text>Glib::RefPtr&lt;i</xsl:text>
      <xsl:value-of select="$iface"/>
      <xsl:text>_Proxy&gt; i</xsl:text>
      <xsl:value-of select="$iface"/>
      <xsl:text>_Proxy::create(&#10;</xsl:text>
      <xsl:text>  const Glib::ustring&amp; object_path,&#10;</xsl:text>
      <xsl:text>  const Glib::ustring&amp; name,&#10;</xsl:text>
      <xsl:text>  Gio::DBus::BusType bus_type,&#10;</xsl:text>
      <xsl:text>  Gio::DBus::ProxyFlags flags)&#10;{&#10;</xsl:text>
      <xsl:text>  return Glib::RefPtr&lt;i</xsl:text>
      <xsl:value-of select="$iface"/>
      <xsl:text>_Proxy&gt;(new i</xsl:text>
      <xsl:value-of select="$iface"/>
      <xsl:text>_Proxy(bus_type, name,&#10;</xsl:text>
      <xsl:text>    object_path, "</xsl:text>
      <xsl:value-of select="$iface_full"/>
      <xsl:text>", flags));&#10;}&#10;&#10;</xsl:text>

      <!-- Register method -->
      <xsl:text>void i</xsl:text>
      <xsl:value-of select="$iface"/>
      <xsl:text>_Service::register_self(const Glib::RefPtr&lt;Gio::DBus::Connection&gt;&amp; connection, const Glib::ustring&amp; object_path)&#10;{&#10;</xsl:text>
      <xsl:text>  static Glib::RefPtr&lt;Gio::DBus::NodeInfo&gt; introspection;&#10;</xsl:text>
      <xsl:text>  if (!introspection)&#10;</xsl:text>
      <xsl:text>    introspection = Gio::DBus::NodeInfo::create_for_xml(xml);&#10;</xsl:text>
      <xsl:text>  guint id = connection->register_object(&#10;</xsl:text>
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
      <xsl:text>  // implicit conversion from Glib::RefPtr to bool is not possible in later versions (&gt;=2.49.1) of Glibmm&#10;</xsl:text>
      <xsl:text>  return static_cast&lt;bool&gt;(connection);&#10;</xsl:text>
      <xsl:text>}&#10;&#10;</xsl:text>

      <!-- getSender method -->
      <xsl:text>const Glib::ustring&amp; i</xsl:text>
      <xsl:value-of select="$iface"/>
      <xsl:text>_Service::getSender() const&#10;{&#10;</xsl:text>
      <xsl:text>  if (!isActive()) throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "Not inside DBus callback on this object");&#10;</xsl:text>
      <xsl:text>  return *sender;&#10;</xsl:text>
      <xsl:text>}&#10;&#10;</xsl:text>

      <!-- getObjectPath method -->
      <xsl:text>const Glib::ustring&amp; i</xsl:text>
      <xsl:value-of select="$iface"/>
      <xsl:text>_Service::getObjectPath() const&#10;{&#10;</xsl:text>
      <xsl:text>  if (!isActive()) throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "Not inside DBus callback on this object");&#10;</xsl:text>
      <xsl:text>  return *objectPath;&#10;</xsl:text>
      <xsl:text>}&#10;&#10;</xsl:text>

      <!-- getConnection method -->
      <xsl:text>const Glib::RefPtr&lt;Gio::DBus::Connection&gt;&amp; i</xsl:text>
      <xsl:value-of select="$iface"/>
      <xsl:text>_Service::getConnection() const&#10;{&#10;</xsl:text>
      <xsl:text>  if (!isActive()) throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "Not inside DBus callback on this object");&#10;</xsl:text>
      <xsl:text>  return connection;&#10;</xsl:text>
      <xsl:text>}&#10;&#10;</xsl:text>

      <!-- Service methods -->
      <xsl:text>void i</xsl:text>
      <xsl:value-of select="$iface"/>
      <xsl:text>_Service::on_method_call(&#10;</xsl:text>
      <xsl:text>  const Glib::RefPtr&lt;Gio::DBus::Connection&gt;&amp; connection_,&#10;</xsl:text>
      <xsl:text>  const Glib::ustring&amp;  sender_, const Glib::ustring&amp; object_path,&#10;</xsl:text>
      <xsl:text>  const Glib::ustring&amp; /* interface_name */, const Glib::ustring&amp; method_name,&#10;</xsl:text>
      <xsl:text>  const Glib::VariantContainerBase&amp; parameters,&#10;</xsl:text>
      <xsl:text>  const Glib::RefPtr&lt;Gio::DBus::MethodInvocation&gt;&amp; invocation)&#10;{&#10;</xsl:text>
      <xsl:text>  sender = &amp;sender_;&#10;</xsl:text>
      <xsl:text>  objectPath = &amp;object_path;&#10;</xsl:text>
      <xsl:text>  connection = connection_;&#10;</xsl:text>
      <xsl:text>  </xsl:text>
      <xsl:for-each select="method">
        <xsl:text>if (method_name == "</xsl:text>
        <xsl:value-of select="@name"/>
        <xsl:text>") {&#10;</xsl:text>
        <xsl:text>    try {&#10;</xsl:text>
        <!-- take a fildescriptor pair from fd_list in case there is any type 'A' present -->
        <xsl:if test="not(count(arg[substring(@type,1,1)='A'])=0) or not(count(arg[@type='h'])=0)">
          <xsl:text>      Glib::RefPtr&lt;Gio::DBus::Message&gt; message = invocation-&gt;get_message();&#10;</xsl:text>
          <xsl:text>      GUnixFDList *fd_list  = g_dbus_message_get_unix_fd_list(message-&gt;gobj());&#10;</xsl:text>
          <xsl:text>      if (!fd_list) { &#10;</xsl:text>
          <xsl:text>        throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "No filedescriptors received");&#10;</xsl:text>
          <xsl:text>      }&#10;</xsl:text>
          <xsl:text>      if (g_unix_fd_list_get_length(fd_list) != 2) { &#10;</xsl:text>
          <xsl:text>        throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "Wrong number of file descriptors received");&#10;</xsl:text>
          <xsl:text>      }&#10;</xsl:text>
          <xsl:text>      int fd_index = 0;&#10;</xsl:text>
          <xsl:if test="not(count(arg[substring(@type,1,1)='A'])=0)">
            <xsl:text>      gint _vector_pipe_fd0 = g_unix_fd_list_get(fd_list, fd_index++, 0);&#10;</xsl:text>
            <xsl:text>      gint _vector_pipe_fd1 = g_unix_fd_list_get(fd_list, fd_index++, 0);&#10;</xsl:text>
          </xsl:if>
        </xsl:if>
        <xsl:for-each select="arg[@direction='in']">
          <xsl:text>      </xsl:text>
          <xsl:choose>
            <xsl:when test="substring(@type,1,1)='A' or @type='h'">
              <xsl:call-template name="raw-type"/>  
            </xsl:when>
            <xsl:otherwise>
              <xsl:call-template name="variant-type"/> 
            </xsl:otherwise>
          </xsl:choose>
          <xsl:text> </xsl:text>
          <xsl:value-of select="@name"/>
          <xsl:if test="@type='h'">
            <xsl:text> = g_unix_fd_list_get(fd_list, fd_index++, 0)</xsl:text>
          </xsl:if>          
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
        <xsl:if test="not(count(arg[substring(@type,1,1)='A'])=0)">
        </xsl:if>
        <xsl:for-each select="arg[@direction='in' and substring(@type,1,1)='A']">
          <xsl:text>      read_vector_from_pipe(_vector_pipe_fd0, </xsl:text>
          <xsl:value-of select="@name"/>
          <xsl:text>);&#10;</xsl:text>
        </xsl:for-each>
        <xsl:for-each select="arg[@direction='in' and not(substring(@type,1,1)='A' or @type='h')]">
          <xsl:text>      parameters.get_child(</xsl:text>
          <xsl:value-of select="@name"/>
          <xsl:text>, </xsl:text>
          <xsl:value-of select="position()-1"/>
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
            <xsl:if test="@direction='in'">
              <xsl:if test="not(substring(@type,1,1)='A' or @type='h')">
                <xsl:text>.get()</xsl:text>
              </xsl:if>
            </xsl:if>
        </xsl:for-each>
        <xsl:text>);&#10;</xsl:text>
        <xsl:if test="not(@reset_connection='false')">
          <xsl:text>        connection.reset();&#10;</xsl:text>
        </xsl:if>
        <xsl:text>      } catch (...) {&#10;</xsl:text>
        <xsl:text>        connection.reset();&#10;</xsl:text>
        <xsl:text>        rethrow("</xsl:text>
        <xsl:value-of select="@name"/>
        <xsl:text>");&#10;</xsl:text>
        <xsl:text>        throw;&#10;</xsl:text>
        <xsl:text>      }&#10;</xsl:text>
        <xsl:text>      std::vector&lt;Glib::VariantBase&gt; response_vector;&#10;</xsl:text>
        <xsl:for-each select="arg[@direction='out']">
          <xsl:if test="not(substring(@type,1,1)='A' or @type='h')">
            <xsl:text>      response_vector.push_back(</xsl:text>
            <xsl:call-template name="variant-type"/>
            <xsl:text>::create(</xsl:text>
            <xsl:value-of select="@name"/>
            <xsl:text>));&#10;</xsl:text>
          </xsl:if>
        </xsl:for-each>
        <xsl:text>      invocation->return_value(Glib::VariantContainerBase::create_tuple(response_vector));&#10;</xsl:text>

        <xsl:for-each select="arg[@direction='out']">
          <xsl:if test="substring(@type,1,1)='A'">
            <xsl:text>      write_vector_to_pipe(_vector_pipe_fd1, </xsl:text>
            <xsl:value-of select="@name"/>
            <xsl:text>);&#10;</xsl:text>
          </xsl:if>
        </xsl:for-each>

        <xsl:if test="not(count(arg[substring(@type,1,1)='A'])=0)">
          <xsl:text>      close(_vector_pipe_fd0);&#10;</xsl:text>
          <xsl:text>      close(_vector_pipe_fd1);&#10;</xsl:text>
        </xsl:if>
        <xsl:text>    } catch (const Gio::DBus::Error&amp; error) {&#10;</xsl:text>
        <xsl:text>      invocation->return_error(error);&#10;</xsl:text>
        <xsl:text>    }&#10;</xsl:text>
        <xsl:text>  } else </xsl:text>
      </xsl:for-each>
      <xsl:text>{&#10;</xsl:text>
      <xsl:text>    connection.reset();&#10;</xsl:text>
      <xsl:text>    Gio::DBus::Error error(Gio::DBus::Error::UNKNOWN_METHOD, "No such method.");&#10;</xsl:text>
      <xsl:text>    invocation->return_error(error);&#10;</xsl:text>
      <xsl:text>  }&#10;}&#10;&#10;</xsl:text>

      <!-- Property getters -->
      <xsl:text>void i</xsl:text>
      <xsl:value-of select="$iface"/>
      <xsl:text>_Service::on_get_property(&#10;</xsl:text>
      <xsl:text>  Glib::VariantBase&amp; property,&#10;</xsl:text>
      <xsl:text>  const Glib::RefPtr&lt;Gio::DBus::Connection&gt;&amp; connection_,&#10;</xsl:text>
      <xsl:text>  const Glib::ustring&amp; sender_, const Glib::ustring&amp; object_path,&#10;</xsl:text>
      <xsl:text>  const Glib::ustring&amp; /*interface_name */, const Glib::ustring&amp; property_name)&#10;{&#10;</xsl:text>
      <xsl:text>  sender = &amp;sender_;&#10;</xsl:text>
      <xsl:text>  objectPath = &amp;object_path;&#10;</xsl:text>
      <xsl:text>  connection = connection_;&#10;</xsl:text>
      <xsl:text>  </xsl:text>
      <xsl:for-each select="property[@access='read' or @access='readwrite']">
        <xsl:text>if (property_name == "</xsl:text>
        <xsl:value-of select="@name"/>
        <xsl:text>") {&#10;</xsl:text>
        <xsl:text>    try {&#10;</xsl:text>
        <xsl:text>      property = </xsl:text>
        <xsl:call-template name="variant-type"/>
        <xsl:text>::create(impl-&gt;get</xsl:text>
        <xsl:value-of select="@name"/>
        <xsl:text>());&#10;</xsl:text>
        <xsl:text>      connection.reset();&#10;</xsl:text>
        <xsl:text>    } catch (...) {&#10;</xsl:text>
        <xsl:text>      connection.reset();&#10;</xsl:text>
        <xsl:text>      rethrow("</xsl:text>
        <xsl:value-of select="@name"/>
        <xsl:text>");&#10;</xsl:text>
        <xsl:text>      throw;&#10;</xsl:text>
        <xsl:text>    }&#10;</xsl:text>
        <xsl:text>  } else </xsl:text>
      </xsl:for-each>
      <xsl:text>{&#10;    // no property found&#10;    connection.reset();&#10;  }&#10;}&#10;&#10;</xsl:text>

      <!-- Property setters -->
      <xsl:text>bool i</xsl:text>
      <xsl:value-of select="$iface"/>
      <xsl:text>_Service::on_set_property(&#10;</xsl:text>
      <xsl:text>  const Glib::RefPtr&lt;Gio::DBus::Connection&gt;&amp; connection_,&#10;</xsl:text>
      <xsl:text>  const Glib::ustring&amp; sender_, const Glib::ustring&amp; object_path,&#10;</xsl:text>
      <xsl:text>  const Glib::ustring&amp; /* interface_name */, const Glib::ustring&amp; property_name,&#10;</xsl:text>
      <xsl:text>  const Glib::VariantBase&amp; value)&#10;{&#10;</xsl:text>
      <xsl:text>  sender = &amp;sender_;&#10;</xsl:text>
      <xsl:text>  objectPath = &amp;object_path;&#10;</xsl:text>
      <xsl:text>  connection = connection_;&#10;</xsl:text>
      <xsl:text>  </xsl:text>
      <xsl:for-each select="property[@access='write' or @access='readwrite']">
        <xsl:text>if (property_name == "</xsl:text>
        <xsl:value-of select="@name"/>
        <xsl:text>") {&#10;</xsl:text>
        <xsl:text>    try {&#10;</xsl:text>
        <xsl:text>      impl-&gt;set</xsl:text>
        <xsl:value-of select="@name"/>
        <xsl:text>(Glib::VariantBase::cast_dynamic&lt; </xsl:text>
        <xsl:call-template name="variant-type"/>
        <xsl:text> &gt;(value).get());&#10;</xsl:text>
        <xsl:text>      connection.reset();&#10;</xsl:text>
        <xsl:text>      return true;&#10;</xsl:text>
        <xsl:text>    } catch (...) {&#10;</xsl:text>
        <xsl:text>      connection.reset();&#10;</xsl:text>
        <xsl:text>      rethrow("</xsl:text>
        <xsl:value-of select="@name"/>
        <xsl:text>");&#10;</xsl:text>
        <xsl:text>      throw;&#10;</xsl:text>
        <xsl:text>    }&#10;</xsl:text>
        <xsl:text>  } else </xsl:text>
      </xsl:for-each>
      <xsl:text>{&#10;    // no property found&#10;    connection.reset();&#10;    return false;&#10;  }&#10;}&#10;&#10;</xsl:text>

      <!-- Forward property changes -->
      <xsl:for-each select="property">
        <xsl:if test="not(annotation[@name = 'org.freedesktop.DBus.Property.EmitsChangedSignal' and @value != 'true'])">
          <xsl:text>void i</xsl:text>
          <xsl:value-of select="$iface"/>
          <xsl:text>_Service::on_prop</xsl:text>
          <xsl:value-of select="@name"/>
          <xsl:text>(</xsl:text>
          <xsl:call-template name="input-type"/>
          <xsl:text> val)&#10;{&#10;</xsl:text>
          <xsl:text>  report_property_change("</xsl:text>
          <xsl:value-of select="@name"/>
          <xsl:text>", </xsl:text>
          <xsl:call-template name="variant-type"/>
          <xsl:text>::create(val));&#10;}&#10;&#10;</xsl:text>
        </xsl:if>
      </xsl:for-each>

      <!-- Forward signals -->
      <xsl:for-each select="signal">
        <xsl:call-template name="signal-mtype">
         <xsl:with-param name="namespace">i<xsl:value-of select="$iface"/>_Service::on_sig</xsl:with-param>
        </xsl:call-template>
        <xsl:text>&#10;{&#10;</xsl:text>
        <xsl:text>  std::vector&lt;Glib::VariantBase&gt; data_vector;&#10;</xsl:text>
        <xsl:for-each select="arg">
         <xsl:text>  data_vector.push_back(</xsl:text>
         <xsl:call-template name="variant-type"/>
         <xsl:text>::create(</xsl:text>
         <xsl:value-of select="@name"/>
         <xsl:text>));&#10;</xsl:text>
        </xsl:for-each>
        <xsl:text>  for (unsigned i = 0; i &lt; exports.size(); ++i) {&#10;</xsl:text>
        <xsl:text>    exports[i].connection->emit_signal(exports[i].object_path,&#10;</xsl:text>
        <xsl:text>      "</xsl:text>
        <xsl:value-of select="$iface_full"/>
        <xsl:text>", "</xsl:text>
        <xsl:value-of select="@name"/>
        <xsl:text>", "", &#10;</xsl:text>
        <xsl:text>      Glib::VariantContainerBase::create_tuple(data_vector));&#10;</xsl:text>
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
      <xsl:for-each select="property">
        <xsl:if test="not(annotation[@name = 'org.freedesktop.DBus.Property.EmitsChangedSignal' and @value != 'true'])">
          <xsl:text>  con_prop</xsl:text>
          <xsl:value-of select="@name"/>
          <xsl:text>(impl_-></xsl:text>
          <xsl:value-of select="@name"/>
          <xsl:text>.connect(sigc::mem_fun(this, &amp;i</xsl:text>
          <xsl:value-of select="$iface"/>
          <xsl:text>_Service::on_prop</xsl:text>
          <xsl:value-of select="@name"/>
          <xsl:text>))),&#10;</xsl:text>
        </xsl:if>
      </xsl:for-each>
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
      <xsl:for-each select="property">
        <xsl:if test="not(annotation[@name = 'org.freedesktop.DBus.Property.EmitsChangedSignal' and @value != 'true'])">
          <xsl:text>  con_prop</xsl:text>
          <xsl:value-of select="@name"/>
          <xsl:text>.disconnect();&#10;</xsl:text>
        </xsl:if>
      </xsl:for-each>
      <xsl:for-each select="signal">
        <xsl:text>  con_sig</xsl:text>
        <xsl:value-of select="@name"/>
        <xsl:text>.disconnect();&#10;</xsl:text>
      </xsl:for-each>
      <xsl:text>}&#10;&#10;</xsl:text>

      <!-- Property reporting boilerplate -->
      <xsl:text>void i</xsl:text>
      <xsl:value-of select="$iface"/>
      <xsl:text>_Service::report_property_change(const char* property, const Glib::VariantBase&amp; value)&#10;{&#10;</xsl:text>
      <xsl:text>  std::map&lt; Glib::ustring, Glib::VariantBase &gt; updated;&#10;</xsl:text>
      <xsl:text>  std::vector&lt; Glib::ustring &gt; invalidated;&#10;</xsl:text>
      <xsl:text>  std::vector&lt;Glib::VariantBase&gt; message_vector;&#10;</xsl:text>
      <xsl:text>  updated[property] = value;&#10;</xsl:text>
      <xsl:text>  message_vector.push_back(Glib::Variant&lt; Glib::ustring &gt;::create("</xsl:text>
      <xsl:value-of select="$iface_full"/>
      <xsl:text>"));&#10;</xsl:text>
      <xsl:text>  message_vector.push_back(Glib::Variant&lt; std::map&lt; Glib::ustring, Glib::VariantBase &gt; &gt;::create(updated));&#10;</xsl:text>
      <xsl:text>  message_vector.push_back(Glib::Variant&lt; std::vector&lt; Glib::ustring &gt; &gt;::create(invalidated));&#10;</xsl:text>
      <xsl:text>  for (unsigned i = 0; i &lt; exports.size(); ++i) {&#10;</xsl:text>
      <xsl:text>    exports[i].connection->emit_signal(exports[i].object_path,&#10;</xsl:text>
      <xsl:text>      "org.freedesktop.DBus.Properties", "PropertiesChanged", "",&#10;</xsl:text>
      <xsl:text>      Glib::VariantContainerBase::create_tuple(message_vector));&#10;</xsl:text>
      <xsl:text>  }&#10;}&#10;&#10;</xsl:text>

      <xsl:text>}&#10;</xsl:text>

    </xsl:document>
  </xsl:for-each>
</xsl:template>

</xsl:stylesheet>
