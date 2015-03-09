<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

<xsl:output method="text" encoding="utf-8" indent="no"/>
<xsl:include href="./common.xsl"/>

<xsl:template match="/node">// This is a generated file. Do not modify.

#include &lt;giomm.h&gt;
#include &lt;glibmm.h&gt;
<xsl:for-each select="interface">#include "<xsl:apply-templates mode="iface-name" select="."/>.h"
</xsl:for-each>
namespace saftlib
{
    <xsl:for-each select="interface">
    <xsl:variable name="iface_full" select="@name"/>
    <xsl:variable name="iface">
      <xsl:apply-templates mode="iface-name" select="."/>
    </xsl:variable>
const Glib::ustring <xsl:value-of select="$iface"/>_Service::xml =
  "&lt;node&gt;<xsl:apply-templates select="." mode="escape"/>&lt;/node&gt;";

void <xsl:value-of select="$iface"/>_Service::on_method_call(
  const Glib::RefPtr&lt;Gio::DBus::Connection&gt;&amp; /* connection */,
  const Glib::ustring&amp;  sender, const Glib::ustring&amp; object_path,
  const Glib::ustring&amp; /* interface_name */, const Glib::ustring&amp; method_name,
  const Glib::VariantContainerBase&amp; parameters,
  const Glib::RefPtr&lt;Gio::DBus::MethodInvocation&gt;&amp; invocation)
{
  Glib::RefPtr&lt;<xsl:value-of select="$iface"/>_Service&gt; object(this);
  reference();

  object->sender = sender;<xsl:text/>
  <xsl:for-each select="method">
  if (method_name == "<xsl:value-of select="@name"/>") {<xsl:text/>
    <xsl:for-each select="arg[@direction='in']">
      <xsl:text>&#10;    </xsl:text>
      <xsl:call-template name="variant-type"/> 
      <xsl:text> </xsl:text>
      <xsl:value-of select="@name"/>
      <xsl:text>;</xsl:text>
    </xsl:for-each>
    <xsl:for-each select="arg[@direction='out']">
      <xsl:text>&#10;    </xsl:text>
      <xsl:call-template name="raw-type"/>
      <xsl:text> </xsl:text>
      <xsl:value-of select="@name"/>
      <xsl:text>;</xsl:text>
    </xsl:for-each>
    <xsl:for-each select="arg[@direction='in']">
      <xsl:text>&#10;    parameters.get_child(</xsl:text>
      <xsl:value-of select="@name"/>
      <xsl:text>, </xsl:text>
      <xsl:value-of select="position()-1"/>
      <xsl:text>);</xsl:text>
    </xsl:for-each>
    try {
      try {
        <xsl:variable name="void" select="count(arg[@direction='out']) != 1"/>
        <xsl:if test="not($void)">
          <xsl:value-of select="arg[@direction='out']/@name"/>
          <xsl:text> = </xsl:text>
        </xsl:if>
        <xsl:text>object-></xsl:text>
        <xsl:value-of select="@name"/>
        <xsl:text>(</xsl:text>
        <xsl:for-each select="arg[$void or @direction='in']">
          <xsl:if test="position()>1">, </xsl:if>
          <xsl:value-of select="@name"/>
          <xsl:if test="@direction='in'">.get()</xsl:if>
        </xsl:for-each>
        <xsl:text>);</xsl:text>
      } catch (...) {
        object->rethrow("<xsl:value-of select="@name"/>");
      }
      std::vector&lt;Glib::VariantBase&gt; response_vector;
      <xsl:for-each select="arg[@direction='out']">
        <xsl:text>response_vector.push_back(</xsl:text>
        <xsl:call-template name="variant-type"/>
        <xsl:text>::create(</xsl:text>
        <xsl:value-of select="@name"/>));
      </xsl:for-each>invocation->return_value(Glib::VariantContainerBase::create_tuple(response_vector));
    } catch (const Gio::DBus::Error&amp; error) {
      invocation->return_error(error);
    }
  } else </xsl:for-each>{
    Gio::DBus::Error error(Gio::DBus::Error::UNKNOWN_METHOD, "No such method.");
    invocation->return_error(error);
  }
}

