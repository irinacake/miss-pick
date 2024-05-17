
#ifndef OTAWA_CACHEMISS_CFG_SET_PROJECTOR_H
#define OTAWA_CACHEMISS_CFG_SET_PROJECTOR_H


#include <elm/io.h>
#include <otawa/otawa.h>
#include <elm/sys/System.h>
#include <otawa/prog/TextDecoder.h>
#include <otawa/hard/CacheConfiguration.h>


using namespace elm;
using namespace otawa;


class BBP {
public:
	BBP () {

	}
	bool toSynth();
private:
	List<int> tags;
	List<BBP> outEdges;
	Block* oldBB;
};


class CFGP {
public:
	CFGP() {

	}
private:
	AllocArray<BBP> BBPs;
	CFG* oldCFG;
};


class CFGCollectionP {
public:
	CFGCollectionP() {

	}
	void add(CFGP& cfgp){
		CFGPs.add(cfgp);
	}
private:
	List<CFGP&> CFGPs;
};


class ProjectedCFGColl {
public:
	virtual CFGCollectionP& getGraph(int set) = 0;
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


	CFGCollectionP& getGraph(int set) override {
		return cfgsP[set];
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


	AllocArray<CFGCollectionP> cfgsP;

};




#endif // OTAWA_CACHEMISS_CFG_SET_PROJECTOR_H