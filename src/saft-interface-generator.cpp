#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <cctype>
#include <cassert>

bool starts_with(const std::string &full, const std::string &start) 
{
	if (start.size() <= full.size() && full.substr(0,start.size()) == start) {
		return true;
	}
	return false;
}

// subsequent occurring delimiters are merged into one delimiter 
// e.g.  split("a     b",' ') => {"a","b"}
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

std::string strip_leading_whitespaces(const std::string &input) {
	std::string result;
	bool non_space_found = false;
	for (auto ch: input) {
		if (!isspace(ch) || non_space_found) {
			non_space_found = true;
			result.push_back(ch);
		}
	}
	return result;
}
std::string strip_trailing_whitespaces(const std::string &input) {
	std::string result(input);
	if (!input.size()) return input;
	while(isspace(result.back())) {
		result.pop_back();
	}
	return result;
}

class MiniCppTokenizer
{
public:
	enum TokenType {
		Other,
		Preprocessor,
		Class,
		SaftbusMethod,
		SaftbusProperty,
		SaftbusSignal,
	};
	bool is_special_char(char ch) {
		switch (ch) {
			case '(': case ')': case '{': case '}': case ';': case ':': case '/': return true;
			default: return false; 
		}
		return false;
	}
	std::vector<std::string> keywords;
	bool is_keyword(const std::string str) {
		for (auto keyword : keywords) {
			if (str == keyword) return true;
		}
		return false;
	}

	MiniCppTokenizer(std::istream &input) 
		: in(input)
	{
		keywords.push_back(";");
		keywords.push_back("namespace");
		keywords.push_back("class");
		// keywords.push_back("static");
		keywords.push_back("{");
		keywords.push_back("}");
		keywords.push_back("(");
		keywords.push_back(")");
		// keywords.push_back("void");
		// keywords.push_back("short");
		// keywords.push_back("int");
		// keywords.push_back("long");
		// keywords.push_back("unsigned");
		// keywords.push_back("unsigned short");
		// keywords.push_back("unsigned int");
		// keywords.push_back("unsigned long");
		// keywords.push_back("float");
		// keywords.push_back("double");
	} 
	bool next_token() {
		// std::cerr << "next token()" << std::endl;
		current_string = "";
		current_type   = Other;
		for(;;) {
			char ch;
			in.get(ch);
			if (!in) {
				if (current_string.size()) break;
				return false;
			}
			// if inside block comment
			if (block_comment) {
				if (ch == '/' && current_string.back()=='*') {
					// std::cerr << "=========> end of block comment" << std::endl;
					block_comment = false;
					current_string = "";
				} else {
					current_string.push_back(ch);
				}
				continue;
			} 
			if (preprocessor_directive) {
				if (ch == '\n') {
					if (current_string.back() == '\\') {
						current_string.pop_back();
						continue;
					} else {
						preprocessor_directive = false;
						current_type = Preprocessor;
						return true;
					}
				}
			}
			// normal operation
			if (isspace(ch) && (current_string.size()==0 || current_string.back()==' ')) continue;
			if (ch == '/') { // detect potential line comment "//" or block comment "/*...*/"
				char nextch;
				in.get(nextch);
				if (!in) continue;
				if (nextch == '/') {
					// std::cerr << "========> found line comment" << std::endl;					
					// line_comment = true;
					std::string line_comment;
					getline(in,line_comment);
					//std::cerr << "line comment ---------------> " << line_comment << std::endl;
					if (starts_with(line_comment," saftbus method")) {
						current_type = SaftbusMethod;
					} else if (starts_with(line_comment, " saftbus property")) {
						current_type = SaftbusProperty;
					} else if (starts_with(line_comment, " saftbus signal")) {
						current_type = SaftbusSignal;
					}
					current_string = "";
					continue;
				} else if (nextch == '*') {
					// std::cerr << "========> found block comment" << std::endl;
					block_comment = true;
					current_string = "";
					continue;
				} else {
					current_string.push_back(ch);
					in.putback(nextch);
					continue;
				}
			} 
			if (ch == ':') { // detect potential double colon "::"
				char nextch;
				in.get(nextch);
				if (!in) continue;
				if (nextch == ':') { // found double colon "::"
					current_string.push_back(ch);
					current_string.push_back(nextch);
					continue;
				} else {
					in.putback(nextch);
				}
			}
			if (ch == '#') { // preprocessor directive
				preprocessor_directive = true;
				current_string.push_back(ch);
				continue;
			}
			if (is_special_char(ch) && current_string.size()) {
				in.putback(ch);
				return true;
			}
			current_string.push_back(ch);
			if (current_string.size() == 1 && is_special_char(current_string[0]) ) return true;
			if (is_keyword(current_string)) return true;
		}
	}
	std::string get_token_string() {
		return strip_trailing_whitespaces(current_string);
	}

