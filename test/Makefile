SRCS := $(wildcard *.cpp)
OBJS := $(SRCS:.cpp=.o)

.PHONY: test all clean

test: CXXFLAGS += -O3
test: all
	./test

debug: CXXFLAGS += -g -fsanitize=address,undefined
debug: all

all: $(OBJS)
	$(CXX) $(CXXFLAGS) -o test $^ objs.a

%.o: %.cpp Makefile
	$(CXX) $(CXXFLAGS) -I../engine -c $< -o $@

clean:
	rm -f $(OBJS) test
