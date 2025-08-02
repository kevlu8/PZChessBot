SRCS := $(wildcard *.cpp)
OBJS := $(SRCS:.cpp=.o)

.PHONY: test all clean

test: FLAGS := $(CXXFLAGS) -O3
test: all
	./test

debug: FLAGS := $(CXXFLAGS) -g -fsanitize=address,undefined
debug: all

all: $(OBJS)
	$(CXX) $(FLAGS) -o test $^ objs.a

%.o: %.cpp Makefile
	$(CXX) $(FLAGS) -I../engine -c $< -o $@

clean:
	rm -f $(OBJS) test
