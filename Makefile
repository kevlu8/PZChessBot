# Project settings
EXE      ?= pzchessbot
EVALFILE ?= nnue.bin

# Compilers
CXX      := g++
WINCXX   := x86_64-w64-mingw32-g++

# Flags
BASEFLAGS   := -std=c++17 -DNNUE_PATH=\"$(EVALFILE)\" -m64
OPTFLAGS    := -O3 -flto=auto
DEBUGFLAGS  := -g -fsanitize=address,undefined

# Sources & objects
SRCS  := $(wildcard engine/*.cpp engine/nnue/*.cpp)
HDRS  := $(wildcard engine/*.hpp engine/nnue/*.hpp)
OBJS  := $(SRCS:.cpp=.o)
DEPS  := $(OBJS:.o=.d)

.PHONY: all native binaries release debug clean test debug-test

# Default: native build
all: native

native: CXXFLAGS = $(BASEFLAGS) $(OPTFLAGS) -march=native
native: $(EXE)

release: CXXFLAGS = $(BASEFLAGS) $(OPTFLAGS) -mavx2 -mbmi2 -mbmi -mavx -mpopcnt -mlzcnt -static
release: $(EXE)

debug: CXXFLAGS = $(BASEFLAGS) $(DEBUGFLAGS)
debug: $(EXE)

# Multi-binary builds
binaries: pzchessbot-linux-avx2 pzchessbot-win-avx2 pzchessbot-linux-avx512 pzchessbot-win-avx512

pzchessbot-linux-avx2: CXXFLAGS = $(BASEFLAGS) $(OPTFLAGS) -mavx2 -mbmi2 -mbmi -mavx -mpopcnt -mlzcnt -static
pzchessbot-linux-avx2: $(SRCS) $(HDRS)
	$(CXX) $(CXXFLAGS) -o $@ $(SRCS)

pzchessbot-win-avx2: CXXFLAGS = $(BASEFLAGS) $(OPTFLAGS) -mavx2 -mbmi2 -mbmi -mavx -mpopcnt -mlzcnt -static
pzchessbot-win-avx2: $(SRCS) $(HDRS)
	$(WINCXX) $(CXXFLAGS) -o $@ $(SRCS)

pzchessbot-linux-avx512: CXXFLAGS = $(BASEFLAGS) $(OPTFLAGS) -mavx2 -mbmi2 -mbmi -mavx -mpopcnt -mlzcnt -mavx512f -mavx512dq -mavx512bw -mavx512vl -static
pzchessbot-linux-avx512: $(SRCS) $(HDRS)
	$(CXX) $(CXXFLAGS) -o $@ $(SRCS)

pzchessbot-win-avx512: CXXFLAGS = $(BASEFLAGS) $(OPTFLAGS) -mavx2 -mbmi2 -mbmi -mavx -mpopcnt -mlzcnt -mavx512f -mavx512dq -mavx512bw -mavx512vl -static
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
	$(MAKE) -C test clean
