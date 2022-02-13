# Run each set of target commands in a single shell. This will make `cd` work
# as expected.
#
# `.SHELLFLAGS += -e` says "fail after the first failed command in the list."
.ONESHELL:
.SHELLFLAGS += -e

.PHONY: clean init

all: pathfinding test_unit

init:
	git submodule init
	git submodule update
	cd submodules/entt/build
	cmake .. -DENTT_BUILD_DOCS=ON -DENTT_BUILD_TESTING=ON
	$(MAKE)
	$(MAKE) test
	cd ../../googletest
	mkdir -p build
	cd build
	cmake ..
	$(MAKE)
	cd ../../..
	$(MAKE)

clean:
	rm -f pathfinding
	rm -f test_unit

includes = -I submodules/entt/src -I submodules/googletest/googletest/include
libs = -L submodules/googletest/build/lib
links = -lgtest -lpthread

pathfinding: pathfinding.cpp
	g++ -g -O2 -Werror -Wall $(includes) -std=c++17 pathfinding.cpp -o pathfinding

test_unit: test_unit.cpp
	g++ -g -O2 -Werror -Wall $(includes) -std=c++17 test_unit.cpp -o test_unit $(libs) $(links)
