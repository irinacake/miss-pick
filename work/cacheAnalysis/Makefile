CXXFLAGS += $(shell otawa-config --cflags) --std=c++11 -g3
LIBS += $(shell otawa-config --libs --rpath)

all: cacheAnalysis

cacheAnalysis: cacheAnalysis.o
	$(CXX) $(CXXFLAGS) -o bin/$@ $^ $(LIBS) 

clean:
	rm cacheAnalysis.o
	rm -f bin/cacheAnalysis p1.dot p2.dot