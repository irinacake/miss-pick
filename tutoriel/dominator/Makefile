CXXFLAGS += $(shell otawa-config --cflags) --std=c++11
LIBS += $(shell otawa-config --libs --rpath)

all: dominator

dominator: dominator.o
	$(CXX) $(CXXFLAGS) -o bin/$@ $^ $(LIBS)
	rm dominator.o

clean:
	rm -f bin/dominator p1.dot p2.dot