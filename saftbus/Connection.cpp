#include "Connection.h"
#include "Socket.h"
#include "core.h"

#include <iostream>
#include <sstream>
#include <iomanip>


namespace saftbus
{

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
	if (_debug_level) std::cerr << "Connection::register_object("<< object_path <<") called" << std::endl;
	guint result = _saftbus_objects.size();
	_saftbus_objects.push_back(std::shared_ptr<InterfaceVTable>(new InterfaceVTable(vtable)));
	Glib::ustring interface_name = interface_info->get_interface_name();
	if (_debug_level) std::cerr << "    got interface_name = " << interface_name << std::endl;
	_saftbus_indices[interface_name][object_path] = result;
	// for(auto iter = _saftbus_objects.begin(); iter != _saftbus_objects.end(); ++iter)
	// {
	// 	std::cerr << *iter << std::endl;
	// }
	return result;
}
bool Connection::unregister_object (guint registration_id)
{
	if (_debug_level) std::cerr << "Connection::unregister_object("<< registration_id <<") called" << std::endl;
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
	if (_debug_level) std::cerr << "Connection::signal_subscribe(" << sender << "," << interface_name << "," << member << "," << object_path << ") called" << std::endl;
	return 0;
}

void Connection::signal_unsubscribe 	( 	guint  	subscription_id	) 
{
	if (_debug_level) std::cerr << "Connection::signal_unsubscribe() called" << std::endl;
}


