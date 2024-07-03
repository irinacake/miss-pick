
#ifndef OTAWA_CACHEMISS_CFG_SET_PROJECTOR_H
#define OTAWA_CACHEMISS_CFG_SET_PROJECTOR_H


#include <elm/io.h>
#include <otawa/otawa.h>
#include <elm/sys/System.h>
#include <otawa/prog/TextDecoder.h>
#include <otawa/hard/CacheConfiguration.h>
#include <elm/data/ListMap.h>


using namespace elm;
using namespace otawa;


class BBP;
class BBPSynth;
class CFGP;


/**
 * Projected Block classes
*/

class BBP: public PropList {
public:
	BBP (Block* oldBB): _oldBB(oldBB) {}

	class BBPEquiv {
	public:
		static inline bool isEqual(BBP *bbp1, BBP *bbp2){
			return bbp1->equals(*bbp2);
		}
	};

	inline List<int> tags(void){ return _tags; }
	inline void addTag(int newTag) { _tags.add(newTag); }

	inline List<BBP*,BBPEquiv> outEdges(void){ return _outEdges; }
	inline void addOutEdge(BBP* newEdge){ if (!_outEdges.contains(newEdge)) _outEdges.add(newEdge); }
	inline void removeOutEdge(BBP* edgeToDelete){ _outEdges.remove(edgeToDelete); }

	//inline List<BBP*,BBPEquiv> prevs(void){ return _prevs; }
	//inline void addPrev(BBP* newPrev){ if (!_prevs.contains(newPrev)) _prevs.add(newPrev); }
	//inline void removePrev(BBP* prevToDelete){ _prevs.remove(prevToDelete); }

	inline Block* oldBB(void){ return _oldBB; }

	inline int index(void) { return _oldBB->index(); }

	inline BBPSynth *toSynth(void);

	inline bool equals(BBP& other){ return _oldBB->index() == other._oldBB->index(); }

	friend elm::io::Output &operator<<(elm::io::Output &output, const BBP &bbp);

private:
	List<int> _tags;
	List<BBP*,BBPEquiv> _outEdges;
	//List<BBP*,BBPEquiv> _prevs;
	Block* _oldBB;
};


class BBPSynth: public BBP {
public:
	BBPSynth(Block* block, CFGP* callee): BBP(block), _callee(callee) {}

	inline CFGP* callee() { return _callee; }
private:
	CFGP* _callee;
};

// delayed definition
inline BBPSynth *BBP::toSynth(void) { return static_cast<BBPSynth *>(this); }



/**
 * Projected CFG and Collection classes
*/

class CFGP {
public:
	CFGP(CFG* cfg): _oldCFG(cfg) {
		involved = false;
		_BBPs.allocate(cfg->count());
		for(int i=0; i < cfg->count(); i++){
			_BBPs[i] = nullptr;
		}
	}
	~CFGP(){ for (auto bbp: _BBPs){ delete(bbp); } }

	inline AllocArray<BBP*>* BBPs(void){ return &_BBPs; }
	inline void addBBP(BBP* bbp) { _BBPs[bbp->index()] = bbp; }
	inline void removeBBP(BBP* bbp) { _BBPs[bbp->index()] = nullptr; delete(bbp); }
	inline BBP* entry(void){ return _BBPs[0]; }
	inline BBP* get(int x){ return _BBPs[x]; }

	inline void setInvolved() { involved = true; }
	inline bool isInvolved() { return involved; }

	inline CFG* oldCFG(void) { return _oldCFG; }

	friend elm::io::Output &operator<<(elm::io::Output &output, const CFGP &cfgp);

private:
	AllocArray<BBP*> _BBPs;
	CFG* _oldCFG;
	bool involved;
};


class CFGCollectionP {
public:
	CFGCollectionP(int set, const CFGCollection* cfgColl): _set(set), _oldCfgColl(cfgColl){}
	~CFGCollectionP(){ for (auto cfgp: _CFGPs){ delete(cfgp); } }

	inline void add(CFGP* cfgp){
		_CFGPs.put(cfgp->oldCFG(), cfgp);
	}
	inline void remove(CFGP* cfgp){
		_CFGPs.remove(cfgp->oldCFG());
		delete(cfgp);
	}
	//inline void add(CFGP* cfgp){ _CFGPs.add(cfgp); }
	inline ListMap<CFG*,CFGP*> CFGPs(){ return _CFGPs; }

	inline CFGP* entry(void) {
		return _CFGPs.get(_oldCfgColl->entry());
	}

	inline CFGP* get(CFG* cfg) {
		return _CFGPs.get(cfg,nullptr);
	}

	friend elm::io::Output &operator<<(elm::io::Output &output, const CFGCollectionP &collP);

private:
	int _set;
	const CFGCollection* _oldCfgColl;
	ListMap<CFG*,CFGP*> _CFGPs;
};




class ProjectedCFGColl {
public:
	virtual CFGCollectionP* graphOfSet(int set) = 0;
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
	~CfgSetProjectorProcessor(){ for (auto cfgp:cfgsP) { delete(cfgp); } }

	CFGCollectionP* graphOfSet(int set) override {
		ASSERTP(set >= 0 && set < setCount, "set value out of bound");
		return cfgsP[set];
	}


protected:
	void setup(WorkSpace *ws) override;
	void processAll(WorkSpace *ws) override;
	void processCFG(WorkSpace *ws, CFG *cfg) override {}
    void dump(WorkSpace *ws, Output &out) override;
	bool belongsTo(Block* bb, int set);

private:
	int setCount;
	AllocArray<CFGCollectionP*> cfgsP;
	const CFGCollection* cfgColl;
	const hard::Cache* icache;
	CFG* entryCfg;
};




#endif // OTAWA_CACHEMISS_CFG_SET_PROJECTOR_H