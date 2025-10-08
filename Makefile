HEADERS = src/lex.hpp src/utils.hpp src/grammar.hpp
SRC = src/main.cpp src/lex.cpp src/grammar.cpp

toycpp: $(SRC) $(HEADERS)
	g++ --std=c++17 -Wall -Wextra -ggdb $(SRC) -o toycpp