void <xsl:value-of select="$iface"/>_Service::on_get_property(
  Glib::VariantBase&amp; property,
  const Glib::RefPtr&lt;Gio::DBus::Connection&gt;&amp; /* connection */,
  const Glib::ustring&amp; sender, const Glib::ustring&amp; object_path,
  const Glib::ustring&amp; /*interface_name */, const Glib::ustring&amp; property_name)
{
  Glib::RefPtr&lt;<xsl:value-of select="$iface"/>_Service&gt; object(this);
  reference();

  object->sender = sender;
  <xsl:for-each select="property[@access='read' or @access='readwrite']">if (property_name == "<xsl:value-of select="@name"/>") {
    try {
      property = <xsl:call-template name="variant-type"/>::create(object->get<xsl:value-of select="@name"/>());
    } catch (...) {
      object->rethrow("get<xsl:value-of select="@name"/>");
    }
  } else </xsl:for-each>{
    // no property found
  }
}

bool <xsl:value-of select="$iface"/>_Service::on_set_property(
  const Glib::RefPtr&lt;Gio::DBus::Connection&gt;&amp; /* connection */, 
  const Glib::ustring&amp; sender, const Glib::ustring&amp; object_path, 
  const Glib::ustring&amp; /* interface_name */, const Glib::ustring&amp; property_name, 
  const Glib::VariantBase&amp; value) 
{
  Glib::RefPtr&lt;<xsl:value-of select="$iface"/>_Service&gt; object(this);
  reference();

  object->sender = sender;
  <xsl:for-each select="property[@access='write' or @access='readwrite']">if (property_name == "<xsl:value-of select="@name"/>") {
    try {
      object->set<xsl:value-of select="@name"/>(Glib::VariantBase::cast_dynamic&lt; <xsl:call-template name="variant-type"/> &gt;(value).get());
      return true;
    } catch (...) {
      object->rethrow("set<xsl:value-of select="@name"/>");
      return false;
    }
  } else </xsl:for-each>{
    return false;// no property found
  }
}

<xsl:value-of select="$iface"/>_Service::<xsl:value-of select="$iface"/>_Service() 
 : interface_vtable(
    sigc::mem_fun(this, &amp;<xsl:value-of select="$iface"/>_Service::on_method_call),
    sigc::mem_fun(this, &amp;<xsl:value-of select="$iface"/>_Service::on_get_property),
    sigc::mem_fun(this, &amp;<xsl:value-of select="$iface"/>_Service::on_set_property))
{
}

<xsl:value-of select="$iface"/>_Service::~<xsl:value-of select="$iface"/>_Service() 
{
  unregister_self();
}

void <xsl:value-of select="$iface"/>_Service::rethrow(const char *method)
{
  throw;
}

void <xsl:value-of select="$iface"/>_Service::register_self(const Glib::RefPtr&lt;Gio::DBus::Connection&gt;&amp; connection, const Glib::ustring&amp; object_path) 
{
  static Glib::RefPtr&lt;Gio::DBus::NodeInfo&gt; introspection;
  if (!introspection)
    introspection = Gio::DBus::NodeInfo::create_for_xml(xml);

  guint id = connection->register_object(
    object_path,
    introspection->lookup_interface(),
    interface_vtable);

  exports.push_back(Export(connection, object_path, id));
}

void <xsl:value-of select="$iface"/>_Service::unregister_self() 
{
  for (unsigned i = 0; i &lt; exports.size(); ++i)
    exports[i].connection->unregister_object(exports[i].id);
  exports.clear();
}

