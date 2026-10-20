# Project settings
EXE				?= pzchessbot
EVALFILE		?= nnue.bin

GIT_SHORT_HASH	:= $(shell git rev-parse --short HEAD)
GIT_DATE		:= $(shell git log -1 --format=%cd --date=format:"%Y%m%d")

VERSION			:= v$(GIT_DATE)-$(GIT_SHORT_HASH)-dev

# Compilers
CXX	?= g++

# Flags
BASEFLAGS   := -std=c++20 -DNNUE_PATH=\"$(EVALFILE)\" -DVERSION=\"$(VERSION)\"
OPTFLAGS    := -O3 -flto=auto
DEBUGFLAGS  := -g -march=x86-64-v3 -fsanitize=address,undefined
LDFLAGS		:= 

ifeq ($(OS),Windows_NT)
	LDFLAGS += -Wl,--stack,8388608
else
	UNAME_S := $(shell uname -s)

	ifeq ($(UNAME_S),Linux)
		LDFLAGS += -Wl,-z,stack-size=8388608
	endif
endif

# NUMA handling
NUMA_NODES := 1

ifeq ($(OS),Windows_NT)
# no numa support here
else
	HAS_LSCPU := $(shell command -v lscpu 2> /dev/null)
	ifneq ($(HAS_LSCPU),)
		NUMA_NODES := $(shell lscpu | grep "^NUMA node(s):" | awk '{print $$3}')
		ifeq ($(NUMA_NODES),)
			NUMA_NODES := 1
		endif
		HAS_NUMA := $(shell $(CXX) -lnuma -shared -o /dev/null -x c /dev/null >/dev/null 2>&1 && echo yes || echo no)
		ifeq ($(HAS_NUMA),no)
			NUMA_NODES := 1
		endif
	endif
endif

ifneq ($(NUMA_NODES),1)
	OPTFLAGS += -DUSE_NUMA
	LDFLAGS += -lnuma
endif

# Sources & objects
SRCS	:= $(shell find engine -name "*.cpp") Pyrrhic/tbprobe.cpp
HDRS	:= $(shell find engine -name "*.hpp") Pyrrhic/tbprobe.h
OBJS	:= $(SRCS:.cpp=.o)
DEPS	:= $(OBJS:.o=.d)

.PHONY: all no-pext v3 v4 vnni arm native debug clean pgo pgo-generate pgo-compile test test-debug

# Default: native build
all: pgo

no-pext: CXXFLAGS += $(BASEFLAGS) $(OPTFLAGS) -march=znver2
no-pext: $(EXE)

v3: CXXFLAGS += $(BASEFLAGS) $(OPTFLAGS) -march=x86-64-v3
v3: $(EXE)

v4: CXXFLAGS += $(BASEFLAGS) $(OPTFLAGS) -march=x86-64-v4
v4: $(EXE)

vnni: CXXFLAGS += $(BASEFLAGS) $(OPTFLAGS) -march=icelake-server
vnni: $(EXE)

arm: CXX := aarch64-linux-gnu-g++
arm: CXXFLAGS += $(BASEFLAGS) $(OPTFLAGS) -static
arm: $(EXE)

native: CXXFLAGS += $(BASEFLAGS) $(OPTFLAGS) -march=native
native: $(EXE)

debug: CXXFLAGS += $(BASEFLAGS) $(DEBUGFLAGS)
debug: $(EXE)

# Link final binary
$(EXE): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)
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
	rm -f $(shell find engine -name "*.gcda") Pyrrhic/*.gcda

pgo-compile: CXXFLAGS += $(BASEFLAGS) $(OPTFLAGS) -fprofile-use -march=native
pgo-compile: $(EXE)
	rm -f $(shell find engine -name "*.gcda") Pyrrhic/*.gcda

# Always regenerate instrumented objects: objects from native/debug/profile-use
# builds do not contain the counters needed to train a PGO build.
pgo-generate: CXXFLAGS += $(BASEFLAGS) $(OPTFLAGS) -fprofile-generate -march=native
pgo-generate: $(EXE)

pgo:
	$(MAKE) clean
	$(MAKE) pgo-generate
	@echo "Running PGO instrumentation..."
	./$(EXE) bench
	rm $(OBJS) $(EXE)
	@echo "Recompiling with PGO optimizations..."
	$(MAKE) pgo-compile

# Build correctness checks independently of the engine's current object flags.
TEST_SRCS := tests/test_accumulator.cpp engine/eval.cpp engine/bitboard.cpp engine/movegen.cpp \
	engine/ttable.cpp engine/nnue/accumulator.cpp engine/nnue/network.cpp engine/nnue/threats.cpp \
	engine/arch/arch.cpp engine/arch/simd/simd.cpp

test:
	mkdir -p build
	$(CXX) $(BASEFLAGS) $(OPTFLAGS) -march=native $(TEST_SRCS) -o build/test-accumulator $(LDFLAGS)
	./build/test-accumulator

test-debug:
	mkdir -p build
	$(CXX) $(BASEFLAGS) $(DEBUGFLAGS) -O1 $(TEST_SRCS) -o build/test-accumulator-debug $(LDFLAGS)
	UBSAN_OPTIONS=halt_on_error=1 ./build/test-accumulator-debug
