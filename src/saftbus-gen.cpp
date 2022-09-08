#include <algorithm>
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>
#include <map>

bool verbose = false;

enum ExportTag {
	NO_EXPORT,
	SIGNAL_EXPORT,
	FUNCTION_EXPORT,
};

// return true if a saftbus export tag was found in a line comment 
static ExportTag remove_line_comments(std::string &line) {
	std::string saftbus_export_tag = " @saftbus-export";
	std::string saftbus_signal_tag = " @saftbus-signal";
	bool in_string = false;
	char previous_ch = ' ';
	for (size_t i = 0; i < line.size(); ++i) {
		if (line[i] == '\"') in_string = !in_string;
		if (line[i] == '/' && previous_ch == '/' && !in_string) {
			// std::cerr << "found//" << line << std::endl;
			if (i+1+saftbus_export_tag.size() <= line.size()) {
				// std::cerr << "++ " << line.substr(i+1,saftbus_export_tag.size()) << std::endl;
				if (line.substr(i+1,saftbus_export_tag.size()) == saftbus_export_tag) {
					line = line.substr(0,i-1);
					// std::cerr << "saftbus export " << line << std::endl;
					return FUNCTION_EXPORT;
				}
			} 
			if (i+1+saftbus_signal_tag.size() <= line.size()) {
				// std::cerr << "++ " << line.substr(i+1,saftbus_signal_tag.size()) << std::endl;
				if (line.substr(i+1,saftbus_signal_tag.size()) == saftbus_signal_tag) {
					line = line.substr(0,i-1);
					// std::cerr << "saftbus export " << line << std::endl;
					return SIGNAL_EXPORT;
				}
			}
			line = line.substr(0,1);
			return NO_EXPORT;
		}
		previous_ch = line[i];
	}
	return NO_EXPORT;
}

static void remove_block_comments(std::string &line, bool &block_comment) {
	// std::cerr << "+++ " << line << std::endl;
	std::string result;
	bool in_string = false;
	char previous_ch = ' ';
	for (size_t i = 0; i < line.size(); ++i) {
		if (block_comment) {
			if (line[i] == '\"') in_string = !in_string;
			if (line[i] == '/' && previous_ch == '*' && !in_string) {
				block_comment = false;
			}
			previous_ch = line[i];
		} else {
			if (line[i] == '\"') in_string = !in_string;
			if (line[i] == '*' && previous_ch == '/' && !in_string) {
				block_comment = true;
				result.pop_back();
			}
			if (block_comment == false) {
				result.push_back(line[i]);
			}
			previous_ch = line[i];
		}
	}
	line = result;
	// std::cerr << "=== " << line << std::endl;
}

static std::string build_namespace(const std::vector<std::string> namespace_names) {
	std::string result;
	for (auto &name: namespace_names) {
		if (name.size() > 0) {
			if (result.size() > 0) {
				result.append("::");
			}
			result.append(name);
		}
	}
	return result;
}

void manage_scopes(const std::string &line, std::vector<std::string> &scope, std::string &latest_name) {

	for (size_t i = 0; i < line.size(); ++i) {
		if (line[i] == '{') {
			scope.push_back(latest_name);
			if (verbose) {
				std::cerr << "enter scope \'" << scope.back() << "\'" << std::endl;
			}
			latest_name = "";
		}
		if (line[i] == '}') {
			if (verbose) {
				std::cerr << "leave scope \'" << scope.back() << "\'" << std::endl;
			}
			scope.pop_back();
		}
		// namespace
		if (line[i] == 'n' && line.substr(i,9) == "namespace") {
			std::istringstream in(line.substr(i+9));
			std::string name;
			in >> name;
			if (!in) {
				throw std::runtime_error("identifier expected after \'namespace\'");
			}
			i += 9 + name.size();
			if (line.find(';') == line.npos && line.find("friend") == line.npos) {
				latest_name = name;
			}
		}
		// class 
		if (line[i] == 'c' && line.substr(i,5) == "class") {
			std::istringstream in(line.substr(i+5));
			std::string name;
			in >> name;
			if (!in) {
				throw std::runtime_error("identifier expected after \'class\'");
			}
			i += 5 + name.size();
			if (line.find(';') == line.npos && line.find("friend") == line.npos) {
				latest_name = name;
			}
		}
		// struct
		if (line[i] == 's' && line.substr(i,6) == "struct") {
			std::istringstream in(line.substr(i+6));
			std::string name;
			in >> name;
			if (!in) {
				throw std::runtime_error("identifier expected after \'struct\'");
			}
			i += 6 + name.size();
			if (line.find(';') == line.npos && line.find("friend") == line.npos) {
				latest_name = name;
			}
		}
	}
}

std::string strip(std::string line) {
	// std::cerr << "strip \'" << line << "\'" << std::endl;
	size_t start = 0;
	while(isspace(line[start])) ++start;
	size_t stop = line.size()-1;
	while(isspace(line[stop])) --stop;
	std::string result = line.substr(start,stop-start+1);
	// std::cerr << "stripped \'" << result << "\'" << std::endl;
	return result;
}
struct FunctionArgument { 
	std::string type;
	std::string name;
	std::string init;
	bool is_output; // non-const reference arguments are taken as outputs of the function
	FunctionArgument(const std::string &argument_string) {
		std::string type_and_name = argument_string;
		auto equals_pos = argument_string.find("=");
		if (equals_pos != argument_string.npos) {
			init = strip(argument_string.substr(equals_pos+1));
			type_and_name = strip(argument_string.substr(0, equals_pos));
		} else {
			type_and_name = strip(argument_string);
		}
		auto name_start = type_and_name.find_last_of(" &");
		type = strip(type_and_name.substr(0,name_start+1));
		name = strip(type_and_name.substr(name_start+1));
		is_output = false;
		if (type.find("&") != type.npos &&      // a ref-type
			type.find("const") == type.npos) {  // that is not const
			is_output = true;
		}
	}
	std::string definition() {
		std::string result;
		result.append(type);
		result.append(" ");
		result.append(name);
		return result;
	}
	std::string declaration() {
		std::string result = definition();
		if (init.size() > 0) {
			result.append(" = ");
			result.append(init);
		}
		return result;
	}
};

