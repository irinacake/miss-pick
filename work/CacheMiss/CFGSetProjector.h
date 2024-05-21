
#ifndef OTAWA_CACHEMISS_CFG_SET_PROJECTOR_H
#define OTAWA_CACHEMISS_CFG_SET_PROJECTOR_H


#include <elm/io.h>
#include <otawa/otawa.h>
#include <elm/sys/System.h>
#include <otawa/prog/TextDecoder.h>
#include <otawa/hard/CacheConfiguration.h>


using namespace elm;
using namespace otawa;


class BBP;
class BBPSynth;
class CFGP;


/**
 * Projected Block classes
*/

class BBP {
public:
	BBP (const Block& oldBB): _oldBB(oldBB) {}

	inline List<int> tags(void){ return _tags; }
	inline void addTag(int newTag) { _tags.add(newTag); }

	inline List<BBP&> outEdges(void){ return _outEdges; }
	inline void addoutEdge(BBP& newEdge){ _outEdges.add(newEdge); }

	inline const Block& oldBB(void){ return _oldBB; }

	inline int index(void) { return _oldBB.index(); }

	inline BBPSynth *toSynth(void);
	
private:
	List<int> _tags;
	List<BBP&> _outEdges;
	const Block& _oldBB;
};



class BBPSynth: public BBP {
public:
	BBPSynth(Block& block, const CFGP& callee): BBP(block), _callee(callee) {}

	inline const CFGP& callee() { return _callee; }
private:
	const CFGP& _callee;
};



// delayed definition
inline BBPSynth *BBP::toSynth(void) { return static_cast<BBPSynth *>(this); }


/**
 * Projected CFG and Collection classes
*/

class CFGP {
public:
	CFGP(const CFG& cfg): _oldCFG(cfg) { _BBPs.allocate(cfg.count()); }
	~CFGP(){
		for (auto bbp: _BBPs)
			delete(bbp);
	}

	inline AllocArray<BBP*>* BBPs(void){ return &_BBPs; }
	inline void addBBP(BBP* bbp) { _BBPs[bbp->index()] = bbp; }
	inline BBP* entry(void){ return _BBPs[0]; }

	inline const CFG& oldCFG(void) { return _oldCFG; }

private:
	AllocArray<BBP*> _BBPs;
	const CFG& _oldCFG;
};


class CFGCollectionP {
public:
	CFGCollectionP() {}
	~CFGCollectionP(){ // Destructor
		for (auto cfgp: _CFGPs)
            delete(cfgp);
    }

	inline void add(CFGP* cfgp){ _CFGPs.add(cfgp); }
	inline List<CFGP*> CFGPs(){ return _CFGPs; }

private:
	List<CFGP*> _CFGPs;
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
		ASSERTP(set > 0 && set < setCount, "set value out of bound");
		return cfgsP[set];
	}


protected:
	void processAll(WorkSpace *ws) override;
	void processCFG(WorkSpace *ws, CFG *cfg) override {}
    void dump(WorkSpace *ws, Output &out) override;
	//void configure() override;

private:
	int exec_time;
	int exit_value;


	int setCount;
	AllocArray<CFGCollectionP> cfgsP;
};




#endif // OTAWA_CACHEMISS_CFG_SET_PROJECTOR_H