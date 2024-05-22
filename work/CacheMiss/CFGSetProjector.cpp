#include "CFGSetProjector.h"
#include <otawa/cfg/Loop.h>
#include "CacheMissDebug.h"


elm::io::Output &operator<<(elm::io::Output &output, const CFGCollectionP &collP) {
    output << "{";
    for (auto c: collP._CFGPs){
        output << c << endl;
    }
    output << "}";
    return output;
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



bool CfgSetProjectorProcessor::belongsTo(Block* bb, int set) {
    for (auto inst : *bb->toBasic()){
        if (set == icache->set(inst->address())){
            return true;
        }
    }
    return false;
}


void CfgSetProjectorProcessor::processAll(WorkSpace *ws) {  

    DEBUG("CFG Projector - processing" << endl);
	
    cfgColl = COLLECTED_CFG_FEATURE.get(workspace());
    icache = hard::CACHE_CONFIGURATION_FEATURE.get(workspace())->instCache();
    entryCfg = cfgColl->entry();


    setCount = icache->setCount();
    cfgsP.allocate(setCount);


    
    Vector<CFG *> todoCfg;
    Vector<Pair<BBP *,Block *>> todo;
    //p::id<bool> SYNTHCALL("SYNTHCALL");
    //for (auto c: *cfgColl) {
    //    SYNTHCALL(c) = false;
    //}

    // set by set loop
    for (int currSet=0; currSet < setCount; currSet++){

        DEBUG("\n\n\n------------\nprocessing set : " << currSet << "\n------------" << endl);
        //CFGCollectionP collP;
        //cfgsP[currSet] = collP;

        todoCfg.add(entryCfg);

        while (!todoCfg.isEmpty()){
            auto cfg = todoCfg.pop();
            auto cfgP = new CFGP(cfg);

            DEBUG("processing cfg : " << cfg->name() << endl);

            cfgsP[currSet].add(cfgP);


            auto alpha = new BBP(cfg->entry());
            cfgP->addBBP(alpha);

            for (auto e: cfg->entry()->outEdges()){
                auto sink = e->sink();
                todo.add(pair(alpha,sink));
            }

            while (!todo.isEmpty()){
                auto currItem = todo.pop();
                auto prev = currItem.fst;
                auto bb = currItem.snd;
                DEBUG("\tprocessing bb  : " << bb << endl);

                if ( bb->isBasic()
                        && !belongsTo(bb, currSet)
                            && !Loop::of(bb)->isHeader(bb) ){
                    
                    DEBUG("\t\tbasic, wrong set and not loop header" << endl);
                    for (auto e: bb->outEdges()){
                        auto sink = e->sink();
                        DEBUG("\t\tto add : " << sink << endl);
                        todo.add(pair(prev,sink));
                        
                    }
                } else {
                    DEBUG("\t\t else" << endl);
                    //if (bb->isSynth() && !SYNTHCALL(bb->toSynth()->callee())){
                    //    SYNTHCALL(bb->toSynth()->callee()) = true;
                    //    todoCfg.add(bb->toSynth()->callee());
                    //}
                    if (bb->isSynth() && !cfgsP[currSet].CFGPs().hasKey(bb->toSynth()->callee()) ){
                        DEBUG("\t\t\tblock is synth and callee is new" << endl);
                        todoCfg.add(bb->toSynth()->callee());
                    }

                    BBP* bbp;
                    if ((*cfgP->BBPs())[bb->index()] == nullptr) {
                        DEBUG("\t\t\tcreating new BBP" << endl);
                        bbp = new BBP(bb);
                        cfgP->addBBP(bbp);

                        DEBUG("\t\t\t\tAdding " << bbp->oldBB() << " to " << prev->oldBB() << endl);
                        prev->addOutEdge(bbp);

                        for (auto e: bb->outEdges()){
                            auto sink = e->sink();
                            DEBUG("\t\t\tto add : " << sink << endl);
                            if ( (*cfgP->BBPs())[sink->index()] == nullptr || !bbp->outEdges().contains((*cfgP->BBPs())[sink->index()]) ){
                                if (!todo.contains(pair(bbp,sink))) {
                                    todo.add(pair(bbp,sink));
                                }
                            }
                        }
                    } else {
                        bbp = (*cfgP->BBPs())[bb->index()];
                        prev->addOutEdge(bbp);
                    }
                }
            }
        }
    }
}



void CfgSetProjectorProcessor::dump(WorkSpace *ws, Output &out) {
    out << "Dumping projector" << endl;

    for (int i=0; i<setCount; i++){
        out << "\nProjection for set " << i << endl;
        for (auto c: cfgsP[i].CFGPs()) {
            out << "Sub CFG : " << c->oldCFG()->name() << endl;
            out << "\t=====" << endl;
            for (auto bp : *c->BBPs()){
                if (bp != nullptr) {
                    out << "\t--Projected BB : " << bp->oldBB() << endl;
                    for (auto e: bp->outEdges()){
                        out << "\t\t-> Edge to : " << e->oldBB() << endl;
                    }
                    out << "\t\t-----" << endl;
                }
            }
            out << "\t=====" << endl;
        }
    }

    out << "done" << endl;
}