std::vector<FunctionArgument> split(std::string argument_list) {
	std::vector<FunctionArgument> result;
	std::string buffer;
	for (auto &ch: argument_list) {
		if (ch == ',') {
			result.push_back(FunctionArgument(strip(buffer)));
			buffer.clear();
		} else {
			buffer.push_back(ch);
		}
	}
	if (buffer.size() > 0) {
		result.push_back(FunctionArgument(strip(buffer)));
	}
	return result;
}

struct FunctionSignature {
	std::string scope;
	std::string name;
	std::string return_type;
	std::vector<FunctionArgument> argument_list;
	FunctionSignature(const std::string &s, const std::string &line) 
		: scope(s) 
	{
		auto paranthesis_open = line.find('(');
		auto paranthesis_close = line.find(')');
		argument_list = split(line.substr(paranthesis_open+1, paranthesis_close-paranthesis_open-1));
		std::string returntype_and_name = line.substr(0,paranthesis_open);
		auto name_start = returntype_and_name.find_last_of(" &");
		name        = strip(line.substr(name_start+1,paranthesis_open-name_start-1));
		return_type = strip(line.substr(0,name_start+1));
	}
	void print() {
		std::cerr << "  Function        " << std::endl;
		std::cerr << "    scope       : " << scope << std::endl;
		std::cerr << "    name        : " << name << std::endl;
		std::cerr << "    return type : " << return_type << std::endl;
		std::cerr << "    arguments   : ";
		for (auto &argument: argument_list) {
			std::cerr << argument.declaration() << ", ";
		}
		std::cerr << std::endl;
	}
};

struct SignalSignature {
	std::string scope;
	std::string name;
	std::vector<FunctionArgument> argument_list;
	SignalSignature(const std::string &s, const std::string &line) 
		: scope(s) 
	{
		auto paranthesis_open = line.find('(');
		auto paranthesis_close = line.find(')');
		auto template_close = line.find('>');
		auto semicolon = line.find(';');
		argument_list = split(line.substr(paranthesis_open+1, paranthesis_close-paranthesis_open-1));
		name = strip(line.substr(template_close+1,semicolon-template_close-1));
	}
	void print() {
		std::cerr << "  Signal        " << std::endl;
		std::cerr << "    scope     : " << scope << std::endl;
		std::cerr << "    name      : " << name << std::endl;
		std::cerr << "    arguments : ";
		for (auto &argument: argument_list) {
			std::cerr << argument.declaration() << ", ";
		}
		std::cerr << std::endl;
	}
};

std::vector<std::string> split_bases(std::string argument_list) {
	std::vector<std::string> result;
	std::string buffer;
	for (auto &ch: argument_list) {
		if (ch == ',' || ch == '{' || ch == ' ') {
			result.push_back(strip(buffer));
			if (result.back() == "public" || result.back() == "") {
				result.pop_back();
			}
			buffer.clear();
			if (ch == '{') {
				return result;
			}
		} else {
			buffer.push_back(ch);
		}
	}
	return result;
}
struct ClassDefinition {
	std::string scope;
	std::string name;
	std::vector<std::string> bases;       // direct base classes
	std::vector<ClassDefinition*> direct_bases;// direct base classes
	std::vector<ClassDefinition*> all_bases;   // base classes and base classes of them
	std::vector<FunctionSignature> exportedfunctions;
	std::vector<SignalSignature> exportedsignals;
	ClassDefinition(const std::string &scope_, const std::string &line) 
		: scope(scope_)
	{
		auto colon_pos = line.find(':');
		if (colon_pos == line.npos) {
			// no base classes
			std::istringstream lin(line);
			lin >> name;
			if (name.back()=='{') {
				name.pop_back();
			}
		} else {
			std::istringstream lin(line.substr(0,colon_pos));
			lin >> name;
			std::string base_list = line.substr(colon_pos+1);
			bases = split_bases(base_list);
			std::sort(bases.begin(), bases.end());
		}
	}

	bool operator==(std::string &n) {
		return n == this->name;
	}

	bool has_exports() {
		return exportedfunctions.size() > 0 || exportedsignals.size() > 0;
	}
	std::vector<ClassDefinition*> generate_all_bases(std::vector<ClassDefinition> &class_definitions) {
		std::vector<ClassDefinition*> result;
		for (auto &base: bases) {
			ClassDefinition* base_class_definition = nullptr;
			for (auto &class_definition: class_definitions) {
				if (class_definition.name == base) {
					base_class_definition = &class_definition;
					break;
				}
			}
			if (base_class_definition != nullptr) {
				result.push_back(base_class_definition);
				for (auto &bbase: base_class_definition->generate_all_bases(class_definitions)) {
					result.push_back(bbase);
				}
			}
		}
		return result;
	}

	std::vector<ClassDefinition*> generate_direct_bases(std::vector<ClassDefinition> &class_definitions) {
		std::vector<ClassDefinition*> result;
		for (auto &base: bases) {
			ClassDefinition* base_class_definition = nullptr;
			for (auto &class_definition: class_definitions) {
				if (class_definition.name == base) {
					base_class_definition = &class_definition;
					break;
				}
			}
			if (base_class_definition != nullptr) {
				result.push_back(base_class_definition);
			}
		}
		return result;
	}


