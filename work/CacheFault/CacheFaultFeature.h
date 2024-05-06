
#ifndef OTAWA_CACHEFAULT_CACHE_FAULT_FEATURE_H
#define OTAWA_CACHEFAULT_CACHE_FAULT_FEATURE_H


#include <otawa/prog/TextDecoder.h>
#include <elm/sys/System.h>


#include "CacheSetState.h"
#include "CacheSetsSaver.h"
#include "MultipleSetsSaver.h"
#include "CacheFaultDebug.h"



using namespace elm;
using namespace otawa;



extern p::feature CACHE_FAULT_ANALYSIS_FEATURE;


class CacheFaultAnalysisProcessor: public CFGProcessor {
public:
	static p::declare reg;
	CacheFaultAnalysisProcessor();

protected:
	void processAll(WorkSpace *ws) override;
	void processCFG(WorkSpace *ws, CFG *cfg) override {}
    void dump(WorkSpace *ws, Output &out) override;
	//void configure() override;

	void initState(CFG *g, string indent = "");
	void printStates();
	void computeAnalysis(CFG *g, CacheSetState *initState, sys::StopWatch& mySW);
	void makeStats(CFG *g, elm::io::Output &output);

	void getStats(CFG *g, int *mins, int *maxs, float *moys, int* bbCount, int waysCount, MultipleSetsSaver* totalStates);

private:
	int exec_time;
	CacheSetState* mycache;
	const otawa::hard::Cache* icache;
};


#endif // OTAWA_CACHEFAULT_CACHEFAULTFEATURE_H