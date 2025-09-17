SRC = src/main.cpp src/lex.cpp src/compile.cpp src/ast.cpp

toycpp: $(SRC)
	g++ -Wall -Wextra -ggdb $(SRC) -o toycpp
