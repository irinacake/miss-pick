#include "CFGSetProjector.h"
#include <otawa/cfg/Loop.h>
#include "CacheMissDebug.h"


/**
 * @defgroup pcfg   Projected CFG (Control Flow Graph)
 * 
 * This module allows to represents programs as Projected Control Flow Graph (CFG),
 * that is to say, a CFG (see otawa::cfg base class) that has been projected based on
 * the cache set.
 * The projected CFG Collection follows the same structure as the base CFG Collection
 * albeit simplified to only provide what is needed. BBP (Projected Basic Blocks) and
 * CFGP (Projected CFGs) have a reference to their associated non projected equivalent.
 * 
*/

/**
 * @class BBP;
 * A node in the projected CFG. 
 * @ingroup pcfg
 */
/**
 * @class BBPSynth;
 * A synthetic node in the projected CFG. 
 * @ingroup pcfg
 */
/**
 * @class CFGP;
 * A projected CFG in the projected collection. 
 * @ingroup pcfg
 */
/**
 * @class CFGCollectionP;
 * A collection of projected CFG. 
 * @ingroup pcfg
 */
/**
 * @class ProjectedCFGColl;
 * The code processor class interface, which allows 
 * the user to access the projected CFG collection
 * @ingroup pcfg
 */


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



/**
 * @class CfgSetProjectorProcessor
 * This processor is used to tranform the CFG of @ref COLLECTED_CFG_FEATURE into
 * a set of Projected CFGs
 *
 * @par Provided Features
 * @ref CFG_SET_PROJECTOR_FEATURE
 *
 * @ingroup pcfg
 */

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



/**
 * @fn belongsTo
 * Checks whether a given Block* belongs to the given set or not
 * 
 * @param bb Block* the basic block to test
 * @param set int the set
 * @return true if it does, false if it does not
 */
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

    // Projection work loop, set by set
    for (int currSet=0; currSet < setCount; currSet++){
        DEBUGP("\n\n\n------------\nprocessing set : " << currSet << "\n------------" << endl);
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
                        //bbp->addPrev(prev);

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
                        //bbp->addPrev(prev);
                        DEBUGP("\t\t\t" << *prev << endl);
                    }
                }
            }
        }


        // Fix involvement values
        for (auto origcfg : *cfgColl){
            // add to the WL cfgs that are involved
            if (cfgsP[currSet]->get(origcfg)->isInvolved()){
                todoMarkedCfg.add(origcfg);
            }
        }
        // for every cfg in the WL, mark all its callers as involved
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
    
    // Simplication loop
    while (change){
        change = false;
        for (int currSet=0; currSet < setCount; currSet++){
            for (auto cfgp: cfgsP[currSet]->CFGPs()){
                for (auto bbp: *cfgp->BBPs()){
                    if (bbp != nullptr){
                        // Create a bypass around synth block calling a cfgp that is not involved
                        for (auto sink: bbp->outEdges()){
                            if (sink->oldBB()->isSynth()){
                                if (!sink->toSynth()->callee()->isInvolved()){
                                    for (auto sinkSink: sink->outEdges()){
                                        DEBUGP("creating bypass from: " << bbp->oldBB() << endl);
                                        DEBUGP("-> to: " << sinkSink->oldBB() << ", in set: " << currSet << endl);
                                        bbp->addOutEdge(sinkSink);
                                        //sinkSink->addPrev(bbp);
                                        //sinkSink->removePrev(sink);
                                    }
                                    bbp->removeOutEdge(sink);
                                    change = true;
                                }
                            }   
                        }
                        
                        // Delete self loops (unless synth or "useful" basic)
                        if (!bbp->oldBB()->isSynth() && bbp->tags().count() == 0){
                            for (auto sink: bbp->outEdges()){
                                if (sink == bbp) {
                                    DEBUGP("removing self loop of: " << bbp->oldBB() << ", from CFG: " << bbp->oldBB()->cfg() << ", in set: " << currSet << endl);
                                    bbp->removeOutEdge(sink);
                                    //bbp->removePrev(sink);
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // Delete synth blocks that call a cfgp that is not involved
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

    // Remove cfgps that are not involved
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
                    //for (auto p: bp->prevs()){
                    //    out << "\t\t-> Prev : " << p->oldBB() << endl;
                    //}
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