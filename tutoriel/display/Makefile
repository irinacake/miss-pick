CXXFLAGS += $(shell otawa-config --cflags) --std=c++11
LIBS += $(shell otawa-config --libs --rpath)

all: display

display: display.o
	$(CXX) $(CXXFLAGS) -o bin/$@ $^ $(LIBS)
	rm display.o

clean:
	rm -f bin/display