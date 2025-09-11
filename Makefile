toycpp: src/main.cpp src/lex.cpp
	g++ -Wall -Wextra -ggdb	\
	  src/main.cpp						\
	  src/lex.cpp						\
	  -o toycpp
