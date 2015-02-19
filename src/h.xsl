<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

<xsl:output method="text" encoding="utf-8" indent="no"/>
<xsl:include href="./common.xsl"/>

<xsl:template match="/node">// This is a generated file. Do not modify.
#include &lt;giomm/dbusproxy.h&gt;

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
      <xsl:if test="@direction='in'">const </xsl:if><xsl:apply-templates mode="iface-type" select="."/>&amp; <xsl:value-of select="@name"/>
    </xsl:for-each>);</xsl:for-each>
};

class <xsl:value-of select="$iface"/>_Proxy : public Gio::DBus::Proxy {
  private:
    // noncopyable
    <xsl:value-of select="$iface"/>_Proxy(const <xsl:value-of select="$iface"/>_Proxy&amp;);
    <xsl:value-of select="$iface"/>_Proxy&amp; operator=(const <xsl:value-of select="$iface"/>_Proxy&amp;);

  public:<xsl:for-each select="method">
    void <xsl:value-of select="@name"/>(<xsl:for-each select="arg">
      <xsl:if test="position()>1">, </xsl:if>
      <xsl:text>
      </xsl:text>
      <xsl:if test="@direction='in'">const </xsl:if><xsl:apply-templates mode="iface-type" select="."/>&amp; <xsl:value-of select="@name"/>
    </xsl:for-each>);</xsl:for-each>

    // See Gio::DBus::Proxy; these methods are the same
    static void create(
      const Glib::RefPtr&lt;Gio::DBus::Connection&gt;&amp; connection,
      const Glib::ustring&amp; name,
      const Glib::ustring&amp; object_path,
      const Glib::ustring&amp; interface_name,
      const Gio::SlotAsyncReady&amp; slot,
      const Glib::RefPtr&lt;Gio::Cancellable&gt;&amp; cancellable,
      const Glib::RefPtr&lt;Gio::DBus::InterfaceInfo&gt;&amp; info = Glib::RefPtr&lt;Gio::DBus::InterfaceInfo&gt;(),
      Gio::DBus::ProxyFlags flags = Gio::DBus::PROXY_FLAGS_NONE);

    static void create(
      const Glib::RefPtr&lt;Gio::DBus::Connection&gt;&amp; connection,
      const Glib::ustring&amp; name,
      const Glib::ustring&amp; object_path,
      const Glib::ustring&amp; interface_name,
      const Gio::SlotAsyncReady&amp; slot,
      const Glib::RefPtr&lt;Gio::DBus::InterfaceInfo&gt;&amp; info = Glib::RefPtr&lt;Gio::DBus::InterfaceInfo&gt;(),
      Gio::DBus::ProxyFlags flags = Gio::DBus::PROXY_FLAGS_NONE);

    static Glib::RefPtr&lt;<xsl:value-of select="$iface"/>_Proxy&gt; create_finish(const Glib::RefPtr&lt;Gio::AsyncResult&gt;&amp; res);

    static Glib::RefPtr&lt;<xsl:value-of select="$iface"/>_Proxy&gt; create_sync(
      const Glib::RefPtr&lt;Gio::DBus::Connection&gt;&amp; connection,
      const Glib::ustring&amp; name,
      const Glib::ustring&amp; object_path,
      const Glib::ustring&amp; interface_name,
      const Glib::RefPtr&lt;Gio::Cancellable&gt;&amp; cancellable,
      const Glib::RefPtr&lt;Gio::DBus::InterfaceInfo&gt;&amp; info = Glib::RefPtr&lt;Gio::DBus::InterfaceInfo&gt;(),
      Gio::DBus::ProxyFlags flags = Gio::DBus::PROXY_FLAGS_NONE);

    static Glib::RefPtr&lt;<xsl:value-of select="$iface"/>_Proxy&gt; create_sync(
      const Glib::RefPtr&lt;Gio::DBus::Connection&gt;&amp; connection,
      const Glib::ustring&amp; name,
      const Glib::ustring&amp; object_path,
      const Glib::ustring&amp; interface_name,
      const Glib::RefPtr&lt;Gio::DBus::InterfaceInfo&gt;&amp; info = Glib::RefPtr&lt;Gio::DBus::InterfaceInfo&gt;(),
      Gio::DBus::ProxyFlags flags = Gio::DBus::PROXY_FLAGS_NONE);

