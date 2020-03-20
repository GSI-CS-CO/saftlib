#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>

#include <gtkmm.h>

#include <cstdio>
#include <cstdlib>

const std::string applicationName = "saft-feet";

std::string call_saftbus_ctl(const std::string &args) {
	std::string command = "saftbus-ctl ";
	command.append(args);
	FILE *fp = popen(command.c_str(), "r");
	if (fp == nullptr) {
		std::cerr << "Error: cannot run saftbus-ctl" << std::endl;
		exit(1);
	}
	char c;
	std::string saftbus_ctl_output;
	saftbus_ctl_output.reserve(2048);
	while ((c = fgetc(fp)) != EOF) {
		saftbus_ctl_output.push_back(c);
	}
	pclose(fp);
	return saftbus_ctl_output;
}

// execute 'saftbus-ctl -s' and extract object_paths and interfaces from the output
std::vector<std::string> get_object_paths() {
	std::string text = call_saftbus_ctl("-s");

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
	return call_saftbus_ctl(args);
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
		std::cerr << "on_row_activated called: " << full_name << std::endl;
		auto pos = full_name.find_last_of('/');
		std::string object_path = full_name.substr(0, pos);
		std::string interface_name = full_name.substr(pos + 1);
		const std::string interface_prefix = "de.gsi.saftlib.";
		if (interface_name.substr(0,interface_prefix.size()) == interface_prefix) {
			std::cerr << object_path << " " << interface_name << std::endl;
			update_interface_box(object_path, interface_name);
			//std::cout << call_saftbus_ctl("--get-properties " + interface_name + " " + object_path) << std::endl;
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
		Gtk::Label name;
		Gtk::Label value;
		Gtk::Button write;
		Gtk::Entry entry;
	};

	Gtk::Label _properties_label;	
	std::vector<Property> _properties;

	Gtk::Label _methods_label;	
	Gtk::Label _signals_label;	
public:
	InterfaceBox() 
		: Gtk::Box(Gtk::ORIENTATION_VERTICAL)
		, _properties_label("Properties", Gtk::ALIGN_START, Gtk::ALIGN_CENTER)
		, _methods_label("Methods", Gtk::ALIGN_START, Gtk::ALIGN_CENTER)
		, _signals_label("Signals", Gtk::ALIGN_START, Gtk::ALIGN_CENTER)
	{
		_properties_label.set_justify(Gtk::JUSTIFY_LEFT);
		add(_properties_label);

		_methods_label.set_justify(Gtk::JUSTIFY_LEFT);
		add(_methods_label);

		_signals_label.set_justify(Gtk::JUSTIFY_LEFT);
		add(_signals_label);

		show_all();
	}

	void update(const std::string &object_path, const std::string &interface_name) {
		//std::cerr<< "update called" << std::endl;
		std::string property_string = call_saftbus_ctl("--get-properties " + interface_name + " " + object_path);
		_properties_label.set_text("Properties\n" + property_string);
		//std::cerr << get_introspection_xml(object_path, interface_name) << std::endl;
	}

};

class MainWindow : public Gtk::Window 
{
private:
	Gtk::Box               _box;
	InterfaceBox           _interface_box;
	Gtk::Paned             _paned;
	ScrolledWindowTreeView _object_path_list;
public:
	MainWindow()
		: _box(Gtk::ORIENTATION_VERTICAL)
		, _paned(Gtk::ORIENTATION_HORIZONTAL)
		, _object_path_list(sigc::mem_fun(&_interface_box, &InterfaceBox::update))
	{
		set_default_size(800, 600);
		_paned.set_position(300);
		_box.pack_end(_object_path_list, true, true);
		_paned.add1(_box);
		_paned.add2(_interface_box);
		add(_paned);
		show_all();
	}
};

int main(int argc, char *argv[])
{
	auto application = Gtk::Application::create(argc, argv, "de.gsi.saft-feet");
	MainWindow window;
	return application->run(window);

	return 0;
}