	void finalize(std::vector<ClassDefinition> &class_definitions) {
		all_bases    = generate_all_bases(class_definitions);
		direct_bases = generate_direct_bases(class_definitions);
	}
	void print() {
		std::cerr << "ClassDefinition: " << std::endl;
		std::cerr << "  scope: " << scope << std::endl;
		std::cerr << "  name : " << name  << std::endl;
		if (bases.size() > 0) {
			std::cerr << "  bases names: ";
			for (auto &base: bases) {
				std::cerr << base << " , ";
			}
			std::cerr << std::endl;
		}
		if (all_bases.size() > 0) {
			std::cerr << "     direct bases: ";
			for (auto &base: direct_bases) {
				std::cerr << base->name << " , ";
			}
			std::cerr << std::endl;
		}
		if (all_bases.size() > 0) {
			std::cerr << "     all bases: ";
			for (auto &base: all_bases) {
				std::cerr << base->name << " , ";
			}
			std::cerr << std::endl;
		}
		for (auto &function: exportedfunctions) {
			function.print();
			std::cerr << std::endl;
		}
		for (auto &signal: exportedsignals) {
			signal.print();
			std::cerr << std::endl;
		}
	}
};


static std::vector<ClassDefinition> cpp_parser(const std::string &source_name, std::map<std::string, std::string> &defines, std::vector<ClassDefinition> &classes, const std::vector<std::string> &include_paths)
{
	std::ifstream in(source_name);
	if (!in) {
		std::ostringstream msg;
		msg << "cannot open file " << source_name;
		throw std::runtime_error(msg.str());
	}

	if (verbose) {
		std::cerr << "parsing config file " << source_name << std::endl;
	}

	bool block_comment = false;
	std::vector<std::string> scope;
	std::string latest_scope_name;

	std::string function_or_signal_signature;
	ExportTag saftbus_export_tag_in_previous_line = NO_EXPORT;

	std::string class_definition;
	bool in_class_definition = false;
	// parse the file line by line
	for (int line_no = 1; ; ++line_no) {
		std::string line;
		std::getline(in, line);
		if (!in) {
			if (block_comment) {
				std::ostringstream msg;
				msg << "parsing error in " << source_name << ":" << line_no << ": block comment not closed at end of input";
				throw std::runtime_error(msg.str());
			}
			break;
		}

		// remove line comments and detect @saftbus-export tag (is has to be the first word in a line comment)
		ExportTag saftbus_export_tag = remove_line_comments(line);

		// detect and remove block_comments
		remove_block_comments(line,block_comment);

		// track scope level
		manage_scopes(line, scope, latest_scope_name);
		// std::cerr << line_no << " " << build_namespace(scope) << std::endl;

		// extract signal or function signature
		if (saftbus_export_tag_in_previous_line == SIGNAL_EXPORT) {
			function_or_signal_signature.append(line);
			if (line.find(';') == line.npos) { // cant find the closing ";" of the function declaration
				continue;
			} else {	
				if (verbose) {
					std::cerr << line_no << ": extract signal signature: " << std::endl;
				}
				if (classes.size() > 0) {
					classes.back().exportedsignals.push_back(SignalSignature(build_namespace(scope),function_or_signal_signature));
				}
			}
			function_or_signal_signature = "";
		}
		if (saftbus_export_tag_in_previous_line == FUNCTION_EXPORT) {
			function_or_signal_signature.append(line);
			if (line.find(';') == line.npos && line.find('{') == line.npos) { // cant find the closing ";" of the function declaration or the start of the definition block '{'
				continue;
			} else {	
				if (verbose) {
					std::cerr << line_no << ": extract function signature: " << std::endl;
				}
				if (classes.size() > 0) {
					classes.back().exportedfunctions.push_back(FunctionSignature(build_namespace(scope),function_or_signal_signature));
				}
			}
			function_or_signal_signature = "";
		}
		saftbus_export_tag_in_previous_line = saftbus_export_tag;


		if (in_class_definition) {
			class_definition.append(line);
			if (class_definition.find('{') != class_definition.npos) {
				classes.push_back(ClassDefinition(build_namespace(scope), class_definition));
				in_class_definition = false;
			}
			continue;
		}



		// input stream for one line without comments
		std::istringstream lin(line);


		// skip empty lines
		char first_ch;
		lin >> first_ch; 
		if (!lin) {
			continue;
		}
		lin.putback(first_ch);

		std::string keyword;
		lin >> keyword;
		if (!lin) {
			std::ostringstream msg;
			msg << "parsing error in " << source_name << ":" << line_no << ": expect a keyword \"source\", \"alias\"";
			throw std::runtime_error(msg.str());
		}
		if (keyword == "class") {
			std::getline(lin, class_definition);
			if (class_definition.find('{') != class_definition.npos) {
				classes.push_back(ClassDefinition(build_namespace(scope), class_definition));
			} else {
				if (class_definition.find(';') == class_definition.npos) { // don't react on class declarations "class MyClass;"
					in_class_definition = true;
				}
			}
		} else if (keyword == "#define") {
			std::string define_name;
			lin >> define_name;
			if (!lin) {
				std::ostringstream msg;
				msg << "parsing error in " << source_name << ":" << line_no << ": <name> expected after keyword \"" << keyword << "\"";
				throw std::runtime_error(msg.str());
			}
			std::string define_replacement;
			std::getline(lin, define_replacement);
			//strip trailing whitespaces
			while(std::isspace(define_replacement.back())) {
				define_replacement.pop_back();
			}
			while(std::isspace(define_replacement.front())) {
				define_replacement = define_replacement.substr(1);
			}
			if (defines.find(define_name) != defines.end()) {
				std::ostringstream msg;
				msg << "parsing error in " << source_name << ":" << line_no << ": #define \"" << define_name << "\" was already defined";
				throw std::runtime_error(msg.str());
			}
			defines[define_name] = define_replacement;

		} else if (keyword == "#ifndef") {
			std::string define_name;
			lin >> define_name;
			if (!lin) {
				std::ostringstream msg;
				msg << "parsing error in " << source_name << ":" << line_no << ": <name> expected after keyword \"" << keyword << "\"";
				throw std::runtime_error(msg.str());
			}
			if (defines.find(define_name) != defines.end()) { // this was defined already -> read and forget input until reaching #endif
				// std::cerr << "found #ifndef, read until #endif" << std::endl;
				for (;;) {
					++line_no;
					std::string line;
					std::getline(in, line);
					if (!in) {
						std::ostringstream msg;
						msg << "parsing error in " << source_name << ":" << line_no << ": unexpected end of input";
						throw std::runtime_error(msg.str());
					}
					// std::cerr << "line " << line_no << " : " << line << std::endl;
					remove_line_comments(line);
					std::istringstream lin(line);
					std::string endif;
					lin >> endif;
					if (endif == "#endif") {
						break;
					}
				}
			}			
		} else if (keyword == "#endif") {
			// nothing to do here
		} else 
		if (keyword == "#include") {
			char ch;
			lin >> ch;
			if (!lin) {
				std::ostringstream msg;
				msg << "parsing error in " << source_name << ":" << line_no << ": expect \"filename\" after \"#include\"";
				throw std::runtime_error(msg.str());
			}
			char include_open = ch;
			if (ch != '\"' && ch != '<') {
				std::ostringstream msg;
				msg << "parsing error in " << source_name << ":" << line_no << ": filename has to be included in doulbe quotes";
				throw std::runtime_error(msg.str());
			}
			std::string include_filename;
			for(;;) {
				ch = lin.get();
				if (!lin) {
					std::ostringstream msg;
					msg << "parsing error in " << source_name << ":" << line_no << ": expecting \" after #include \"" << include_filename;
					throw std::runtime_error(msg.str());
				}
				if (include_open == '\"' && ch == '\"') {
					break;
				}
				if (include_open == '<' && ch == '>') {
					break;
				}
				include_filename.push_back(ch);
			}
			if (include_filename.empty()) {
				std::ostringstream msg;
				msg << "parsing error in " << source_name << ":" << line_no << ": empty include filname";
				throw std::runtime_error(msg.str());
			}
			if (include_open == '<') {
				continue; // dont open sytem headers
			}
			std::string original_include_filename = include_filename;
			if (source_name.find('/') != source_name.npos && include_filename[0] != '/') {
				auto last_slash_pos = source_name.find_last_of('/');
				include_filename = source_name.substr(0,last_slash_pos+1) + include_filename;
			}
			// test if the file can be opened...

			std::ifstream included_file_in(include_filename);
			if (!included_file_in) {
				std::cerr << "cannot open file: " << include_filename << std::endl;
				// see if it can be opened in one of the include paths
				bool success = false;
				std::cerr << "check include paths " << include_paths.size() << std::endl;
				for (auto &include_path: include_paths) {
					std::string path_and_include_filename = include_path + "/" + original_include_filename;
					std::cerr << "trying to open " << path_and_include_filename << std::endl;
					std::ifstream path_and_included_file_in(path_and_include_filename.c_str());
					if (!path_and_included_file_in) {
						std::cerr << "cannot open file: " << path_and_include_filename << std::endl;
					} else {
						if (verbose) {
							std::cerr << "recursively parse " << path_and_include_filename << std::endl;
						}
						cpp_parser(path_and_include_filename, defines, classes, include_paths);
						success = true;
						break;
					}
				}
				if (!success) {
					std::ostringstream msg;
					msg << "parsing error in " << source_name << ":" << line_no << ": cannot open file \"" << include_filename << "\"";
					throw std::runtime_error(msg.str());
				}
			} else {
				if (verbose) {
				//... if it can be opened, call the parse function recursively
					std::cerr << "recursively parse " << include_filename << std::endl;
				}
				cpp_parser(include_filename, defines, classes, include_paths);
			}
		} else if (!saftbus_export_tag) {
			// do nothing
		} else {
			std::ostringstream msg;
			msg << "parsing error in " << source_name << ":" << line_no << ": unexpected keyword \"" << keyword << "\"";
			throw std::runtime_error(msg.str());
		}
	}
	return classes;
}