<xsl:for-each select="method">
  <xsl:call-template name="method-service-type">
    <xsl:with-param name="namespace"><xsl:value-of select="$iface"/>_Service::</xsl:with-param>
  </xsl:call-template>
{
  throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "Method unimplemented.");
}

</xsl:for-each>

<xsl:for-each select="signal">
  <xsl:call-template name="signal-service-type">
    <xsl:with-param name="namespace"><xsl:value-of select="$iface"/>_Service::</xsl:with-param>
  </xsl:call-template>
{
  std::vector&lt;Glib::VariantBase&gt; data_vector;
  <xsl:for-each select="arg">
   <xsl:text>data_vector.push_back(</xsl:text>
   <xsl:call-template name="variant-type"/>
   <xsl:text>::create(</xsl:text>
   <xsl:value-of select="@name"/>));
  </xsl:for-each>for (unsigned i = 0; i &lt; exports.size(); ++i) {
    exports[i].connection->emit_signal(exports[i].object_path, 
      "<xsl:value-of select="$iface_full"/>", "<xsl:value-of select="@name"/>", "", 
      Glib::VariantContainerBase::create_tuple(data_vector));
  }
}

</xsl:for-each>
void <xsl:value-of select="$iface"/>_Service::report_property_change(const char* property, const Glib::VariantBase&amp; value)
{
  std::map&lt; Glib::ustring, Glib::VariantBase &gt; updated;
  std::vector&lt; Glib::ustring &gt; invalidated;
  std::vector&lt;Glib::VariantBase&gt; message_vector;
  updated[property] = value;
  message_vector.push_back(Glib::Variant&lt; Glib::ustring &gt;::create("<xsl:value-of select="$iface_full"/>"));
  message_vector.push_back(Glib::Variant&lt; std::map&lt; Glib::ustring, Glib::VariantBase &gt; &gt;::create(updated));
  message_vector.push_back(Glib::Variant&lt; std::vector&lt; Glib::ustring &gt; &gt;::create(invalidated));
  for (unsigned i = 0; i &lt; exports.size(); ++i) {
    exports[i].connection->emit_signal(exports[i].object_path, 
      "org.freedesktop.DBus.Properties", "PropertiesChanged", "", 
      Glib::VariantContainerBase::create_tuple(message_vector));
  }
}

<xsl:for-each select="property">
  <xsl:call-template name="prop-service-gettype">
    <xsl:with-param name="namespace"><xsl:value-of select="$iface"/>_Service::</xsl:with-param>
  </xsl:call-template>
{
  return <xsl:value-of select="@name"/>;
}

</xsl:for-each>

<xsl:for-each select="property">
  <xsl:call-template name="prop-service-settype">
    <xsl:with-param name="namespace"><xsl:value-of select="$iface"/>_Service::</xsl:with-param>
  </xsl:call-template>
{<xsl:if test="not(annotation[@name = 'org.freedesktop.DBus.Property.EmitsChangedSignal' and @value = 'false'])">
  if (<xsl:value-of select="@name"/> != val)
    report_property_change("<xsl:value-of select="@name"/>", <xsl:call-template name="variant-type"/>::create(val));</xsl:if>
  <xsl:text>
  </xsl:text><xsl:value-of select="@name"/> = val;
}

</xsl:for-each>

