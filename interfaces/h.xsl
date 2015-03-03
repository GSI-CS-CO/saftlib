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
#ifndef <xsl:value-of select="$big_iface"/>_IFACE_H
#define <xsl:value-of select="$big_iface"/>_IFACE_H

namespace saftlib {

class <xsl:value-of select="$iface"/>_Service : public Glib::Object {
  public:
    // Methods -- implement these in a derived class!<xsl:for-each select="method">
    virtual void <xsl:value-of select="@name"/>(<xsl:for-each select="arg">
      <xsl:if test="position()>1">, </xsl:if>
      <xsl:text>
      </xsl:text>
      <xsl:if test="@direction='in'">const </xsl:if><xsl:apply-templates mode="iface-type" select="."/>&amp; <xsl:value-of select="@name"/>
    </xsl:for-each>);</xsl:for-each>
    // Emit signals<xsl:for-each select="signal">
    void <xsl:value-of select="@name"/>(<xsl:for-each select="arg">
      <xsl:if test="position()>1">, </xsl:if>
      <xsl:text>
      const </xsl:text>
      <xsl:apply-templates mode="iface-type" select="."/>&amp; <xsl:value-of select="@name"/>
    </xsl:for-each>);</xsl:for-each>
    // Property accessors<xsl:for-each select="property">
    virtual const <xsl:apply-templates mode="iface-type" select="."/>&amp; get<xsl:value-of select="@name"/>() const;</xsl:for-each>
    <xsl:for-each select="property">
    virtual void set<xsl:value-of select="@name"/>(const <xsl:apply-templates mode="iface-type" select="."/>&amp; val);</xsl:for-each>
    // Standard Service methods
    <xsl:value-of select="$iface"/>_Service(const Glib::ustring&amp; object_path_);
    virtual ~<xsl:value-of select="$iface"/>_Service();
    virtual void rethrow(const char *method);
    void register_self(const Glib::RefPtr&lt;Gio::DBus::Connection&gt;&amp; connection_);
    void unregister_self();
    const Glib::RefPtr&lt;Gio::DBus::Connection&gt;&amp; getConnection() const { return connection; }
    const Glib::ustring&amp; getSender() const { return sender; }
    const Glib::ustring&amp; getObjectPath() const { return object_path; }
    void setObjectPath(const Glib::ustring&amp; object_path);
  private:
    void report_property_change(const char* name, const Glib::VariantBase&amp; value);
    Glib::ustring object_path;
    Glib::ustring sender;
    Glib::RefPtr&lt;Gio::DBus::Connection&gt; connection;
    guint id;<xsl:for-each select="property">
    <xsl:text>
    </xsl:text><xsl:apply-templates mode="iface-type" select="."/>
    <xsl:text> </xsl:text>
    <xsl:value-of select="@name"/>;</xsl:for-each>
    // non copyable
    <xsl:value-of select="$iface"/>_Service(const <xsl:value-of select="$iface"/>_Service&amp;);
    <xsl:value-of select="$iface"/>_Service&amp; operator = (const <xsl:value-of select="$iface"/>_Service&amp;);
  friend class <xsl:value-of select="$iface"/>_Service_Binding;
};

class <xsl:value-of select="$iface"/>_Proxy : public Gio::DBus::Proxy {
  public:
    // Methods<xsl:for-each select="method">
    void <xsl:value-of select="@name"/>(<xsl:for-each select="arg">
      <xsl:if test="position()>1">, </xsl:if>
      <xsl:text>
      </xsl:text>
      <xsl:if test="@direction='in'">const </xsl:if><xsl:apply-templates mode="iface-type" select="."/>&amp; <xsl:value-of select="@name"/>
    </xsl:for-each>);</xsl:for-each>
    // Signals<xsl:for-each select="signal">
    sigc::signal&lt; void<xsl:for-each select="arg">,
      const <xsl:apply-templates mode="iface-type" select="."/>&amp;</xsl:for-each> &gt; <xsl:value-of select="@name"/>;</xsl:for-each>
    // Property accessors<xsl:for-each select="property[@access='read' or @access='readwrite']"><xsl:text>
    </xsl:text><xsl:apply-templates mode="iface-type" select="."/> get<xsl:value-of select="@name"/>() const;</xsl:for-each>
    <xsl:for-each select="property[@access='write' or @access='readwrite']">
    void set<xsl:value-of select="@name"/>(const <xsl:apply-templates mode="iface-type" select="."/>&amp; val);</xsl:for-each>
    <xsl:for-each select="property[@access='read' or @access='readwrite']">
    sigc::signal&lt; void, const <xsl:apply-templates mode="iface-type" select="."/>&amp; &gt; <xsl:value-of select="@name"/>;</xsl:for-each>

  public:
    // See Gio::DBus::Proxy; these methods are the same
    static void create(
      const Glib::RefPtr&lt;Gio::DBus::Connection&gt;&amp; connection,
      const Glib::ustring&amp; name,
      const Glib::ustring&amp; object_path,
      const Gio::SlotAsyncReady&amp; slot,
      const Glib::RefPtr&lt;Gio::Cancellable&gt;&amp; cancellable,
      const Glib::RefPtr&lt;Gio::DBus::InterfaceInfo&gt;&amp; info = Glib::RefPtr&lt;Gio::DBus::InterfaceInfo&gt;(),
      Gio::DBus::ProxyFlags flags = Gio::DBus::PROXY_FLAGS_NONE);