void move_file_if_not_identical(const std::string &source_file, const std::string &dest_file) {
	std::ifstream in1(source_file.c_str());
	std::ifstream in2(dest_file.c_str());
	bool identical = true;
	for (;;) {
		char ch1, ch2;
		// in1 >> ch1;
		// in2 >> ch2;
		ch1 = in1.get();
		ch2 = in2.get();
		if (!in1 && !in2) {
			// both files ended at the same time
			break;
		}
		if (!in1 || !in2) {
			// only one file endend
			identical = false;
			break;
		}
		if (ch1 != ch2) {
			// file content differs
			identical = false;
			break;
		}
	}
	if (identical) {
		// remove the source_file
		remove(source_file.c_str());
		return;
	}
	// move the file using a syscall
	in1.close();
	in2.close();
	rename(source_file.c_str(), dest_file.c_str());
}


void generate_service_header(const std::string &outputdirectory, ClassDefinition &class_definition) {
	std::string header_filename = outputdirectory;
	if (header_filename.size()) {
		header_filename.append("/");
	}
	header_filename.append(class_definition.name);
	header_filename.append("_Service.hpp");
	//std::ofstream header_out(header_filename.c_str());
	std::string tmp_filename = header_filename;
	tmp_filename.append(".tmp");
	std::ofstream header_out(tmp_filename);

	header_out << "#ifndef " << class_definition.name << "_SERVICE_HPP_" << std::endl;
	header_out << "#define " << class_definition.name << "_SERVICE_HPP_" << std::endl;
	header_out << std::endl;
	header_out << "#include <saftbus/service.hpp>" << std::endl;
	header_out << "#include <saftbus/saftbus.hpp>" << std::endl;
	header_out << std::endl;
	header_out << "#include <functional>" << std::endl;
	header_out << std::endl;

	header_out << "namespace " << class_definition.scope.substr(0, class_definition.scope.size()-class_definition.name.size()-2) << " {" << std::endl;
	header_out << std::endl;

	header_out << "\tclass " << class_definition.name << ";" << std::endl;
	header_out << "\tclass " << class_definition.name << "_Service : public saftbus::Service {" << std::endl;
	header_out << "\t\tstd::unique_ptr<" << class_definition.name << "> d;" << std::endl;
	header_out << "\t\t" << "static std::vector<std::string> gen_interface_names();" << std::endl;
	header_out << "\tpublic:" << std::endl;

	header_out << "\t\t" <<        class_definition.name << "_Service(std::unique_ptr<" << class_definition.name <<"> instance);" << std::endl;
	header_out << "\t\t" <<        class_definition.name << "_Service();" << std::endl;
	header_out << "\t\t" << "~" << class_definition.name << "_Service();" << std::endl;
	header_out << "\t\t" << "void call(unsigned interface_no, unsigned function_no, int client_fd, saftbus::Deserializer &received, saftbus::Serializer &send);" << std::endl;
	header_out << std::endl;

	// function declaration for the functions that will be connected to the signals (e.g. std::function objects)
	std::vector<ClassDefinition*> class_and_all_base_classes;
	class_and_all_base_classes.push_back(&class_definition);
	class_and_all_base_classes.insert(class_and_all_base_classes.end(), class_definition.all_bases.begin(), class_definition.all_bases.end());
	for (auto& class_definition: class_and_all_base_classes) {
		for (auto &signal: class_definition->exportedsignals) {
			header_out << "\t\t" << "void " << signal.name << "_dispatch_function(";
			for (unsigned i  = 0; i < signal.argument_list.size(); ++i) {
				header_out << signal.argument_list[i].definition();
				if (i < signal.argument_list.size()-1) {
					header_out << ',';
				}
			}
			header_out << ");" << std::endl;
		}
	}

	header_out << "\t};" << std::endl;

	header_out << std::endl;
	header_out << "}" << std::endl;
	header_out << std::endl;

	header_out << "#endif" << std::endl;

	header_out.close();
  
	move_file_if_not_identical(tmp_filename, header_filename);
}

