
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
	BBP (const Block& block): storedOldBB(block) {
	}

	inline List<int> tags(void){
		return storedTags;
	}
	inline void addTag(int newTag) {
		storedTags.add(newTag);
	}

	inline List<BBP&> outEdges(void){
		return storedOutEdges;
	}
	inline void addOutEdge(BBP& newEdge){
		storedOutEdges.add(newEdge);
	}

	inline const Block& oldBB(void){
		return storedOldBB;
	}

	inline int index(void) {
		return storedOldBB.index();
	}

	inline BBPSynth *toSynth(void);
	
private:
	List<int> storedTags;
	List<BBP&> storedOutEdges;
	const Block& storedOldBB;
};



class BBPSynth: public BBP {
public:
	BBPSynth(Block& block, CFGP& toSet): BBP(block), calledCFGP(toSet) {
	}

	inline CFGP& callee() { 
		return calledCFGP; 
	}
private:
	CFGP& calledCFGP;
};



// delayed definition
inline BBPSynth *BBP::toSynth(void) {
	return static_cast<BBPSynth *>(this);
}


/**
 * Projected CFG and Collection classes
*/

class CFGP {
public:
	CFGP(const CFG& cfg): storedOldCFG(cfg) {
		storedBBPs.allocate(cfg.count());
	}

	inline AllocArray<BBP*>* BBPs(void){
		return &storedBBPs;
	}
	inline void addBBP(BBP* bbp) {
		storedBBPs[bbp->index()] = bbp;
		
	}

	inline const CFG& oldCFG(void) {
		return storedOldCFG;
	}

private:
	AllocArray<BBP*> storedBBPs;
	const CFG& storedOldCFG;
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