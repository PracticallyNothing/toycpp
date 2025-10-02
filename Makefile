SRC = src/main.cpp src/lex.cpp src/grammar.cpp

toycpp: $(SRC)
	g++ --std=c++17 -Wall -Wextra -ggdb $(SRC) -o toycpp
