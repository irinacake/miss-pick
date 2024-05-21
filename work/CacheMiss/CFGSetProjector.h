
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

class BBP {
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
	inline void addOutEdge(BBP* newEdge){ _outEdges.add(newEdge); }

	inline Block* oldBB(void){ return _oldBB; }

	inline int index(void) { return _oldBB->index(); }

	inline BBPSynth *toSynth(void);

	inline bool equals(BBP& other){
		cout << "equals called : " << (_oldBB->index() == other.index()) << endl;
		return _oldBB->index() == other._oldBB->index();
		}

private:
	List<int> _tags;
	List<BBP*,BBPEquiv> _outEdges;
	Block* _oldBB;
};


class BBPSynth: public BBP {
public:
	BBPSynth(Block* block, const CFGP& callee): BBP(block), _callee(callee) {}

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
	CFGP(CFG* cfg): _oldCFG(cfg) { _BBPs.allocate(cfg->count()); }
	~CFGP(){
		for (auto bbp: _BBPs)
			delete(bbp);
	}

	inline AllocArray<BBP*>* BBPs(void){ return &_BBPs; }
	inline void addBBP(BBP* bbp) { _BBPs[bbp->index()] = bbp; }
	inline BBP* entry(void){ return _BBPs[0]; }

	inline CFG* oldCFG(void) { return _oldCFG; }

private:
	AllocArray<BBP*> _BBPs;
	CFG* _oldCFG;
};


class CFGCollectionP {
public:
	CFGCollectionP() {}
	~CFGCollectionP(){ // Destructor
		for (auto cfgp: _CFGPs)
            delete(cfgp);
    }

	inline void add(CFGP* cfgp){
		_CFGPs.put(cfgp->oldCFG(), cfgp);
	}
	//inline void add(CFGP* cfgp){ _CFGPs.add(cfgp); }
	inline ListMap<CFG*,CFGP*> CFGPs(){ return _CFGPs; }

	friend elm::io::Output &operator<<(elm::io::Output &output, const CFGCollectionP &collP);

private:
	ListMap<CFG*,CFGP*> _CFGPs;
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
	bool belongsTo(Block* bb, int set);

private:
	int exec_time;
	int exit_value;


	int setCount;
	AllocArray<CFGCollectionP> cfgsP;
	const CFGCollection* cfgColl;
	const hard::Cache* icache;
	CFG* entryCfg;
};




#endif // OTAWA_CACHEMISS_CFG_SET_PROJECTOR_H