    static void create(
      const Glib::RefPtr&lt;Gio::DBus::Connection&gt;&amp; connection,
      const Glib::ustring&amp; name,
      const Glib::ustring&amp; object_path,
      const Gio::SlotAsyncReady&amp; slot,
      const Glib::RefPtr&lt;Gio::DBus::InterfaceInfo&gt;&amp; info = Glib::RefPtr&lt;Gio::DBus::InterfaceInfo&gt;(),
      Gio::DBus::ProxyFlags flags = Gio::DBus::PROXY_FLAGS_NONE);

    static Glib::RefPtr&lt;<xsl:value-of select="$iface"/>_Proxy&gt; create_finish(const Glib::RefPtr&lt;Gio::AsyncResult&gt;&amp; res);

    static Glib::RefPtr&lt;<xsl:value-of select="$iface"/>_Proxy&gt; create_sync(
      const Glib::RefPtr&lt;Gio::DBus::Connection&gt;&amp; connection,
      const Glib::ustring&amp; name,
      const Glib::ustring&amp; object_path,
      const Glib::RefPtr&lt;Gio::Cancellable&gt;&amp; cancellable,
      const Glib::RefPtr&lt;Gio::DBus::InterfaceInfo&gt;&amp; info = Glib::RefPtr&lt;Gio::DBus::InterfaceInfo&gt;(),
      Gio::DBus::ProxyFlags flags = Gio::DBus::PROXY_FLAGS_NONE);

    static Glib::RefPtr&lt;<xsl:value-of select="$iface"/>_Proxy&gt; create_sync(
      const Glib::RefPtr&lt;Gio::DBus::Connection&gt;&amp; connection,
      const Glib::ustring&amp; name,
      const Glib::ustring&amp; object_path,
      const Glib::RefPtr&lt;Gio::DBus::InterfaceInfo&gt;&amp; info = Glib::RefPtr&lt;Gio::DBus::InterfaceInfo&gt;(),
      Gio::DBus::ProxyFlags flags = Gio::DBus::PROXY_FLAGS_NONE);

    static void create_for_bus(
      Gio::DBus::BusType bus_type,
      const Glib::ustring&amp; name,
      const Glib::ustring&amp; object_path,
      const Gio::SlotAsyncReady&amp; slot,
      const Glib::RefPtr&lt;Gio::Cancellable&gt;&amp; cancellable,
      const Glib::RefPtr&lt;Gio::DBus::InterfaceInfo&gt;&amp; info = Glib::RefPtr&lt;Gio::DBus::InterfaceInfo&gt;(),
      Gio::DBus::ProxyFlags flags = Gio::DBus::PROXY_FLAGS_NONE);

    static void create_for_bus(
      Gio::DBus::BusType bus_type,
      const Glib::ustring&amp; name,
      const Glib::ustring&amp; object_path,
      const Gio::SlotAsyncReady&amp; slot,
      const Glib::RefPtr&lt;Gio::DBus::InterfaceInfo&gt;&amp; info = Glib::RefPtr&lt;Gio::DBus::InterfaceInfo&gt;(),
      Gio::DBus::ProxyFlags flags = Gio::DBus::PROXY_FLAGS_NONE);

    static Glib::RefPtr&lt;<xsl:value-of select="$iface"/>_Proxy&gt; create_for_bus_finish(const Glib::RefPtr&lt;Gio::AsyncResult&gt;&amp; res);

    static Glib::RefPtr&lt;<xsl:value-of select="$iface"/>_Proxy&gt; create_for_bus_sync(
      Gio::DBus::BusType bus_type,
      const Glib::ustring&amp; name,
      const Glib::ustring&amp; object_path,
      const Glib::RefPtr&lt;Gio::Cancellable&gt;&amp; cancellable,
      const Glib::RefPtr&lt;Gio::DBus::InterfaceInfo&gt;&amp; info = Glib::RefPtr&lt;Gio::DBus::InterfaceInfo&gt;(),
      Gio::DBus::ProxyFlags flags = Gio::DBus::PROXY_FLAGS_NONE);

    static Glib::RefPtr&lt;<xsl:value-of select="$iface"/>_Proxy&gt; create_for_bus_sync(
      Gio::DBus::BusType bus_type,
      const Glib::ustring&amp; name,
      const Glib::ustring&amp; object_path,
      const Glib::RefPtr&lt;Gio::DBus::InterfaceInfo&gt;&amp; info = Glib::RefPtr&lt;Gio::DBus::InterfaceInfo&gt;(),
      Gio::DBus::ProxyFlags flags = Gio::DBus::PROXY_FLAGS_NONE);

  private:
    // noncopyable
    <xsl:value-of select="$iface"/>_Proxy(const <xsl:value-of select="$iface"/>_Proxy&amp;);
    <xsl:value-of select="$iface"/>_Proxy&amp; operator=(const <xsl:value-of select="$iface"/>_Proxy&amp;);

  protected:
    void on_properties_changed(const MapChangedProperties&amp; changed_properties, const std::vector&lt; Glib::ustring &gt;&amp; invalidated_properties);
    void on_signal(const Glib::ustring&amp; sender_name, const Glib::ustring&amp; signal_name, const Glib::VariantContainerBase&amp; parameters);
    void update_property(const char* name, const Glib::VariantBase&amp; val);

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