<xsl:for-each select="method">
  <xsl:call-template name="method-proxy-type">
    <xsl:with-param name="namespace"><xsl:value-of select="$iface"/>_Proxy::</xsl:with-param>
  </xsl:call-template>
{
  std::vector&lt;Glib::VariantBase&gt; query_vector;
  <xsl:for-each select="arg[@direction='in']">
   <xsl:text>query_vector.push_back(</xsl:text>
   <xsl:call-template name="variant-type"/>
   <xsl:text>::create(</xsl:text>
   <xsl:value-of select="@name"/>));
  </xsl:for-each>const Glib::VariantContainerBase&amp; query = Glib::VariantContainerBase::create_tuple(query_vector);
  const Glib::VariantContainerBase&amp; response = call_sync("<xsl:value-of select="@name"/>", query);<xsl:text/>
  <xsl:for-each select="arg[@direction='out']">
    <xsl:text>&#10;  </xsl:text>
    <xsl:call-template name="variant-type"/> ov_<xsl:value-of select="@name"/>
    <xsl:text>;</xsl:text>
  </xsl:for-each>
  <xsl:for-each select="arg[@direction='out']">
  response.get_child(ov_<xsl:value-of select="@name"/>, <xsl:value-of select="position()-1"/>);<xsl:text/>
  </xsl:for-each>
  <xsl:choose>
    <xsl:when test="count(arg[@direction='out']) = 1">
      <xsl:text>&#10;  return ov_</xsl:text>
      <xsl:value-of select="arg[@direction='out']/@name"/>
      <xsl:text>.get();</xsl:text>
    </xsl:when>
    <xsl:otherwise>
      <xsl:for-each select="arg[@direction='out']">
        <xsl:text>&#10;  </xsl:text>
        <xsl:value-of select="@name"/>
        <xsl:text> = ov_</xsl:text>
        <xsl:value-of select="@name"/>
        <xsl:text>.get();</xsl:text>
      </xsl:for-each>
    </xsl:otherwise>
  </xsl:choose>
}  

</xsl:for-each>void <xsl:value-of select="$iface"/>_Proxy::on_properties_changed(
  const MapChangedProperties&amp; changed_properties,
  const std::vector&lt; Glib::ustring &gt;&amp; invalidated_properties)
{
  Gio::DBus::Proxy::on_properties_changed(changed_properties, invalidated_properties);

  for (MapChangedProperties::const_iterator i = changed_properties.begin(); i != changed_properties.end(); ++i) {
    <xsl:for-each select="property[@access='read' or @access='readwrite']">if (i->first == "<xsl:value-of select="@name"/>") {
      <xsl:choose>
        <xsl:when test="annotation[@name = 'org.freedesktop.DBus.Property.EmitsChangedSignal' and @value = 'false']">
          <xsl:text>// EmitsChangedSignal = false</xsl:text>
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="@name"/>(Glib::VariantBase::cast_dynamic&lt; <xsl:call-template name="variant-type"/> &gt;(i->second).get());<xsl:text/>
        </xsl:otherwise>
      </xsl:choose>
    } else </xsl:for-each>{
      // noop
    }
  }
}

void <xsl:value-of select="$iface"/>_Proxy::on_signal(
  const Glib::ustring&amp; sender_name, 
  const Glib::ustring&amp; signal_name, 
  const Glib::VariantContainerBase&amp; parameters)
{
  Gio::DBus::Proxy::on_signal(sender_name, signal_name, parameters);

  <xsl:for-each select="signal">if (signal_name == "<xsl:value-of select="@name"/>") {<xsl:text/>
    <xsl:for-each select="arg">
      <xsl:text>&#10;    </xsl:text>
      <xsl:call-template name="variant-type"/> <xsl:value-of select="@name"/>;
      parameters.get_child(<xsl:value-of select="@name"/>, <xsl:value-of select="position()-1"/>);
    </xsl:for-each>
    <xsl:text>&#10;    </xsl:text>
    <xsl:value-of select="@name"/>(<xsl:for-each select="arg">
      <xsl:if test="position()>1">, </xsl:if>
      <xsl:value-of select="@name"/>.get()</xsl:for-each>);
  } else </xsl:for-each>{
    // noop
  }
}

