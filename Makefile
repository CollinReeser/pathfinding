# Run each set of target commands in a single shell. This will make `cd` work
# as expected.
#
# `.SHELLFLAGS += -e` says "fail after the first failed command in the list."
.ONESHELL:
.SHELLFLAGS += -e

.PHONY: clean realclean init runtests

BUILD_DIR := build
BUILD_TEST_DIR := build_test

BINARY_NAMES := main_pathfinding
BINARIES := $(BINARY_NAMES:%=$(BUILD_DIR)/%)

TEST_BINARY_NAMES := test_unit
TEST_BINARIES := $(TEST_BINARY_NAMES:%=$(BUILD_TEST_DIR)/%)

all: $(BINARIES) tests

tests: $(TEST_BINARIES)

# Submodule versions:
#
# entt: git tag `v3.9.0`
# googletest: git tag `release-1.11.0`
# libSDL2pp: git tag `0.16.1`
# imgui: git tag `v1.87`

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
	cd ../../libSDL2pp
	cmake . -DSDL2PP_WITH_WERROR=ON -DSDL2PP_CXXSTD=c++17 -DSDL2PP_STATIC=ON
	$(MAKE)
	cd ../..
	mkdir -p src_imgui
	mkdir -p obj_imgui
	cd submodules/imgui
	cp *.cpp *.h ../../src_imgui
	cp backends/imgui_impl_sdl.* ../../src_imgui
	cp backends/imgui_impl_sdlrenderer.* ../../src_imgui
	cd ../..
	mkdir -p obj
	mkdir -p obj_test
	mkdir -p build
	mkdir -p build_test
	$(MAKE)
	$(MAKE) runtests

runtests: tests
	for test in $(TEST_BINARIES);\
		do\
			echo "Running" $$test "...";\
			$$test || exit 1;\
			echo "Passed" $$test;\
		done

clean:
	rm -f build/*
	rm -f build_test/*
	rm -f obj/*
	rm -f obj_imgui/*
	rm -f obj_test/*

realclean: clean
	rm -rf submodules/*

SRC_IMGUI_DIR := src_imgui
OBJ_IMGUI_DIR := obj_imgui
SRC_IMGUI_FILES := $(wildcard $(SRC_IMGUI_DIR)/*.cpp)
OBJ_IMGUI_FILES := $(patsubst $(SRC_IMGUI_DIR)/%.cpp,$(OBJ_IMGUI_DIR)/%.o,$(SRC_IMGUI_FILES))

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

INCLUDES_IMGUI := -I src_imgui `sdl2-config --cflags`
INCLUDES := $(INCLUDES_IMGUI) `sdl2-config --cflags` -I submodules/entt/src -I submodules/libSDL2pp
INCLUDES_TEST := -I src -I submodules/googletest/googletest/include

# Remove from the test object files any main obj files that have `main()`s.
OBJ_TEST_FILES := $(filter-out $(OBJ_DIR)/main_%.o, $(OBJ_TEST_FILES))

CXXFLAGS      := -std=c++17 -g -O2 -Wall -Werror -MMD
CXXFLAGS_TEST := -std=c++17 -g     -Wall -Werror -MMD

LIB_FLAGS := `sdl2-config --libs` -L submodules/libSDL2pp
LD_FLAGS := $(LIB_FLAGS) -lSDL2 -lSDL2_image -lSDL2_ttf -lSDL2_mixer -lSDL2pp

LIB_TEST_FLAGS := -L submodules/googletest/build/lib
LD_TEST_FLAGS := $(LIB_TEST_FLAGS) -lgtest -lpthread

$(BINARIES): $(OBJ_IMGUI_FILES) $(OBJ_FILES)
	g++ -o $@ $^ $(LD_FLAGS)

$(TEST_BINARIES): $(OBJ_TEST_FILES)
	g++ -o $@ $^ $(LD_TEST_FLAGS)

$(OBJ_IMGUI_DIR)/%.o: $(SRC_IMGUI_DIR)/%.cpp
	g++ $(CXXFLAGS) $(INCLUDES_IMGUI) -c -o $@ $<

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	g++ $(CXXFLAGS) $(INCLUDES) -c -o $@ $<

$(OBJ_TEST_DIR)/%.o: $(SRC_TEST_DIR)/%.cpp
	g++ $(CXXFLAGS_TEST) $(INCLUDES_TEST) -c -o $@ $<

-include $(OBJ_IMGUI_FILES:.o=.d)
-include $(OBJ_FILES:.o=.d)
-include $(OBJ_TEST_FILES:.o=.d)