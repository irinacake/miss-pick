#include "CFGSetProjector.h"
#include <otawa/cfg/Loop.h>
#include "CacheMissDebug.h"



elm::io::Output &operator<<(elm::io::Output &output, const BBP &bbp) {
    output << "Projected BB : " << bbp._oldBB << endl;
    for (auto e: bbp._outEdges){
        output << "\t-> Edge to : " << e->oldBB() << endl;
    }
    output << "\t-----" << endl;
    return output;
}


elm::io::Output &operator<<(elm::io::Output &output, const CFGP &cfgp) {
    output << "Sub CFG : " << cfgp._oldCFG << endl;
    output << "\t=====" << endl;
    for (auto bbp : cfgp._BBPs){
        if (bbp != nullptr) {
            output << "\t--Projected BB : " << bbp->oldBB() << endl;
            for (auto e: bbp->outEdges()){
                output << "\t\t-> Edge to : " << e->oldBB() << endl;
            }
            output << "\t\t-----" << endl;
        }
    }
    output << "\t=====" << endl;
    return output;
}


elm::io::Output &operator<<(elm::io::Output &output, const CFGCollectionP &collP) {
    for (auto c: collP._CFGPs) {
        output << "Sub CFG : " << c->oldCFG()->name() << endl;
        output << "\t=====" << endl;
        for (auto bbp : *c->BBPs()){
            if (bbp != nullptr) {
                output << "\t--Projected BB : " << bbp->oldBB() << endl;
                for (auto e: bbp->outEdges()){
                    output << "\t\t-> Edge to : " << e->oldBB() << endl;
                }
                output << "\t\t-----" << endl;
            }
        }
        output << "\t=====" << endl;
    }
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

void CfgSetProjectorProcessor::setup(WorkSpace *ws){
    cfgColl = COLLECTED_CFG_FEATURE.get(workspace());
    icache = hard::CACHE_CONFIGURATION_FEATURE.get(workspace())->instCache();
    entryCfg = cfgColl->entry();

    setCount = icache->setCount();
    cfgsP.allocate(setCount);

}

void CfgSetProjectorProcessor::processAll(WorkSpace *ws){
    DEBUGP("CFG Projector - processing" << endl);
    
    Vector<CFGP *> todoCfg;
    Vector<Pair<BBP *,Block *>> todo;

    Vector<CFG *> todoMarkedCfg;

    // set by set loop
    for (int currSet=0; currSet < setCount; currSet++){

        //cout << "\n\n\n------------\nprocessing set : " << currSet << "\n------------" << endl;
        DEBUGP("\n\n\n------------\nprocessing set : " << currSet << "\n------------" << endl);
        //CFGCollectionP collP;
        cfgsP[currSet] = new CFGCollectionP(currSet, cfgColl);


        auto cfgP = new CFGP(entryCfg);
        cfgsP[currSet]->add(cfgP);
        todoCfg.add(cfgP);
        
        while (!todoCfg.isEmpty()){
            auto currCfgp = todoCfg.pop();
            auto cfg = currCfgp->oldCFG();

            DEBUGP("processing cfg : " << cfg->name() << endl);

            auto alpha = new BBP(cfg->entry());
            currCfgp->addBBP(alpha);

            for (auto e: cfg->entry()->outEdges()){
                auto sink = e->sink();
                DEBUGP("(entry)to add : " << sink << endl);
                todo.add(pair(alpha,sink));
            }

            while (!todo.isEmpty()){
                auto currItem = todo.pop();
                auto prev = currItem.fst;
                auto bb = currItem.snd;
                DEBUGP("\tprocessing bb  : " << bb << endl);

                if ( bb->isBasic()
                        && !belongsTo(bb, currSet)
                            && !Loop::of(bb)->isHeader(bb) ){
                    
                    DEBUGP("\t\tbasic, wrong set and not loop header" << endl);
                    for (auto e: bb->outEdges()){
                        auto sink = e->sink();
                        DEBUGP("\t\tto add : " << sink << endl);
                        if (!todo.contains(pair(prev,sink))) {
                            todo.add(pair(prev,sink));
                        }
                        
                    }
                } else {
                    DEBUGP("\t\t else" << endl);
                    //if (bb->isSynth() && !SYNTHCALL(bb->toSynth()->callee())){
                    //    SYNTHCALL(bb->toSynth()->callee()) = true;
                    //    todoCfg.add(bb->toSynth()->callee());
                    //}
                    
                    if (bb->isSynth() && !cfgsP[currSet]->CFGPs().hasKey(bb->toSynth()->callee()) ){
                        DEBUGP("\t\t\tblock is synth and callee is new, adding : " << bb->toSynth()->callee()->name() << endl);
                        auto newCfgP = new CFGP(bb->toSynth()->callee());
                        cfgsP[currSet]->add(newCfgP);
                        todoCfg.add(newCfgP);
                    }

                    BBP* bbp;
                    if (currCfgp->get(bb->index()) == nullptr) {
                        DEBUGP("\t\t\tcreating new BBP" << endl);
                        if (bb->isSynth()) {
                            bbp = new BBPSynth(bb, cfgsP[currSet]->get(bb->toSynth()->callee()));
                        } else {
                            bbp = new BBP(bb);
                            if (bb->isBasic()) {
                                int currTag = -1;
                                for (auto inst : *bb->toBasic()){
                                    if (currTag != icache->block(inst->address())){ // TAG
                                        currTag = icache->block(inst->address());
                                        if (currSet == icache->set(inst->address())){
                                            bbp->addTag(icache->block(inst->address()));
                                            currCfgp->setInvolved();
                                        }
                                    }
                                }
                            }
                        }
                        currCfgp->addBBP(bbp);

                        DEBUGP("\t\t\tAdding " << bbp->oldBB() << " to " << prev->oldBB() << endl);
                        prev->addOutEdge(bbp);

                        for (auto e: bb->outEdges()){
                            auto sink = e->sink();
                            DEBUGP("\t\t\tto add : " << sink << endl);
                            if ( currCfgp->get(sink->index()) == nullptr || !bbp->outEdges().contains(currCfgp->get(sink->index())) ){
                                if (!todo.contains(pair(bbp,sink))) {
                                    todo.add(pair(bbp,sink));
                                }
                            }
                        }
                    } else {
                        DEBUGP("\t\t\tretrieving old BBP" << endl);
                        bbp = currCfgp->get(bb->index());
                        DEBUGP("\t\t\tAdding " << bbp->oldBB() << " to " << prev->oldBB() << endl);
                        prev->addOutEdge(bbp);
                        DEBUGP("\t\t\t" << *prev << endl);
                    }
                }
            }
        }

        for (auto origcfg : *cfgColl){
            cfgsP[currSet]->get(origcfg)->isInvolved();
            if (cfgsP[currSet]->get(origcfg)->isInvolved()){
                todoMarkedCfg.add(origcfg);
            }
        }
        while(!todoMarkedCfg.isEmpty()){
            auto currCfg = todoMarkedCfg.pop();
            for (auto prevSynth: currCfg->callers()){
                auto prevCfgp = cfgsP[currSet]->get(prevSynth->cfg());
                if (!prevCfgp->isInvolved()){
                    prevCfgp->setInvolved();
                    todoMarkedCfg.add(prevSynth->cfg());
                }
            }
        }
    }


    DEBUGP("CFG Projector - simplifying" << endl);

    bool change = true;

    while (change){
        change = false;
        for (int currSet=0; currSet < setCount; currSet++){
            for (auto cfgp: cfgsP[currSet]->CFGPs()){
                for (auto bbp: *cfgp->BBPs()){
                    if (bbp != nullptr){
                        for (auto sink: bbp->outEdges()){
                            if (sink->oldBB()->isSynth()){
                                if (!sink->toSynth()->callee()->isInvolved()){
                                    //cout << *sink << endl;
                                    for (auto sinkSink: sink->outEdges()){
                                        DEBUGP("creating bypass from: " << bbp->oldBB() << endl);
                                        DEBUGP("-> to: " << sinkSink->oldBB() << ", in set: " << currSet << endl);
                                        bbp->addOutEdge(sinkSink);
                                    }
                                    bbp->removeOutEdge(sink);
                                    change = true;
                                }
                            }   
                        }

                        
                        if (!bbp->oldBB()->isSynth() && bbp->tags().count() == 0){
                            for (auto sink: bbp->outEdges()){
                                if (sink == bbp) {
                                    DEBUGP("removing self loop of: " << bbp->oldBB() << ", from CFG: " << bbp->oldBB()->cfg() << ", in set: " << currSet << endl);
                                    bbp->removeOutEdge(sink);
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    for (int currSet=0; currSet < setCount; currSet++){
        for (auto cfgp: cfgsP[currSet]->CFGPs()){
            for (auto bbp: *cfgp->BBPs()){
                if (bbp != nullptr){
                    if (bbp->oldBB()->isSynth()){
                        if (!bbp->toSynth()->callee()->isInvolved()){
                            DEBUGP("removing bbp : " << *bbp << endl);
                            cfgp->removeBBP(bbp);
                        }
                    }   
                }
            }
        }
    }

    for (int currSet=0; currSet < setCount; currSet++){
        for (auto cfgp: cfgsP[currSet]->CFGPs()){
            if (!cfgp->isInvolved()){
                DEBUGP("removing cfg : " << cfgp->oldCFG() << endl);
                cfgsP[currSet]->remove(cfgp);
            }
        }
    }
}

void CfgSetProjectorProcessor::dump(WorkSpace *ws, Output &out) {
    for (int i=0; i<setCount; i++){
        out << "\nProjection for set " << i << endl;
        for (auto c: cfgsP[i]->CFGPs()) {
            out << "Sub CFG : " << c->oldCFG()->name() << ", involved : " << c->isInvolved() << endl;
            out << "\t=====" << endl;
            for (auto bp : *c->BBPs()){
                if (bp != nullptr) {
                    out << "\t--Projected BB : " << bp->oldBB() << endl;
                    for (auto e: bp->outEdges()){
                        out << "\t\t-> Edge to : " << e->oldBB() << endl;
                    }
                    out << "\t\t-----" << endl;
                    if (bp->oldBB()->isSynth()) {
                        out << "\t\t--> SynthLink to : " << bp->toSynth()->callee()->oldCFG()->name() << endl;
                        out << "\t\t-----" << endl;
                    }
                }
            }
            out << "\t=====" << endl;
        }
    }
}