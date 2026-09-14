CXX = g++
CXXFLAGS = -Wall -Wextra -g
CURSES_LIB ?= -lncursesw

main: main.o generation.o pathfinding.o queue.o trainer.o ui.o parser.o pokemon.o battle.o
	$(CXX) $(CXXFLAGS) -o main main.o generation.o pathfinding.o queue.o trainer.o ui.o parser.o pokemon.o battle.o $(CURSES_LIB)

main.o: main.cpp generation.h pathfinding.h constants.h trainer.h pokemon.h queue.h ui.h parser.h
	$(CXX) $(CXXFLAGS) -c main.cpp

generation.o: generation.cpp generation.h constants.h trainer.h pokemon.h
	$(CXX) $(CXXFLAGS) -c generation.cpp

pathfinding.o: pathfinding.cpp pathfinding.h constants.h trainer.h pokemon.h
	$(CXX) $(CXXFLAGS) -c pathfinding.cpp

queue.o: queue.cpp queue.h constants.h trainer.h pokemon.h
	$(CXX) $(CXXFLAGS) -c queue.cpp

trainer.o: trainer.cpp trainer.h constants.h pokemon.h pathfinding.h
	$(CXX) $(CXXFLAGS) -c trainer.cpp

ui.o: ui.cpp ui.h generation.h constants.h trainer.h pokemon.h battle.h
	$(CXX) $(CXXFLAGS) -c ui.cpp

battle.o: battle.cpp battle.h constants.h trainer.h pokemon.h
	$(CXX) $(CXXFLAGS) -c battle.cpp

parser.o: parser.cpp parser.h
	$(CXX) $(CXXFLAGS) -c parser.cpp

pokemon.o: pokemon.cpp pokemon.h parser.h
	$(CXX) $(CXXFLAGS) -c pokemon.cpp

clean:
	rm -f main main.o generation.o pathfinding.o queue.o trainer.o ui.o parser.o pokemon.o battle.o