void generate_service_implementation(const std::string &outputdirectory, ClassDefinition &class_definition) {
	std::string filename = outputdirectory;
	if (filename.size()) {
		filename.append("/");
	}
	filename.append(class_definition.name);
	filename.append("_Service.cpp");
	// std::ofstream out(filename.c_str());
	std::string tmp_filename = filename;
	tmp_filename.append(".tmp");
	std::ofstream out(tmp_filename);

	out << "#include \"" << class_definition.name << ".hpp\"" << std::endl;
	out << "#include \"" << class_definition.name << "_Service.hpp\"" << std::endl;
	out << "#include <saftbus/make_unique.hpp> " << std::endl;
	out << std::endl;

	out << "namespace " << class_definition.scope.substr(0, class_definition.scope.size()-class_definition.name.size()-2) << " {" << std::endl;
	out << std::endl;

	std::vector<ClassDefinition*> class_and_all_base_classes;
	class_and_all_base_classes.push_back(&class_definition);
	class_and_all_base_classes.insert(class_and_all_base_classes.end(), class_definition.all_bases.begin(), class_definition.all_bases.end());

	out << "\t" << "std::vector<std::string> " << class_definition.name << "_Service::gen_interface_names() {" << std::endl;
	out << "\t\t" << "std::vector<std::string> result; " << std::endl;
	out << "\t\t" << "result.push_back(\"" << class_definition.name << "\");" << std::endl;
	for (auto &base: class_definition.all_bases) {
		out << "\t\t" << "result.push_back(\"" << base->name << "\");" << std::endl;
	}
	out << "\t\t" << "return result;" << std::endl;
	out << "\t" << "}" << std::endl;

	// constructor with instance pointer
	out << "\t" << class_definition.name << "_Service::" << class_definition.name << "_Service(std::unique_ptr< " << class_definition.name << "> instance) " << std::endl;
	out << "\t" << ": saftbus::Service(gen_interface_names()), d(std::move(instance))" << std::endl;
	out << "\t" << "{" << std::endl;
	for (auto& class_def: class_and_all_base_classes) {
		for (auto &signal: class_def->exportedsignals) {
			out << "\t\t" << "d->" << signal.name << " = std::bind(&" << class_definition.name << "_Service::" << signal.name << "_dispatch_function, this";
			for (unsigned i = 0; i < signal.argument_list.size(); ++i) {
				out << ", std::placeholders::_" << i+1;
			}
			out << ");" << std::endl;
		}
	}
	out << "\t" << "}" << std::endl;


	out << "\t" << class_definition.name << "_Service::~" << class_definition.name << "_Service() " << std::endl;
	out << "\t" << "{}" << std::endl;

	out << "\t" << "void " << class_definition.name << "_Service::call(unsigned interface_no, unsigned function_no, int client_fd, saftbus::Deserializer &received, saftbus::Serializer &send) {" << std::endl;
	out << "\t\ttry {" << std::endl;
	out << "\t\tswitch(interface_no) {" << std::endl;

	for (unsigned interface_no = 0; interface_no < class_and_all_base_classes.size(); ++interface_no) {

		out << "\t\t\t" << "case " << interface_no << ": " << std::endl;
		out << "\t\t\tswitch(function_no) {" << std::endl;
		for (unsigned function_no  = 0; function_no  < class_and_all_base_classes[interface_no]->exportedfunctions.size(); ++function_no ) {
			auto &function = class_and_all_base_classes[interface_no]->exportedfunctions[function_no];
			out << "\t\t\t\t" << "case " << function_no << ": {" << std::endl;
			for (unsigned i = 0; i < function.argument_list.size(); ++i) {
				std::string type = function.argument_list[i].type;
				if (type.find("&") != type.npos) { // remove the reference from type signature (remove the "&")
					type = type.substr(0,type.find("&"));
				}
				if (type.find("const") != type.npos) { // remove const 
					type = type.substr(type.find("const")+6);
				}
				out << "\t\t\t\t\t" << type << " " << function.argument_list[i].name << ";" << std::endl;
			}
			for (unsigned i = 0; i < function.argument_list.size(); ++i) {
				if (function.argument_list[i].is_output == false) {
					out << "\t\t\t\t\t" << "received.get(" << function.argument_list[i].name << ");" << std::endl;
				} 
			}
			if (function.return_type != "void") {
				out << "\t\t\t\t\t" << function.return_type << " function_call_result = " << "d->" << function.name << "(";	
			} else {
				out << "\t\t\t\t\t" << "d->" << function.name << "(";
			}
			for (unsigned i = 0; i < function.argument_list.size(); ++i) {
				out << function.argument_list[i].name;
				if (i != function.argument_list.size()-1) {
					out << ", ";
				}
			}
			out << ");" << std::endl;
			out << "\t\t\t\t\t" << "send.put(saftbus::FunctionResult::RETURN);" << std::endl;
			for (unsigned i = 0; i < function.argument_list.size(); ++i) {
				if (function.argument_list[i].is_output == true) {
					out << "\t\t\t\t\t" << "send.put(" << function.argument_list[i].name << ");" << std::endl;
				} 
			}
			if (function.return_type != "void") {
				out << "\t\t\t\t\t" << "send.put(function_call_result);" << std::endl;
			}
			out << "\t\t\t\t" << "} return;" << std::endl;
		}
		out << "\t\t\t};" << std::endl;
	}

	out << std::endl;
	out << "\t\t};" << std::endl;

	out << "\t\t} catch (std::runtime_error &e) {" << std::endl;
	out << "\t\t\t" << "send.put(saftbus::FunctionResult::EXCEPTION);" << std::endl;
	out << "\t\t\t" << "std::string what(e.what());" << std::endl;
	out << "\t\t\t" << "send.put(what);" << std::endl;
	out << "\t\t} catch (...) {" << std::endl;
	out << "\t\t\t" << "send.put(saftbus::FunctionResult::EXCEPTION);" << std::endl;
	out << "\t\t\t" << "std::string what(\"unknown exception\");" << std::endl;
	out << "\t\t\t" << "send.put(what);" << std::endl;
	out << "\t\t} "<< std::endl;

	out << "\t}" << std::endl;
	out << std::endl;

	// signal dispatch functions
	// for (auto &signal: class_definition.exportedsignals) {
	// 	out << "\t\t" << "d->" << signal.name << " = std::bind(&" << class_definition.name << "_Service::" << signal.name << "_dispatch_function, this";
	// 	for (unsigned i = 0; i < signal.argument_list.size(); ++i) {
	// 		out << ", std::placeholders::_" << i+1;
	// 	}
	// 	out << ");" << std::endl;
	// }
	for (int interface_no = 0; interface_no < static_cast<int>(class_and_all_base_classes.size()); ++interface_no) {
		auto &class_def = class_and_all_base_classes[interface_no];
		for (int signal_no = 0; signal_no < static_cast<int>(class_def->exportedsignals.size()); ++signal_no) {
			auto &signal = class_def->exportedsignals[signal_no];
			out << "\t" << "void " << class_definition.name << "_Service::" << signal.name << "_dispatch_function(";
			for (unsigned i  = 0; i < signal.argument_list.size(); ++i) {
				out << signal.argument_list[i].definition();
				if (i < signal.argument_list.size()-1) {
					out << ", ";
				}
			}
			out << ") {" << std::endl;
			out << "\t\t" << "std::cerr << \"service dispatch function called!!!!!!!!!!!!\" << std::endl;" << std::endl;
			out << "\t\t" << "saftbus::Serializer serialized_signal;" << std::endl;
			out << "\t\t" << "serialized_signal.put(get_object_id());" << std::endl;
			out << "\t\t" << "serialized_signal.put(" << interface_no    << ");" << std::endl;
			out << "\t\t" << "serialized_signal.put(" << signal_no       << ");" << std::endl;
			for (unsigned i = 0; i < signal.argument_list.size(); ++i) {
				out << "\t\t" << "serialized_signal.put(" << signal.argument_list[i].name << ");" << std::endl;
			}
			out << "\t\t" << "emit(serialized_signal);" << std::endl;
			out << "\t" << "}" << std::endl;
		}
	}


	out << std::endl;
	out << "}" << std::endl;
	out << std::endl;

	out.close();
	move_file_if_not_identical(tmp_filename, filename);	
}


