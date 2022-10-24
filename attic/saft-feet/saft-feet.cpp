// @brief GUI front end for saftbus-ctl
// @author Michael Reese  <m.reese@gsi.de>
//
// Copyright (C) 2020 GSI Helmholtz Centre for Heavy Ion Research GmbH 
//
// A GUI that allows to see saftbus objects, call their methods and get 
// or set their properties
//
//*****************************************************************************
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//*****************************************************************************
//
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>

#include <gtkmm.h>

#include <cstdio>
#include <cstdlib>

#include <fcntl.h>

std::string applicationName;

Gtk::Window *main_window;

class SaftbusCtl 
{
public:
	SaftbusCtl()  {
		system("rm -f /tmp/saftbus_output; mkfifo /tmp/saftbus_output");
		fp_write = popen("saftbus-ctl -i > /tmp/saftbus_output", "w");
		fd_read = open("/tmp/saftbus_output", O_RDONLY);
		saftbus_id = read_until("> ");
		std::cerr << "got saftbus id: " << saftbus_id << std::endl;
		std::ostringstream window_title;
		window_title << "saft-feet   " << saftbus_id;
		applicationName = window_title.str();
		applicationName.pop_back(); // remove the trailing "> " characters
		applicationName.pop_back(); // remove the trailing "> " characters
	}
	std::string read_until(const std::string &end) {
		std::string result;
		for (;;) {
			char buffer[1024];
			int read_bytes;
			if ((read_bytes = read(fd_read, buffer, 1024)) <= 0) {
				break;
			}
			result.append(buffer, read_bytes);
			if (result.size() >= end.size()) {
				if (result.compare(result.size()-end.size(), end.size(), end) == 0) {
					break;
				}
			}
		}
		return result;
	}
	std::string get_saftbus_id() {
		return saftbus_id;
	}
	~SaftbusCtl() {
		pclose(fp_write);
	}
	std::string call(std::string args) {
		if (args[0] == '-' && args[1] == '-') args = args.substr(2);
		//std::cerr << args << std::endl;
		fprintf(fp_write, "%s\n", args.c_str());
		fflush(fp_write);
		std::string response = read_until(saftbus_id);
		response = response.substr(0, response.size()-saftbus_id.size());
		if (response[response.size()-1] == '\n')  {
			response = response.substr(0, response.size()-1);
		}
		return response;
	}
private:
	FILE *fp_write;
	int  fd_read;
	std::string saftbus_id;
};

SaftbusCtl saftbus_ctl;


// execute 'saftbus-ctl -s' and extract object_paths and interfaces from the output
std::vector<std::string> get_object_paths() {
	std::string text = saftbus_ctl.call("--status");

	// optionally cut the prefix from object_path or interface name
	std::string object_path_prefix = "";//"/de/gsi";
	std::string interface_prefix = "";//"de.gsi.";

	// process saftbus-ctl's output
	std::string object_path;
	std::vector<std::string> result;
	std::istringstream in(text);
	for(;;) {
		std::string token;
		in >> token;
		if (!in) {
			break;
		}
		if (token[0] == '/') {
			object_path = token.substr(object_path_prefix.size()); // take away the "/de/gsi" prefix
		} else if (object_path.size() > 0) {
			if (token[0] == '_') {
				break;
			}
			auto interface = token.substr(interface_prefix.size(),token.find(',')-interface_prefix.size());
			if (interface.find('[') != std::string::npos) {
				interface = interface.substr(0,interface.find('['));
			}
			result.push_back(object_path);
			result.back().append("/");
			result.back().append(interface);
		}
	}
	return result;
}

std::string get_introspection_xml(const std::string &object_path, const std::string &interface_name) {
	std::string args = "--introspect ";
	args.append(interface_name);
	args.append(" ");
	args.append(object_path);
	return saftbus_ctl.call(args);
}

