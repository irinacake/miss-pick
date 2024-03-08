CXXFLAGS += $(shell otawa-config --cflags) --std=c++11
LIBS += $(shell otawa-config --libs --rpath)

all: first

first: first.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS) 

clean:
	rm -f first first.o