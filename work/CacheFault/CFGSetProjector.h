
#ifndef OTAWA_CACHEFAULT_CFG_SET_PROJECTOR_H
#define OTAWA_CACHEFAULT_CFG_SET_PROJECTOR_H


#include <elm/io.h>
#include <otawa/otawa.h>
#include <elm/sys/System.h>
#include <otawa/prog/TextDecoder.h>
#include <otawa/hard/CacheConfiguration.h>


using namespace elm;
using namespace otawa;


class ProjectedCFGColl {
public:
	virtual int getGraph(int set) = 0;
};

extern p::interfaced_feature<ProjectedCFGColl> CFG_SET_PROJECTOR_FEATURE;


class CfgSetProjectorProcessor: public CFGProcessor, public ProjectedCFGColl {
public:
	static p::declare reg;
	CfgSetProjectorProcessor();
	void *interfaceFor(const AbstractFeature &f) override {
		if(f == CFG_SET_PROJECTOR_FEATURE)
			return static_cast<ProjectedCFGColl*>(this);
		else
			return nullptr;
	}


	int getGraph(int set) override {
		return 0;
	}


protected:
	void processAll(WorkSpace *ws) override;
	void processCFG(WorkSpace *ws, CFG *cfg) override {}
    void dump(WorkSpace *ws, Output &out) override;
	//void configure() override;

	void initState();

private:
	int exec_time;
	int exit_value;
};




#endif // OTAWA_CACHEFAULT_CFG_SET_PROJECTOR_H