std::vector<std::string> split(const std::string &str, char delimeter) {
	std::string token;
	std::vector<std::string> result;
	for (auto c : str) {
		if (c == delimeter) {
			if (token.size() > 0) {
				result.push_back(token);
				token.clear();
			}
		} else {
			token.push_back(c);
		}
	}
	if (token.size() > 0) {
		result.push_back(token);
	}
	return result;
}

// return true if str stars with prefix
bool starts_with(const std::string &str, const std::string &prefix) {
	auto len = std::min(str.size(), prefix.size());
	return str.substr(0, len) == prefix;
}

bool contained_in(const std::string &path, const std::vector<std::string> &object_paths) {
	for (const auto &object_path : object_paths) {
		if (starts_with(object_path, path)) {
			return true;
		}
	}
	return false;
}

class ObjectPathTreeStore : public Gtk::TreeStore
{
private:
	Glib::RefPtr<Gtk::TreeStore>        _treestore;
	Gtk::TreeModelColumnRecord          _columns;
	Gtk::TreeModelColumn<Glib::ustring> _col_text;

	void insert_object_path(std::vector<std::string>::iterator object_path_begin, 
		                    std::vector<std::string>::iterator object_path_end, 
		                    Gtk::TreeNodeChildren children) 
	{
		auto object_path_size = object_path_end - object_path_begin;
		Gtk::TreeModel::iterator iter_insert_after = children.end();
		for (auto child : children) {
			Gtk::TreeModel::Row row = *child;
			if (row[_col_text] < *object_path_begin) {
				iter_insert_after = child;
			}
			if (row[_col_text] == *object_path_begin) {
				if (object_path_size > 1) {
					// first part of object path already present 
					//   -> descend into this branch of the treestore
					insert_object_path(object_path_begin + 1, object_path_end, child.children());
				}
				return;
			}
		}
		// nothing found or children.size()==0 
		//   -> insert new child at correct position 
		Gtk::TreeModel::iterator new_child = _treestore->prepend(children);
		if (iter_insert_after != children.end()) { 
			_treestore->move(new_child, ++iter_insert_after); 
		}
		Gtk::TreeModel::Row row = *new_child;
		row[_col_text] = *object_path_begin;
		if (object_path_size > 1) {
			// descend into just inserted branch of treestore
			insert_object_path(object_path_begin + 1, object_path_end, new_child->children());
		}
	}

	void remove_if_not_present(const std::vector<std::string> &object_paths, Gtk::TreeNodeChildren children, const std::string &parent_path = "" ) {
		for (;;) {
			bool removed = false;
			for (Gtk::TreeStore::iterator child = children.begin(); child != children.end(); ++child) {
				Gtk::TreeModel::Row row = *child;
				std::ostringstream path;
				path << parent_path << '/' << row[_col_text];
				if (!contained_in(path.str(), object_paths)) {
					child = _treestore->erase(child);
					removed = true;
					break;
				}
				if (child->children().size() > 0) {
					remove_if_not_present(object_paths, child->children(), path.str());
				}
			}
			if (!removed) break;
		}
	}
public:
	ObjectPathTreeStore() {
		// prepare the column layout (1 columns containing text)
		_columns.add(_col_text);
		_treestore = Gtk::TreeStore::create(_columns);
	}
	Gtk::TreeModelColumn<Glib::ustring> &col_text() { 
		return _col_text; 
	}
	Glib::RefPtr<Gtk::TreeStore> get_model() { 
		return _treestore; 
	}
	void insert_object_path(const std::string &object_path) {
		std::vector<std::string> object_path_parts = split(object_path, '/');
		insert_object_path(object_path_parts.begin(), 
			               object_path_parts.end(),
			               _treestore->children());
	}
	void remove_if_not_present(const std::vector<std::string> &object_paths) {
		remove_if_not_present(object_paths, _treestore->children());
	}

	std::string get_full_name(Gtk::TreePath path) {
		std::string full_name;
		for(;;) {
			auto row = *_treestore->get_iter(path);
			std::ostringstream row_name;
			row_name << row[_col_text];
			full_name = "/" + row_name.str() + full_name;
			std::string path_str = path.to_string();
			if (path_str.find(':') == path_str.npos) break;
			path.up();
		} 
		return full_name;
	}