void <xsl:value-of select="$iface"/>_Proxy::fetch_property(const char* name, Glib::VariantBase&amp; val) const
{
  std::vector&lt; Glib::VariantBase &gt; params;
  params.push_back(Glib::Variant&lt; Glib::ustring &gt;::create("<xsl:value-of select="$iface_full"/>"));
  params.push_back(Glib::Variant&lt; Glib::ustring &gt;::create(name)); 
  Glib::RefPtr&lt;Gio::DBus::Connection&gt; connection = 
    Glib::RefPtr&lt;Gio::DBus::Connection&gt;::cast_const(get_connection());
  connection->reference(); // work around get_connection does not increase reference bug
  const Glib::VariantContainerBase&amp; result =
    connection->call_sync(get_object_path(), "org.freedesktop.DBus.Properties", "Get", 
      Glib::VariantContainerBase::create_tuple(params), get_name());
  Glib::Variant&lt;Glib::VariantBase&gt; variant;
  result.get_child(variant, 0);
  variant.get(val);
}

<xsl:for-each select="property[@access='read' or @access='readwrite']">
  <xsl:call-template name="prop-proxy-gettype">
    <xsl:with-param name="namespace"><xsl:value-of select="$iface"/>_Proxy::</xsl:with-param>
  </xsl:call-template>
{
  <xsl:call-template name="variant-type"/> value;<xsl:text/>
  <xsl:choose>
    <xsl:when test="annotation[@name = 'org.freedesktop.DBus.Property.EmitsChangedSignal' and @value = 'false']">
  fetch_property("<xsl:value-of select="@name"/>", value);<xsl:text/>
    </xsl:when>
    <xsl:otherwise>
  get_cached_property(value, "<xsl:value-of select="@name"/>");<xsl:text/>
    </xsl:otherwise>
  </xsl:choose>
  return value.get();
}

</xsl:for-each>void <xsl:value-of select="$iface"/>_Proxy::update_property(const char* name, const Glib::VariantBase&amp; val)
{
  std::vector&lt; Glib::VariantBase &gt; params;
  params.push_back(Glib::Variant&lt; Glib::ustring &gt;::create("<xsl:value-of select="$iface_full"/>"));
  params.push_back(Glib::Variant&lt; Glib::ustring &gt;::create(name)); 
  params.push_back(Glib::Variant&lt; Glib::VariantBase &gt;::create(val));
  Glib::RefPtr&lt;Gio::DBus::Connection&gt; connection = get_connection();
  connection->reference(); // work around get_connection does not increase reference bug
  connection->call_sync(get_object_path(), "org.freedesktop.DBus.Properties", "Set", 
    Glib::VariantContainerBase::create_tuple(params), get_name());
}

<xsl:for-each select="property[@access='write' or @access='readwrite']">
  <xsl:call-template name="prop-proxy-settype">
    <xsl:with-param name="namespace"><xsl:value-of select="$iface"/>_Proxy::</xsl:with-param>
  </xsl:call-template>
{
  update_property("<xsl:value-of select="@name"/>", <xsl:call-template name="variant-type"/>::create(val));
}

</xsl:for-each>

<xsl:value-of select="$iface"/>_Proxy::<xsl:value-of select="$iface"/>_Proxy(
  const Glib::RefPtr&lt;Gio::DBus::Connection&gt;&amp; connection,
  const Glib::ustring&amp; name,   
  const Glib::ustring&amp; object_path,
  const Glib::ustring&amp; interface_name,
  const Gio::SlotAsyncReady&amp; slot,
  const Glib::RefPtr&lt;Gio::Cancellable&gt;&amp; cancellable,
  const Glib::RefPtr&lt;Gio::DBus::InterfaceInfo&gt;&amp; info,
  Gio::DBus::ProxyFlags flags)
: Proxy(connection, name, object_path, interface_name, slot, cancellable, info, flags)
{
}

<xsl:value-of select="$iface"/>_Proxy::<xsl:value-of select="$iface"/>_Proxy(
  const Glib::RefPtr&lt;Gio::DBus::Connection&gt;&amp; connection,
  const Glib::ustring&amp; name,
  const Glib::ustring&amp; object_path,
  const Glib::ustring&amp; interface_name,
  const Gio::SlotAsyncReady&amp; slot,
  const Glib::RefPtr&lt;Gio::DBus::InterfaceInfo&gt;&amp; info,
  Gio::DBus::ProxyFlags flags)
