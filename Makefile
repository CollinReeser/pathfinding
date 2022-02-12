
all: pathfinding

pathfinding: pathfinding.cpp
	g++ -std=c++17 -O2 pathfinding.cpp -o pathfinding 