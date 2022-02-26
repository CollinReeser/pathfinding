# Recognized ENV vars:
#
# PROFILE : Enable profiling with -pg/gprof.
# RELEASE : Enable release build.

# Run each set of target commands in a single shell. This will make `cd` work
# as expected.
#
# `.SHELLFLAGS += -e` says "fail after the first failed command in the list."
.ONESHELL:
.SHELLFLAGS += -e

.PHONY: clean realclean init init-win tests runtests

BUILD_DIR := build
BUILD_TEST_DIR := build_test

BINARY_NAMES := main_pathfinding
BINARIES := $(BINARY_NAMES:%=$(BUILD_DIR)/%)

TEST_BINARY_NAMES := test_unit
TEST_BINARIES := $(TEST_BINARY_NAMES:%=$(BUILD_TEST_DIR)/%)

all: $(BINARIES) tests

tests: $(TEST_BINARIES)

clean:
	rm -f build/*
	rm -f build_test/*
	rm -f obj/*
	rm -f obj_test/*

realclean: clean
	rm -f obj_imgui/*
	rm -rf submodules/*

# Submodule versions:
#
# entt: git tag `v3.9.0`
# googletest: git tag `release-1.11.0`
# libSDL2pp: git tag `0.16.1`
# imgui: git tag `v1.87`
#

# Initialize project for Linux.
init:
	$(MAKE) _init-submodule-build
	mkdir -p obj
	mkdir -p obj_test
	mkdir -p build
	mkdir -p build_test
	$(MAKE)
	$(MAKE) runtests

LINUX_SUBMODULE_TARGETS := init-entt init-googletest init-libsdl2pp init-sdl-gpu init-imgui

.PHONY: _init-submodule-build _submodule-update $(LINUX_SUBMODULE_TARGETS)

_init-submodule-build: $(LINUX_SUBMODULE_TARGETS)

_submodule-update:
	git submodule init
	git submodule update --init --recursive

init-entt: _submodule-update
	cd submodules/entt/build
	cmake .. -DENTT_BUILD_DOCS=ON -DENTT_BUILD_TESTING=ON
	$(MAKE)
	$(MAKE) test

init-win-entt: _submodule-update
	cd submodules/entt/build
	cmake .. -DENTT_BUILD_TESTING=ON -G "MinGW Makefiles"
	$(MAKE)
	$(MAKE) test

init-googletest: _submodule-update
	cd submodules/googletest
	mkdir -p build
	cd build
	cmake ..
	$(MAKE)

init-win-googletest: _submodule-update
	cd submodules/googletest
	mkdir -p build
	cd build
	cmake .. -G "MinGW Makefiles"
	$(MAKE)

init-libsdl2pp: _submodule-update
	cd submodules/libSDL2pp
	cmake . -DSDL2PP_WITH_WERROR=ON -DSDL2PP_CXXSTD=c++17 -DSDL2PP_STATIC=ON
	$(MAKE)

init-win-libsdl2pp: _submodule-update
	cd submodules/libSDL2pp
	cmake . -DSDL2PP_WITH_WERROR=ON -DSDL2PP_CXXSTD=c++17 -DSDL2PP_STATIC=ON -G "MinGW Makefiles"
	$(MAKE)

init-sdl-fontcache: _submodule-update
	cd submodules/nfont/SDL_FontCache
	mkdir shared
	g++ -DFC_USE_SDL_GPU -g -fPIC -rdynamic -shared `sdl2-config --cflags` -I ../../sdl-gpu/install/include/SDL2  SDL_FontCache.c -o shared/libSDL_FontCache.so

init-nfont: _submodule-update init-sdl-fontcache
	cd submodules/nfont
	mkdir shared
	g++ -DFC_USE_SDL_GPU -g -fPIC -rdynamic -shared -I ../SDL_FontCache `sdl2-config --cflags` -I ../../sdl-gpu/install/include/SDL2   NFont.cpp -o shared/libNFont.so

init-sdl-gpu: _submodule-update init-nfont
	cd submodules/sdl-gpu
	mkdir install
	cmake . -DCMAKE_INSTALL_PREFIX=install -G "Unix Makefiles"
	$(MAKE)
	$(MAKE) install

init-win-sdl-gpu: _submodule-update
	cd submodules/sdl-gpu
	cmake . -G "MinGW Makefiles"
	$(MAKE)

init-imgui: _submodule-update
	cd submodules/imgui
	mkdir -p ../../src_imgui
	mkdir -p ../../obj_imgui
	cp *.cpp *.h ../../src_imgui
	cp backends/imgui_impl_sdl.* ../../src_imgui
	cp backends/imgui_impl_sdlrenderer.* ../../src_imgui

# Initialize project for Windows.
init-win:
	$(MAKE) _init-win-submodule-build
	mkdir -p obj
	mkdir -p obj_test
	mkdir -p build
	mkdir -p build_test
	$(MAKE)
	$(MAKE) runtests

WINDOWS_SUBMODULE_TARGETS := init-win-entt init-win-googletest init-win-libsdl2pp init-win-sdl-gpu init-imgui

.PHONY: _init-win-submodule-build $(WINDOWS_SUBMODULE_TARGETS)

_init-win-submodule-build: $(WINDOWS_SUBMODULE_TARGETS)

runtests: tests
	for test in $(TEST_BINARIES);\
		do\
			echo "Running" $$test "...";\
			$$test || exit 1;\
			echo "Passed" $$test;\
		done

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
INCLUDES := $(INCLUDES_IMGUI) `sdl2-config --cflags` -I submodules/entt/src -I submodules/libSDL2pp -I submodules/sdl-gpu/install/include/SDL2 -DFC_USE_SDL_GPU -I submodules/nfont/NFont
INCLUDES_TEST := -I src -I submodules/googletest/googletest/include

# Remove from the test object files any main obj files that have `main()`s.
OBJ_TEST_FILES := $(filter-out $(OBJ_DIR)/main_%.o, $(OBJ_TEST_FILES))

# ENV vars to control build.
ifeq ($(PROFILE), 1)
	GPROF_ENABLE = -pg
endif

ifeq ($(RELEASE), 1)
	OPTIMIZE_ARGS = -flto -O3
else
	OPTIMIZE_ARGS = -O2
endif

CXXFLAGS      := -std=c++20 -g $(GPROF_ENABLE) $(OPTIMIZE_ARGS) -Wall -Werror -MMD
CXXFLAGS_TEST := -std=c++20 -g $(GPROF_ENABLE) -Wall -Werror -MMD
CXXFLAGS_IMGUI := -std=c++17 -g $(OPTIMIZE_ARGS) -Wall -Werror -MMD

LD_FLAGS := $(GPROF_ENABLE) $(OPTIMIZE_ARGS) -L submodules/libSDL2pp -lSDL2pp `sdl2-config --libs` -lSDL2_image -lSDL2_ttf -lSDL2_mixer -L submodules/nfont/SDL_FontCache/shared -Wl,-rpath,submodules/nfont/SDL_FontCache/shared -lSDL_FontCache -L submodules/nfont/NFont/shared -Wl,-rpath,submodules/nfont/NFont/shared -lNFont -L submodules/sdl-gpu/install/lib -Wl,-rpath,submodules/sdl-gpu/install/lib -lSDL2_gpu

LD_TEST_FLAGS := -L submodules/googletest/build/lib -lgtest -lpthread

$(BINARIES): $(OBJ_IMGUI_FILES) $(OBJ_FILES)
	g++ -o $@ $^ $(LD_FLAGS)

$(TEST_BINARIES): $(OBJ_TEST_FILES)
	g++ -o $@ $^ $(LD_TEST_FLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	g++ $(CXXFLAGS) $(INCLUDES) -c -o $@ $<

$(OBJ_TEST_DIR)/%.o: $(SRC_TEST_DIR)/%.cpp
	g++ $(CXXFLAGS_TEST) $(INCLUDES_TEST) -c -o $@ $<

$(OBJ_IMGUI_DIR)/%.o: $(SRC_IMGUI_DIR)/%.cpp
	g++ $(CXXFLAGS_IMGUI) $(INCLUDES_IMGUI) -c -o $@ $<

-include $(OBJ_IMGUI_FILES:.o=.d)
-include $(OBJ_FILES:.o=.d)
-include $(OBJ_TEST_FILES:.o=.d)