: Proxy(connection, name, object_path, interface_name, slot, info, flags)
{
}

<xsl:value-of select="$iface"/>_Proxy::<xsl:value-of select="$iface"/>_Proxy(
  const Glib::RefPtr&lt;Gio::DBus::Connection&gt;&amp; connection,
  const Glib::ustring&amp; name, 
  const Glib::ustring&amp; object_path,
  const Glib::ustring&amp; interface_name,
  const Glib::RefPtr&lt;Gio::Cancellable&gt;&amp; cancellable,
  const Glib::RefPtr&lt;Gio::DBus::InterfaceInfo&gt;&amp; info,
  Gio::DBus::ProxyFlags flags)
: Proxy(connection, name, object_path, interface_name, cancellable, info, flags)
{
}

<xsl:value-of select="$iface"/>_Proxy::<xsl:value-of select="$iface"/>_Proxy(
  const Glib::RefPtr&lt;Gio::DBus::Connection&gt;&amp; connection,
  const Glib::ustring&amp; name,
  const Glib::ustring&amp; object_path,
  const Glib::ustring&amp; interface_name,
  const Glib::RefPtr&lt;Gio::DBus::InterfaceInfo&gt;&amp; info,
  Gio::DBus::ProxyFlags flags)
: Proxy(connection, name, object_path, interface_name, info, flags)
{
}

<xsl:value-of select="$iface"/>_Proxy::<xsl:value-of select="$iface"/>_Proxy(
  Gio::DBus::BusType bus_type,
  const Glib::ustring&amp; name,  
  const Glib::ustring&amp; object_path,
  const Glib::ustring&amp; interface_name,
  const Gio::SlotAsyncReady&amp; slot,
  const Glib::RefPtr&lt;Gio::Cancellable&gt;&amp; cancellable,
  const Glib::RefPtr&lt;Gio::DBus::InterfaceInfo&gt;&amp; info,
  Gio::DBus::ProxyFlags flags)
: Proxy(bus_type, name, object_path, interface_name, slot, cancellable, info, flags)
{
}

<xsl:value-of select="$iface"/>_Proxy::<xsl:value-of select="$iface"/>_Proxy(
  Gio::DBus::BusType bus_type,
  const Glib::ustring&amp; name,  
  const Glib::ustring&amp; object_path,
  const Glib::ustring&amp; interface_name,
  const Gio::SlotAsyncReady&amp; slot,
  const Glib::RefPtr&lt;Gio::DBus::InterfaceInfo&gt;&amp; info,
  Gio::DBus::ProxyFlags flags)
: Proxy(bus_type, name, object_path, interface_name, slot, info, flags)
{
}

<xsl:value-of select="$iface"/>_Proxy::<xsl:value-of select="$iface"/>_Proxy(
  Gio::DBus::BusType bus_type,
  const Glib::ustring&amp; name,  
  const Glib::ustring&amp; object_path,
  const Glib::ustring&amp; interface_name,
  const Glib::RefPtr&lt;Gio::Cancellable&gt;&amp; cancellable,
  const Glib::RefPtr&lt;Gio::DBus::InterfaceInfo&gt;&amp; info,
  Gio::DBus::ProxyFlags flags)
: Proxy(bus_type, name, object_path, interface_name, cancellable, info, flags)
{
}

<xsl:value-of select="$iface"/>_Proxy::<xsl:value-of select="$iface"/>_Proxy(
  Gio::DBus::BusType bus_type,
  const Glib::ustring&amp; name,  
  const Glib::ustring&amp; object_path,
  const Glib::ustring&amp; interface_name,
  const Glib::RefPtr&lt;Gio::DBus::InterfaceInfo&gt;&amp; info,
  Gio::DBus::ProxyFlags flags)