    static void create_for_bus(
      Gio::DBus::BusType bus_type,
      const Glib::ustring&amp; name,
      const Glib::ustring&amp; object_path,
      const Glib::ustring&amp; interface_name,
      const Gio::SlotAsyncReady&amp; slot,
      const Glib::RefPtr&lt;Gio::Cancellable&gt;&amp; cancellable,
      const Glib::RefPtr&lt;Gio::DBus::InterfaceInfo&gt;&amp; info = Glib::RefPtr&lt;Gio::DBus::InterfaceInfo&gt;(),
      Gio::DBus::ProxyFlags flags = Gio::DBus::PROXY_FLAGS_NONE);

    static void create_for_bus(
      Gio::DBus::BusType bus_type,
      const Glib::ustring&amp; name,
      const Glib::ustring&amp; object_path,
      const Glib::ustring&amp; interface_name,
      const Gio::SlotAsyncReady&amp; slot,
      const Glib::RefPtr&lt;Gio::DBus::InterfaceInfo&gt;&amp; info = Glib::RefPtr&lt;Gio::DBus::InterfaceInfo&gt;(),
      Gio::DBus::ProxyFlags flags = Gio::DBus::PROXY_FLAGS_NONE);

    static Glib::RefPtr&lt;<xsl:value-of select="$iface"/>_Proxy&gt; create_for_bus_finish(const Glib::RefPtr&lt;Gio::AsyncResult&gt;&amp; res);

    static Glib::RefPtr&lt;<xsl:value-of select="$iface"/>_Proxy&gt; create_for_bus_sync(
      Gio::DBus::BusType bus_type,
      const Glib::ustring&amp; name,
      const Glib::ustring&amp; object_path,
      const Glib::ustring&amp; interface_name,
      const Glib::RefPtr&lt;Gio::Cancellable&gt;&amp; cancellable,
      const Glib::RefPtr&lt;Gio::DBus::InterfaceInfo&gt;&amp; info = Glib::RefPtr&lt;Gio::DBus::InterfaceInfo&gt;(),
      Gio::DBus::ProxyFlags flags = Gio::DBus::PROXY_FLAGS_NONE);

    static Glib::RefPtr&lt;<xsl:value-of select="$iface"/>_Proxy&gt; create_for_bus_sync(
      Gio::DBus::BusType bus_type,
      const Glib::ustring&amp; name,
      const Glib::ustring&amp; object_path,
      const Glib::ustring&amp; interface_name,
      const Glib::RefPtr&lt;Gio::DBus::InterfaceInfo&gt;&amp; info = Glib::RefPtr&lt;Gio::DBus::InterfaceInfo&gt;(),
      Gio::DBus::ProxyFlags flags = Gio::DBus::PROXY_FLAGS_NONE);

  protected:
    <xsl:value-of select="$iface"/>_Proxy(const Glib::RefPtr&lt;Gio::DBus::Connection&gt;&amp; connection,
      const Glib::ustring&amp; name,
      const Glib::ustring&amp; object_path,
      const Glib::ustring&amp; interface_name,
      const Gio::SlotAsyncReady&amp; slot,
      const Glib::RefPtr&lt;Gio::Cancellable&gt;&amp; cancellable,
      const Glib::RefPtr&lt;Gio::DBus::InterfaceInfo&gt;&amp; info = Glib::RefPtr&lt;Gio::DBus::InterfaceInfo&gt;(),
      Gio::DBus::ProxyFlags flags = Gio::DBus::PROXY_FLAGS_NONE);

    <xsl:value-of select="$iface"/>_Proxy(const Glib::RefPtr&lt;Gio::DBus::Connection&gt;&amp; connection,
      const Glib::ustring&amp; name,
      const Glib::ustring&amp; object_path,
      const Glib::ustring&amp; interface_name,
      const Gio::SlotAsyncReady&amp; slot,
      const Glib::RefPtr&lt;Gio::DBus::InterfaceInfo&gt;&amp; info = Glib::RefPtr&lt;Gio::DBus::InterfaceInfo&gt;(),
      Gio::DBus::ProxyFlags flags = Gio::DBus::PROXY_FLAGS_NONE);

