#include "parser.h"
#include "type_checker.h"
#include "lexer.h"
#include "backend.h"
#include "location.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <exception>
#include <errno.h>
#include <cstring>

int main(int argc, char** argv) {
	if(argc != 2) {
		std::cout<<"USAGE: latc_x86_64 path_to_file.lat\n";
		return 1;
	}
	std::stringstream s;
	std::string si;
	try {
		std::string filename(argv[1]);
		if(filename.size() < 5 || filename.substr(filename.size() - 4) != ".lat") {
			throw std::runtime_error("Expected .lat file!");
		}
		std::ifstream input;
		input.exceptions(std::ifstream::failbit | std::ifstream::badbit);

		input.open(filename);

		input.exceptions(std::ifstream::badbit);

		while (std::getline(input, si)) {
		  s << si << '\n';
		}
		si = s.str();

		input.close();

		filename.resize(filename.size() - 2);
		filename[filename.size() - 1] = 's';

		auto t = tokenize(si);
		auto p = parse(t);
		auto f = TypeChecker::check_types(p);

		std::ofstream output;
		output.exceptions(std::ofstream::failbit | std::ofstream::badbit);
		output.open(filename, std::ios_base::trunc);

		emit_code(f, output);

		output.close();

		if(system(("nasm -f elf64 " + filename).data())) {
			throw std::runtime_error("nasm execution error.");
		}

		filename.resize(filename.size() - 2);

		if(system(("ld -o " + filename + " " + filename + ".o lib/runtime.o").data())) {
			throw std::runtime_error("ld execution error.");
		}

		if(system(("rm " + filename + ".o").data())) {
			throw std::runtime_error("cleanup error.");
		}

		std::cerr << "OK\n";
		return 0;
	} catch (const LexerException& e) {
		LocationTranslator lt(si);
		std::cerr << "ERROR\n" << "Unrecognized token at " << lt(e.pos) << '\n';
		return 1;
	} catch (const ParserException& e) {
		LocationTranslator lt(si);
		std::cerr << "ERROR\n";
		e(std::cerr, lt);
		return 1;
	} catch (const TypeChecker::TypeCheckerException& e) {
		std::cerr << "ERROR\n" << "Type checker error, details:\n";
		LocationTranslator lt(si);
		for(const TypeChecker::TypeCheckerError& err : e.errors) {
			err(std::cerr, lt);
		}
		return 1;
	} catch (const std::ios_base::failure&) {
		std::cout << "ERROR\nIO Error!\n" << std::strerror(errno) << '\n';
		return 1;
	} catch (const std::exception& e) {
		std::cerr << "ERROR\nDetails:\n" << e.what() << '\n';
		return 1;
	} catch (...) {
		std::cerr << "ERROR\nSomething went horribly wrong.\n";
		return 1;
	}
}
