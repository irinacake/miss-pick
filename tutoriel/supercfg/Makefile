CXXFLAGS += $(shell otawa-config --cflags) --std=c++11
LIBS += $(shell otawa-config --libs --rpath)

all: supercfg

supercfg: supercfg.o
	$(CXX) $(CXXFLAGS) -o bin/$@ $^ $(LIBS)
	rm supercfg.o

clean:
	rm -f bin/supercfg p1.dot p2.dot