	TokenType get_token_type() {
		if (current_string == "class") {
			return Class;
		}
		return current_type;
	}

private:
	std::istream &in;
	bool line_comment;
	bool block_comment;
	bool preprocessor_directive;

	std::string current_string;
	TokenType   current_type;
};


// keep tack of "namespace <name> { }"
// and count number of opening '{' and closing '}' braces 
class ScopeManager
{
private:
	struct Scope {
		std::string name;
		int open_braces;
		bool is_class;
		Scope(const std::string &n, bool isclass = false) : name(strip_trailing_whitespaces(n)), open_braces(0), is_class(isclass) {}
	};

	std::vector<Scope> scope;
	bool read_name;
	bool read_class_name;

public:
	ScopeManager() 
		: scope(1, Scope("")) 
		, read_name(false)
		, read_class_name(false)
		{}
	void take_into_account(const std::string &token) {
		if (token == "namespace") {
			read_name = true;
		} else if (token == "class") {
			read_class_name = true;
		} else if (read_name) {
			scope.push_back(Scope(token));
			read_name = false;
		} else if (read_class_name) {
			scope.push_back(Scope(token,true));
			read_class_name = false;
		} else if (token == "{") {
			++scope.back().open_braces;
		} else if (token == "}") {
			--scope.back().open_braces;
			if (scope.back().open_braces == 0) {
				scope.pop_back();
			}
		} else {
			// nothing
		}
	}
	std::string get_current_name() {
		std::string full_name;
		for (int i = 0; i < scope.size(); ++i) {
			full_name.append(scope[i].name);
			if (scope[i].is_class) {
				full_name.append("*");
			}
			if (i != scope.size()-1) {
				full_name.append("::");
			}
		}
		return full_name;
	}
};


enum ParameterInOut
{
	In,
	Out,
	InOut
};

struct MethodParameter
{
	MethodParameter(const std::string type_and_name, bool is_output = false) {
		std::vector<std::string> parts = split(type_and_name,' ');
		assert(parts.size() >= 2);
		name = strip_leading_whitespaces(parts.back());
		type = strip_leading_whitespaces(type_and_name.substr(0,type_and_name.find_last_of(" ")));
		if (name[0] == '&') {
			type.append(name.substr(0,1));
			name = name.substr(1);
		}
		inout = In;
		if (is_output || (type_and_name.find("&") != type_and_name.npos && parts[0] != "const")) {
			inout = Out;
		}
	}
	ParameterInOut inout;
	std::string type;
	std::string name;
};
std::ostream& operator<<(std::ostream &out, const MethodParameter& p) {
	out << ((p.inout==In)?"input":"output") << " " << p.type << " " << p.name;
	return out;
}

struct Method
{
	Method(const std::string &return_space_name, const std::string &comma_separated_parameter_list) 
	{
		for (auto type_and_name: split(comma_separated_parameter_list,',')) {
			parameters.push_back(MethodParameter(type_and_name));
		}
		std::vector<std::string> type_name;
		auto pos = return_space_name.find_last_of(" ");
		type_name.push_back(return_space_name.substr(0,pos));
		type_name.push_back(return_space_name.substr(pos+1));
		//std::cerr << "\'" << return_space_name << "\' type_name.size()=" << type_name.size() << std::endl;
		assert(type_name.size() == 2);
		name             = type_name[1];
		std::string type = type_name[0];
		if (type != "void") {
			parameters.push_back(MethodParameter(type + " " + name, true));
		}
	}
	std::vector<MethodParameter> parameters;
	std::string name;
};
std::ostream& operator<<(std::ostream &out, const Method &m) {
	out << "saftbus method: " << m.name << std::endl;
	for (auto parameter: m.parameters) {
		out << "      " << parameter << std::endl;
	}
	return out;
}