void 	Connection::emit_signal (const Glib::ustring& object_path, const Glib::ustring& interface_name, const Glib::ustring& signal_name, const Glib::ustring& destination_bus_name, const Glib::VariantContainerBase& parameters)
{
	if (_debug_level) std::cerr << "Connection::emit_signal(" << object_path << "," << interface_name << "," << signal_name << "," << destination_bus_name << ") called" << std::endl;
	for (unsigned n = 0; n < parameters.get_n_children(); ++n)
	{
		Glib::VariantBase child;
		parameters.get_child(child, n);
		if (_debug_level) std::cerr << "parameter[" << n << "].type = " << child.get_type_string() << "    .value = " << child.print() << std::endl;
	}

	std::vector<Glib::VariantBase> signal_msg;
	signal_msg.push_back(Glib::Variant<Glib::ustring>::create(object_path));
	signal_msg.push_back(Glib::Variant<Glib::ustring>::create(interface_name));
	signal_msg.push_back(Glib::Variant<Glib::ustring>::create(signal_name));
	signal_msg.push_back(parameters);
	Glib::Variant<std::vector<Glib::VariantBase> > var_signal_msg = Glib::Variant<std::vector<Glib::VariantBase> >::create(signal_msg);
	std::cerr << "signal message " << var_signal_msg.get_type_string() << " " << var_signal_msg.print() << std::endl;

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
					if (_debug_level) std::cerr << "     writing client id " << _client_id << std::endl;
					saftbus::write(socket->get_fd(), _client_id);
					++_client_id;
				}
				break;
				case saftbus::METHOD_CALL: 
				{
					if (_debug_level) std::cerr << "Connection::dispatch() METHOD_CALL received" << std::endl;
					guint32 size;
					saftbus::read(socket->get_fd(), size);
					if (_debug_level) std::cerr << "expecting message with size " << size << std::endl;
					std::vector<char> buffer(size);
					saftbus::read_all(socket->get_fd(), &buffer[0], size);
					Glib::Variant<std::vector<Glib::VariantBase> > payload;
					deserialize(payload, &buffer[0], buffer.size());
					Glib::Variant<Glib::ustring> object_path    = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring> >(payload.get_child(0));
					Glib::Variant<Glib::ustring> sender         = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring> >(payload.get_child(1));
					Glib::Variant<Glib::ustring> interface_name = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring> >(payload.get_child(2));
					Glib::Variant<Glib::ustring> name           = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring> >(payload.get_child(3));
					Glib::VariantContainerBase parameters       = Glib::VariantBase::cast_dynamic<Glib::VariantContainerBase>   (payload.get_child(4));
					if (_debug_level) std::cerr << "sender = " << sender.get() <<  "    name = " << name.get() << "   object_path = " << object_path.get() <<  "    interface_name = " << interface_name.get() << std::endl;
					if (_debug_level) std::cerr << "parameters = " << parameters.print() << std::endl;
					if (interface_name.get() == "org.freedesktop.DBus.Properties") {
						if (_debug_level) std::cerr << " interface for Set/Get property was called" << std::endl;

						Glib::Variant<Glib::ustring> derived_interface_name = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring> >(parameters.get_child(0));
						Glib::Variant<Glib::ustring> property_name          = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring> >(parameters.get_child(1));

						if (_debug_level) std::cerr << "property_name = " << property_name.get() << "   derived_interface_name = " << derived_interface_name.get() << std::endl;

						for (unsigned n = 0; n < parameters.get_n_children(); ++n)
						{
							Glib::VariantBase child;
							parameters.get_child(child, n);
							if (_debug_level) std::cerr << "parameter[" << n << "].type = " << child.get_type_string() << "    .value = " << child.print() << std::endl;
						}					
						int index = _saftbus_indices[derived_interface_name.get()][object_path.get()];
						if (_debug_level) std::cerr << "found saftbus object at index " << index << std::endl;
						
						if (name.get() == "Get") {
							Glib::VariantBase result;
							_saftbus_objects[index]->get_property(result, saftbus::connection, sender.get(), object_path.get(), derived_interface_name.get(), property_name.get());
							if (_debug_level) std::cerr << "getting property " << result.get_type_string() << " " << result.print() << std::endl;
							if (_debug_level) std::cerr << "result is: " << result.get_type_string() << "   .value = " << result.print() << std::endl;

							std::vector<Glib::VariantBase> response;
							response.push_back(result);
							Glib::Variant<std::vector<Glib::VariantBase> > var_response = Glib::Variant<std::vector<Glib::VariantBase> >::create(response);

							if (_debug_level) std::cerr << "response is " << var_response.get_type_string() << " " << var_response.print() << std::endl;
							size = var_response.get_size();
							if (_debug_level) std::cerr << " size of response is " << size << std::endl;
							const char *data_ptr = static_cast<const char*>(var_response.get_data());
							saftbus::write(socket->get_fd(), saftbus::METHOD_REPLY);
							saftbus::write(socket->get_fd(), size);
							saftbus::write_all(socket->get_fd(), data_ptr, size);

						} else if (name.get() == "Set") {
							if (_debug_level) std::cerr << "setting property" << std::endl;
							auto par2 = parameters.get_child(2);
							std::cerr << "par2 = " << par2.get_type_string() << " "<< par2.print() << std::endl;
							//Glib::Variant<bool> value = Glib::VariantBase::cast_dynamic<Glib::Variant<bool> >(parameters.get_child(2));
							Glib::Variant<bool> vb = Glib::Variant<bool>::create(true);
							std::cerr << "vb = " << vb.get_type_string() << " " << vb.print() << std::endl;

							//std::cerr << "value = " << value << std::endl;
							// if (_debug_level) std::cerr << " value = " << value.get_type_string() << " " << value.print() << std::endl;
							bool result = _saftbus_objects[index]->set_property(saftbus::connection, sender.get(), object_path.get(), derived_interface_name.get(), property_name.get(), vb);

							std::vector<Glib::VariantBase> response;
							response.push_back(Glib::Variant<bool>::create(result));
							Glib::Variant<std::vector<Glib::VariantBase> > var_response = Glib::Variant<std::vector<Glib::VariantBase> >::create(response);

							if (_debug_level) std::cerr << "response is " << var_response.get_type_string() << " " << var_response.print() << std::endl;
							size = var_response.get_size();
							if (_debug_level) std::cerr << " size of response is " << size << std::endl;
							const char *data_ptr = static_cast<const char*>(var_response.get_data());
							saftbus::write(socket->get_fd(), saftbus::METHOD_REPLY);
							saftbus::write(socket->get_fd(), size);
							saftbus::write_all(socket->get_fd(), data_ptr, size);
						}

					}
					else
					{
						if (_debug_level) std::cerr << "a real method call: " << std::endl;
						int index = _saftbus_indices[interface_name.get()][object_path.get()];
						if (_debug_level) std::cerr << "found saftbus object at index " << index << std::endl;
						Glib::RefPtr<MethodInvocation> method_invocation_rptr(new MethodInvocation);
						if (_debug_level) std::cerr << "doing the function call" << std::endl;
						if (_debug_level) std::cerr << "saftbus::connection.get() " << saftbus::connection.get() << std::endl;
						_saftbus_objects[index]->method_call(saftbus::connection, sender.get(), object_path.get(), interface_name.get(), name.get(), parameters, method_invocation_rptr);
						Glib::VariantContainerBase result;
						result = method_invocation_rptr->get_return_value();
						if (_debug_level) std::cerr << "result is " << result.get_type_string() << " " << result.print() << std::endl;

						std::vector<Glib::VariantBase> response;
						response.push_back(result);
						Glib::Variant<std::vector<Glib::VariantBase> > var_response = Glib::Variant<std::vector<Glib::VariantBase> >::create(response);

						if (_debug_level) std::cerr << "response is " << var_response.get_type_string() << " " << var_response.print() << std::endl;
						size = var_response.get_size();
						const char *data_ptr = static_cast<const char*>(var_response.get_data());
						saftbus::write(socket->get_fd(), saftbus::METHOD_REPLY);
						saftbus::write(socket->get_fd(), size);
						saftbus::write_all(socket->get_fd(), data_ptr, size);

					}
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
