#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <cctype>

class MiniCppTokenizer
{
public:
	enum TokenType {
		Namespace,
		Class,
		Type,
		OpeningBrace,
		ClosingBrace,
		Identifier,
		Other,
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

	MiniCppTokenizer(std::istream &input) : in(input) 
	{
		keywords.push_back(";");
		keywords.push_back("namespace");
		keywords.push_back("class");
		keywords.push_back("static");
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
			if (line_comment) {
				//std::cerr << ch;
				if (ch == '\n') {
					// std::cerr << " <== end of line commment" << std::endl;
					line_comment = false;
					current_string = "";
				}
				continue;
			} else if (block_comment) {
				if (ch == '/' && current_string.back()=='*') {
					// std::cerr << "=========> end of block comment" << std::endl;
					block_comment = false;
					current_string = "";
				} else {
					current_string.push_back(ch);
				}
				continue;
			}
			// normal operation
			if (isspace(ch) && (current_string.size()==0 || current_string.back()==' ')) continue;
			if (ch == '/') { // detect potential line comment "//" or block comment "/*...*/"
				char nextch;
				in.get(nextch);
				if (!in) continue;
				if (nextch == '/') {
					// std::cerr << "========> found line comment" << std::endl;
					line_comment = true;
					current_string = "";
					continue;
				} else if (nextch == '*') {
					// std::cerr << "========> found block comment" << std::endl;
					block_comment = true;
					current_string = "";
					continue;
				} else {
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

private:
	std::istream &in;
	bool line_comment;
	bool block_comment;

	std::string current_string;
	TokenType   current_type;
};


enum ParameterInOut
{
	In,
	Out,
};

struct MethodParameter
{
	ParameterInOut inout;
	std::string type;
	std::string name;
};

struct Method
{
	std::vector<MethodParameter> parameters;
	std::string name;
};

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

enum PropertyAccess
{
	Read,
	Write,
	ReadWrite
};

struct Property
{
	PropertyAccess readwrite;
	std::string type;
	std::string name;
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

	int count_braces(const std::string &token) {
		int result = 0;
		for (auto ch: token) {
			if (ch == '{') ++result;
			if (ch == '}') --result;
		}
		return result;
	}

public:
	ScopeManager() : scope(1, Scope("::")) {}
	void take_into_account(const std::string &token, std::istream &in) {
		if (token == "namespace") {
			std::string new_scope;
			in >> new_scope;
			scope.push_back(Scope(new_scope));
		} else {
			int delta_brace = count_braces(token);
			if (scope.back().open_braces + delta_brace <= 0) {
				// scope must be closed... multiple scopes eventually if token is something like "}}}}}";
				while (scope.back().open_braces + delta_brace <= 0) {
					delta_brace += scope.back().open_braces;
					scope.pop_back();
				}
			}
		}
	}
	std::string get_current_name() {
		return scope.back().name;
	}
};

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
		std::string token;
		for(;;) {
			in >> token;
			scope_manager.take_into_account(token, in);
			std::cerr << scope_manager.get_current_name() << std::endl;
		}
	}

};


int main(int argc, char *argv[])
{
	for (int i = 1; i < argc; ++i) {
		std::cout << "generating saftbus glue code for " << argv[i] << std::endl;
		std::ifstream sourcefile(argv[i]);
		MiniCppTokenizer minitok(sourcefile);
		while(minitok.next_token()) {
			std::cerr << "TOKEN: " << minitok.get_token_string() << std::endl;
		}
	}

	return 0;
}