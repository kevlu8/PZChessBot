EXE ?= pzchessbot
EVALFILE ?= nnue.bin

CXX := g++
CXXFLAGS := -O3 -std=c++17 -march=native -DNNUE_PATH=\"$(EVALFILE)\"

SRCS := $(wildcard engine/*.cpp engine/nnue/*.cpp)
OBJS := $(SRCS:.cpp=.o)

$(EXE): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^
	@echo "Build complete. Run with './$(EXE)'"

%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

.PHONY: clean
clean:
	@echo "Cleaning up..."
	rm -f $(EXE)
	rm -f $(OBJS)