
all: pathfinding

pathfinding: pathfinding.cpp
	g++ -Werror -Wall -I submodules/entt/src -std=c++17 -O2 pathfinding.cpp -o pathfinding 