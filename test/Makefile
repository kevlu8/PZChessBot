SRCS := $(wildcard *.cpp)
OBJS := $(SRCS:.cpp=.o)

.PHONY: test all clean

test: $(OBJS)
	$(CXX) $(CXXFLAGS) -o test $^ objs.a
	./test -d

%.o: %.cpp $(HDRS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) test