	void on_row_activated(Gtk::TreePath path, Gtk::TreeViewColumn* column, sigc::slot<void, std::string, std::string> update_interface_box) {
		std::string full_name = get_full_name(path);
		//std::cerr << "on_row_activated called: " << full_name << std::endl;
		auto pos = full_name.find_last_of('/');
		std::string object_path = full_name.substr(0, pos);
		std::string interface_name = full_name.substr(pos + 1);
		const std::string interface_prefix = "de.gsi.saftlib.";
		if (interface_name.substr(0,interface_prefix.size()) == interface_prefix) {
			//std::cerr << object_path << " " << interface_name << std::endl;
			update_interface_box(object_path, interface_name);
			//std::cout << saftbus_ctl.call("--get-properties " + interface_name + " " + object_path) << std::endl;
		}
	}
};


class ScrolledWindowTreeView : public Gtk::ScrolledWindow
{	
public:
	ScrolledWindowTreeView(sigc::slot<void, std::string, std::string> update_interface_box) 
	{
		update_object_paths(); 

		Glib::signal_timeout().connect(
			sigc::mem_fun(this, &ScrolledWindowTreeView::update_object_paths), 100);

		// attach the model to the treeview
		_treeview.set_model(_treestore.get_model());
		_treeview.append_column("Objects and Interfaces", _treestore.col_text());


		_treeview.signal_row_activated().connect(
			sigc::bind(sigc::mem_fun(_treestore, &ObjectPathTreeStore::on_row_activated), update_interface_box ) );
		
		add(_treeview);
		show_all();
		
		_treeview.set_activate_on_single_click(true);
	}
private:

	bool update_object_paths() {
		auto object_paths = get_object_paths();
		_treestore.remove_if_not_present(object_paths);
		for(auto object_path: object_paths) {
			_treestore.insert_object_path(object_path);
		}
		return true;
	}

	Gtk::TreeView       _treeview;
	ObjectPathTreeStore _treestore;

};

class InterfaceBox : public Gtk::Box
{
private:
	struct Property {
		std::string interface_name;
		std::string object_path;
		std::string property_name;
		Gtk::Box    box;
		std::string type;
		Gtk::Label  name;
		Gtk::Button get;
		Gtk::Entry  value;
		Gtk::Button set;
		Gtk::Entry  entry;
		bool wr;
		void set_property() 
		{
			std::string new_value = entry.get_text();
			if (new_value.size() == 0) {
				Gtk::MessageDialog dialog(*main_window, "missing value");
				dialog.run();
				return;
			}
			std::string command = "--set-property " + interface_name + " " + object_path + " " + property_name + " " + type + " " + new_value;
			// std::cerr << command << std::endl;
			std::string result = saftbus_ctl.call(command);
			// std::cerr << result << std::endl;
			if (result.find("Set property failed:") != std::string::npos) {
				Gtk::MessageDialog dialog(*main_window, result);
				dialog.run();
			}
		}
		void get_property() 
		{
			std::string command = "--get-property " + interface_name + " " + object_path + " " + property_name + " " + type;
			// std::cerr << command << std::endl;
			std::string result = saftbus_ctl.call(command);
			// std::cerr << result << std::endl;
			value.set_text(result);
		}
		Property(const std::string &ifn, const std::string &obp, std::string t, std::string n, std::string v, bool w)
			: interface_name(ifn), object_path(obp), property_name(n), box(Gtk::ORIENTATION_HORIZONTAL), type(t), get("get >"), name(t+":"+n), set("< set"), wr(w)
		{
			value.set_text(v);
			value.set_editable(false);
			get.signal_pressed().connect(sigc::mem_fun(this, &InterfaceBox::Property::get_property));
			box.add(name);
			box.add(get);
			box.add(value);
			if (wr) {
				box.add(set);
				set.signal_pressed().connect(sigc::mem_fun(this, &InterfaceBox::Property::set_property));
				box.add(entry);
			}
		}
		~Property() {
			box.remove(name);
			box.remove(value);
			if (wr) {
				box.remove(set);
				box.remove(entry);
			}
		}
	};

