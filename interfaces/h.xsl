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
    // Methods -- implement these in a derived class!<xsl:text/>
    <xsl:for-each select="method">
      <xsl:text>&#10;    virtual </xsl:text>
      <xsl:call-template name="method-service-type"/>
      <xsl:text>;</xsl:text>
    </xsl:for-each>
    // Emit signals<xsl:text/>
    <xsl:for-each select="signal">
      <xsl:text>&#10;    virtual </xsl:text>
      <xsl:call-template name="signal-service-type"/>
      <xsl:text>;</xsl:text>
    </xsl:for-each>
    // Property accessors<xsl:text/>
    <xsl:for-each select="property">
      <xsl:text>&#10;    virtual </xsl:text>
      <xsl:call-template name="prop-service-gettype"/>
      <xsl:text>;</xsl:text>
    </xsl:for-each>
    <xsl:for-each select="property">
      <xsl:text>&#10;    virtual </xsl:text>
      <xsl:call-template name="prop-service-settype"/>
      <xsl:text>;</xsl:text>
    </xsl:for-each>
    // Standard Service methods
    <xsl:value-of select="$iface"/>_Service();
    virtual ~<xsl:value-of select="$iface"/>_Service();
    virtual void rethrow(const char *method);
    void register_self(const Glib::RefPtr&lt;Gio::DBus::Connection&gt;&amp; connection_, const Glib::ustring&amp; path);
    void unregister_self();
  private:
    const Gio::DBus::InterfaceVTable interface_vtable;
  protected:
    // export state
    struct Export {
      Export(const Glib::RefPtr&lt;Gio::DBus::Connection&gt;&amp; c, const Glib::ustring&amp; o, int i) : 
       connection(c), object_path(o), id(i) { }
      Glib::RefPtr&lt;Gio::DBus::Connection&gt; connection;
      Glib::ustring object_path;
      int id;
    };
    std::vector&lt;Export&gt; exports;
    Glib::ustring sender;
  private:
    void report_property_change(const char* name, const Glib::VariantBase&amp; value);
    // property variables<xsl:for-each select="property">
    <xsl:text>
    </xsl:text><xsl:call-template name="raw-type"/>
    <xsl:text> </xsl:text>
    <xsl:value-of select="@name"/>;</xsl:for-each>
    // non copyable
    <xsl:value-of select="$iface"/>_Service(const <xsl:value-of select="$iface"/>_Service&amp;);
    <xsl:value-of select="$iface"/>_Service&amp; operator = (const <xsl:value-of select="$iface"/>_Service&amp;);
    // parsers
    static const Glib::ustring xml;
    void on_method_call(
      const Glib::RefPtr&lt;Gio::DBus::Connection&gt;&amp; /* connection */,
      const Glib::ustring&amp; sender, const Glib::ustring&amp; object_path,
      const Glib::ustring&amp; /* interface_name */, const Glib::ustring&amp; method_name,
      const Glib::VariantContainerBase&amp; parameters,
      const Glib::RefPtr&lt;Gio::DBus::MethodInvocation&gt;&amp; invocation);
    void on_get_property(
      Glib::VariantBase&amp; property,
      const Glib::RefPtr&lt;Gio::DBus::Connection&gt;&amp; /* connection */,
      const Glib::ustring&amp; sender, const Glib::ustring&amp; object_path,
      const Glib::ustring&amp; /*interface_name */, const Glib::ustring&amp; property_name);
    bool on_set_property(
      const Glib::RefPtr&lt;Gio::DBus::Connection&gt;&amp; /* connection */, 
      const Glib::ustring&amp; sender, const Glib::ustring&amp; object_path, 
      const Glib::ustring&amp; /* interface_name */, const Glib::ustring&amp; property_name, 
      const Glib::VariantBase&amp; value) ;
};

class <xsl:value-of select="$iface"/>_Proxy : public Gio::DBus::Proxy {
  public:
    // Methods<xsl:text/>
    <xsl:for-each select="method">
      <xsl:text>&#10;    </xsl:text>
      <xsl:call-template name="method-proxy-type"/>
      <xsl:text>;</xsl:text>
    </xsl:for-each>
    // Signals<xsl:text/>
    <xsl:for-each select="signal">
      <xsl:text>&#10;    </xsl:text>
      <xsl:call-template name="signal-proxy-type"/>
      <xsl:text>;</xsl:text>
    </xsl:for-each>
    // Property accessors<xsl:text/>
    <xsl:for-each select="property[@access='read' or @access='readwrite']">
      <xsl:text>&#10;    </xsl:text>
      <xsl:call-template name="prop-proxy-gettype"/>
      <xsl:text>;</xsl:text>
    </xsl:for-each>
    <xsl:for-each select="property[@access='write' or @access='readwrite']">
      <xsl:text>&#10;    </xsl:text>
      <xsl:call-template name="prop-proxy-settype"/>
      <xsl:text>;</xsl:text>
    </xsl:for-each>
    <xsl:for-each select="property[@access='read' or @access='readwrite']">
      <xsl:if test="not(annotation[@name = 'org.freedesktop.DBus.Property.EmitsChangedSignal' and @value = 'false'])">
        <xsl:text>&#10;    </xsl:text>
        <xsl:call-template name="prop-proxy-sigtype"/>
        <xsl:text>;</xsl:text>
      </xsl:if>
    </xsl:for-each>
  public:
    static Glib::RefPtr&lt;<xsl:value-of select="$iface"/>_Proxy&gt; create(
      const Glib::ustring&amp; object_path<xsl:text/>
        <xsl:if test="annotation[@name='de.gsi.saftlib.path']"> = "<xsl:value-of select="annotation[@name='de.gsi.saftlib.path']/@value"/>"</xsl:if>,
      const Glib::ustring&amp; name<xsl:text/>
        <xsl:if test="annotation[@name='de.gsi.saftlib.name']"> = "<xsl:value-of select="annotation[@name='de.gsi.saftlib.name']/@value"/>"</xsl:if>,
      Gio::DBus::BusType bus_type = Gio::DBus::BUS_TYPE_SESSION,
      Gio::DBus::ProxyFlags flags = Gio::DBus::PROXY_FLAGS_NONE);

  private:
    // noncopyable
    <xsl:value-of select="$iface"/>_Proxy(const <xsl:value-of select="$iface"/>_Proxy&amp;);
    <xsl:value-of select="$iface"/>_Proxy&amp; operator=(const <xsl:value-of select="$iface"/>_Proxy&amp;);

  protected:
    void on_properties_changed(const MapChangedProperties&amp; changed_properties, const std::vector&lt; Glib::ustring &gt;&amp; invalidated_properties);
    void on_signal(const Glib::ustring&amp; sender_name, const Glib::ustring&amp; signal_name, const Glib::VariantContainerBase&amp; parameters);
    void fetch_property(const char* name, Glib::VariantBase&amp; val) const;
    void update_property(const char* name, const Glib::VariantBase&amp; val);

    <xsl:value-of select="$iface"/>_Proxy(
      Gio::DBus::BusType bus_type,
      const Glib::ustring&amp; name,
      const Glib::ustring&amp; object_path,
      const Glib::ustring&amp; interface_name,
      Gio::DBus::ProxyFlags flags);
};

}

#endif
</xsl:for-each>
</xsl:template>

</xsl:stylesheet>
