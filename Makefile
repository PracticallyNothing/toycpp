SRC = src/main.cpp src/lex.cpp src/compile.cpp src/ast.cpp

toycpp: $(SRC)
	g++ --std=c++17 -Wall -Wextra -ggdb $(SRC) -o toycpp
