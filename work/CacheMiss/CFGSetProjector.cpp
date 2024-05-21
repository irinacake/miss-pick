#include "CFGSetProjector.h"

#include <otawa/cfg/Loop.h>



bool belongsTo(Block* bb, int set) {
    // TODO
    return true;
}

p::interfaced_feature<ProjectedCFGColl> CFG_SET_PROJECTOR_FEATURE("CFG_SET_PROJECTOR_FEATURE", p::make<CfgSetProjectorProcessor>());


CfgSetProjectorProcessor::CfgSetProjectorProcessor(): CFGProcessor(reg) {
}

p::declare CfgSetProjectorProcessor::reg = p::init("CfgSetProjectorProcessor", Version(1, 0, 0))
    .make<CfgSetProjectorProcessor>()
    .extend<CFGProcessor>()
    .provide(CFG_SET_PROJECTOR_FEATURE)
    .require(DECODED_TEXT)
    .require(COLLECTED_CFG_FEATURE)
    .require(otawa::hard::CACHE_CONFIGURATION_FEATURE)
    .require(EXTENDED_LOOP_FEATURE);


void CfgSetProjectorProcessor::processAll(WorkSpace *ws) {  

    cout << "CFG Projector - processing" << endl;
	
    cfgColl = COLLECTED_CFG_FEATURE.get(workspace());
    icache = hard::CACHE_CONFIGURATION_FEATURE.get(workspace())->instCache();
    entryCfg = cfgColl->entry();


    setCount = icache->setCount();
    cfgsP.allocate(setCount);


    
    Vector<const CFG *> todoCfg;
    Vector<Pair<BBP *,Block *>> todo;
    p::id<bool> SYNTHCALL("SYNTHCALL");
    for (auto c: *cfgColl) {
        SYNTHCALL(c) = false;
    }

    // set by set loop
    for (int currSet=0; currSet < setCount; currSet++){

        //CFGCollectionP collP;
        //cfgsP[currSet] = collP;

        todoCfg.add(entryCfg);

        while (!todoCfg.isEmpty()){
            auto cfg = todoCfg.pop();
            auto cfgP = new CFGP(*cfg);

            cfgsP[currSet].add(cfgP);


            auto alpha = new BBP(*cfg->entry());
            cfgP->addBBP(alpha);

            for (auto e: cfg->entry()->outEdges()){
                auto sink = e->sink();
                todo.add(pair(alpha,sink));
            }

            while (!todo.isEmpty()){
                auto currItem = todo.pop();
                auto prev = currItem.fst;
                auto bb = currItem.snd;

                if ( bb->isBasic()
                        && !belongsTo(bb, currSet)
                            && Loop::of(bb)->isHeader(bb) ){
                    for (auto e: bb->outEdges()){
                        auto sink = e->sink();
                        if ( !( e->isReturn() && Loop::of(bb)->equals(Loop::of(&prev->oldBB())) ) ){
                            todo.add(pair(prev,sink));
                        }
                    }
                } else {
                    if (bb->isSynth() && !SYNTHCALL(bb->toSynth()->callee())){
                        SYNTHCALL(bb->toSynth()->callee()) = true;
                        todoCfg.add(bb->toSynth()->callee());
                    }

                    BBP* bbp;
                    if ((*cfgP->BBPs())[bb->index()] == nullptr) {
                        bbp = new BBP(*bb);
                        cfgP->addBBP(bbp);
                    } else {
                        bbp = (*cfgP->BBPs())[bb->index()];
                    }

                    prev->addOutEdge(bbp);

                    for (auto e: bb->outEdges()){
                        auto sink = e->sink();
                        if ( bbp->outEdges().contains((*cfgP->BBPs())[sink->index()]) ){
                            todo.add(pair(bbp,sink));
                        }
                    }
                }
            }
        }
    }
}



void CfgSetProjectorProcessor::dump(WorkSpace *ws, Output &out) {

}