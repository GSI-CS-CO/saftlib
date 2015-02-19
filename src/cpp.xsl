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
  
  <xsl:for-each select="method">if (method_name == "<xsl:value-of select="@name"/>") {
    // Input parameters<xsl:for-each select="arg[@direction='in']">
    Glib::Variant&lt; <xsl:apply-templates mode="iface-type" select="."/> &gt; iv_<xsl:value-of select="@name"/>;
    parameters.get_child(iv_<xsl:value-of select="@name"/>, <xsl:value-of select="position()-1"/>);
    const <xsl:apply-templates mode="iface-type" select="."/> ip_<xsl:value-of select="@name"/> = iv_<xsl:value-of select="@name"/>.get();</xsl:for-each>
    // Output parameters
    <xsl:for-each select="arg[@direction='out']">
      <xsl:text></xsl:text>
      <xsl:apply-templates mode="iface-type" select="."/>
      <xsl:text> op_</xsl:text>
      <xsl:value-of select="@name"/>
      <xsl:text>;
    </xsl:text>
    </xsl:for-each>
    try {
      object-><xsl:value-of select="@name"/>
    <xsl:text>(</xsl:text>
    <xsl:for-each select="arg">
      <xsl:if test="position()>1">, </xsl:if>
      <xsl:if test="@direction='in'">ip_</xsl:if>
      <xsl:if test="@direction='out'">op_</xsl:if>
      <xsl:value-of select="@name"/>
    </xsl:for-each>
    <xsl:text>);</xsl:text>
      std::vector&lt;Glib::VariantBase&gt; response_vector;
      <xsl:for-each select="arg[@direction='out']">
        <xsl:text>response_vector.push_back(Glib::Variant&lt; </xsl:text>
        <xsl:apply-templates mode="iface-type" select="."/>
        <xsl:text> &gt;::create(op_</xsl:text>
        <xsl:value-of select="@name"/>));
      </xsl:for-each>Glib::VariantContainerBase response = Glib::VariantContainerBase::create_tuple(response_vector);
      invocation->return_value(response);
    } catch (const Gio::DBus::Error&amp; error) {
      invocation->return_error(error);
    }
  } else </xsl:for-each>{
    Gio::DBus::Error error(Gio::DBus::Error::UNKNOWN_METHOD, "No such method.");
    invocation->return_error(error);
  }
}

void on_get_property_ECA(
  Glib::VariantBase&amp; property,
  const Glib::RefPtr&lt;Gio::DBus::Connection&gt;&amp; connection,
  const Glib::ustring&amp; /* sender */, const Glib::ustring&amp; object_path,
  const Glib::ustring&amp; /*interface_name */, const Glib::ustring&amp; property_name)
{
  property = Glib::Variant&lt;Glib::ustring&gt;::create("hello world");
}

bool on_set_property_ECA(
  const Glib::RefPtr&lt;Gio::DBus::Connection&gt;&amp; connection, 
  const Glib::ustring&amp; /* sender */, const Glib::ustring&amp; object_path, 
  const Glib::ustring&amp; /* interface_name */, const Glib::ustring&amp; property_name, 
  const Glib::VariantBase&amp; value) 
{
  return false; 
}

static const Gio::DBus::InterfaceVTable interface_vtable_<xsl:value-of select="$iface"/>(
  sigc::ptr_fun(&amp;on_method_call_<xsl:value-of select="$iface"/>),
  sigc::ptr_fun(&amp;on_get_property_<xsl:value-of select="$iface"/>),
  sigc::ptr_fun(&amp;on_set_property_<xsl:value-of select="$iface"/>));

<xsl:value-of select="$iface"/>_Service::~<xsl:value-of select="$iface"/>_Service() {
}

guint <xsl:value-of select="$iface"/>_Service::register_object(const Glib::RefPtr&lt;Gio::DBus::Connection&gt;&amp; connection, const Glib::ustring&amp; object_path) {
  static Glib::RefPtr&lt;Gio::DBus::NodeInfo&gt; introspection;
  if (!introspection)
    introspection = Gio::DBus::NodeInfo::create_for_xml(xml_<xsl:value-of select="$iface"/>);

  guint out = connection->register_object(
    object_path,
    introspection->lookup_interface(),
    interface_vtable_<xsl:value-of select="$iface"/>);
  this->reference();
  registry_<xsl:value-of select="$iface"/>[object_path] = Glib::RefPtr&lt;<xsl:value-of select="$iface"/>_Service&gt;(this);
  return out;
}
<xsl:for-each select="method">
void <xsl:value-of select="$iface"/>_Service::<xsl:value-of select="@name"/>
  <xsl:text>(</xsl:text>
  <xsl:for-each select="arg">
    <xsl:if test="position()>1">, </xsl:if>
  <xsl:text>
  </xsl:text>
  <xsl:if test="@direction='in'">const </xsl:if><xsl:apply-templates mode="iface-type" select="."/>&amp; <xsl:value-of select="@name"/>
  </xsl:for-each>
<xsl:text>) {</xsl:text>
  throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "Method unimplemented.");
}
</xsl:for-each>

</xsl:for-each>
}

</xsl:template>

</xsl:stylesheet>
