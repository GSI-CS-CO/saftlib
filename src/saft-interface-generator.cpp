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
		if (ch != ' ' || non_space_found) {
			non_space_found = true;
			result.push_back(ch);
		}
	}
	return result;
}


class MiniCppTokenizer
{
public:
	enum TokenType {
		Other,
		Preprocessor,
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
			// if in some form of comment
			// if (line_comment) {
			// 	//std::cerr << ch;
			// 	if (ch == '\n') {
			// 		// std::cerr << " <== end of line commment" << std::endl;
			// 		line_comment = false;
			// 		current_string = "";
			// 	}
			// 	continue;
			// } else 
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
		return current_string;
	}

	TokenType get_token_type() {
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
		Scope(const std::string &n) : name(n), open_braces(0) {	}
	};

	std::vector<Scope> scope;
	bool read_namespace;

public:
	ScopeManager() 
		: scope(1, Scope("")) 
		, read_namespace(false)
		{}
	void take_into_account(const std::string &token) {
		if (token == "namespace") {
			read_namespace = true;
		} else if (read_namespace) {
			scope.push_back(Scope(token));
			read_namespace = false;
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
		std::string full_name("::");
		for (auto part: scope) {
			full_name.append(part.name);
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
		std::vector<std::string> type_name(split(return_space_name,' '));
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
		out << parameter << std::endl;
	}
	return out;
}

struct SignalParameter
{
	std::string type;
	std::string name;
};

struct Signal
{
	std::vector<SignalParameter> parameters;
	std::string name;
};


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
	out << ((p.inout==InOut)?"readwrite":((p.inout==Out)?"read":"write")) << " " << p.type << " " << p.name;
	return out;
}


class Interface
{
private:
	std::vector<Method> methods;
	std::vector<Signal> signals;
	std::vector<Property> properties;
	ScopeManager scope_manager;
public:
	Interface(std::istream &in)
	{
		MiniCppTokenizer::TokenType type;
		MiniCppTokenizer minitok(in);
		ScopeManager scopeman;
		while(minitok.next_token()) {
			//std::cerr << scopeman.get_current_name() << "  TOKEN: " << minitok.get_token_string() ;
			//std::cerr << std::endl;

			switch(minitok.get_token_type()) {
				case MiniCppTokenizer::SaftbusMethod: {
					std::string return_and_name = minitok.get_token_string();
					minitok.next_token(); // '('
					minitok.next_token(); // the (comma separated) list of arguments
					std::string method_args = minitok.get_token_string();
					if (method_args == ")") {  // handle the case of no args
						method_args = ""; 
					}
					// std::cerr << " SAFTBUS METHOD: " << return_and_name << "(" << method_args << ")" << std::endl;
					methods.push_back(Method(return_and_name, method_args));
					std::cerr << "------> " << methods.back() << std::endl;
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
					properties.push_back(Property(return_and_name, method_args));
					bool found_duplicate = false;
					for (int i = 0; i < properties.size()-1; ++i) {
						if (properties[i].name  == properties.back().name) {
							if (properties[i].inout != properties.back().inout) {
								properties[i].inout = InOut;
							}
							found_duplicate = true;
						}
					}
					if (found_duplicate) {
						properties.pop_back();
					}
					std::cerr << "-------> " << properties.back() << std::endl;
				}
				break;
				default:;
			}

			scopeman.take_into_account(minitok.get_token_string());
		}
	}

};


int main(int argc, char *argv[])
{
	for (int i = 1; i < argc; ++i) {
		std::cout << "generating saftbus glue code for " << argv[i] << std::endl;
		std::ifstream sourcefile(argv[i]);
		Interface interface(sourcefile);
	}

	return 0;
}