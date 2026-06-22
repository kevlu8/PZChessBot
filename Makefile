# Project settings
EXE				?= pzchessbot
EVALFILE		?= nnue.bin

GIT_SHORT_HASH	:= $(shell git rev-parse --short HEAD)
GIT_DATE		:= $(shell git log -1 --format=%cd --date=format:"%Y%m%d")

VERSION			:= v$(GIT_DATE)-$(GIT_SHORT_HASH)-dev

# Compilers
CXX	?= g++

# Flags
BASEFLAGS	:= -std=c++20 -DNNUE_PATH=\"$(EVALFILE)\" -DVERSION=\"$(VERSION)\"
OPTFLAGS	:= -O3 -flto=auto
DEBUGFLAGS	:= -g -m64 -march=x86-64-v3 -fsanitize=address,undefined

# Sources & objects
SRCS	:= $(shell find engine -name "*.cpp") Pyrrhic/tbprobe.cpp
HDRS	:= $(shell find engine -name "*.hpp") Pyrrhic/tbprobe.h
OBJS	:= $(SRCS:.cpp=.o)
DEPS	:= $(OBJS:.o=.d)

.PHONY: all no-pext v3 v4 vnni arm native debug clean pgo pgo-compile

# Default: native build
all: pgo

no-pext: CXXFLAGS := $(BASEFLAGS) $(OPTFLAGS) -march=znver2
no-pext: $(EXE)

v3: CXXFLAGS := $(BASEFLAGS) $(OPTFLAGS) -march=x86-64-v3
v3: $(EXE)

v4: CXXFLAGS := $(BASEFLAGS) $(OPTFLAGS) -march=x86-64-v4
v4: $(EXE)

vnni: CXXFLAGS := $(BASEFLAGS) $(OPTFLAGS) -march=icelake-server
vnni: $(EXE)

arm: CXX := aarch64-linux-gnu-g++
arm: CXXFLAGS := $(BASEFLAGS) $(OPTFLAGS) -static
arm: $(EXE)

native: CXXFLAGS := $(BASEFLAGS) $(OPTFLAGS) -march=native
native: $(EXE)

debug: CXXFLAGS := $(BASEFLAGS) $(DEBUGFLAGS)
debug: $(EXE)

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
	rm -f $(shell find engine -name "*.gcda") Pyrrhic/*.gcda

pgo-compile: CXXFLAGS = $(BASEFLAGS) $(OPTFLAGS) -fprofile-use -march=native
pgo-compile: $(EXE)
	rm -f $(shell find engine -name "*.gcda") Pyrrhic/*.gcda

pgo: CXXFLAGS = $(BASEFLAGS) $(OPTFLAGS) -fprofile-generate -march=native
pgo: $(EXE)
	@echo "Running PGO instrumentation..."
	./$(EXE) bench
	rm $(OBJS)
	@echo "Recompiling with PGO optimizations..."
	$(MAKE) pgo-compile
