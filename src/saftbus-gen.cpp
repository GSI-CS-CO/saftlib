#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>
#include <map>

// return true if a saftbus export tag was found in a line comment 
static bool remove_line_comments(std::string &line) {
	std::string saftbus_export_tag = " @saftbus-export";
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
					return true;
				}
			}
			line = line.substr(0,1);
			return false;
		}
		previous_ch = line[i];
	}
	return false;
}

static void remove_block_comments(std::string &line, bool &block_comment) {
	bool in_string = false;
	char previous_ch = ' ';
	if (block_comment) {
		for (size_t i = 0; i < line.size(); ++i) {
			if (line[i] == '\"') in_string = !in_string;
			if (line[i] == '/' && previous_ch == '*' && !in_string) {
				line = line.substr(i+1);
				block_comment = false;
			}
			previous_ch = line[i];
		}
	} else {
		for (size_t i = 0; i < line.size(); ++i) {
			if (line[i] == '\"') in_string = !in_string;
			if (line[i] == '*' && previous_ch == '/' && !in_string) {
				line = line.substr(0,i-1);
				block_comment = true;
			}
			previous_ch = line[i];
		}
	}
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
			latest_name = "";
		}
		if (line[i] == '}') scope.pop_back();
		// namespace
		if (line[i] == 'n' && line.substr(i,9) == "namespace") {
			std::istringstream in(line.substr(i+9));
			std::string name;
			in >> name;
			if (!in) {
				throw std::runtime_error("identifier expected after \'namespace\'");
			}
			i += 9 + latest_name.size();
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
			i += 5 + latest_name.size();
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
			i += 6 + latest_name.size();
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
		std::cerr << "   scope         : " << scope << std::endl;
		std::cerr << "   function name : " << name << std::endl;
		std::cerr << "   return type   : " << return_type << std::endl;
		std::cerr << "   arguments     : ";
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
		if (ch == ',' || ch == '{') {
			result.push_back(strip(buffer));
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
	std::vector<std::string> bases;
	std::vector<FunctionSignature> exportedfunctions;
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
		}
	}
	void print() {
		std::cerr << "ClassDefinition: " << std::endl;
		std::cerr << "  scope: " << scope << std::endl;
		std::cerr << "  name : " << name  << std::endl;
		if (bases.size() > 0) {
			std::cerr << "  bases: ";
			for (auto &base: bases) {
				std::cerr << base << " , ";
			}
			std::cerr << std::endl;
		}
		for (auto &function: exportedfunctions) {
			function.print();
			std::cerr << std::endl;
		}
	}
};

std::vector<ClassDefinition> classes;

static void cpp_parser(const std::string &source_name, std::map<std::string, std::string> &defines)
{
	std::ifstream in(source_name);
	if (!in) {
		std::ostringstream msg;
		msg << "cannot open file " << source_name;
		throw std::runtime_error(msg.str());
	}

	std::cerr << "parsing config file " << source_name << std::endl;

	bool block_comment = false;
	std::vector<std::string> scope;
	std::string latest_scope_name;

	std::string function_signature;
	bool saftbus_export_tag_in_last_line = false;

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
		bool saftbus_export_tag = remove_line_comments(line);

		// detect and remove block_comments
		remove_block_comments(line,block_comment);
		if (block_comment) {
			continue;
		}

		// extract function signature
		if (saftbus_export_tag_in_last_line) {
			function_signature.append(line);
			if (line.find(';') == line.npos && line.find('{') == line.npos) { // cant find the closing ";" of the function declaration or the start of the definition block '{'
				continue;
			} else {	
				std::cerr << line_no << ": extract function signature: " << std::endl;
				if (classes.size() > 0) {
					classes.back().exportedfunctions.push_back(FunctionSignature(build_namespace(scope),function_signature));
				}
			}
			function_signature = "";
		}
		saftbus_export_tag_in_last_line = saftbus_export_tag;

		// track scope level
		manage_scopes(line, scope, latest_scope_name);
		// std::cerr << line_no << " " << build_namespace(scope) << std::endl;

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
				in_class_definition = true;
			}
			// std::string classname; 
			// lin >> classname;
			// if (!lin) {
			// 	std::ostringstream msg;
			// 	msg << "parsing error in " << source_name << ":" << line_no << ": <classname> expected after \"" << keyword << "\"";
			// 	throw std::runtime_error(msg.str());
			// }
			// char colon_or_comma;
			// std::vector<std::string> basenames;
			// for (;;) {
			// 	lin >> colon_or_comma;
			// 	if (!lin || colon_or_comma == '{') {
			// 		break;
			// 	} else {
			// 		std::string basename;
			// 		for (;;) {
			// 			lin >> basename;
			// 			if (!lin) {
			// 				std::ostringstream msg;
			// 				msg << "parsing error in " << source_name << ":" << line_no << ": <base-classname> expected after \"" << keyword << " : \"";
			// 				throw std::runtime_error(msg.str());
			// 			}
			// 			if (basename.back() == ',') {
			// 				basename.pop_back();
			// 				lin.putback(',');
			// 			}
			// 			basename = strip(basename);
			// 			if (basename != "public" && basename != "protected" && basename != "private") {
			// 				break;
			// 			}
			// 		}
			// 		basenames.push_back(basename);
			// 	}
			// }
			// std::cerr << "found class definition: " << classname << "  ";
			// for (auto &basename: basenames) {
			// 	std::cerr << basename << " ";
			// }
			// std::cerr << std::endl;
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
			if (source_name.find('/') != source_name.npos && include_filename[0] != '/') {
				auto last_slash_pos = source_name.find_last_of('/');
				include_filename = source_name.substr(0,last_slash_pos+1) + include_filename;
			}
			if (include_open == '<') {
				continue; // dont open sytem headers
			}
			std::ifstream included_file_in(include_filename);
			if (!included_file_in) {
				std::ostringstream msg;
				msg << "parsing error in " << source_name << ":" << line_no << ": cannot open file \"" << include_filename << "\"";
				throw std::runtime_error(msg.str());
			} else {
				cpp_parser(include_filename, defines);
			}
		} else if (!saftbus_export_tag) {
			// do nothing
		} else {
			std::ostringstream msg;
			msg << "parsing error in " << source_name << ":" << line_no << ": unexpected keyword \"" << keyword << "\"";
			throw std::runtime_error(msg.str());
		}
	}
	std::cerr << "end of file" << std::endl;
}




int main(int argc, char **argv) 
{

	if (argc != 2) {
		std::cerr << "usage: " << argv[0] << " <c++-file>" << std::endl;
		return 1;
	}
	std::map<std::string, std::string> defines;
	cpp_parser(argv[1], defines);

	for (auto &class_def: classes) {
		class_def.print();
	}
	return 0;
}