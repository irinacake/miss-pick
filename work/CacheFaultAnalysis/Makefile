CXXFLAGS += $(shell otawa-config --cflags) --std=c++11 -g3
LIBS += $(shell otawa-config --libs --rpath)
OBJDIR = bin/
OBJS =  $(OBJDIR)/tester.o $(OBJDIR)/CacheFaultAnalysisUtils.o $(OBJDIR)/CacheFaultAnalysis.o


tester: $(OBJS)
	@mkdir -p build
	$(CXX) $(CXXFLAGS) -o  build/$@ $^ $(LIBS)

$(OBJDIR)/%.o: %.cpp CacheFaultAnalysisFeature.h
	@mkdir -p $(OBJDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f build/tester
	rm -rf $(OBJDIR)