    <xsl:value-of select="$iface"/>_Proxy(const Glib::RefPtr&lt;Gio::DBus::Connection&gt;&amp; connection,
      const Glib::ustring&amp; name,
      const Glib::ustring&amp; object_path,
      const Glib::ustring&amp; interface_name,
      const Glib::RefPtr&lt;Gio::Cancellable&gt;&amp; cancellable,
      const Glib::RefPtr&lt;Gio::DBus::InterfaceInfo&gt;&amp; info = Glib::RefPtr&lt;Gio::DBus::InterfaceInfo&gt;(),
      Gio::DBus::ProxyFlags flags = Gio::DBus::PROXY_FLAGS_NONE);

    <xsl:value-of select="$iface"/>_Proxy(const Glib::RefPtr&lt;Gio::DBus::Connection&gt;&amp; connection,
      const Glib::ustring&amp; name,
      const Glib::ustring&amp; object_path,
      const Glib::ustring&amp; interface_name,
      const Glib::RefPtr&lt;Gio::DBus::InterfaceInfo&gt;&amp; info = Glib::RefPtr&lt;Gio::DBus::InterfaceInfo&gt;(),
      Gio::DBus::ProxyFlags flags = Gio::DBus::PROXY_FLAGS_NONE);

    <xsl:value-of select="$iface"/>_Proxy(Gio::DBus::BusType bus_type,
      const Glib::ustring&amp; name,
      const Glib::ustring&amp; object_path,
      const Glib::ustring&amp; interface_name,
      const Gio::SlotAsyncReady&amp; slot,
      const Glib::RefPtr&lt;Gio::Cancellable&gt;&amp; cancellable,
      const Glib::RefPtr&lt;Gio::DBus::InterfaceInfo&gt;&amp; info = Glib::RefPtr&lt;Gio::DBus::InterfaceInfo&gt;(),
      Gio::DBus::ProxyFlags flags = Gio::DBus::PROXY_FLAGS_NONE);

    <xsl:value-of select="$iface"/>_Proxy(Gio::DBus::BusType bus_type,
      const Glib::ustring&amp; name,
      const Glib::ustring&amp; object_path,
      const Glib::ustring&amp; interface_name,
      const Gio::SlotAsyncReady&amp; slot,
      const Glib::RefPtr&lt;Gio::DBus::InterfaceInfo&gt;&amp; info = Glib::RefPtr&lt;Gio::DBus::InterfaceInfo&gt;(),
      Gio::DBus::ProxyFlags flags = Gio::DBus::PROXY_FLAGS_NONE);

    <xsl:value-of select="$iface"/>_Proxy(Gio::DBus::BusType bus_type,
      const Glib::ustring&amp; name,
      const Glib::ustring&amp; object_path,
      const Glib::ustring&amp; interface_name,
      const Glib::RefPtr&lt;Gio::Cancellable&gt;&amp; cancellable,
      const Glib::RefPtr&lt;Gio::DBus::InterfaceInfo&gt;&amp; info = Glib::RefPtr&lt;Gio::DBus::InterfaceInfo&gt;(),
      Gio::DBus::ProxyFlags flags = Gio::DBus::PROXY_FLAGS_NONE);

    <xsl:value-of select="$iface"/>_Proxy(Gio::DBus::BusType bus_type,
      const Glib::ustring&amp; name,
      const Glib::ustring&amp; object_path,
      const Glib::ustring&amp; interface_name,
      const Glib::RefPtr&lt;Gio::DBus::InterfaceInfo&gt;&amp; info = Glib::RefPtr&lt;Gio::DBus::InterfaceInfo&gt;(),
      Gio::DBus::ProxyFlags flags = Gio::DBus::PROXY_FLAGS_NONE);
};

}

#endif
</xsl:for-each>
</xsl:template>

</xsl:stylesheet>
