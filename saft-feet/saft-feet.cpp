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

std::vector<std::string> get_object_paths() {
	FILE *fp = popen("saftbus-ctl -s", "r");
	if (fp == nullptr) {
		std::cerr << "Error: cannot run saftbus-ctl" << std::endl;
		exit(1);
	}
	char c;
	std::string text;
	while ((c = fgetc(fp)) != EOF) {
		text.push_back(c);
	}
	pclose(fp);

	std::vector<std::string> result;
	std::istringstream in(text);
	for(;;) {
		std::string token;
		in >> token;
		if (!in) break;
		if (token[0] == '/') {
			result.push_back(token);
		}
	}
	return result;
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
		for (auto iter : children) {
			Gtk::TreeModel::Row row = *iter;
			if (row[_col_text] < *object_path_begin) {
				iter_insert_after = iter;
			}
			if (row[_col_text] == *object_path_begin &&	object_path_size > 1) {
				// first part of object path already present 
				//   -> descend into this branch of the treestore
				insert_object_path(object_path_begin + 1, object_path_end, iter.children());
				return;
			}
		}
		// nothing found or children.size()==0 
		//   -> insert new child at correct position 
		Gtk::TreeModel::iterator iter = _treestore->prepend(children);
		if (iter_insert_after != children.end()) { 
			_treestore->move(iter, ++iter_insert_after); 
		}
		Gtk::TreeModel::Row row = *iter;
		row[_col_text] = *object_path_begin;
		if (object_path_size > 1) {
			// descend into just inserted branch of treestore
			insert_object_path(object_path_begin + 1, object_path_end, iter->children());
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
	// void append_toplevel(const std::string &object_path) {
	// 	Gtk::TreeModel::iterator iter = _treestore->append(_treestore->children());
	// 	Gtk::TreeModel::Row      row  = *iter;
	// 	row[_col_text] = object_path;
	// }
	// void append_next(const std::string &object_path) {
	// 	for(auto iter : _treestore->children()) {
	// 		Gtk::TreeModel::Row      row  =  *_treestore->append(iter.children());
	// 		row[_col_text] = object_path;
	// 	}
	// }
	void insert_object_path(const std::string &object_path) {
		std::vector<std::string> object_path_parts = split(object_path, '/');
		insert_object_path(object_path_parts.begin(), 
			               object_path_parts.end(),
			               _treestore->children());
	}


	void add_object_path() {

		for(auto iter : _treestore->children()) {
			std::cout << iter[_col_text] << std::endl;
		}
	}
};


class ScrolledWindowTreeView : public Gtk::ScrolledWindow
{	
public:
	ScrolledWindowTreeView() 
	{
		set_propagate_natural_height(true);
		set_propagate_natural_width(true);

		for(auto object_path: get_object_paths()) {
			_treestore.insert_object_path(object_path);
		}

		// attach the model to the treeview
		_treeview.set_model(_treestore.get_model());
		_treeview.append_column("Object Path", _treestore.col_text());
		add(_treeview);
		show_all();
	}
private:
	Gtk::TreeView       _treeview;
	ObjectPathTreeStore _treestore;
};

class MainWindow : public Gtk::Window 
{
private:
	ScrolledWindowTreeView _object_path_list;
	Gtk::Button            _testbutton;
	Gtk::Box               _box;
public:
	MainWindow()
		: _testbutton("Hallo"), _box(Gtk::ORIENTATION_VERTICAL)
	{
		_box.add(_testbutton);
		_box.pack_end(_object_path_list, true, true);
		add(_box);
		show_all();
	}
};

int main(int argc, char *argv[])
{
	auto application = Gtk::Application::create(argc, argv, "de.gsi.saft-feet");
	MainWindow window;
	window.set_default_size(600, 600);


	return application->run(window);

	return 0;
}