struct SignalParameter
{
	std::string type;
	std::string name;
	SignalParameter(const std::string &t, const std::string &n) 
		: type(t), name(n) {}
};

struct Signal
{
	std::vector<SignalParameter> parameters;
	std::string name;
};
std::ostream& operator<<(std::ostream& out, const Signal &s) {
	out << "  saftbus signal: " << s.name << std::endl;
	for (auto par: s.parameters) {
		out << "      " << par.type << " " << par.name << std::endl;
	}
	return out;
}

struct Property
{
	Property(const std::string &type_space_name, const std::string parameter)
	{
		Method m(type_space_name, parameter);
		// std::cerr << "Property helper Method " << m << std::endl;
		assert(m.parameters.size() > 0);
		inout = m.parameters[0].inout;
		type  = m.parameters[0].type;
		name  = m.name.substr(3);
	}
	ParameterInOut inout;
	std::string type;
	std::string name;
};
std::ostream &operator<<(std::ostream &out, const Property& p) {
	out << "  saftbus property: " << ((p.inout==InOut)?"readwrite":((p.inout==Out)?"read":"write")) << " " << p.type << " " << p.name;
	return out;
}

struct Interface 
{
	std::string              name;
	std::vector<std::string> parents; // parent classes
	std::vector<Method>      methods;
	std::vector<Signal>      signals;
	std::vector<Property>    properties;
	Interface(const std::string &interface_name, const std::vector<std::string> &interface_parents) 
		: name(interface_name)
		, parents(interface_parents)
		{}
};
std::ostream& operator<<(std::ostream& out, Interface &interface) {
	out << "saftbus interface "  << interface.name;
	if (interface.parents.size() > 0) { 
		out << " : ";
		for (auto parent: interface.parents) {
			out << parent << " ";
		}
	}
	out << std::endl;
	out << "  saftbus methods:" << std::endl;
	for (auto method: interface.methods) {
		out << "    " << method << std::endl;
	}
	out << "  saftbus properties:" << std::endl;
	for (auto property: interface.properties) {
		out << "    " << property << std::endl;
	}
	out << "  saftbus signals:" << std::endl;
	for (auto signal: interface.signals) {
		out << "    " << signal << std::endl;
	}
	return out;
}