void generate_proxy_header(const std::string &outputdirectory, ClassDefinition &class_definition) {
	std::string header_filename = outputdirectory;
	if (header_filename.size()) {
		header_filename.append("/");
	}
	header_filename.append(class_definition.name);
	header_filename.append("_Proxy.hpp");
	// std::ofstream header_out(header_filename.c_str());
	std::string tmp_filename = header_filename;
	tmp_filename.append(".tmp");
	std::ofstream header_out(tmp_filename);

	header_out << "#ifndef " << class_definition.name << "_PROXY_HPP_" << std::endl;
	header_out << "#define " << class_definition.name << "_PROXY_HPP_" << std::endl;
	header_out << std::endl;
	header_out << "#include <saftbus/client.hpp>" << std::endl;	
	header_out << std::endl;
	header_out << "#include <functional>" << std::endl;	
	header_out << std::endl;

	std::string include_prefix("");
	if (outputdirectory != "") {
		include_prefix = outputdirectory;
		include_prefix.append("/");
	}
	for (auto &base: class_definition.direct_bases) {
		if (base->has_exports()) {
			header_out << "#include \"" << include_prefix << base->name << "_Proxy.hpp\"" << std::endl;
		}
	}
	header_out << std::endl;

	header_out << "namespace " << class_definition.scope.substr(0, class_definition.scope.size()-class_definition.name.size()-2) << " {" << std::endl;
	header_out << std::endl;

	header_out << "\tclass " << class_definition.name << "_Proxy ";
	int base_count = 0;
	if (class_definition.bases.size()) {
		for (unsigned i = 0; i < class_definition.direct_bases.size(); ++i) {
			if (class_definition.direct_bases[i]->has_exports()) {
				header_out <<  (i?',':':') << " public " << " " << class_definition.direct_bases[i]->name << "_Proxy ";
				++base_count;
			}
		}
	} 
	if (base_count == 0) {
		header_out << ": public virtual saftbus::Proxy" << std::endl;
	}
	header_out << " { " << std::endl;
	header_out << "\t\t" << "static std::vector<std::string> gen_interface_names();" << std::endl;
	header_out << "\tpublic:" << std::endl;
	header_out << "\t\t" << class_definition.name << "_Proxy(const std::string &object_path, saftbus::SignalGroup &signal_group, const std::vector<std::string> &interface_names = std::vector<std::string>());" << std::endl;
	header_out << "\t\t" << "static std::shared_ptr<" << class_definition.name << "_Proxy> create(const std::string &object_path, saftbus::SignalGroup &signal_group = saftbus::SignalGroup::get_global(), const std::vector<std::string> &interface_names = std::vector<std::string>());" << std::endl;
	header_out << "\t\t" << "bool signal_dispatch(int interface_no, int signal_no, saftbus::Deserializer &signal_content);" << std::endl;
	for (auto &function: class_definition.exportedfunctions) {
		header_out << "\t\t" << function.return_type << " " << function.name << "(";
		for (unsigned i = 0; i < function.argument_list.size(); ++i) {
			header_out << function.argument_list[i].declaration();
			if (i != function.argument_list.size()-1) {
				header_out << ", ";
			}
		}
		header_out << ");" << std::endl;
	}

	// signals
	for (auto &signal: class_definition.exportedsignals) {
		header_out << "\t\t" << "std::function<void(";
		for (unsigned i = 0; i < signal.argument_list.size(); ++i) {
			if (i > 0) {
				header_out << ", ";
			}
			header_out << signal.argument_list[i].definition();
		}
		header_out << ")> " << signal.name << ";" << std::endl;
	}



	header_out << "\tprivate:" << std::endl;
	header_out << "\t\tint interface_no;" << std::endl;

	header_out << std::endl;

	header_out << "\t};" << std::endl;

	header_out << std::endl;
	header_out << "}" << std::endl;
	header_out << std::endl;

	header_out << "#endif" << std::endl;

	header_out.close();
	move_file_if_not_identical(tmp_filename, header_filename);
}


