CXXFLAGS += $(shell otawa-config --cflags) --std=c++11 -g3
LIBS += $(shell otawa-config --libs --rpath)
OBJDIR = bin
OBJS =  $(OBJDIR)/test.o $(OBJDIR)/CacheSetState.o $(OBJDIR)/CacheSetsSaver.o $(OBJDIR)/MultipleSetsSaver.o $(OBJDIR)/CacheFaultAnalyser.o $(OBJDIR)/CFGSetProjector.o


test: $(OBJS)
	@mkdir -p build
	$(CXX) $(CXXFLAGS) -o build/$@ $^ $(LIBS)

$(OBJDIR)/test.o: test.cpp MultipleSetsSaver.h
	@mkdir -p $(OBJDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@
	
$(OBJDIR)/CacheFaultAnalyser.o: CacheFaultAnalyser.cpp MultipleSetsSaver.h CFGSetProjector.h
	@mkdir -p $(OBJDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJDIR)/%.o: %.cpp %.h
	@mkdir -p $(OBJDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f build/test
	rm -rf $(OBJDIR)