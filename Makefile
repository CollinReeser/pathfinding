# Run each set of target commands in a single shell. This will make `cd` work
# as expected.
#
# `.SHELLFLAGS += -e` says "fail after the first failed command in the list."
.ONESHELL:
.SHELLFLAGS += -e

.PHONY: clean init

all: main_pathfinding test_unit

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
	mkdir -p obj
	mkdir -p obj_test
	mkdir -p build
	$(MAKE)

clean:
	rm -f main_*
	rm -f test_*
	rm obj/*
	rm obj_test/*

INCLUDES := -I submodules/entt/src
INCLUDES_TEST := -I submodules/googletest/googletest/include

SRC_DIR := src
OBJ_DIR := obj
SRC_FILES := $(wildcard $(SRC_DIR)/*.cpp)
OBJ_FILES := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRC_FILES))

SRC_TEST_DIR := src_test
OBJ_TEST_DIR := obj_test
SRC_TEST_FILES := $(wildcard $(SRC_TEST_DIR)/*.cpp)
OBJ_TEST_FILES := $(patsubst $(SRC_TEST_DIR)/%.cpp,$(OBJ_TEST_DIR)/%.o,$(SRC_TEST_FILES))
SRC_TEST_FILES += $(SRC_FILES)
OBJ_TEST_FILES += $(OBJ_FILES)

# Remove from the test object files any main obj files that have `main()`s.
OBJ_TEST_FILES := $(filter-out $(OBJ_DIR)/main_%.o, $(OBJ_TEST_FILES))

CXXFLAGS := -std=c++17 -g -Wall -Werror -MMD

LIB_TEST_FLAGS := -L submodules/googletest/build/lib
LD_TEST_FLAGS := $(LIB_TEST_FLAGS) -lgtest -lpthread

main_pathfinding: $(OBJ_FILES)
	g++ -o build/$@ $^ $(LDFLAGS)

test_unit: $(OBJ_TEST_FILES)
	g++ -o build/$@ $^ $(LD_TEST_FLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	g++ $(CXXFLAGS) $(INCLUDES) -c -o $@ $<

$(OBJ_TEST_DIR)/%.o: $(SRC_TEST_DIR)/%.cpp
	g++ $(CXXFLAGS) $(INCLUDES_TEST) -c -o $@ $<

-include $(OBJ_FILES:.o=.d)
-include $(OBJ_TEST_FILES:.o=.d)