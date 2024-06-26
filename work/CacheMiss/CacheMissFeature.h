
#ifndef OTAWA_CACHEMISS_CACHE_MISS_FEATURE_H
#define OTAWA_CACHEMISS_CACHE_MISS_FEATURE_H


#include <otawa/prog/TextDecoder.h>
#include <elm/sys/System.h>
#include <elm/options.h>
#include <elm/data/ListSet.h>

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
};


#endif // OTAWA_CACHEMISS_CACHEMISSFEATURE_H