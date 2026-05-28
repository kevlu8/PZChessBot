# Project settings
EXE      ?= pzchessbot
EVALFILE ?= nnue.bin

GIT_SHORT_HASH := $(shell git rev-parse --short HEAD)
GIT_DATE       := $(shell git log -1 --format=%cd --date=format:"%Y%m%d")
VERSION        := v$(GIT_DATE)-$(GIT_SHORT_HASH)-dev

# Detect OS and architecture
UNAME_S := $(shell uname -s)
UNAME_M := $(shell uname -m)

# Compilers
CXX    := g++
WINCXX := x86_64-w64-mingw32-g++

# On macOS, prefer the clang++ that ships with the Xcode Command Line Tools
ifeq ($(UNAME_S),Darwin)
    CXX := clang++
endif

# Architecture-specific flags. x86 builds target a baseline ISA and tune for
# the host; ARM64 (Apple Silicon) has no AVX tiers so it just builds native.
ifeq ($(UNAME_M),arm64)
    BASEFLAGS    := -std=c++20 -DNNUE_PATH=\"$(EVALFILE)\" -DVERSION=\"$(VERSION)\"
    DEBUGFLAGS   := -g -fsanitize=address,undefined
    NATIVE_MARCH :=
else
    BASEFLAGS    := -std=c++20 -DNNUE_PATH=\"$(EVALFILE)\" -m64 -DVERSION=\"$(VERSION)\"
    DEBUGFLAGS   := -g -march=x86-64-v3 -fsanitize=address,undefined
    NATIVE_MARCH := -march=native
endif
OPTFLAGS := -O3 -flto=auto

# PGO profile handling differs: Apple Clang emits LLVM .profraw (merged with
# llvm-profdata), GCC emits .gcda read directly by -fprofile-use.
ifeq ($(UNAME_S),Darwin)
    PGO_GENERATE := -fprofile-generate
    PGO_USE      := -fprofile-use=pgo.profdata
    PGO_MERGE    := xcrun llvm-profdata merge -output=pgo.profdata *.profraw
    PGO_CLEAN    := rm -f *.profraw pgo.profdata
else
    PGO_GENERATE := -fprofile-generate
    PGO_USE      := -fprofile-use
    PGO_MERGE    := true
    PGO_CLEAN    := rm -f engine/*.gcda engine/nnue/*.gcda Pyrrhic/*.gcda
endif

# Sources & objects
SRCS := $(wildcard engine/*.cpp engine/nnue/*.cpp Pyrrhic/tbprobe.cpp)
HDRS := $(wildcard engine/*.hpp engine/nnue/*.hpp Pyrrhic/tbprobe.h)
OBJS := $(SRCS:.cpp=.o)
DEPS := $(OBJS:.o=.d)

.PHONY: all native binaries debug clean pgo pgo-generate pgo-use

# Default: profile-guided optimized build
all: pgo

native: CXXFLAGS = $(BASEFLAGS) $(OPTFLAGS) $(NATIVE_MARCH)
native: $(EXE)

debug: CXXFLAGS = $(BASEFLAGS) $(DEBUGFLAGS)
debug: $(EXE)

# Multi-binary builds (Linux/Windows cross-compile only)
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

# Cleanup
clean:
	@echo "Cleaning up..."
	rm -f $(OBJS) $(DEPS)
	rm -f engine/*.gcda engine/nnue/*.gcda Pyrrhic/*.gcda
	rm -f *.profraw pgo.profdata

# Profile-guided optimization. Make does not treat CXXFLAGS as a prerequisite,
# so each phase forces a clean rebuild via a sub-make with the flags it needs;
# otherwise stale objects from a prior build would link in uninstrumented and
# produce no profile data.
pgo:
	$(MAKE) clean
	$(MAKE) pgo-generate
	@echo "Running PGO instrumentation..."
	./$(EXE) bench
	$(PGO_MERGE)
	rm -f $(OBJS) $(DEPS)
	@echo "Recompiling with PGO optimizations..."
	$(MAKE) pgo-use
	$(PGO_CLEAN)

pgo-generate: CXXFLAGS = $(BASEFLAGS) $(OPTFLAGS) $(PGO_GENERATE) $(NATIVE_MARCH)
pgo-generate: $(EXE)

pgo-use: CXXFLAGS = $(BASEFLAGS) $(OPTFLAGS) $(PGO_USE) $(NATIVE_MARCH)
pgo-use: $(EXE)
