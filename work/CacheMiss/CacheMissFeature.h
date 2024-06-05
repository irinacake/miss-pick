
#ifndef OTAWA_CACHEMISS_CACHE_MISS_FEATURE_H
#define OTAWA_CACHEMISS_CACHE_MISS_FEATURE_H


#include <otawa/prog/TextDecoder.h>
#include <elm/sys/System.h>
#include <elm/options.h>


#include "CacheSetState.h"
#include "CacheSetsSaver.h"
#include "MultipleSetsSaver.h"
#include "CacheMissDebug.h"

#include "CFGSetProjector.h"


using namespace elm;
using namespace otawa;



extern p::id<bool> PROJECTION;

extern p::feature CACHE_MISS_FEATURE;


class CacheMissProcessor: public CFGProcessor {
public:
	static p::declare reg;
	CacheMissProcessor();

protected:
	void processAll(WorkSpace *ws) override;
	void processCFG(WorkSpace *ws, CFG *cfg) override {}
    void dump(WorkSpace *ws, Output &out) override;
	void configure(const PropList& props) override;


	void initState();
	void initStateP();
	void printStates();
	void computeAnalysis(CacheSetState *initState, sys::StopWatch& mySW);
	void computeProjectedAnalysis(CacheSetState *initState, sys::StopWatch& mySW);
	void makeStats(elm::io::Output &output);

	void getStats(int *mins, int *maxs, float *moys, int* bbCount, int waysCount, MultipleSetsSaver* totalStates);
	void getStatsP(int *mins, int *maxs, float *moys, int* bbCount, int* usedBbCount, int waysCount, MultipleSetsSaver* totalStates);

private:
	int exec_time;
	CacheSetState* mycache;
	const otawa::hard::Cache* icache;
	ProjectedCFGColl* pColl;
	CFG* maincfg;
	int exit_value;
	bool projection;
};


#endif // OTAWA_CACHEMISS_CACHEMISSFEATURE_H