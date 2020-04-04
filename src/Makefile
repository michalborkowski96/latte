CXXFLAGS=-O3 -Wextra -Wall -Werror -g -std=c++14
DEPS=helper_visitors.h lexer.h parser.h type_checker.h type_info_builder.h program_tree.h backend.h location.h
OBJ=helper_visitors.o lexer.o parser.o type_checker.o type_info_builder.o program_tree.o backend_x86_64.o location.o main.o
.DEFAULT_GOAL=latc_x86_64

latc_x86_64: $(OBJ)
	$(CXX) -o $@ $^ $(CXXFLAGS)

%.o: %.c $(DEPS)
	$(CXX) -c -o $@ $< $(CXXFLAGS)

clean:
	rm -f *.o >/dev/null 2>/dev/null
	rm -f latc_x86_64 >/dev/null 2>/dev/null