: Proxy(bus_type, name, object_path, interface_name, info, flags)
{
}

void <xsl:value-of select="$iface"/>_Proxy::create(
  const Glib::RefPtr&lt;Gio::DBus::Connection&gt;&amp; connection,
  const Glib::ustring&amp; name,
  const Glib::ustring&amp; object_path,
  const Gio::SlotAsyncReady&amp; slot,
  const Glib::RefPtr&lt;Gio::Cancellable&gt;&amp; cancellable,
  const Glib::RefPtr&lt;Gio::DBus::InterfaceInfo&gt;&amp; info,
  Gio::DBus::ProxyFlags flags)
{
  <xsl:value-of select="$iface"/>_Proxy(connection, name, object_path, "<xsl:value-of select="$iface_full"/>", slot,
    cancellable, info, flags);
}

void <xsl:value-of select="$iface"/>_Proxy::create(
  const Glib::RefPtr&lt;Gio::DBus::Connection&gt;&amp; connection,
  const Glib::ustring&amp; name,
  const Glib::ustring&amp; object_path,
  const Gio::SlotAsyncReady&amp; slot,
  const Glib::RefPtr&lt;Gio::DBus::InterfaceInfo&gt;&amp; info,
  Gio::DBus::ProxyFlags flags)
{
  <xsl:value-of select="$iface"/>_Proxy(connection, name, object_path, "<xsl:value-of select="$iface_full"/>", slot, info, flags);
}

Glib::RefPtr&lt;<xsl:value-of select="$iface"/>_Proxy&gt; <xsl:value-of select="$iface"/>_Proxy::create_sync(
  const Glib::RefPtr&lt;Gio::DBus::Connection&gt;&amp; connection,
  const Glib::ustring&amp; name,
  const Glib::ustring&amp; object_path,
  const Glib::RefPtr&lt;Gio::Cancellable&gt;&amp; cancellable,
  const Glib::RefPtr&lt;Gio::DBus::InterfaceInfo&gt;&amp; info,
  Gio::DBus::ProxyFlags flags)
{
  return Glib::RefPtr&lt;<xsl:value-of select="$iface"/>_Proxy&gt;(new <xsl:value-of select="$iface"/>_Proxy(connection, name,
    object_path, "<xsl:value-of select="$iface_full"/>", cancellable, info, flags));
}

Glib::RefPtr&lt;<xsl:value-of select="$iface"/>_Proxy&gt; <xsl:value-of select="$iface"/>_Proxy::create_sync(
  const Glib::RefPtr&lt;Gio::DBus::Connection&gt;&amp; connection,
  const Glib::ustring&amp; name,
  const Glib::ustring&amp; object_path,
  const Glib::RefPtr&lt;Gio::DBus::InterfaceInfo&gt;&amp; info,
  Gio::DBus::ProxyFlags flags)
{
  return Glib::RefPtr&lt;<xsl:value-of select="$iface"/>_Proxy&gt;(new <xsl:value-of select="$iface"/>_Proxy(connection, name,
    object_path, "<xsl:value-of select="$iface_full"/>", info, flags));
}

void <xsl:value-of select="$iface"/>_Proxy::create_for_bus(
  Gio::DBus::BusType bus_type,
  const Glib::ustring&amp; name,
  const Glib::ustring&amp; object_path,
  const Gio::SlotAsyncReady&amp; slot,
  const Glib::RefPtr&lt;Gio::Cancellable&gt;&amp; cancellable,
  const Glib::RefPtr&lt;Gio::DBus::InterfaceInfo&gt;&amp; info,
  Gio::DBus::ProxyFlags flags)
{
  <xsl:value-of select="$iface"/>_Proxy(bus_type, name, object_path, "<xsl:value-of select="$iface_full"/>", slot, cancellable,
    info, flags);
}

