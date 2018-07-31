#include "Connection.h"
#include "Socket.h"
#include "core.h"

#include <iostream>
#include <sstream>
#include <iomanip>


namespace saftbus
{

// Connection::SaftbusObject::SaftbusObject()
// 	: object_path("")
// 	, interface_info()
// 	, vtable(InterfaceVTable::SlotInterfaceMethodCall())
// {}

// Connection::SaftbusObject::SaftbusObject(const std::string &object, const Glib::RefPtr<InterfaceInfo> &info, const InterfaceVTable &table)
// 	: object_path(object)
// 	, interface_info(info)
// 	, vtable(table)
// {}

// Connection::SaftbusObject::SaftbusObject(const SaftbusObject &rhs)
// 	: object_path(rhs.object_path)
// 	, interface_info(rhs.interface_info)
// 	, vtable(rhs.vtable)
// {}

Connection::Connection(int number_of_sockets, const std::string& base_name)
	: _client_id(1)
{
	for (int i = 0; i < number_of_sockets; ++i)
	{
		std::ostringstream name_out;
		name_out << base_name << std::setw(2) << std::setfill('0') << i;
		_sockets.push_back(std::shared_ptr<Socket>(new Socket(name_out.str(), this)));
	}
}

guint Connection::register_object (const Glib::ustring& object_path, const Glib::RefPtr< InterfaceInfo >& interface_info, const InterfaceVTable& vtable)
{
	std::cerr << "Connection::register_object("<< object_path <<") called" << std::endl;
	//_saftbus_objects[object_path] = counter++;
	//guint result = _saftbus_objects.size();
	//SaftbusObject object(object_path, interface_info, vtable);
	//_saftbus_objects.push_back(object);
	guint result = _saftbus_objects.size();
	_saftbus_objects.push_back(std::shared_ptr<InterfaceVTable>(new InterfaceVTable(vtable)));
	Glib::ustring interface_name = interface_info->get_interface_name();
	std::cerr << "    got interface_name = " << interface_name << std::endl;
	_saftbus_indices[interface_name][object_path] = result;
	// for(auto iter = _saftbus_objects.begin(); iter != _saftbus_objects.end(); ++iter)
	// {
	// 	std::cerr << *iter << std::endl;
	// }
	return result;
}
bool Connection::unregister_object (guint registration_id)
{
	if (registration_id < _saftbus_objects.size())
	{
		//_saftbus_objects[registration_id].object_path = "";
		return true;
	}
	return false;
}


guint Connection::signal_subscribe 	( 	const SlotSignal&  	slot,
										const Glib::ustring&  	sender,
										const Glib::ustring&  	interface_name,
										const Glib::ustring&  	member,
										const Glib::ustring&  	object_path,
										const Glib::ustring&  	arg0//,
										)//SignalFlags  	flags)
{
	std::cerr << "Connection::signal_subscribe(" << sender << "," << interface_name << "," << member << "," << object_path << ") called" << std::endl;
	return 0;
}

void Connection::signal_unsubscribe 	( 	guint  	subscription_id	) 
{
	std::cerr << "Connection::signal_unsubscribe() called" << std::endl;
}


void 	Connection::emit_signal (const Glib::ustring& object_path, const Glib::ustring& interface_name, const Glib::ustring& signal_name, const Glib::ustring& destination_bus_name, const Glib::VariantContainerBase& parameters)
{
	std::cerr << "Connection::emit_signal(" << object_path << "," << interface_name << "," << signal_name << "," << destination_bus_name << ") called" << std::endl;
	for (unsigned n = 0; n < parameters.get_n_children(); ++n)
	{
		Glib::VariantBase child;
		parameters.get_child(child, n);
		std::cerr << "parameter[" << n << "].type = " << child.get_type_string() << "    .value = " << child.print() << std::endl;
	}
}

bool Connection::dispatch(Glib::IOCondition condition, Socket *socket) 
{
		static int cnt = 0;
		++cnt;
		if (_debug_level) std::cerr << "Connection::dispatch() called one socket[" << socket_nr(socket) << "]" << std::endl;
		MessageTypeC2S type;
		int result = saftbus::read(socket->get_fd(), type);
		if (result == -1) {
			if (_debug_level) std::cerr << "client disconnected" << std::endl;
			socket->close_connection();
			socket->wait_for_client();
		} else {
			switch(type)
			{
				case saftbus::REGISTER_CLIENT: 
				{
					if (_debug_level) std::cerr << "Connection::dispatch() REGISTER_CLIENT received" << std::endl;
					saftbus::write(socket->get_fd(), saftbus::CLIENT_REGISTERED);
					std::cerr << "     writing client id " << _client_id << std::endl;
					saftbus::write(socket->get_fd(), _client_id);
					++_client_id;
				}
				break;
				case saftbus::METHOD_CALL: 
				{
					if (_debug_level) std::cerr << "Connection::dispatch() METHOD_CALL received" << std::endl;
					guint32 size;
					saftbus::read(socket->get_fd(), size);
					std::cerr << "expecting message with size " << size << std::endl;
					std::vector<char> buffer(size);
					saftbus::read_all(socket->get_fd(), &buffer[0], size);
					Glib::Variant<std::vector<Glib::VariantBase> > payload;
					deserialize(payload, &buffer[0], buffer.size());
					Glib::Variant<Glib::ustring> object_path    = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring> >(payload.get_child(0));
					Glib::Variant<Glib::ustring> sender         = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring> >(payload.get_child(1));
					Glib::Variant<Glib::ustring> interface_name = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring> >(payload.get_child(2));
					Glib::Variant<Glib::ustring> name           = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring> >(payload.get_child(3));
					Glib::VariantContainerBase parameters       = Glib::VariantBase::cast_dynamic<Glib::VariantContainerBase>   (payload.get_child(4));
					std::cerr << "sender = " << sender.get() <<  "    name = " << name.get() << "   object_path = " << object_path.get() <<  "    interface_name = " << interface_name.get() << std::endl;
					std::cerr << "parameters = " << parameters.print() << std::endl;
					Glib::VariantBase result;
					if (interface_name.get() == "org.freedesktop.DBus.Properties") {
						std::cerr << " interface for Set/Get property was called" << std::endl;

						Glib::Variant<Glib::ustring> derived_interface_name = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring> >(parameters.get_child(0));
						Glib::Variant<Glib::ustring> property_name          = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring> >(parameters.get_child(1));

						std::cerr << "property_name = " << property_name.get() << "   derived_interface_name = " << derived_interface_name.get() << std::endl;

						for (unsigned n = 0; n < parameters.get_n_children(); ++n)
						{
							Glib::VariantBase child;
							parameters.get_child(child, n);
							std::cerr << "parameter[" << n << "].type = " << child.get_type_string() << "    .value = " << child.print() << std::endl;
						}					
						int index = _saftbus_indices[derived_interface_name.get()][object_path.get()];
						std::cerr << "found saftbus object at index " << index << std::endl;
						
						_saftbus_objects[index]->get_property(result, saftbus::connection, sender.get(), object_path.get(), derived_interface_name.get(), property_name.get());

						std::cerr << "result is: " << result.get_type_string() << "   .value = " << result.print() << std::endl;

						std::vector<Glib::VariantBase> response;
						response.push_back(result);
						Glib::Variant<std::vector<Glib::VariantBase> > var_response = Glib::Variant<std::vector<Glib::VariantBase> >::create(response);

						std::cerr << "response is " << var_response.get_type_string() << " " << var_response.print() << std::endl;
						size = var_response.get_size();
						const char *data_ptr = static_cast<const char*>(var_response.get_data());
						saftbus::write(socket->get_fd(), saftbus::METHOD_REPLY);
						saftbus::write(socket->get_fd(), size);
						saftbus::write_all(socket->get_fd(), data_ptr, size);

					}
					else
					{
						std::cerr << "a real method call: " << std::endl;
						int index = _saftbus_indices[interface_name.get()][object_path.get()];
						std::cerr << "found saftbus object at index " << index << std::endl;
						Glib::RefPtr<MethodInvocation> method_invocation_rptr(new MethodInvocation);
						std::cerr << "doing the function call" << std::endl;
						std::cerr << "saftbus::connection.get() " << saftbus::connection.get() << std::endl;
						_saftbus_objects[index]->method_call(saftbus::connection, sender.get(), object_path.get(), interface_name.get(), name.get(), parameters, method_invocation_rptr);
						result = method_invocation_rptr->get_return_value();
						std::cerr << "result is " << result.get_type_string() << " " << result.print() << std::endl;

						// std::vector<Glib::VariantBase> response;
						// response.push_back(result);
						// Glib::Variant<std::vector<Glib::VariantBase> > var_response = Glib::Variant<std::vector<Glib::VariantBase> >::create(response);

						std::cerr << "response is " << result.get_type_string() << " " << result.print() << std::endl;
						size = result.get_size();
						const char *data_ptr = static_cast<const char*>(result.get_data());
						saftbus::write(socket->get_fd(), saftbus::METHOD_REPLY);
						saftbus::write(socket->get_fd(), size);
						saftbus::write_all(socket->get_fd(), data_ptr, size);

					}

					// for (auto itr = _saftbus_indices.begin(); itr != _saftbus_indices.end(); ++itr)
					// {
					// 	for (auto it = itr->second.begin(); it != itr->second.end(); ++it)
					// 	{
					// 		std::cerr << "_saftbus_objects[" << itr->first << "][" << it->first << "] = " << it->second << std::endl;
					// 	}
					// }
 					// TODO ... this is not correct ... the receiver complains like this: 
 					//       (process:31705): GLib-CRITICAL **: 21:08:01.314: g_variant_get_variant: assertion 'g_variant_is_of_type (value, G_VARIANT_TYPE_VARIANT)' failed


					//saftbus::write(socket->get_fd(), saftbus::CLIENT_REGISTERED);
				}
				break;

				default:
					if (_debug_level) std::cerr << "Connection::dispatch() unknown message type: " << type << std::endl;
					return false;
				break;				
			}

			return true;
		}
		return false;
}

int Connection::socket_nr(Socket *socket)
{
	for (guint32 i = 0; i < _sockets.size(); ++i) {
		if (_sockets[i].get() == socket)
			return i;
	}
	return -1;
}


// Glib::VariantContainerBase Connection::call_sync (const Glib::ustring& object_path, const Glib::ustring& interface_name, const Glib::ustring& method_name, const Glib::VariantContainerBase& parameters, const Glib::ustring& bus_name, int timeout_msec)
// {
// 	std::cerr << "Connection::call_sync(" << object_path << "," << interface_name << "," << method_name << ") called" << std::endl;
// 	return Glib::VariantContainerBase();
// }



}
