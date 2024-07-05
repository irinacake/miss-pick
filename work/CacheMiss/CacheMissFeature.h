
#ifndef OTAWA_CACHEMISS_CACHE_MISS_FEATURE_H
#define OTAWA_CACHEMISS_CACHE_MISS_FEATURE_H


#include <otawa/prog/TextDecoder.h>
#include <elm/sys/System.h>
#include <elm/options.h>
#include <elm/data/ListSet.h>
#include <otawa/events/features.h>
#include <otawa/ipet.h>


#include "CacheSetState.h"
#include "AbstractCacheSetState.h"
#include "CacheSetsSaver.h"
#include "MultipleSetsSaver.h"
#include "CacheMissDebug.h"

#include "CFGSetProjector.h"


using namespace elm;
using namespace otawa;


/**
 * @defgroup cachemiss  
 * 
 * This module allows for the computation of WCET values for Projected CFGs Collections
 * with the Cache Miss Analysis Method.
 * The main objectives are :
 * - determine the possible Cache Set States when entering a Basic Block, which makes it possible to detect Always Hit and Always Miss situations
 * - determine which Basic Block is responsible for the eviction of other Basic Block, which helps calculating tighter WCET values
 * 
*/




class CacheMissEvent: public Event {
public:
	CacheMissEvent(otawa::Inst *i, Event::occurrence_t occ, const otawa::hard::Cache* icache): Event(i), _occ(occ), _icache(icache) {}
	CacheMissEvent(otawa::Inst *i, Event::occurrence_t occ, const otawa::hard::Cache* icache, BBP* bbp): Event(i), _occ(occ), _icache(icache), _bbp(bbp) {}

	ot::time cost(void) const override;
	void estimate(ilp::Constraint *cons, bool on) const override;
	bool isEstimating(bool on) const override;
	Event::kind_t kind(void) const override;
	cstring name(void) const override;
	Event::occurrence_t occurrence(void) const override;
	type_t type() const override;
	int weight(void) const override;
	string detail(void) const override;

private:
	//otawa::Inst *_inst;
	const otawa::hard::Cache* _icache;
	Event::occurrence_t _occ;
	BBP* _bbp;
};




extern p::id<bool> PROJECTION;
extern p::feature CACHE_MISS_FEATURE;
extern p::id<MultipleSetsSaver*> SAVED;
extern p::id<CacheSetsSaver*> SAVEDP;
extern p::id<ListSet<Block*>> KICKERS;
extern p::id<int> MISSVALUE;



class CacheMissProcessor: public CFGProcessor {
public:
	static p::declare reg;
	CacheMissProcessor();
	void dumpStats(WorkSpace *ws, Output &out) {dump(ws, out);}

protected:
	void processAll(WorkSpace *ws) override;
	void processCFG(WorkSpace *ws, CFG *cfg) override {}
	void destroy(WorkSpace *ws) override;
    void dump(WorkSpace *ws, Output &out) override;
	void configure(const PropList& props) override;


	void initState();
	void initStateP();
	void printStates();
	void printStatesP();
	void kickedByP();
	void computeAnalysis(AbstractCacheSetState *initState, sys::StopWatch& mySW);
	void computeProjectedAnalysis(AbstractCacheSetState *initState, sys::StopWatch& mySW);
	void makeStats(elm::io::Output &output);

	void getStats(int *mins, int *maxs, float *moys, int* bbCount, MultipleSetsSaver* totalStates);
	void getStatsP(int *mins, int *maxs, float *moys, int* bbCount, int* usedBbCount, MultipleSetsSaver* totalStates);

private:
	int exec_time;
	AbstractCacheSetState* mycache;
	const otawa::hard::Cache* icache;
	ProjectedCFGColl* pColl;
	CFG* maincfg;
	int exit_value;
	bool projection;
	int _amcpt = 0;
	int _ahcpt = 0;
	int _fmcpt = 0;
	int _nccpt = 0;
};


#endif // OTAWA_CACHEMISS_CACHEMISSFEATURE_H