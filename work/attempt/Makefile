CXXFLAGS += $(shell otawa-config --cflags) --std=c++11
LIBS += $(shell otawa-config --libs --rpath)

all: attempt

attempt: attempt.o
	$(CXX) $(CXXFLAGS) -o bin/$@ $^ $(LIBS)
	rm attempt.o

clean:
	rm -f bin/attempt p1.dot p2.dot