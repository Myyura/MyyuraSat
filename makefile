INCLUDE = ./include
SOURCE = ./source
OBJECT = ./object

OPTION = -std=c++14

MyyuraSat: solver.o main.o
	g++ $(OPTION) $(OBJECT)/solver.o $(OBJECT)/main.o -o MyyuraSat

main.o: $(INCLUDE)/core/solver.hpp $(SOURCE)/main.cpp
	g++ $(OPTION) -c $(SOURCE)/main.cpp -o $(OBJECT)/main.o

solver.o: $(INCLUDE)/core/solver.hpp $(SOURCE)/solver.cpp
	g++ $(OPTION) -c $(SOURCE)/solver.cpp -o $(OBJECT)/solver.o