SRC = src/main.cpp src/lex.cpp src/compile.cpp

toycpp: $(SRC)
	g++ -Wall -Wextra -ggdb $(SRC) -o toycpp