	struct Method {
		struct Arg{
			Gtk::Label  name;
			Gtk::Entry  value;
			std::string type;
			bool in;
			Arg(std::string description) // description is inout:type:name e.g. 'in:t:event' or 'out:i:result' 
				: name(description), in(description.substr(0,2)=="in"?true:false)
			{
				value.set_editable(in);
				auto pos1 = description.find(":");
				auto pos2 = description.find_last_of(":");
				type = description.substr(pos1+1,pos2-pos1-1);
			}
		};
		std::string interface_name;
		std::string object_path;
		std::string method_name;
		std::string in_signature;
		std::string out_signature;
		Gtk::Box    box;
		Gtk::Label  name;
		Gtk::Button call;
		std::vector<std::shared_ptr<Arg> > args;
		// bool execute(GdkEventButton* event) 
		// {
		// 	std::cerr << "calling function" << std::endl;
		// 	return true;
		// }
		void execute() 
		{
//			std::cerr << "calling function" << std::endl;
			std::string command = "--call " + interface_name + " " + object_path + " " + method_name + " " + out_signature + " " + in_signature + " ";
			for (auto &a : args) {
				if (a->in) {
					std::string arg = a->value.get_text();
					if (arg.size() == 0) {
						std::ostringstream ss;
						ss << "missing value for in argument " << a->name.get_text();
						//std::cerr << ss.str() << std::endl;
						Gtk::MessageDialog dialog(*main_window, ss.str());
						dialog.run();
						return;
					}
					command.append(a->value.get_text());
					command.append(" ");
				}
			}
			// std::cerr << command << std::endl;
			std::string result = saftbus_ctl.call(command);
			// std::cerr << result << std::endl;
			if (result.find("Method call failed:") != std::string::npos) {
				Gtk::MessageDialog dialog(*main_window, result);
				dialog.run();
				return;
			}

			std::istringstream in(result);
			int arg_idx = 0;
			for (;;) {
				std::string value;
				in >> value;
				if (!in) break;
				while(args[arg_idx]->in) ++arg_idx;
				if (value == "exception" || value == "unsupported") {
					args[arg_idx]->value.set_text(result);
					break;
				}
				args[arg_idx]->value.set_text(value);
			}
		}
		Method(const std::string &ifn, const std::string &obp, std::string n, std::vector<std::shared_ptr<Arg> > a)
			: interface_name(ifn), object_path(obp), method_name(n), box(Gtk::ORIENTATION_HORIZONTAL), name(n), call("call"), args(a)
		{
			box.add(name);
			//call.signal_button_press_event().connect(sigc::mem_fun(this, &InterfaceBox::Method::execute));
			call.signal_pressed().connect(sigc::mem_fun(this, &InterfaceBox::Method::execute));
			box.add(call);
			for (auto &a : args) {
				box.add(a->name);
				box.add(a->value);
				if (a->in) in_signature.append(a->type);
				else       out_signature.append(a->type);
			}
			if (in_signature.size() == 0) in_signature = "v";
			if (out_signature.size() == 0) out_signature = "v";
		}
		~Method() {
			box.remove(name);
			box.remove(call);
			for (auto &a : args) {
				box.remove(a->name);
				box.remove(a->value);
			}
		}
	};

	Gtk::Label _properties_label;	
	std::vector<std::shared_ptr<Property> > _properties;

	Gtk::Separator _sep1;

