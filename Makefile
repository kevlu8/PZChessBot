# Project settings
EXE      ?= pzchessbot
EVALFILE ?= nnue.bin

GIT_SHORT_HASH := $(shell git rev-parse --short HEAD)
GIT_DATE := $(shell git log -1 --format=%cd --date=format:"%Y%m%d")

VERSION := v$(GIT_DATE)-$(GIT_SHORT_HASH)-dev

# Compilers
CXX      := g++
WINCXX   := x86_64-w64-mingw32-g++

# Flags
BASEFLAGS   := -std=c++20 -DNNUE_PATH=\"$(EVALFILE)\" -m64 -DVERSION=\"$(VERSION)\"
OPTFLAGS    := -O3 -flto=auto
DEBUGFLAGS  := -g -march=x86-64-v3 -fsanitize=address,undefined

# Sources & objects
SRCS  := $(wildcard engine/*.cpp engine/nnue/*.cpp Pyrrhic/tbprobe.cpp)
HDRS  := $(wildcard engine/*.hpp engine/nnue/*.hpp Pyrrhic/tbprobe.h)
OBJS  := $(SRCS:.cpp=.o)
DEPS  := $(OBJS:.o=.d)

.PHONY: all native binaries debug clean test debug-test pgo pgo-compile

# Default: native build
all: pgo

native: CXXFLAGS = $(BASEFLAGS) $(OPTFLAGS) -march=native
native: $(EXE)

debug: CXXFLAGS = $(BASEFLAGS) $(DEBUGFLAGS)
debug: $(EXE)

# Multi-binary builds
binaries: pzchessbot-linux-avx2 pzchessbot-win-avx2 pzchessbot-linux-avx512 pzchessbot-win-avx512

pzchessbot-linux-avx2: CXXFLAGS = $(BASEFLAGS) $(OPTFLAGS) -march=x86-64-v3 -static
pzchessbot-linux-avx2: $(SRCS) $(HDRS)
	$(CXX) $(CXXFLAGS) -o $@ $(SRCS)

pzchessbot-win-avx2: CXXFLAGS = $(BASEFLAGS) $(OPTFLAGS) -march=x86-64-v3 -static
pzchessbot-win-avx2: $(SRCS) $(HDRS)
	$(WINCXX) $(CXXFLAGS) -o $@ $(SRCS)

pzchessbot-linux-avx512: CXXFLAGS = $(BASEFLAGS) $(OPTFLAGS) -march=x86-64-v4 -static
pzchessbot-linux-avx512: $(SRCS) $(HDRS)
	$(CXX) $(CXXFLAGS) -o $@ $(SRCS)

pzchessbot-win-avx512: CXXFLAGS = $(BASEFLAGS) $(OPTFLAGS) -march=x86-64-v4 -static
pzchessbot-win-avx512: $(SRCS) $(HDRS)
	$(WINCXX) $(CXXFLAGS) -o $@ $(SRCS)

# Link final binary
$(EXE): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^
	@echo "Build complete. Run with ./$(EXE)"

# Compile objects with dependency generation
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

# Include generated dependency files
-include $(DEPS)

# Tests
test: CXXFLAGS = $(BASEFLAGS) $(OPTFLAGS)
test: $(OBJS)
	$(AR) rcs test/objs.a $(OBJS)
	$(MAKE) -C test CXXFLAGS="$(CXXFLAGS)"

debug-test: CXXFLAGS = $(BASEFLAGS) $(DEBUGFLAGS)
debug-test: $(OBJS)
	$(AR) rcs test/objs.a $(OBJS)
	$(MAKE) -C test CXXFLAGS="$(CXXFLAGS)" debug

# Cleanup
clean:
	@echo "Cleaning up..."
	rm -f $(EXE) *.exe
	rm -f $(OBJS) $(DEPS)
	rm -f test/objs.a
	rm -f engine/*.gcda engine/nnue/*.gcda Pyrrhic/*.gcda
	$(MAKE) -C test clean

pgo-compile: CXXFLAGS = $(BASEFLAGS) $(OPTFLAGS) -fprofile-use -march=native
pgo-compile: $(EXE)

pgo: CXXFLAGS = $(BASEFLAGS) $(OPTFLAGS) -fprofile-generate -march=native
pgo: $(EXE)
	@echo "Running PGO instrumentation..."
	./$(EXE) bench
	rm engine/*.o Pyrrhic/*.o engine/nnue/*.o
	@echo "Recompiling with PGO optimizations..."
	$(MAKE) pgo-compile
	rm -f engine/*.gcda engine/nnue/*.gcda Pyrrhic/*.gcda
