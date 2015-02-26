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
static Glib::ustring xml_<xsl:value-of select="$iface"/> = 
  "&lt;node&gt;<xsl:apply-templates select="." mode="escape"/>&lt;/node&gt;";

static std::map&lt;Glib::ustring, Glib::RefPtr&lt;<xsl:value-of select="$iface"/>_Service&gt; &gt; registry_<xsl:value-of select="$iface"/>;

static void on_method_call_<xsl:value-of select="$iface"/>(
  const Glib::RefPtr&lt;Gio::DBus::Connection&gt;&amp; /* connection */,
  const Glib::ustring&amp; /* sender */, const Glib::ustring&amp; object_path,
  const Glib::ustring&amp; /* interface_name */, const Glib::ustring&amp; method_name,
  const Glib::VariantContainerBase&amp; parameters,
  const Glib::RefPtr&lt;Gio::DBus::MethodInvocation&gt;&amp; invocation)
{
  Glib::RefPtr&lt;<xsl:value-of select="$iface"/>_Service&gt; object = registry_<xsl:value-of select="$iface"/>[object_path];
  if (!object) {
    Gio::DBus::Error error(Gio::DBus::Error::UNKNOWN_OBJECT, "Non-existant object.");
    invocation->return_error(error);
    return;
  }
  
  <xsl:for-each select="method">if (method_name == "<xsl:value-of select="@name"/>") {<xsl:for-each select="arg[@direction='in']">
    Glib::Variant&lt; <xsl:apply-templates mode="iface-type" select="."/> &gt; <xsl:value-of select="@name"/>;</xsl:for-each>
    <xsl:for-each select="arg[@direction='out']">
      <xsl:text>
    </xsl:text>
      <xsl:apply-templates mode="iface-type" select="."/>
      <xsl:text> </xsl:text>
      <xsl:value-of select="@name"/>
      <xsl:text>;</xsl:text>
    </xsl:for-each>
    <xsl:for-each select="arg[@direction='in']">
    parameters.get_child(<xsl:value-of select="@name"/>, <xsl:value-of select="position()-1"/>);</xsl:for-each>
    try {
      object-><xsl:value-of select="@name"/>
    <xsl:text>(</xsl:text>
    <xsl:for-each select="arg">
      <xsl:if test="position()>1">, </xsl:if>
      <xsl:value-of select="@name"/>
      <xsl:if test="@direction='in'">.get()</xsl:if>
    </xsl:for-each>
    <xsl:text>);</xsl:text>
      std::vector&lt;Glib::VariantBase&gt; response_vector;
      <xsl:for-each select="arg[@direction='out']">
        <xsl:text>response_vector.push_back(Glib::Variant&lt; </xsl:text>
        <xsl:apply-templates mode="iface-type" select="."/>
        <xsl:text> &gt;::create(</xsl:text>
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

void on_get_property_<xsl:value-of select="$iface"/>(
  Glib::VariantBase&amp; property,
  const Glib::RefPtr&lt;Gio::DBus::Connection&gt;&amp; /* connection */,
  const Glib::ustring&amp; /* sender */, const Glib::ustring&amp; object_path,
  const Glib::ustring&amp; /*interface_name */, const Glib::ustring&amp; property_name)
{
  Glib::RefPtr&lt;<xsl:value-of select="$iface"/>_Service&gt; object = registry_<xsl:value-of select="$iface"/>[object_path];
  if (!object) return;

  <xsl:for-each select="property[@access='read' or @access='readwrite']">if (property_name == "<xsl:value-of select="@name"/>") {
    property = Glib::Variant&lt; <xsl:apply-templates mode="iface-type" select="."/> &gt;::create(object->get<xsl:value-of select="@name"/>());
  } else </xsl:for-each>{
    // no property found
  }
}

bool on_set_property_<xsl:value-of select="$iface"/>(
  const Glib::RefPtr&lt;Gio::DBus::Connection&gt;&amp; /* connection */, 
  const Glib::ustring&amp; /* sender */, const Glib::ustring&amp; object_path, 
  const Glib::ustring&amp; /* interface_name */, const Glib::ustring&amp; property_name, 
  const Glib::VariantBase&amp; value) 
{
  Glib::RefPtr&lt;<xsl:value-of select="$iface"/>_Service&gt; object = registry_<xsl:value-of select="$iface"/>[object_path];
  if (!object) return false;

  <xsl:for-each select="property[@access='write' or @access='readwrite']">if (property_name == "<xsl:value-of select="@name"/>") {
    object->set<xsl:value-of select="@name"/>(Glib::VariantBase::cast_dynamic&lt; Glib::Variant&lt; <xsl:apply-templates mode="iface-type" select="."/> &gt; &gt;(value).get());
    return true;
  } else </xsl:for-each>{
    return false;// no property found
  }
}

static const Gio::DBus::InterfaceVTable interface_vtable_<xsl:value-of select="$iface"/>(
  sigc::ptr_fun(&amp;on_method_call_<xsl:value-of select="$iface"/>),
  sigc::ptr_fun(&amp;on_get_property_<xsl:value-of select="$iface"/>),
  sigc::ptr_fun(&amp;on_set_property_<xsl:value-of select="$iface"/>));

<xsl:value-of select="$iface"/>_Service::<xsl:value-of select="$iface"/>_Service(const Glib::ustring&amp; object_path_)
 : object_path(object_path_)
{
}

<xsl:value-of select="$iface"/>_Service::~<xsl:value-of select="$iface"/>_Service() 
{
  unregister_self();
}

void <xsl:value-of select="$iface"/>_Service::register_self(const Glib::RefPtr&lt;Gio::DBus::Connection&gt;&amp; connection_) 
{
  static Glib::RefPtr&lt;Gio::DBus::NodeInfo&gt; introspection;
  if (!introspection)
    introspection = Gio::DBus::NodeInfo::create_for_xml(xml_<xsl:value-of select="$iface"/>);

  if (connection_ == connection) return;

  guint id_ = connection_->register_object(
    object_path,
    introspection->lookup_interface(),
    interface_vtable_<xsl:value-of select="$iface"/>);

  this->reference();
  unregister_self();

  connection = connection_;
  id = id_;

  registry_<xsl:value-of select="$iface"/>[object_path] = Glib::RefPtr&lt;<xsl:value-of select="$iface"/>_Service&gt;(this);
}

void <xsl:value-of select="$iface"/>_Service::unregister_self() 
{
  if (!connection) return;
  connection->unregister_object(id);
  connection.reset();
  registry_<xsl:value-of select="$iface"/>.erase(object_path);
  object_path.clear();
}

void <xsl:value-of select="$iface"/>_Service::setObjectPath(const Glib::ustring&amp; object_path_)
{
  if (object_path == object_path_) return;
  Glib::RefPtr&lt;Gio::DBus::Connection&gt; connection_ = connection;
  unregister_self();
  if (connection_) register_self(connection_);
}
<xsl:for-each select="method">
void <xsl:value-of select="$iface"/>_Service::<xsl:value-of select="@name"/>
  <xsl:text>(</xsl:text>
  <xsl:for-each select="arg">
    <xsl:if test="position()>1">, </xsl:if>
  <xsl:text>
  </xsl:text>
  <xsl:if test="@direction='in'">const </xsl:if><xsl:apply-templates mode="iface-type" select="."/>&amp; /* <xsl:value-of select="@name"/> */</xsl:for-each>
<xsl:text>)
{</xsl:text>
  throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "Method unimplemented.");
}
</xsl:for-each>

<xsl:for-each select="signal">
void <xsl:value-of select="$iface"/>_Service::<xsl:value-of select="@name"/>
  <xsl:text>(</xsl:text>
  <xsl:for-each select="arg">
    <xsl:if test="position()>1">, </xsl:if>
  <xsl:text>
  const </xsl:text>
  <xsl:apply-templates mode="iface-type" select="."/>&amp; <xsl:value-of select="@name"/>
  </xsl:for-each>
<xsl:text>)
{</xsl:text>
  std::vector&lt;Glib::VariantBase&gt; data_vector;
  <xsl:for-each select="arg">
   <xsl:text>data_vector.push_back(Glib::Variant&lt; </xsl:text>
   <xsl:apply-templates mode="iface-type" select="."/>
   <xsl:text> &gt;::create(</xsl:text>
   <xsl:value-of select="@name"/>));
  </xsl:for-each>connection->emit_signal(object_path, "<xsl:value-of select="$iface_full"/>", "<xsl:value-of select="@name"/>", "", 
    Glib::VariantContainerBase::create_tuple(data_vector));
}
</xsl:for-each>
void <xsl:value-of select="$iface"/>_Service::report_property_change(const char* property, const Glib::VariantBase&amp; value)
{
  std::map&lt; Glib::ustring, Glib::VariantBase &gt; updated;
  std::vector&lt; Glib::ustring &gt; invalidated;
  std::vector&lt;Glib::VariantBase&gt; message_vector;
  updated[property] = value;
  message_vector.push_back(Glib::Variant&lt;Glib::ustring&gt;::create("<xsl:value-of select="$iface_full"/>"));
  message_vector.push_back(Glib::Variant&lt; std::map&lt; Glib::ustring, Glib::VariantBase &gt; &gt;::create(updated));
  message_vector.push_back(Glib::Variant&lt; std::vector&lt; Glib::ustring &gt; &gt;::create(invalidated));
  connection->emit_signal(object_path, "org.freedesktop.DBus.Properties", "PropertiesChanged", "", 
    Glib::VariantContainerBase::create_tuple(message_vector));
}
<xsl:for-each select="property">
const <xsl:apply-templates mode="iface-type" select="."/>&amp; <xsl:value-of select="$iface"/>_Service::get<xsl:value-of select="@name"/>() const
{
  return <xsl:value-of select="@name"/>;
}

void <xsl:value-of select="$iface"/>_Service::set<xsl:value-of select="@name"/>(const <xsl:apply-templates mode="iface-type" select="."/>&amp; val)
{
  if (connection &amp;&amp; <xsl:value-of select="@name"/> != val)
    report_property_change("<xsl:value-of select="@name"/>", Glib::Variant&lt; <xsl:apply-templates mode="iface-type" select="."/> &gt;::create(val));
  <xsl:value-of select="@name"/> = val;
}
</xsl:for-each>

<xsl:for-each select="method">
void <xsl:value-of select="$iface"/>_Proxy::<xsl:value-of select="@name"/>
  <xsl:text>(</xsl:text>
  <xsl:for-each select="arg">
    <xsl:if test="position()>1">, </xsl:if>
  <xsl:text>
  </xsl:text>
  <xsl:if test="@direction='in'">const </xsl:if><xsl:apply-templates mode="iface-type" select="."/>&amp; <xsl:value-of select="@name"/>
  </xsl:for-each>
<xsl:text>)
{</xsl:text>
  std::vector&lt;Glib::VariantBase&gt; query_vector;
  <xsl:for-each select="arg[@direction='in']">
   <xsl:text>query_vector.push_back(Glib::Variant&lt; </xsl:text>
   <xsl:apply-templates mode="iface-type" select="."/>
   <xsl:text> &gt;::create(</xsl:text>
   <xsl:value-of select="@name"/>));
  </xsl:for-each>const Glib::VariantContainerBase&amp; query = Glib::VariantContainerBase::create_tuple(query_vector);
  const Glib::VariantContainerBase&amp; response = call_sync("<xsl:value-of select="@name"/>", query);<xsl:for-each select="arg[@direction='out']">
  Glib::Variant&lt; <xsl:apply-templates mode="iface-type" select="."/> &gt; ov_<xsl:value-of select="@name"/>;
  response.get_child(ov_<xsl:value-of select="@name"/>, <xsl:value-of select="position()-1"/>);
  <xsl:value-of select="@name"/> = ov_<xsl:value-of select="@name"/>.get();</xsl:for-each>
}  
</xsl:for-each>

void <xsl:value-of select="$iface"/>_Proxy::on_properties_changed(
  const MapChangedProperties&amp; changed_properties,
  const std::vector&lt; Glib::ustring &gt;&amp; invalidated_properties)
{
  Gio::DBus::Proxy::on_properties_changed(changed_properties, invalidated_properties);

  for (MapChangedProperties::const_iterator i = changed_properties.begin(); i != changed_properties.end(); ++i) {
    <xsl:for-each select="property[@access='read' or @access='readwrite']">if (i->first == "<xsl:value-of select="@name"/>") {
      <xsl:value-of select="@name"/>(Glib::VariantBase::cast_dynamic&lt; Glib::Variant&lt; <xsl:apply-templates mode="iface-type" select="."/> &gt; &gt;(i->second).get());
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

  <xsl:for-each select="signal">if (signal_name == "<xsl:value-of select="@name"/>") {<xsl:for-each select="arg">
    Glib::Variant&lt; <xsl:apply-templates mode="iface-type" select="."/> &gt; <xsl:value-of select="@name"/>;
    parameters.get_child(<xsl:value-of select="@name"/>, <xsl:value-of select="position()-1"/>);</xsl:for-each>
    <xsl:text>
    </xsl:text>
    <xsl:value-of select="@name"/>(<xsl:for-each select="arg">
      <xsl:if test="position()>1">, </xsl:if>
      <xsl:value-of select="@name"/>.get()</xsl:for-each>);
  } else </xsl:for-each>{
    // noop
  }
}
<xsl:for-each select="property[@access='read' or @access='readwrite']">
  <xsl:text>
</xsl:text>
  <xsl:apply-templates mode="iface-type" select="."/>
  <xsl:text> </xsl:text>
  <xsl:value-of select="$iface"/>_Proxy::get<xsl:value-of select="@name"/>() const
{
  Glib::Variant&lt; <xsl:apply-templates mode="iface-type" select="."/> &gt; value;
  get_cached_property(value, "<xsl:value-of select="@name"/>");
  return value.get();
}
</xsl:for-each>
void <xsl:value-of select="$iface"/>_Proxy::update_property(const char* name, const Glib::VariantBase&amp; val)
{
  std::vector&lt; Glib::VariantBase &gt; params;
  params.push_back(Glib::Variant&lt; Glib::ustring &gt;::create("<xsl:value-of select="$iface_full"/>"));
  params.push_back(Glib::Variant&lt; Glib::ustring &gt;::create(name)); 
  params.push_back(Glib::Variant&lt; Glib::VariantBase &gt;::create(val));
  get_connection()->call_sync(get_object_path(), "org.freedesktop.DBus.Properties", "Set", 
    Glib::VariantContainerBase::create_tuple(params), get_name());
}
<xsl:for-each select="property[@access='write' or @access='readwrite']">
void <xsl:value-of select="$iface"/>_Proxy::set<xsl:value-of select="@name"/>(const <xsl:apply-templates mode="iface-type" select="."/>&amp; val)
{
  update_property("<xsl:value-of select="@name"/>", Glib::Variant&lt; <xsl:apply-templates mode="iface-type" select="."/> &gt;::create(val));
}
</xsl:for-each>

<xsl:text>
</xsl:text>

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
