INCLUDE = ./include
SOURCE = ./source
OBJECT = ./object

OPTION = -std=c++14

MyyuraSat: main.o
	g++ $(OPTION) $(OBJECT)/main.o -o MyyuraSat

# solver.o: $(INCLUDE)/core/solver.hpp $(SOURCE)/solver.cpp
# 	g++ $(OPTION) -c $(SOURCE)/solver.cpp -o $(OBJECT)/solver.o

main.o: $(INCLUDE)/core/solver.hpp $(SOURCE)/solver_basic.cpp $(SOURCE)/solver_search.cpp $(SOURCE)/solver_simplify.cpp $(SOURCE)/solver_debug.cpp $(SOURCE)/main.cpp
	g++ $(OPTION) -c $(SOURCE)/main.cpp -o $(OBJECT)/main.o

clean: 
	rm ./MyyuraSat $(OBJECT)/*.o