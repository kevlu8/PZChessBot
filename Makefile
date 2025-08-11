EXE ?= pzchessbot
EVALFILE ?= nnue.bin

CXX := g++
CXXFLAGS := -std=c++17 -DNNUE_PATH=\"$(EVALFILE)\" -mavx2 -mbmi2 -mbmi -mavx -m64 -mpopcnt -mlzcnt -DHCE
RELEASEFLAGS = -O3
DEBUGFLAGS = -g -fsanitize=address,undefined

SRCS := $(wildcard engine/*.cpp engine/nnue/*.cpp)
HDRS := $(wildcard engine/*.hpp engine/nnue/*.hpp)
ROBJS := $(SRCS:.cpp=.o)
DOBJS := $(SRCS:.cpp=.d.o)

.PHONY: release debug test clean

release: CXXFLAGS += $(RELEASEFLAGS) -static
release: $(ROBJS)
	make OBJS="$(ROBJS)" CXXFLAGS="$(CXXFLAGS) $(RELEASEFLAGS)" $(EXE)

debug: CXXFLAGS += $(DEBUGFLAGS)
debug: $(DOBJS)
	make OBJS="$(DOBJS)" CXXFLAGS="$(CXXFLAGS) $(DEBUGFLAGS)" $(EXE)

$(EXE): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^
	@echo "Build complete. Run with './$(EXE)'"

%.o: %.cpp $(HDRS) Makefile
%.d.o: %.cpp $(HDRS) Makefile
	$(CXX) $(CXXFLAGS) -c $< -o $@

debug-test: CXXFLAGS += $(DEBUGFLAGS)
debug-test: $(DOBJS)
	$(AR) rcs test/objs.a $(DOBJS)
	make -C test CXXFLAGS="$(CXXFLAGS)" debug

test: CXXFLAGS += $(RELEASEFLAGS)
test: $(ROBJS)
	$(AR) rcs test/objs.a $(ROBJS)
	make -C test CXXFLAGS="$(CXXFLAGS)"

clean:
	@echo "Cleaning up..."
	rm -f $(EXE)
	rm -f $(ROBJS) $(DOBJS)
	rm -f test/objs.a
	make -C test clean
