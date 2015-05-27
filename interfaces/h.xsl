<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

<xsl:output method="text" encoding="utf-8" indent="no"/>
<xsl:include href="./common.xsl"/>

<xsl:template match="/node">
  <xsl:variable name="name" select="@name"/>

  <xsl:document href="{$name}.h" method="text" encoding="utf-8" indent="no">
    <xsl:text>// This is a generated file. Do not modify.&#10;&#10;</xsl:text>
    <xsl:text>#ifndef </xsl:text>
    <xsl:call-template name="caps-name"><xsl:with-param name="name" select="$name"/></xsl:call-template>
    <xsl:text>_OBJECT_H&#10;</xsl:text>
    <xsl:text>#define </xsl:text>
    <xsl:call-template name="caps-name"><xsl:with-param name="name" select="$name"/></xsl:call-template>
    <xsl:text>_OBJECT_H&#10;&#10;</xsl:text>
    
    <!-- Include the interface definitions -->
    <xsl:for-each select="interface">
      <xsl:text>#include "i</xsl:text>
      <xsl:apply-templates mode="iface-name" select="."/>
      <xsl:text>.h"&#10;</xsl:text>
    </xsl:for-each>
    <xsl:text>&#10;</xsl:text>

    <xsl:text>namespace saftlib {&#10;&#10;</xsl:text>

    <!-- Forward definitions -->
    <xsl:text>class </xsl:text>
    <xsl:value-of select="$name"/>
    <xsl:text>;&#10;</xsl:text>
    <xsl:text>class </xsl:text>
    <xsl:value-of select="$name"/>
    <xsl:text>_Proxy;&#10;</xsl:text>
    <xsl:text>class </xsl:text>
    <xsl:value-of select="$name"/>
    <xsl:text>_Service;&#10;&#10;</xsl:text>

    <!-- Convenience method -->
    <xsl:text>class </xsl:text>
    <xsl:value-of select="$name"/>
    <xsl:text> {&#10;</xsl:text>
    <xsl:text>  public:&#10;</xsl:text>
    <xsl:text>    static Glib::RefPtr&lt;</xsl:text>
    <xsl:value-of select="$name"/>
    <xsl:text>_Proxy&gt; create(&#10;</xsl:text>
    <xsl:text>      const Glib::ustring&amp; object_path</xsl:text>
    <xsl:if test="annotation[@name='de.gsi.saftlib.path']"> = "<xsl:value-of select="annotation[@name='de.gsi.saftlib.path']/@value"/>"</xsl:if>
    <xsl:text>,&#10;</xsl:text>
    <xsl:text>      const Glib::ustring&amp; name</xsl:text>
    <xsl:if test="annotation[@name='de.gsi.saftlib.name']"> = "<xsl:value-of select="annotation[@name='de.gsi.saftlib.name']/@value"/>"</xsl:if>
    <xsl:text>,&#10;</xsl:text>
    <xsl:text>      Gio::DBus::BusType bus_type = Gio::DBus::BUS_TYPE_SYSTEM,&#10;</xsl:text>
    <xsl:text>      Gio::DBus::ProxyFlags flags = Gio::DBus::PROXY_FLAGS_NONE);&#10;</xsl:text>
    <xsl:text>};&#10;&#10;</xsl:text>

    <!-- Proxy class implements all interfaces -->
    <xsl:text>class </xsl:text>
    <xsl:value-of select="$name"/>
    <xsl:text>_Proxy : public Glib::Object</xsl:text>
    <xsl:for-each select="interface">
      <xsl:text>, public i</xsl:text>
      <xsl:apply-templates mode="iface-name" select="."/>
    </xsl:for-each>
    <xsl:text> {&#10;</xsl:text>
    <xsl:text>  public:&#10;</xsl:text>

    <!-- Constructor -->
    <xsl:text>    static Glib::RefPtr&lt;</xsl:text>
    <xsl:value-of select="$name"/>
    <xsl:text>_Proxy&gt; create(&#10;</xsl:text>
    <xsl:text>      const Glib::ustring&amp; object_path</xsl:text>
    <xsl:if test="annotation[@name='de.gsi.saftlib.path']"> = "<xsl:value-of select="annotation[@name='de.gsi.saftlib.path']/@value"/>"</xsl:if>
    <xsl:text>,&#10;</xsl:text>
    <xsl:text>      const Glib::ustring&amp; name</xsl:text>
    <xsl:if test="annotation[@name='de.gsi.saftlib.name']"> = "<xsl:value-of select="annotation[@name='de.gsi.saftlib.name']/@value"/>"</xsl:if>
    <xsl:text>,&#10;</xsl:text>
    <xsl:text>      Gio::DBus::BusType bus_type = Gio::DBus::BUS_TYPE_SYSTEM,&#10;</xsl:text>
    <xsl:text>      Gio::DBus::ProxyFlags flags = Gio::DBus::PROXY_FLAGS_NONE);&#10;</xsl:text>

    <xsl:text>    ~</xsl:text>
    <xsl:value-of select="$name"/>
    <xsl:text>_Proxy();&#10;&#10;</xsl:text>

    <!-- Methods -->
    <xsl:text>    // Proxy all methods to interfaces&#10;</xsl:text>
    <xsl:for-each select="interface">
      <xsl:for-each select="method">
        <xsl:text>    </xsl:text>
        <xsl:call-template name="method-type"/>
        <xsl:text> { return </xsl:text>
        <xsl:apply-templates mode="iface-name" select=".."/>
        <xsl:text>-></xsl:text>
        <xsl:call-template name="method-invoke"/>
        <xsl:text> };&#10;</xsl:text>
      </xsl:for-each>
    </xsl:for-each>

    <!-- Property Getters -->
    <xsl:text>    // Proxy all property gets to interfaces&#10;</xsl:text>
    <xsl:for-each select="interface">
      <xsl:for-each select="property[@access='read' or @access='readwrite']">
        <xsl:text>    </xsl:text>
        <xsl:call-template name="prop-gettype"/>
        <xsl:text> { return </xsl:text>
        <xsl:apply-templates mode="iface-name" select=".."/>
        <xsl:text>-></xsl:text>
        <xsl:call-template name="prop-getinvoke"/>
        <xsl:text> };&#10;</xsl:text>
      </xsl:for-each>
    </xsl:for-each>

    <!-- Property Setters -->
    <xsl:text>    // Proxy all property sets to interfaces&#10;</xsl:text>
    <xsl:for-each select="interface">
      <xsl:for-each select="property[@access='write' or @access='readwrite']">
        <xsl:text>    </xsl:text>
        <xsl:call-template name="prop-settype"/>
        <xsl:text> { return </xsl:text>
        <xsl:apply-templates mode="iface-name" select=".."/>
        <xsl:text>-></xsl:text>
        <xsl:call-template name="prop-setinvoke"/>
        <xsl:text> };&#10;</xsl:text>
      </xsl:for-each>
    </xsl:for-each>

    <!-- Property Signal pointers -->
    <xsl:text>    // These property signals are available from base classes&#10;</xsl:text>
    <xsl:for-each select="interface">
      <xsl:for-each select="property[@access='read' or @access='readwrite']">
        <xsl:if test="not(annotation[@name = 'org.freedesktop.DBus.Property.EmitsChangedSignal' and @value != 'true'])">
          <xsl:text>    //   </xsl:text>
          <xsl:call-template name="prop-sigtype"/>
          <xsl:text>;&#10;</xsl:text>
        </xsl:if>
      </xsl:for-each>
    </xsl:for-each>

    <!-- Signal pointers -->
    <xsl:text>    // These signals are available from base classes&#10;</xsl:text>
    <xsl:for-each select="interface">
      <xsl:for-each select="signal">
        <xsl:text>    //   </xsl:text>
        <xsl:call-template name="signal-type"/>
        <xsl:text>;&#10;</xsl:text>
      </xsl:for-each>
    </xsl:for-each>

    <!-- Constructor -->
    <xsl:text>  protected:&#10;</xsl:text>
    <xsl:text>    </xsl:text>
    <xsl:value-of select="$name"/>
    <xsl:text>_Proxy(&#10;</xsl:text>
    <xsl:text>      const Glib::ustring&amp; object_path,&#10;</xsl:text>
    <xsl:text>      const Glib::ustring&amp; name,&#10;</xsl:text>
    <xsl:text>      Gio::DBus::BusType bus_type = Gio::DBus::BUS_TYPE_SYSTEM,&#10;</xsl:text>
    <xsl:text>      Gio::DBus::ProxyFlags flags = Gio::DBus::PROXY_FLAGS_NONE);&#10;</xsl:text>

    <!-- The proxy objects -->
    <xsl:for-each select="interface">
      <xsl:text>    Glib::RefPtr&lt;i</xsl:text>
      <xsl:apply-templates mode="iface-name" select="."/>
      <xsl:text>_Proxy&gt; </xsl:text>
      <xsl:apply-templates mode="iface-name" select="."/>
      <xsl:text>;&#10;</xsl:text>
    </xsl:for-each>

    <!-- Connections to the embedded signals -->
    <xsl:for-each select="interface">
      <xsl:for-each select="property[@access='read' or @access='readwrite']">
        <xsl:if test="not(annotation[@name = 'org.freedesktop.DBus.Property.EmitsChangedSignal' and @value != 'true'])">
          <xsl:text>    sigc::connection con_prop</xsl:text>
          <xsl:value-of select="@name"/>
          <xsl:text>;&#10;</xsl:text>
        </xsl:if>
      </xsl:for-each>
    </xsl:for-each>

    <xsl:for-each select="interface">
      <xsl:for-each select="signal">
        <xsl:text>    sigc::connection con_sig</xsl:text>
        <xsl:value-of select="@name"/>
        <xsl:text>;&#10;</xsl:text>
      </xsl:for-each>
    </xsl:for-each>

    <!-- Non-copyable -->
    <xsl:text>  private:&#10;</xsl:text>
    <xsl:text>    // non-copyable&#10;</xsl:text>
    <xsl:text>    </xsl:text>
    <xsl:value-of select="$name"/>
    <xsl:text>_Proxy(const </xsl:text>
    <xsl:value-of select="$name"/>
    <xsl:text>_Proxy&amp;);&#10;</xsl:text>
    <xsl:text>    </xsl:text>
    <xsl:value-of select="$name"/>
    <xsl:text>_Proxy&amp; operator = (const </xsl:text>
    <xsl:value-of select="$name"/>
    <xsl:text>_Proxy&amp;);&#10;</xsl:text>

    <!-- End of proxy class -->
    <xsl:text>};&#10;&#10;</xsl:text>

    <!-- Service class implements all interfaces -->
    <xsl:text>class </xsl:text>
    <xsl:value-of select="$name"/>
    <xsl:text>_Service :</xsl:text>
    <xsl:for-each select="interface">
      <xsl:if test="position()>1">,</xsl:if>
      <xsl:text> public i</xsl:text>
      <xsl:apply-templates mode="iface-name" select="."/>
    </xsl:for-each>
    <xsl:text> {&#10;</xsl:text>
    <xsl:text>  public:&#10;</xsl:text>
    <xsl:text>    </xsl:text>
    <xsl:value-of select="$name"/>
    <xsl:text>_Service();&#10;&#10;</xsl:text>
    <xsl:text>    void register_self(const Glib::RefPtr&lt;Gio::DBus::Connection&gt;&amp; con, const Glib::ustring&amp; path);&#10;</xsl:text>
    <xsl:text>    void unregister_self();&#10;</xsl:text>
    <xsl:text>    virtual void rethrow(const char *name) const;&#10;</xsl:text>
    <xsl:text>&#10;</xsl:text>

    <!-- The service objects -->
    <xsl:text>  private:&#10;</xsl:text>
    <xsl:for-each select="interface">
      <xsl:text>    i</xsl:text>
      <xsl:apply-templates mode="iface-name" select="."/>
      <xsl:text>_Service </xsl:text>
      <xsl:apply-templates mode="iface-name" select="."/>
      <xsl:text>;&#10;</xsl:text>
    </xsl:for-each>

    <xsl:text>};&#10;&#10;</xsl:text>

    <xsl:text>}&#10;&#10;</xsl:text>
    <xsl:text>#endif&#10;</xsl:text>
  </xsl:document>

  <!-- Generate interface headers -->
  <xsl:for-each select="interface">
    <xsl:variable name="iface">
      <xsl:apply-templates mode="iface-name" select="."/>
    </xsl:variable>

    <xsl:document href="i{$iface}.h" method="text" encoding="utf-8" indent="no">
      <xsl:text>// This is a generated file. Do not modify.&#10;&#10;</xsl:text>
      <xsl:text>#ifndef </xsl:text>
      <xsl:call-template name="caps-name"><xsl:with-param name="name" select="$iface"/></xsl:call-template>
      <xsl:text>_IFACE_H&#10;</xsl:text>
      <xsl:text>#define </xsl:text>
      <xsl:call-template name="caps-name"><xsl:with-param name="name" select="$iface"/></xsl:call-template>
      <xsl:text>_IFACE_H&#10;&#10;</xsl:text>

      <xsl:text>#include &lt;giomm/dbusproxy.h&gt;&#10;&#10;</xsl:text>
      <xsl:text>namespace saftlib {&#10;&#10;</xsl:text>

      <!-- Forward definitions -->
      <xsl:text>class i</xsl:text>
      <xsl:value-of select="$iface"/>
      <xsl:text>;&#10;</xsl:text>
      <xsl:text>class i</xsl:text>
      <xsl:value-of select="$iface"/>
      <xsl:text>_Proxy;&#10;</xsl:text>
      <xsl:text>class i</xsl:text>
      <xsl:value-of select="$iface"/>
      <xsl:text>_Service;&#10;&#10;</xsl:text>

      <!-- The base interface class -->
      <xsl:text>class i</xsl:text>
      <xsl:value-of select="$iface"/>
      <xsl:text> {&#10;</xsl:text>
      <xsl:text>  public:&#10;</xsl:text>
      <xsl:text>    virtual ~i</xsl:text>
      <xsl:value-of select="$iface"/>
      <xsl:text>();&#10;</xsl:text>

      <xsl:text>    // Methods&#10;</xsl:text>
      <xsl:for-each select="method">
        <xsl:text>    virtual </xsl:text>
        <xsl:call-template name="method-type"/>
        <xsl:text> = 0;&#10;</xsl:text>
      </xsl:for-each>

      <xsl:text>    // Property getters&#10;</xsl:text>
      <xsl:for-each select="property[@access='read' or @access='readwrite']">
        <xsl:text>    virtual </xsl:text>
        <xsl:call-template name="prop-gettype"/>
        <xsl:text> = 0;&#10;</xsl:text>
      </xsl:for-each>

      <xsl:text>    // Property setters&#10;</xsl:text>
      <xsl:for-each select="property[@access='write' or @access='readwrite']">
        <xsl:text>    virtual </xsl:text>
        <xsl:call-template name="prop-settype"/>
        <xsl:text> = 0;&#10;</xsl:text>
      </xsl:for-each>

      <!-- Property Signal pointers -->
      <xsl:text>    // Property signals&#10;</xsl:text>
      <xsl:for-each select="property[@access='read' or @access='readwrite']">
        <xsl:if test="not(annotation[@name = 'org.freedesktop.DBus.Property.EmitsChangedSignal' and @value != 'true'])">
          <xsl:text>    </xsl:text>
          <xsl:call-template name="prop-sigtype"/>
          <xsl:text>;&#10;</xsl:text>
        </xsl:if>
      </xsl:for-each>

      <!-- Signal pointers -->
      <xsl:text>    // Signals&#10;</xsl:text>
      <xsl:for-each select="signal">
        <xsl:text>    </xsl:text>
        <xsl:call-template name="signal-type"/>
        <xsl:text>;&#10;</xsl:text>
      </xsl:for-each>

      <xsl:text>};&#10;&#10;</xsl:text>

      <!-- The proxy interface class -->
      <xsl:text>class i</xsl:text>
      <xsl:value-of select="$iface"/>
      <xsl:text>_Proxy : public Gio::DBus::Proxy, public i</xsl:text>
      <xsl:value-of select="$iface"/>
      <xsl:text> {&#10;</xsl:text>
      <xsl:text>  public:</xsl:text>
    static Glib::RefPtr&lt;i<xsl:value-of select="$iface"/>_Proxy&gt; create(
      const Glib::ustring&amp; object_path,
      const Glib::ustring&amp; name,
      Gio::DBus::BusType bus_type, 
      Gio::DBus::ProxyFlags flags);&#10;&#10;<xsl:text/>
      
      <!-- Concrete implementations -->
      <xsl:text>    // Implement abstract base class methods&#10;</xsl:text>
      <xsl:for-each select="method">
        <xsl:text>    </xsl:text>
        <xsl:call-template name="method-type"/>
        <xsl:text>;&#10;</xsl:text>
      </xsl:for-each>
      <xsl:for-each select="property[@access='read' or @access='readwrite']">
        <xsl:text>    </xsl:text>
        <xsl:call-template name="prop-gettype"/>
        <xsl:text>;&#10;</xsl:text>
      </xsl:for-each>
      <xsl:for-each select="property[@access='write' or @access='readwrite']">
        <xsl:text>    </xsl:text>
        <xsl:call-template name="prop-settype"/>
        <xsl:text>;&#10;</xsl:text>
      </xsl:for-each>

      <!-- Forbid copy -->
      <xsl:text>&#10;  private:&#10;</xsl:text>
      <xsl:text>    // noncopyable&#10;</xsl:text>
      <xsl:text>    i</xsl:text>
      <xsl:value-of select="$iface"/>_Proxy(const i<xsl:value-of select="$iface"/>_Proxy&amp;);&#10;<xsl:text/>
      <xsl:text>    i</xsl:text>
      <xsl:value-of select="$iface"/>_Proxy&amp; operator=(const i<xsl:value-of select="$iface"/>_Proxy&amp;);&#10;<xsl:text/>

      <!-- Implementation -->
  protected:
    void on_properties_changed(const MapChangedProperties&amp; changed_properties, const std::vector&lt; Glib::ustring &gt;&amp; invalidated_properties);
    void on_signal(const Glib::ustring&amp; sender_name, const Glib::ustring&amp; signal_name, const Glib::VariantContainerBase&amp; parameters);
    void fetch_property(const char* name, Glib::VariantBase&amp; val) const;
    void update_property(const char* name, const Glib::VariantBase&amp; val);

    i<xsl:value-of select="$iface"/>_Proxy(
      Gio::DBus::BusType bus_type,
      const Glib::ustring&amp; name,
      const Glib::ustring&amp; object_path,
      const Glib::ustring&amp; interface_name,
      Gio::DBus::ProxyFlags flags);
};

class i<xsl:value-of select="$iface"/>_Service {
  public:
    i<xsl:value-of select="$iface"/>_Service(i<xsl:value-of select="$iface"/>* impl, sigc::slot&lt;void, const char*&gt; rethrow);
    ~i<xsl:value-of select="$iface"/>_Service();
    void register_self(const Glib::RefPtr&lt;Gio::DBus::Connection&gt;&amp; connection, const Glib::ustring&amp; path);
    void unregister_self();
    Glib::ustring sender;
  private:
    // non-copyable
    i<xsl:value-of select="$iface"/>_Service(const i<xsl:value-of select="$iface"/>_Service&amp;);
    i<xsl:value-of select="$iface"/>_Service&amp; operator=(const i<xsl:value-of select="$iface"/>_Service&amp;);
  private:
    i<xsl:value-of select="$iface"/> *impl;
    sigc::slot&lt;void, const char*&gt; rethrow;<xsl:text/>
    <xsl:for-each select="property">
      <xsl:if test="not(annotation[@name = 'org.freedesktop.DBus.Property.EmitsChangedSignal' and @value != 'true'])">
    sigc::connection con_prop<xsl:value-of select="@name"/>;<xsl:text/>
      </xsl:if>
    </xsl:for-each>
    <xsl:for-each select="signal">
    sigc::connection con_sig<xsl:value-of select="@name"/>;<xsl:text/>
    </xsl:for-each>
    const Gio::DBus::InterfaceVTable interface_vtable;
    struct Export {
      Export(const Glib::RefPtr&lt;Gio::DBus::Connection&gt;&amp; c, const Glib::ustring&amp; o, int i) :
        connection(c), object_path(o), id(i) { }
      Glib::RefPtr&lt;Gio::DBus::Connection&gt; connection;
      Glib::ustring object_path;
      guint id;
    };
    std::vector&lt;Export&gt; exports;
    static const Glib::ustring xml;
    void report_property_change(const char* name, const Glib::VariantBase&amp; value);&#10;<xsl:text/>
    <xsl:for-each select="property">
      <xsl:if test="not(annotation[@name = 'org.freedesktop.DBus.Property.EmitsChangedSignal' and @value != 'true'])">
        <xsl:text>    void on_prop</xsl:text>
        <xsl:value-of select="@name"/>
        <xsl:text>(</xsl:text>
        <xsl:call-template name="input-type"/>
        <xsl:text>);&#10;</xsl:text>
      </xsl:if>
    </xsl:for-each>
    <xsl:for-each select="signal">
      <xsl:text>    </xsl:text>
      <xsl:call-template name="signal-mtype">
        <xsl:with-param name="namespace" select="'on_sig'"/>
      </xsl:call-template>
      <xsl:text>;&#10;</xsl:text>
    </xsl:for-each>
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
      const Glib::VariantBase&amp; value);
};

}

#endif
</xsl:document>
</xsl:for-each>
</xsl:template>

</xsl:stylesheet>