class InterfaceManager
{
private:
	std::vector<Interface> interfaces;
	ScopeManager scope_manager;
public:
	void scan_file(std::istream &in)
	{
		MiniCppTokenizer::TokenType type;
		MiniCppTokenizer minitok(in);
		ScopeManager scopeman;
		while(minitok.next_token()) {
			std::cerr << scopeman.get_current_name() << "  TOKEN: " << minitok.get_token_string() << std::endl;

			switch(minitok.get_token_type()) {
				case MiniCppTokenizer::SaftbusMethod: {
					assert(interfaces.size()>0);
					std::string return_and_name = minitok.get_token_string();
					minitok.next_token(); // '('
					minitok.next_token(); // the (comma separated) list of arguments
					std::string method_args = minitok.get_token_string();
					if (method_args == ")") {  // handle the case of no args
						method_args = ""; 
					}
					// std::cerr << " SAFTBUS METHOD: " << return_and_name << "(" << method_args << ")" << std::endl;
					interfaces.back().methods.push_back(Method(return_and_name, method_args));
					// std::cerr << methods.back() << std::endl;
				}
				break;
				case MiniCppTokenizer::SaftbusSignal: {
					assert(interfaces.size()>0);
					std::string signal_and_name = minitok.get_token_string(); // sigc::signal< ... , ...  > name
					auto pos_type_begin = signal_and_name.find_first_of('<')+1;
					auto pos_type_end   = signal_and_name.find_last_of('>');
					auto pos_name_begin = signal_and_name.find_last_of(' ');

					std::string signal_type = signal_and_name.substr(pos_type_begin, pos_type_end-pos_type_begin);
					std::cerr << "signal_type=" << signal_type << std::endl;
					std::string signal_name = strip_leading_whitespaces(signal_and_name.substr(pos_name_begin));
					std::cerr << "signal_name=" << signal_name << std::endl;
					std::vector<std::string> parameter_list = split(signal_type, ',');
					assert (parameter_list.size()>1);
					interfaces.back().signals.push_back(Signal());
					interfaces.back().signals.back().name = signal_name;
					for (int i = 1; i < parameter_list.size(); ++i) {
						std::ostringstream parameter_name;
						parameter_name << "par" << i-1;
						interfaces.back().signals.back().parameters.push_back(SignalParameter(parameter_list[i], parameter_name.str()));
					}
				}
				break;
				case MiniCppTokenizer::SaftbusProperty: {
					std::string return_and_name = minitok.get_token_string();
					minitok.next_token(); // '('
					minitok.next_token(); // the (comma separated) list of arguments
					std::string method_args = minitok.get_token_string();
					if (method_args == ")") {  // handle the case of no args
						method_args = ""; 
					}
					// std::cerr << " SAFTBUS PROPERTY: " << return_and_name << "(" << method_args << ")" << std::endl;
					interfaces.back().properties.push_back(Property(return_and_name, method_args));
					bool found_duplicate = false;
					for (int i = 0; i < interfaces.back().properties.size()-1; ++i) {
						if (interfaces.back().properties[i].name  == interfaces.back().properties.back().name) {
							if (interfaces.back().properties[i].inout != interfaces.back().properties.back().inout) {
								interfaces.back().properties[i].inout = InOut;
							}
							found_duplicate = true;
						}
					}
					if (found_duplicate) {
						interfaces.back().properties.pop_back();
					}
					// std::cerr << properties.back() << std::endl;
				}
				break;
				case MiniCppTokenizer::Class: {
					minitok.next_token(); // classname
					std::string classname = minitok.get_token_string();
					std::cerr << "************ classname=\"" << classname << "\"" << std::endl;
					minitok.next_token(); // ':' or '{' or ';'
					if (minitok.get_token_string() == ":") {
						// read parent classes
						minitok.next_token(); // parent classes
						std::string parents = minitok.get_token_string();
						std::vector<std::string> parent_list = split(parents,',');
						std::vector<std::string> interface_parents;

						// std::cerr << "=========> CLASS " << classname << " : ";
						for (auto parent: parent_list) {
							std::vector<std::string> parts = split(parent, ' ');
							if (parts.size() > 1) {
								interface_parents.push_back(parts[1]);
								//std::cerr << parts[1] << " ";
							}
						}
						interfaces.push_back(Interface(scopeman.get_current_name() + "::" + classname, interface_parents));
						// std::cerr << std::endl;
						minitok.next_token(); // '{'
						scopeman.take_into_account("class");
						scopeman.take_into_account(classname);
					}
					else if (minitok.get_token_string() == "{") {
						std::vector<std::string> interface_parents;
						interfaces.push_back(Interface(scopeman.get_current_name() + "::" + classname, interface_parents));
						scopeman.take_into_account("class");
						scopeman.take_into_account(classname);
					}
				}
				break;
				default:;
			}

			scopeman.take_into_account(minitok.get_token_string());
		}
	}
	friend std::ostream& operator<<(std::ostream& out, InterfaceManager &interfacemanager);
};
std::ostream& operator<<(std::ostream& out, InterfaceManager &interfacemanager) 
{
	for (auto interface: interfacemanager.interfaces) {
		out << interface << std::endl;
	}
	return out;
}



int main(int argc, char *argv[])
{
	InterfaceManager interfaces;
	for (int i = 1; i < argc; ++i) {
		std::cout << "extracting saftbus interface from " << argv[i] << std::endl;
		std::ifstream sourcefile(argv[i]);
		interfaces.scan_file(sourcefile);
	}
	std::cout << interfaces << std::endl;

	return 0;
}