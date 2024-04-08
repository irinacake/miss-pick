CXXFLAGS += $(shell otawa-config --cflags) --std=c++11 -g3
LIBS += $(shell otawa-config --libs --rpath)

OBJS = cacheAnalysisUtils.o cacheAnalysis.o


cacheAnalysis: $(OBJS)
	$(CXX) $(CXXFLAGS) -o bin/$@ $^ $(LIBS)

#cacheAnalysis: $(OBJS)
#	@echo "all"
#	$(CXX) $(CXXFLAGS) -o bin/$@  $(OBJS) $^ $(LIBS) 
#
cacheAnalysisUtils.o: cacheAnalysisUtils.cpp cacheAnalysisUtils.hpp
	$(CXX) $(CXXFLAGS) -c $<

cacheAnalysis.o: cacheAnalysis.cpp cacheAnalysisUtils.hpp
	$(CXX) $(CXXFLAGS) -c $< 


clean:
	rm -f cacheAnalysis.o cacheAnalysisUtils.o
	rm -f bin/cacheAnalysis