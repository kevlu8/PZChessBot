EXE ?= pzchessbot
EVALFILE ?= nnue.bin

CXX := g++
CXXFLAGS := -std=c++17 -static -DNNUE_PATH=\"$(EVALFILE)\" -mavx2 -mbmi2 -mbmi -mavx -m64 -mpopcnt -mlzcnt
RELEASEFLAGS = -O3
DEBUGFLAGS = -g -fsanitize=address,undefined

SRCS := $(wildcard engine/*.cpp engine/nnue/*.cpp)
HDRS := $(wildcard engine/*.hpp engine/nnue/*.hpp)
OBJS := $(SRCS:.cpp=.o)

.PHONY: release debug clean

release: CXXFLAGS += $(RELEASEFLAGS)
release: $(EXE)

debug: CXXFLAGS += $(DEBUGFLAGS)
debug: $(EXE)

$(EXE): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^
	@echo "Build complete. Run with './$(EXE)'"

%.o: %.cpp $(HDRS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	@echo "Cleaning up..."
	rm -f $(EXE)
	rm -f $(OBJS)
