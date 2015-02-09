#include <dbus/dbus-glib-bindings.h>

/*
typedef struct
{
  DBusGConnection *connection;
} ServerObjectClass;

class_init(ServerObjectClass *klass)
{
  GError *error = NULL;
  
  // Init the DBus connection
  klass->connection = dbus_g_bus_get(DBUS_BUS_SESSION, &error);
  if (klass->connection == NULL)
  {
    g_warning("Unable to connect to dbus: %s\n", error->message);
    g_error_free(error);
    return;
  }
  
  dbus_g_object_type_install_info(OBJECT_TYPE_SERVER, &dbus_glib__object_info);
}
*/

int main() {
//  DBusGConnection *connection;
  
//  dbus_g_connection_register_g_object(conjection, 
  return 0;
}