void generate_proxy_implementation(const std::string &outputdirectory, ClassDefinition &class_definition) {
	std::string cpp_filename = outputdirectory;
	if (cpp_filename.size()) {
		cpp_filename.append("/");
	}
	cpp_filename.append(class_definition.name);
	cpp_filename.append("_Proxy.cpp");
	// std::ofstream cpp_out(cpp_filename.c_str());
	std::string tmp_filename = cpp_filename;
	tmp_filename.append(".tmp");
	std::ofstream cpp_out(tmp_filename);

	cpp_out << "#include \"" << class_definition.name << "_Proxy.hpp\"" << std::endl;
	cpp_out << "#include <saftbus/saftbus.hpp>" << std::endl;
	cpp_out << "#include <saftbus/make_unique.hpp>" << std::endl;
	cpp_out << "#include <cassert>" << std::endl;
	cpp_out << std::endl;
	// cpp_out << "namespace " << class_definition.scope.substr(0, class_definition.scope.size()-class_definition.name.size()-2) << " {" << std::endl;
	// cpp_out << std::endl;

	cpp_out << "namespace " << class_definition.scope.substr(0, class_definition.scope.size()-class_definition.name.size()-2) << " {" << std::endl;
	cpp_out << std::endl;

	cpp_out << "\t" << "std::vector<std::string> " << class_definition.name << "_Proxy::gen_interface_names() {" << std::endl;
	cpp_out << "\t\t" << "std::vector<std::string> result; " << std::endl;
	cpp_out << "\t\t" << "result.push_back(\"" << class_definition.name << "\");" << std::endl;
	for (auto &base: class_definition.all_bases) {
		if (base->has_exports()) {
			cpp_out << "\t\t" << "result.push_back(\"" << base->name << "\");" << std::endl;
		}
	}
	cpp_out << "\t\t" << "return result;" << std::endl;
	cpp_out << "\t" << "}" << std::endl;

	cpp_out << class_definition.name << "_Proxy::" << class_definition.name << "_Proxy(const std::string &object_path, saftbus::SignalGroup &signal_group, const std::vector<std::string> &interface_names)" << std::endl;
	cpp_out << "\t" << ": saftbus::Proxy(object_path, signal_group, interface_names)" << std::endl;
	if (class_definition.bases.size()) {
		for (unsigned i = 0; i < class_definition.direct_bases.size(); ++i) {
			if (class_definition.direct_bases[i]->has_exports()) {
				cpp_out << (true?"\t, ":"") << class_definition.direct_bases[i]->name << "_Proxy(object_path, signal_group, interface_names) " << std::endl;
			}
		}
	}
	cpp_out << "{" << std::endl;
	cpp_out << "\tinterface_no = saftbus::Proxy::interface_no_from_name(\"" << class_definition.name << "\");" << std::endl;
	cpp_out << "}" << std::endl;
	cpp_out << "std::shared_ptr<" << class_definition.name << "_Proxy> " << class_definition.name << "_Proxy::create(const std::string &object_path, saftbus::SignalGroup &signal_group, const std::vector<std::string> &interface_names) {" << std::endl;
	cpp_out << "\t" << "return std2::make_unique<" << class_definition.name << "_Proxy>(object_path, signal_group, gen_interface_names()); " << std::endl;
	cpp_out << "}" << std::endl;
	cpp_out << "\t"     << "bool " << class_definition.name << "_Proxy::signal_dispatch(int interface_no, int signal_no, saftbus::Deserializer &signal_content) {" << std::endl;
	cpp_out << "\t\t"   <<   "if (interface_no == this->interface_no) {" << std::endl;
	if ( class_definition.exportedsignals.size() > 0) {
		cpp_out << "\t\t\t"   <<   "switch(signal_no) {" << std::endl;
		for (unsigned signal_no = 0; signal_no < class_definition.exportedsignals.size(); ++signal_no) {
			auto &signal = class_definition.exportedsignals[signal_no];
			cpp_out << "\t\t\t" << "case " << signal_no << ": {" << std::endl;
			for (unsigned i = 0; i < signal.argument_list.size(); ++i) {
				cpp_out << "\t\t\t\t" << signal.argument_list[i].definition() << ";" << std::endl;
				cpp_out << "\t\t\t\t" << "signal_content.get(" << signal.argument_list[i].name << ");" << std::endl;
			}
			cpp_out << "\t\t\t\t" << "if (" << signal.name << ") " << signal.name << "(";
			for (unsigned i = 0; i < signal.argument_list.size(); ++i) {
				if (i > 0) {
					cpp_out << ", ";
				}
				cpp_out << signal.argument_list[i].name;
			}
			cpp_out << ");" << std::endl;
			cpp_out << "\t\t\t\t" <<     "// execute signal e.g. std::function" << std::endl;
			cpp_out << "\t\t\t" <<     "} return true;" << std::endl;
		}
		cpp_out << "\t\t\t"   <<   "}" << std::endl;
	}
	cpp_out << "\t\t"   <<   "}" << std::endl;
	for (auto & base: class_definition.direct_bases) {
		if (base->has_exports()) {
			cpp_out << "\t\t"   <<   "if (" << base->name << "_Proxy::signal_dispatch(interface_no, signal_no, signal_content)) return true;" << std::endl;
		}
	}
	cpp_out << "\t\t"     <<   "return false;" << std::endl;
	cpp_out << "\t"     << "}" << std::endl;


	for (unsigned function_no  = 0; function_no  < class_definition.exportedfunctions.size(); ++function_no ) {
		auto &function = class_definition.exportedfunctions[function_no];
		cpp_out << function.return_type << " " << class_definition.name << "_Proxy::" << function.name << "(";
		for (unsigned i = 0; i < function.argument_list.size(); ++i) {
			cpp_out << function.argument_list[i].definition();
			if (i != function.argument_list.size()-1) {
				cpp_out << ", ";
			}
		}
		cpp_out << ") {" << std::endl;
		cpp_out << "\t" << "get_send().put(get_saftlib_object_id());" << std::endl;
		cpp_out << "\t" << "get_send().put(interface_no);" << std::endl;
		cpp_out << "\t" << "get_send().put(" << function_no  << "); // function_no" << std::endl;
		int num_outputs = 0;
		if (function.return_type != "void") {
			num_outputs = 1;
		}
		for (unsigned i = 0; i < function.argument_list.size(); ++i) {
			if (function.argument_list[i].is_output == false) {
				cpp_out << "\t" << "get_send().put(" << function.argument_list[i].name << ");" << std::endl;
			} else {
				++num_outputs;
			}
		}
		cpp_out << "\t{" << std::endl;
		cpp_out << "\t\t" << "std::lock_guard<std::mutex> lock(get_client_socket());" << std::endl;
		cpp_out << "\t\t" << "get_connection().send(get_send());" << std::endl;
		cpp_out << "\t\t" << "get_connection().receive(get_received());" << std::endl;
		cpp_out << "\t}" << std::endl;

		cpp_out << "\t" << "saftbus::FunctionResult function_result_;" << std::endl;
		cpp_out << "\t" << "get_received().get(function_result_);" << std::endl;
		cpp_out << "\t" << "if (function_result_ == saftbus::FunctionResult::EXCEPTION) {" << std::endl;
		cpp_out << "\t\t" << "std::string what;" << std::endl;
		cpp_out << "\t\t" << "get_received().get(what);" << std::endl;
		cpp_out << "\t\t" << "throw std::runtime_error(what);" << std::endl;
		cpp_out << "\t" << "}" << std::endl;
		cpp_out << "\t" << "assert(function_result_ == saftbus::FunctionResult::RETURN);" << std::endl;
		for (unsigned i = 0; i < function.argument_list.size(); ++i) {
			if (function.argument_list[i].is_output == true) {
				cpp_out << "\t" << "get_received().get(" << function.argument_list[i].name << ");" << std::endl;
			}
		}

		if (function.return_type != "void") {
			cpp_out << "\t" << function.return_type << " return_value_result_;" << std::endl;
			cpp_out << "\t" << "get_received().get(return_value_result_);" << std::endl;
			cpp_out << "\t" << "return return_value_result_;" << std::endl;
		}

		cpp_out << "}" << std::endl;
	}

	cpp_out << std::endl;
	cpp_out << "}" << std::endl;
	cpp_out << std::endl;

	cpp_out.close();
	move_file_if_not_identical(tmp_filename, cpp_filename);
}


