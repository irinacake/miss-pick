CXXFLAGS += $(shell otawa-config otawa/trivial --cflags) --std=c++11 -g3 -fsanitize=address
LIBS += $(shell otawa-config otawa/trivial --libs --rpath)
OBJDIR = bin
OBJS =  $(OBJDIR)/test.o $(OBJDIR)/CacheSetState.o $(OBJDIR)/CacheSetsSaver.o $(OBJDIR)/MultipleSetsSaver.o $(OBJDIR)/AbstractCacheSetState.o $(OBJDIR)/CacheMissAnalyser.o $(OBJDIR)/CFGSetProjector.o


test: $(OBJS)
	@mkdir -p build
	$(CXX) $(CXXFLAGS) -o build/$@ $^ $(LIBS)

$(OBJDIR)/test.o: test.cpp CacheMissFeature.h
	@mkdir -p $(OBJDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@
	
$(OBJDIR)/CacheMissAnalyser.o: CacheMissAnalyser.cpp CacheMissFeature.h
	@mkdir -p $(OBJDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJDIR)/%.o: %.cpp %.h
	@mkdir -p $(OBJDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f build/test
	rm -rf $(OBJDIR)