void <xsl:value-of select="$iface"/>_Proxy::create_for_bus(
  Gio::DBus::BusType bus_type,
  const Glib::ustring&amp; name,
  const Glib::ustring&amp; object_path,
  const Gio::SlotAsyncReady&amp; slot,
  const Glib::RefPtr&lt;Gio::DBus::InterfaceInfo&gt;&amp; info,
  Gio::DBus::ProxyFlags flags)
{
  <xsl:value-of select="$iface"/>_Proxy(bus_type, name, object_path, "<xsl:value-of select="$iface_full"/>", slot, info, flags);
}

Glib::RefPtr&lt;<xsl:value-of select="$iface"/>_Proxy&gt; <xsl:value-of select="$iface"/>_Proxy::create_for_bus_sync(
  Gio::DBus::BusType bus_type,
  const Glib::ustring&amp; name,
  const Glib::ustring&amp; object_path,
  const Glib::RefPtr&lt;Gio::Cancellable&gt;&amp; cancellable,
  const Glib::RefPtr&lt;Gio::DBus::InterfaceInfo&gt;&amp; info,
  Gio::DBus::ProxyFlags flags)
{
  return Glib::RefPtr&lt;<xsl:value-of select="$iface"/>_Proxy&gt;(new <xsl:value-of select="$iface"/>_Proxy(bus_type, name,
    object_path, "<xsl:value-of select="$iface_full"/>", cancellable, info, flags));
}

Glib::RefPtr&lt;<xsl:value-of select="$iface"/>_Proxy&gt; <xsl:value-of select="$iface"/>_Proxy::create_for_bus_sync(
  Gio::DBus::BusType bus_type,
  const Glib::ustring&amp; name,
  const Glib::ustring&amp; object_path,
  const Glib::RefPtr&lt;Gio::DBus::InterfaceInfo&gt;&amp; info,
  Gio::DBus::ProxyFlags flags)
{
  return Glib::RefPtr&lt;<xsl:value-of select="$iface"/>_Proxy&gt;(new <xsl:value-of select="$iface"/>_Proxy(bus_type, name,
    object_path, "<xsl:value-of select="$iface_full"/>", info, flags));
}

static Glib::RefPtr&lt;<xsl:value-of select="$iface"/>_Proxy&gt; wrap_<xsl:value-of select="$iface"/>(GDBusProxy* object)
{
  return Glib::RefPtr&lt;<xsl:value-of select="$iface"/>_Proxy&gt;(dynamic_cast&lt;<xsl:value-of select="$iface"/>_Proxy*&gt;(Glib::wrap_auto((GObject*)object, false)));
}

Glib::RefPtr&lt;<xsl:value-of select="$iface"/>_Proxy&gt; <xsl:value-of select="$iface"/>_Proxy::create_finish(const Glib::RefPtr&lt;Gio::AsyncResult&gt;&amp; res)
{
  GError* gerror = 0;  
  Glib::RefPtr&lt;<xsl:value-of select="$iface"/>_Proxy&gt; retvalue = wrap_<xsl:value-of select="$iface"/>(g_dbus_proxy_new_finish(Glib::unwrap(res), &amp;(gerror)));
  if (gerror)
    ::Glib::Error::throw_exception(gerror);
  return retvalue;
} 

Glib::RefPtr&lt;<xsl:value-of select="$iface"/>_Proxy&gt; <xsl:value-of select="$iface"/>_Proxy::create_for_bus_finish(const Glib::RefPtr&lt;Gio::AsyncResult&gt;&amp; res)
{
  GError* gerror = 0;
  Glib::RefPtr&lt;<xsl:value-of select="$iface"/>_Proxy&gt; retvalue = wrap_<xsl:value-of select="$iface"/>(g_dbus_proxy_new_for_bus_finish(Glib::unwrap(res), &amp;(gerror)));
  if(gerror)
    ::Glib::Error::throw_exception(gerror);
  return retvalue;
}
</xsl:for-each>
}
</xsl:template>

</xsl:stylesheet>