int main(int argc, char **argv) 
{
	std::vector<std::string> include_paths;
	std::vector<std::string> source_files;
	std::string output_directory;

	for (int i = 1; i < argc; ++i) {
		std::string argvi = argv[i];
		if (argvi.substr(0,2) == "-I") {
			if (argvi.size() > 2) {
				include_paths.push_back(argvi.substr(2));
			} else {
				if (++i < argc) {
					std::cerr << "add include_path: " << argv[i] << std::endl;
					include_paths.push_back(argv[i]);
				} else {
					throw std::runtime_error("expecting pathname after \'-I\'");
				}
			}
		} else if (argvi == "-o") {
			if (++i < argc) {
				output_directory = argv[i];
			}
		} else if (argvi == "-v") {
			verbose = true;
		} else {
			source_files.push_back(argvi);
		}
	}

	// return 0;
	for (auto &source_file: source_files) {
		std::map<std::string, std::string> defines;
		std::vector<ClassDefinition> classes;
		cpp_parser(source_file, defines, classes, include_paths);

		for (auto &class_def: classes) {
			class_def.finalize(classes);
			if (verbose) {
				class_def.print();
			}
		}

		for (auto &class_def: classes) {
			if (class_def.exportedfunctions.size() > 0 || class_def.exportedsignals.size() > 0) {
				generate_service_header(output_directory, class_def);
				generate_service_implementation(output_directory, class_def);

				generate_proxy_header(output_directory, class_def);
				generate_proxy_implementation(output_directory, class_def);

			}

		}




	}

	return 0;
}