	Gtk::Label _methods_label;	
	std::vector<std::shared_ptr<Method> > _methods;
	Gtk::Label _signals_label;	
public:
	InterfaceBox() 
		: Gtk::Box(Gtk::ORIENTATION_VERTICAL)
		, _properties_label("Properties", Gtk::ALIGN_START, Gtk::ALIGN_CENTER)
		, _sep1(Gtk::ORIENTATION_HORIZONTAL)
		, _methods_label("Methods", Gtk::ALIGN_START, Gtk::ALIGN_CENTER)
		, _signals_label("Signals", Gtk::ALIGN_START, Gtk::ALIGN_CENTER)
	{
		_properties_label.set_justify(Gtk::JUSTIFY_LEFT);
		add(_properties_label);

		add(_sep1);

		_methods_label.set_justify(Gtk::JUSTIFY_LEFT);
		add(_methods_label);

		// _signals_label.set_justify(Gtk::JUSTIFY_LEFT);
		// add(_signals_label);

		//show_all();
	}

	void update(const std::string &object_path, const std::string &interface_name) {
		//std::cerr<< "update called" << std::endl;

		{ // properties
			for (auto & p : _properties) {
				remove(p->box);
			}
			_properties.clear();
			remove(_properties_label);
			add(_properties_label);

			std::string property_string = saftbus_ctl.call("--get-properties " + interface_name + " " + object_path);
			std::istringstream in(property_string);
			for (;;) {
				std::string line;
				std::getline(in, line);
				if (!in) break;
				auto pos_equal = line.find('=');
				std::string definition = line.substr(0, pos_equal);
				std::string value      = line.substr(pos_equal + 1);
				auto property_parts = split(definition, ':');
				if (property_parts.size() != 3) break;
				std::string type = property_parts[0];
				std::string access = property_parts[1];
				std::string name = property_parts[2];
				//std::cerr << type << " " << name << " " << value << std::endl;
				//_properties.push_back(Property(name,value,false));
				_properties.push_back(std::make_shared<Property>(interface_name, object_path, type, name, value, access=="write"||access=="readwrite"));
				//std::make_shared<Gtk::Label>(name)
				add(_properties.back()->box);
			}
		}

		remove(_sep1);
		add(_sep1);

		{ // methods
			for (auto & m : _methods) {
				remove(m->box);
			}
			_methods.clear();
			remove(_methods_label);
			add(_methods_label);

			std::string property_string = saftbus_ctl.call("--list-methods " + interface_name + " " + object_path);
			std::istringstream in(property_string);
			for (;;) {

				std::string line;
				std::getline(in, line);
				if (!in) break;
				std::istringstream lin(line);
				std::string name;
				lin >> name;
				if (name.size() == 0) break;

				std::vector<std::shared_ptr<Method::Arg> > args;
				for (;;) {
					std::string arg;
					lin >> arg;
					if (!lin) break;
					args.push_back(std::make_shared<Method::Arg>(arg));
				}
				_methods.push_back(std::make_shared<Method>(interface_name, object_path, name, args));
				//std::make_shared<Gtk::Label>(name)
				add(_methods.back()->box);
			}
		}



		show_all();
		//_properties_label.set_text("Properties\n" + property_string);
		//std::cerr << get_introspection_xml(object_path, interface_name) << std::endl;
	}

};

class MainWindow : public Gtk::Window 
{
private:
	Gtk::Box               _box;
	InterfaceBox           _interface_box;
	Gtk::ScrolledWindow    _interface_box_scrolled;
	Gtk::Paned             _paned;
	ScrolledWindowTreeView _object_path_list;
public:
	MainWindow()
		: _box(Gtk::ORIENTATION_VERTICAL)
		, _paned(Gtk::ORIENTATION_HORIZONTAL)
		, _object_path_list(sigc::mem_fun(&_interface_box, &InterfaceBox::update))
	{
		set_default_size(600, 400);
		set_title(applicationName);
		_paned.set_position(300);
		_box.pack_end(_object_path_list, true, true);
		_paned.add1(_box);
		_interface_box_scrolled.add(_interface_box);
		_paned.add2(_interface_box_scrolled);
		add(_paned);
		show_all();
	}
};

int main(int argc, char *argv[])
{
	auto application = Gtk::Application::create(argc, argv, "de.gsi.saft-feet");
	MainWindow window;
	main_window = &window;
	return application->run(window);

	return 0;
}
