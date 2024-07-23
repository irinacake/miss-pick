#include "CacheMissFeature.h"
#include <otawa/cfg/Loop.h>
#include <otawa/trivial/features.h>

#include <elm/avl/Set.h>


/**
 * @class CacheMissProcessor
 * Code processor for the WCET calculation by Cache Miss Analysis.
 * This code processor sets up several properties:
 * - SAVED / SAVEDP (based on projection) mapped on Block* / BBP* that provides the possible entry states of the cache
 * - KICKERS mapped on BBP* (projection only) that provides a list of Block* that evicted this BBP* (regardless of how many tags it possesses)
 * - MISSVALUE mapped on ??? (TODO) that provides how many misses does ??? have
 * 
 * @ingroup cachemiss
 */



p::id<MultipleSetsSaver*> SAVED("SAVED");
p::id<CacheSetsSaver*> SAVEDP("SAVEDP");
p::id<bool> PROJECTION("PROJECTION");
#ifdef newKickers
p::id<ListSet<LoopBlock*,LoopBlockComparator>> KICKERS("KICKERS");
#else
p::id<ListSet<Block*>> KICKERS("KICKERS");
#endif
p::id<int> MISSVALUE("MISSVALUE");




ot::time CacheMissEvent::cost(void) const {
    return _icache->missPenalty();
}


#ifdef newKickers
void CacheMissEvent::estimate(ilp::Constraint *cons, bool on) const {
    if (on){
        auto kickers = *KICKERS(_bbp);
        if (kickers.count() == 0){
            cons->addLeft(1);
        } else {
            for (auto k : *KICKERS(_bbp)){
                if (k->type == LoopOrBlock::BLOCK){
                    cons->addLeft(1,ipet::VAR(k->lob.block));
                } else {
                    for (auto e: k->lob.loop->entries()){
                        if (!BACK_EDGE(e)){
                            cons->addLeft(1,ipet::VAR(e));
                        }
                    }
                    
                }
            }
        }
    }
}
#else

void CacheMissEvent::estimate(ilp::Constraint *cons, bool on) const {
    if (on){
        auto kickers = *KICKERS(_bbp);
        if (kickers.count() == 0){
            cons->addLeft(1);
        } else {
            for (auto k : *KICKERS(_bbp)){
                cons->addLeft(1,ipet::VAR(k));
            }
        }
    }
}

#endif

bool CacheMissEvent::isEstimating(bool on) const {
	return true;
}

Event::kind_t CacheMissEvent::kind(void) const {
    return Event::kind_t::FETCH;
}

cstring CacheMissEvent::name(void) const {
    switch (_icache->replacementPolicy())
    {
    case hard::Cache::replace_policy_t::FIFO:
        return "CMA_FIFO";
        break;
    case hard::Cache::replace_policy_t::LRU:
        return "CMA_LRU";
        break;
    case hard::Cache::replace_policy_t::PLRU:
        return "CMA_PLRU";
        break;
    default:
        ASSERTP(false,"Policy not yet supported");
        break;
    }
	return "";
}

Event::occurrence_t CacheMissEvent::occurrence(void) const {
	return _occ;
}
Event::type_t CacheMissEvent::type() const {
    return Event::type_t::LOCAL;
}
int CacheMissEvent::weight(void) const {
	return 1;
}
string CacheMissEvent::detail(void) const {
	switch (_icache->replacementPolicy())
    {
    case hard::Cache::replace_policy_t::FIFO:
        return "CacheMiss Analysis for FIFO policy";
        break;
    case hard::Cache::replace_policy_t::LRU:
        return "CacheMiss Analysis for LRU policy";
        break;
    case hard::Cache::replace_policy_t::PLRU:
        return "CacheMiss Analysis for PLRU policy";
        break;
    default:
        ASSERTP(false,"Policy not yet supported");
        break;
    }
	return "";
}






/**
 * @fn printStates
 * 
 * @warning can only be executed for non-projection analysis
 */
void CacheMissProcessor::printStates() {
    for(auto v: cfgs().blocks()){
        if(v->isSynth()) {
            cout << v << endl;
        } else if (v->isBasic()) {
            cout << v << **SAVED(v) << endl;
        }
    }
}



/**
 * @fn printStatesP
 * 
 * @warning can only be executed for projected analysis
 */
void CacheMissProcessor::printStatesP() {
    for (int i=0; i<icache->setCount(); i++){
        cout << "----------\nPrinting for set " << i << endl << endl;
        for (auto c: pColl->graphOfSet(i)->CFGPs()) {
            for (auto bbp : *c->BBPs()){
                if (bbp != nullptr) {
                    cout << "Printing for bbp: " << bbp->oldBB() << endl;
                    cout << **SAVEDP(bbp) << endl;
                }
            }
        }
    }
}





/**
 * @fn kickedByP
 * Produces the data to store in the KICKERS property about kickers
 * 
 * @warning can only be executed for projected analysis
 */
void CacheMissProcessor::kickedByP() {
    // always hit, after miss, first miss, not classified counters
    int ahcpt = 0; int amcpt = 0; int fmcpt = 0; int nccpt = 0;
    // iterate over every set
    for (int i=0; i<icache->setCount(); i++){
        DEBUGK("----------\nPrinting for set " << i << endl << endl);
        // iterate over every CFGP
        for (auto c: pColl->graphOfSet(i)->CFGPs()) {
            // iterate over every BBP
            for (auto bbp : *c->BBPs()){
                if (bbp != nullptr) {
                    if (logFor(otawa::Monitor::log_level_t::LOG_INST) && bbp->tags().count()>0){
                        cout << "Processing BB: " << bbp->oldBB() << " from set " << i << endl;
                    }
                    DEBUGK("--------\n- Printing for bbp: " << bbp->oldBB() << ". Tags are:"<< endl);
                    for (auto t: bbp->tags()){
                        DEBUGK(" - " << t << endl);
                    }

                    // retrieve css
                    auto css = *SAVEDP(bbp);

                    if (bbp->tags().count() > 0){
                        DEBUGK("- This bbp contains the following entries:" << endl);
                        for (auto acs: *css->getSavedCacheSets()){
                            DEBUGK(" - "; acs->print(cout); cout << endl);
                        }
                    }

                    DEBUGK("- The kickers are:" << endl);
#ifdef newKickers

                    // With new  singular W :
                    // no need for the FOR loop
                    // just get the W through the property from the BBP

                    // search in every of the bbp's entry states
                    for (auto acs: *css->getSavedCacheSets()){
                        // static cast is mandatory
                        auto ccss = static_cast<const CompoundCacheSetState&>(*acs);
                        // search in every entry states' W list to see if the tags of the current bbp have been kicked
                        auto w = ccss.getW();
                        for (auto p: w->pairs()){
                            // if p.fst is a tag that belongs to bbp, p.snd is a kicker
                            if (bbp->tags().contains(p.fst)){
                                (*KICKERS(bbp)).add(p.snd);
                            }
                        }
                    }
                    for (auto k: (*KICKERS(bbp))){
                        if (k->type == LoopOrBlock::BLOCK){
                            DEBUGK(" - " << k->lob.block << endl);
                        } else {
                            DEBUGK(" - " << k->lob.loop << endl);
                        }
                    }
#else
                    // search in every of the bbp's entry states
                    for (auto acs: *css->getSavedCacheSets()){
                        // static cast is mandatory
                        auto ccss = static_cast<const CompoundCacheSetState&>(*acs);
                        // search in every entry states' W list to see if the tags of the current bbp have been kicked
                        auto w = ccss.getW();
                        for (auto p: w->pairs()){
                            // if p.fst is a tag that belongs to bbp, p.snd is a kicker
                            if (bbp->tags().contains(p.fst)){
                                (*KICKERS(bbp)).add(p.snd);
                            }
                        }
                    }
                    for (auto k: (*KICKERS(bbp))){
                        DEBUGK(" - " << k << endl);
                    }
#endif


                    DEBUGK("- AH, AM, NC:" << endl);
                    // at tag (inst) level (or l-block)
                    for (auto t: bbp->tags()) {
                        // retrieve the corresponding instruction (for otawa Events)
                        otawa::Inst* instoft;
                        for (auto inst : *bbp->oldBB()->toBasic()){
                            if (t == icache->block(inst->address())){
                                instoft = inst;
                                break;
                            }
                        }

                        if (logFor(otawa::Monitor::log_level_t::LOG_INST)){
                            cout << "\tInstruction: " << instoft;
                        }

                        // both AH and AM are true by default
                        bool ah = true;
                        bool am = true;

                        // search through every state of the bbp if the current tag is contained
                        for (auto acs: *css->getSavedCacheSets()){
                            auto state = acs->getState();

                            // state is a basic int* type, there is no contains method, hence the manual search
                            bool contained = false;
                            for (int j=0; j < (1 << icache->wayBits()) && !contained ; j++){
                                if (state[j] == t){
                                    contained = true;
                                }
                            }
                            if (contained){
                                // if it was contained at least once, then it is no longer AM
                                am = false;
                            } else {
                                // if it was not at least once, then it is no longer AH
                                ah = false;
                            }
                            if (!am && !ah){
                                // If both are false then it is already NC/FM, there is no need to check more states
                                break;
                            }
                        }

                        // once we know if it is ah/am/nc, create a corresponding otawa Event
                        DEBUGK(" - tag " << t << " is ");
                        if (ah) {
                            DEBUGK("Always Hit");
                            ahcpt++;

                            EVENT(bbp->oldBB()).add(new CacheMissEvent(instoft,Event::occurrence_t::NEVER,icache,bbp));

                            if (logFor(otawa::Monitor::log_level_t::LOG_INST)){
                                cout << " -> Always Hit" << endl;
                            }
                        } else if (am) {
                            DEBUGK("Always Miss");
                            amcpt++;

                            EVENT(bbp->oldBB()).add(new CacheMissEvent(instoft,Event::occurrence_t::ALWAYS,icache,bbp));

                            if (logFor(otawa::Monitor::log_level_t::LOG_INST)){
                                cout << " -> Always Miss" << endl;
                            }
                        } else {
                            // when NC, we can distinguish the case where there are no kickers, which implies that there can only be a single miss, the first time the instruction is loaded, regardless of how many paths leaded to that situation (if there are several paths, they are mutually exclusive)
                            auto kickers = *KICKERS(bbp);
                            if (kickers.count() == 0){
                                DEBUGK("First Miss");
                                fmcpt++;
                            } else {
                                DEBUGK("Not Classified");
                                nccpt++;
                            }

                            EVENT(bbp->oldBB()).add(new CacheMissEvent(instoft,Event::occurrence_t::SOMETIMES,icache,bbp));
                            
                            if (logFor(otawa::Monitor::log_level_t::LOG_INST)){
                                cout << " -> Not Classified" << endl;
                                for (auto k : *KICKERS(bbp)){
#ifdef newKickers
                                    if (k->type == LoopOrBlock::BLOCK){
                                        cout << "\t\t" << ipet::VAR(k->lob.block) << endl;
                                    } else {
                                        for (auto e: k->lob.loop->entries()){
                                            if (Loop::of(e->source()) != k->lob.loop){
                                                cout << "\t\t" << ipet::VAR(e->source()) << endl;
                                            }
                                        }
                                        
                                    }
#else
                                    cout << "\t\t" << ipet::VAR(k) << endl;
#endif
                                }
                            }

                        }
                        DEBUGK(endl);
                    }
                }
            }
        }
    }
    _ahcpt = ahcpt;
    _amcpt = amcpt;
    _nccpt = nccpt;
    _fmcpt = fmcpt;
    DEBUGK("ahcpt: " << ahcpt << endl); // always hit
    DEBUGK("amcpt: " << amcpt << endl); // after miss
    DEBUGK("bmcpt: " << nccpt << endl); // bounded miss
    DEBUGK("fmcpt: " << fmcpt << endl); // first miss
}


/**
 * @fn initState
 * Initialises the SAVED property
 * 
 * @warning can only be executed for non-projection analysis
 */
void CacheMissProcessor::initState() {
    for(auto v: cfgs().blocks()){
        if(v->isBasic() || v->isExit()) {
            MultipleSetsSaver* newSetsSaver = new MultipleSetsSaver;
            newSetsSaver->setupMSS(icache->setCount(),icache->wayCount());
            SAVED(v) = newSetsSaver;
        }
    }
}


/**
 * @fn initStateP
 * Initialises the SAVEDP property
 * 
 * @warning can only be executed for projected analysis
 */
void CacheMissProcessor::initStateP() {
    for (int i=0; i<icache->setCount(); i++){
        for (auto c: pColl->graphOfSet(i)->CFGPs()) {
            for (auto bbp : *c->BBPs()){
                if (bbp != nullptr) {
                    CacheSetsSaver* newSetsSaver = new CacheSetsSaver();
                    SAVEDP(bbp) = newSetsSaver;
                    ListMap<int,LoopBlock*>* newW = new ListMap<int,LoopBlock*>();
                    WIPEOUT(bbp) = newW;
                }
            }
        }
    }
}


// Inner loop working list item for non-projection analysis
struct todoItem {
    Block* block;
    AbstractCacheSetState* cacheSetState;
};

// Outer loop working list item for non-projection analysis 
struct callStack {
    Block* caller;
    Vector<AbstractCacheSetState*> exitCS;
    Vector<todoItem> workingList;
    bool exitBypass = false;
};



/**
 * @fn computeAnalysis
 * 
 * @param initState AbstractCacheSetState* the initial state of the Cache
 * @param mySW sys::StopWatch& elm timer to prevent executions from being too long
 * @warning can only be executed for non-projection analysis
 */
void CacheMissProcessor::computeAnalysis(AbstractCacheSetState *initState, sys::StopWatch& mySW){

    // // Please do check the computeProjectedAnalysis for more comments, the algorithm is similar
    // int currTag;
    // t::uint64 completedCfg;
    // Vector<callStack> todo;

    // int i = 0;

    // //cout << "computing CacheMissProcessor" << endl;
    // for (int set = 0; set < icache->setCount(); set++) {
    //     //cout << "computing new set : " << set << endl;
    //     DEBUG("computing new set : " << set << endl);
    //     completedCfg = 0;
    //     currTag = 0;

    //     todoItem initItem;
    //     initItem.block = maincfg->entry();
    //     initItem.cacheSetState = initState->clone();

    //     callStack initCallStack;
    //     initCallStack.caller = nullptr;
    //     initCallStack.workingList.add(initItem);

    //     todo.add(initCallStack);




    //     while (!todo.isEmpty()){
    //         i++;
    //         if (i%1000000 == 0 && mySW.currentDelay().mins() > 30){
    //             exit_value = 1 + set;
    //             break;
    //         }


    //         if (todo.top().workingList.isEmpty()){
    //             DEBUG("\nCurrent callStack is done:" << endl);
    //             auto callstack = todo.pop();
    //             for (auto cs: callstack.exitCS){
    //                 DEBUG("- Adding exitCS item " << *cs << ", to :" << endl);
    //                 for (auto e: callstack.caller->outEdges()){
    //                     auto sink = e->sink();
    //                     DEBUG("- - " << sink << endl);
    //                     todoItem itemToAdd;
    //                     itemToAdd.block = sink;
    //                     itemToAdd.cacheSetState = cs->clone();
    //                     todo.top().workingList.add(itemToAdd);
    //                 }
    //                 delete(cs);
    //             }
    //             if (callstack.caller != nullptr) {
    //                 DEBUG("Marked cfg" << endl);
    //                 completedCfg = completedCfg | (1 << callstack.caller->toSynth()->callee()->index());
    //             }
    //             continue;
    //         }



    //         auto curItem = todo.top().workingList.pop();

    //         DEBUG("\nTodo: " << curItem.block << endl);
    //         DEBUG("From CFG: " << curItem.block->cfg() << endl);
    //         DEBUG("Initial State :" << endl);
    //         DEBUG(*curItem.cacheSetState << endl);


    //         if (curItem.block->isEntry()) {
    //             DEBUG("is Entry block:" << endl);
    //             bool addition = false;
    //             for (auto e: curItem.block->outEdges()){
    //                 auto sink = e->sink();
    //                 if(!sink->isBasic() || !SAVED(sink)->contains(curItem.cacheSetState,set)){
    //                     addition = true;
    //                     DEBUG("- Adding " << sink << endl);
    //                     todoItem itemToAdd;
    //                     itemToAdd.block = sink;
    //                     itemToAdd.cacheSetState = curItem.cacheSetState->clone();
    //                     todo.top().workingList.add(itemToAdd);
    //                 }
    //             }
    //             if (!addition){
    //                 DEBUG("Not adding, bypass to exit" << endl);
    //                 if ((todo.top().exitBypass == false) && (completedCfg & (1 << curItem.block->cfg()->index())) ) {
    //                     auto exitbb = curItem.block->cfg()->exit();
    //                     DEBUG("Exitbb found : " << exitbb->cfg() << endl);
    //                     MultipleSetsSaver* sState = *SAVED(exitbb);
    //                     for (auto* s: *sState->getSaver(set)->getSavedCacheSets()){
    //                         todo.top().exitCS.add(s->clone());
    //                     }
    //                     todo.top().exitBypass = true;
    //                 }
    //             }

    //             delete(curItem.cacheSetState);

    //         } else if(curItem.block->isExit()) {
    //             DEBUG("is Exit block:" << endl);
    //             AbstractCacheSetState* newState = curItem.cacheSetState->clone();
    //             if (SAVED(curItem.block)->add(newState, set)) {
    //                 if (todo.top().caller != nullptr){
    //                     DEBUG("- Adding "<< *curItem.cacheSetState << " to exitCS" << endl);
    //                     todo.top().exitCS.add(curItem.cacheSetState);
    //                 }
    //             } else {
    //                 DEBUG("Not adding, bypass to exit" << endl);
    //                 if ((todo.top().exitBypass == false) && (completedCfg & (1 << curItem.block->cfg()->index())) ) {
    //                     auto exitbb = curItem.block->cfg()->exit();
    //                     DEBUG("Exitbb found : " << exitbb->cfg() << endl);
    //                     MultipleSetsSaver* sState = *SAVED(exitbb);
    //                     for (auto* s: *sState->getSaver(set)->getSavedCacheSets()){
    //                         todo.top().exitCS.add(s->clone());
    //                     }
    //                     todo.top().exitBypass = true;
    //                 }
    //             }

    //         } else if(curItem.block->isSynth()) {
    //             DEBUG("is Synth block:" << endl);
    //             if ( curItem.block->toSynth()->callee() != nullptr ){
    //                 DEBUG("- Adding " << curItem.block->toSynth()->callee()->entry() << endl);
    //                 todoItem itemToAdd;
    //                 itemToAdd.block = curItem.block->toSynth()->callee()->entry();
    //                 itemToAdd.cacheSetState = curItem.cacheSetState;

    //                 callStack newCallStack;
    //                 newCallStack.caller = curItem.block;
    //                 newCallStack.workingList.add(itemToAdd);

    //                 todo.add(newCallStack);
    //             } else { // L'ajout des todos suivant devrait avoir des TOP partout (undefined behaviour)
    //                 for (auto e: curItem.block->outEdges()){
    //                     auto sink = e->sink();
    //                     if(!sink->isBasic() || !SAVED(sink)->contains(curItem.cacheSetState,set)){
    //                         DEBUG("- Adding " << sink << endl);
    //                         todoItem itemToAdd;
    //                         itemToAdd.block = sink;
    //                         itemToAdd.cacheSetState = curItem.cacheSetState->clone();
    //                         todo.top().workingList.add(itemToAdd);
    //                     }
    //                 }
    //                 delete(curItem.cacheSetState);
    //             }

    //         } else if (curItem.block->isBasic()) {
    //             currTag = -1;

    //             DEBUG("is Basic block:" << endl);
    //             DEBUG("Before : " << **SAVED(curItem.block) << endl);

    //             AbstractCacheSetState* newState = curItem.cacheSetState->clone();
    //             if (SAVED(curItem.block)->add(newState, set)){

    //                 DEBUG("After : " << **SAVED(curItem.block) << endl);

    //                 for (auto inst : *curItem.block->toBasic()){
    //                     //DEBUG(icache->block(inst->address()) << endl);
    //                     if (currTag != icache->block(inst->address())){ // TAG
    //                         DEBUG(" - new tag" << endl;)
    //                         currTag = icache->block(inst->address());
    //                         if (set == icache->set(inst->address())){
    //                             DEBUG("   - matches set" << endl;)
    //                             curItem.cacheSetState->update(icache->block(inst->address()),curItem.block);
    //                         }
    //                     }
    //                 }

    //                 DEBUG("Final State :" << endl);
    //                 DEBUG(*curItem.cacheSetState << endl);

    //                 for (auto e: curItem.block->outEdges()){
    //                     auto sink = e->sink();
    //                     DEBUG("Verifying exist (Basic) : " << sink << endl);
    //                     if(!sink->isBasic() || !SAVED(sink)->contains(curItem.cacheSetState,set)){
    //                         DEBUG("- Adding " << sink << endl);
    //                         todoItem itemToAdd;
    //                         itemToAdd.block = sink;
    //                         itemToAdd.cacheSetState = curItem.cacheSetState->clone();
    //                         todo.top().workingList.add(itemToAdd);
    //                     }
    //                 }
    //             } else {
    //                 DEBUG("Not adding, bypass to exit" << endl);
    //                 if ((todo.top().exitBypass == false) && (completedCfg & (1 << curItem.block->cfg()->index())) ) {
    //                     auto exitbb = curItem.block->cfg()->exit();
    //                     DEBUG("Exitbb found : " << exitbb->cfg() << endl);
    //                     MultipleSetsSaver* sState = *SAVED(exitbb);
    //                     for (auto* s: *sState->getSaver(set)->getSavedCacheSets()){
    //                         todo.top().exitCS.add(s->clone());
    //                     }
    //                     todo.top().exitBypass = true;
    //                 }
    //             }

    //             delete(curItem.cacheSetState);
    //         } else if (curItem.block->isPhony()) {
    //             DEBUG("is Phony block:" << endl);
    //             for (auto e: curItem.block->outEdges()){
    //                 auto sink = e->sink();
    //                 if(!sink->isBasic() || !SAVED(sink)->contains(curItem.cacheSetState,set)){
    //                     DEBUG("- Adding " << sink << endl);
    //                     todoItem itemToAdd;
    //                     itemToAdd.block = sink;
    //                     itemToAdd.cacheSetState = curItem.cacheSetState->clone();
    //                     todo.top().workingList.add(itemToAdd);
    //                 }
    //             }

    //             delete(curItem.cacheSetState);
    //         } else if (curItem.block->isUnknown()) {
    //         } else {
    //             ASSERTP(false,"Unexpected block type");
    //         }
    //     }
    // }
    // exit_value = 0;
}


// Inner loop working list item for projected analysis
struct todoItemP {
    BBP* block; // current BBP to analyse
    AbstractCacheSetState* cacheSetState; // associated cache state
    ListMap<int,LoopBlock*>* W;
};


ListMap<int,LoopBlock*>* cloneW(ListMap<int,LoopBlock*>* W){
    ListMap<int,LoopBlock*>* newW = new ListMap<int,LoopBlock*>();
    ASSERTP(W != nullptr, "nullptr in cloneW");
    if (W->count() > 0){
        auto w = W->pairs().begin();
        for (; w != W->pairs().end(); ++w) {
            newW->put((*w).fst,(*w).snd);
        }
    }
    return newW;
}

bool joinW(BBP* bbp, ListMap<int,LoopBlock*>* W){
    auto* bbpW = *WIPEOUT(bbp);

    auto w = W->pairs().begin();
    for (; w != W->pairs().end(); ++w) {

        // reduce if the new item is a loop
        if ((*w).snd->type == LoopOrBlock::LOOP){
            auto l = (*w).snd->lob.loop;
            auto bbpw = bbpW->pairs().begin();
            for (; bbpw != bbpW->pairs().end(); ++bbpw) {
                if ( (*bbpw).snd->type == LoopOrBlock::LOOP ){
                    if ((*bbpw).snd->lob.loop->parent() == l){
                        auto tag = (*bbpw).fst;
                        W->put(tag, new LoopBlock(LoopOrBlock::LOOP, l));
                    }
                } else {
                    if (Loop::of((*bbpw).snd->lob.block) == l){
                        auto tag = (*bbpw).fst;
                        W->put(tag,new LoopBlock(LoopOrBlock::LOOP, l));
                    }
                }
            }
        }
        if (bbpW->hasKey((*w).fst)) {

        } else {
            bbpW->put((*w).fst,(*w).snd);
        }
    }
    //TODO
    return true;
}

class EquivTodoItemP {
public:
    int doCompare(const todoItemP *tip1, const todoItemP *tip2) const {
        int csscmp = tip1->cacheSetState->compare(*tip2->cacheSetState);
        if( csscmp == 0){
            int bcmp = tip1->block->oldBB() - tip2->block->oldBB();
            if (bcmp == 0){
                auto a = tip1->W->pairs().begin();
                auto b = tip2->W->pairs().begin();
                for (; a != tip1->W->pairs().end() && b != tip2->W->pairs().end(); ++a,++b) {
                    if ((*a).fst != (*b).fst){
                        return (*a).fst - (*b).fst;
                    }
                    if ((*a).snd->type == (*b).snd->type){
                        if ((*a).snd->type == LoopOrBlock::BLOCK){
                            if ((*a).snd->lob.block->index() != (*b).snd->lob.block->index()){
                                return (*a).snd->lob.block->index() - (*b).snd->lob.block->index();
                            }
                        } else {
                            if ((*a).snd->lob.loop != (*b).snd->lob.loop){
                                return (*a).snd->lob.loop->address() - (*b).snd->lob.loop->address();
                            }
                        }
                    } else {
                        return (*a).snd->type - (*b).snd->type;
                    }
                }
                if (a == tip1->W->pairs().end() && b == tip2->W->pairs().end()){
                    return 0;
                } else {
                    if (a != tip1->W->pairs().end()){
                        return -1;
                    } else {
                        return 1;
                    }
                }
            }
            return tip1->block->index() - tip2->block->index();
        }
        return csscmp;
    }
};

// Outer loop working list item for projected analysis
struct callStackP {
    BBP* caller; // BBP responsible for the cfg call
    Vector<AbstractCacheSetState*> exitCS; // list of CS to add to the caller's successors once the current cfg is done
    avl::Set<todoItemP*,EquivTodoItemP> workingList; // inner loop working list
    bool exitBypass = false; // shared bool to prevent multiple identical bypasses to exit
};



/**
 * @fn computeProjectedAnalysis
 * 
 * @param initState AbstractCacheSetState* the initial state of the Cache
 * @param mySW sys::StopWatch& elm timer to prevent executions from being too long
 * @warning can only be executed for projected analysis
 */
void CacheMissProcessor::computeProjectedAnalysis(AbstractCacheSetState *initState, sys::StopWatch& mySW){
    Vector<callStackP*> todo;

    int i = 0;

    exit_value = 0;
    for (int set = 0; set < icache->setCount(); set++) {
        //if (set % 5 == 0)
            cout << "computing new set : " << set << endl;
        
        DEBUG("computing new set : " << set << endl);

        // bitfield to mark whether a cfg has been entirely completed or not
        // caps the maximum amount of cfgs to 64
        t::uint64 completedCfg = 0;

        // if there are exactly 0 involved blocks for the current set in the CFG, the projection will have marked the task CFG as nullptr, and it is safe to skip to the next set 
        if (pColl->graphOfSet(set)->get(maincfg) == nullptr) { continue; }
        
        // create the first inner item
        todoItemP* initItem = new todoItemP;
        initItem->block = pColl->graphOfSet(set)->get(maincfg)->entry();
        initItem->cacheSetState = initState->clone();
        initItem->W = new ListMap<int,LoopBlock*>();

        // create the first outer item, with nullptr has a caller (stop condition)
        callStackP* initCallStack = new callStackP;
        initCallStack->caller = nullptr;
        initCallStack->workingList.add(initItem);

        todo.add(initCallStack);

        while (!todo.isEmpty()){
            // prevent the analysis from being too long (safety measure). The usage of i is to prevent the call to mySW every iteration of the loop
            i++;
            if (i%1000000 == 0 && mySW.currentDelay().mins() > 30){
                exit_value = 1 + set;
                break;
            }

            // if the inner WL is empty, then the current CFG is done and the states in the exitCS have to be added to the successors of the caller
            
            if (todo.top()->workingList.isEmpty()){
                DEBUG("\nCurrent callStack is done:" << endl);
                auto callstack = todo.pop();
                // Due to the "completedCfg" bitfield, the maincfg will never be able to add CS to the exitCS, and this loop will not be executed
                for (auto cs: callstack->exitCS){
                    DEBUG("- Adding exitCS item " << *cs << ", to :" << endl);
                    for (auto sink: callstack->caller->outEdges()){
                        DEBUG("- - " << sink->oldBB() << endl);
                        todoItemP* itemToAdd = new todoItemP;;
                        itemToAdd->block = sink;
                        itemToAdd->cacheSetState = cs->clone();
                        auto exitbb = callstack->caller->toSynth()->callee()->exit();
                        itemToAdd->W = cloneW(WIPEOUT(exitbb));
                        if (!todo.top()->workingList.contains(itemToAdd)){
                            todo.top()->workingList.add(itemToAdd);
                        } else {
                            delete itemToAdd->W;
                            delete itemToAdd->cacheSetState;
                            delete itemToAdd;
                        }
                    }
                }
                
                // prevent bad access for the maincfg
                if (callstack->caller != nullptr) {
                    DEBUG("Marked cfg" << endl);
                    completedCfg = completedCfg | (1 << callstack->caller->oldBB()->toSynth()->callee()->index());
                }
                delete callstack;
                // skip to the next outer WL item
                continue;
            }

            
            //cout << "Outer WL size : " << todo.count() << endl;
            //cout << "Inner WL size : " << todo.top()->workingList.count() << endl;

            auto* curItem = *todo.top()->workingList.begin();
            todo.top()->workingList.remove(curItem);

            DEBUG("\nTodo: " << curItem->block->oldBB() << endl);
            DEBUG("From CFG: " << curItem->block->oldBB()->cfg() << endl);
            DEBUG("Initial State :" << endl);
            DEBUG(*curItem->cacheSetState << endl);

            //DEBUG("Before : " << **SAVEDP(curItem->block) << endl);
            // the curItem.cacheSetState is the state that gets updated, so a clone must be created in order to add it to the curItem.block's CSSaver
            AbstractCacheSetState* newState = curItem->cacheSetState->clone();
            if (SAVEDP(curItem->block)->add(newState) && joinW(curItem->block,curItem->W)){ 
                //DEBUG("After : " << **SAVEDP(curItem->block) << endl);

                if (curItem->block->oldBB()->isEntry()) {
                    // for entry blocks, simply add the successors
                    DEBUG("is Entry block:" << endl);
                    for (auto sink: curItem->block->outEdges()){
                        DEBUG("- Adding " << sink->oldBB() << endl);
                        todoItemP* itemToAdd = new todoItemP;;
                        itemToAdd->block = sink;
                        itemToAdd->cacheSetState = newState->clone();
                        itemToAdd->W = cloneW(WIPEOUT(curItem->block));
                        if (!todo.top()->workingList.contains(itemToAdd)){
                            todo.top()->workingList.add(itemToAdd);
                        } else {
                            delete itemToAdd->W;
                            delete itemToAdd->cacheSetState;
                            delete itemToAdd;
                        }
                    }

                } else if (curItem->block->oldBB()->isExit()) {
                    // for exit blocks, forward the newly created state to the exitCS, unless the current cfg is the maincfg
                    DEBUG("is Exit block:" << endl);
                    if (todo.top()->caller != nullptr){
                        DEBUG("- Adding "<< *newState << " to exitCS" << endl);
                        todo.top()->exitCS.add(newState);
                    }
                    

                } else if (curItem->block->oldBB()->isSynth()) {
                    // for synth blocks, create a new outer WL item which will "pause" the exploration of the current cfg
                    DEBUG("is Synth block:" << endl);
                    
                    if ( curItem->block->toSynth()->callee() != nullptr ){
                        DEBUG("- Adding " << curItem->block->toSynth()->callee()->entry()->oldBB() << endl);
                        todoItemP* itemToAdd = new todoItemP;
                        itemToAdd->block = curItem->block->toSynth()->callee()->entry();
                        itemToAdd->cacheSetState = newState->clone();
                        itemToAdd->W = cloneW(WIPEOUT(curItem->block));


                        callStackP* newCallStack = new callStackP;
                        newCallStack->caller = curItem->block;
                        newCallStack->workingList.add(itemToAdd);

                        todo.add(newCallStack);
                        
                    } //else { // L'ajout des todos suivant devrait avoir des TOP partout (undefined behaviour)
                    //}

                } else if (curItem->block->oldBB()->isBasic()) {
                    // for basic blocks, update the curItem.cacheSetState with the curItem.block's tags, and add its successors
                    DEBUG("is Basic block: (updating curCS) -> ");

                    for (auto inst : curItem->block->tags()){
#ifdef newKickers
                        curItem->cacheSetState->update(inst,curItem->block);
#else
                        curItem->cacheSetState->update(inst,curItem->block->oldBB());
#endif
                    }

                    DEBUG(*curItem->cacheSetState << endl);
                    

                    for (auto sink: curItem->block->outEdges()){
                        DEBUG("- Adding " << sink->oldBB() << endl);
                        todoItemP* itemToAdd = new todoItemP;
                        itemToAdd->block = sink;
                        itemToAdd->cacheSetState = curItem->cacheSetState->clone();
                        itemToAdd->W = cloneW(WIPEOUT(curItem->block));
                        if (!todo.top()->workingList.contains(itemToAdd)){
#ifdef newKickers
                            // ??????? TODO
                            if (Loop::of(curItem->block->oldBB()) != Loop::of(sink->oldBB())){
                                itemToAdd->cacheSetState->reduce(Loop::of(curItem->block->oldBB()));
                            }
#endif
                            todo.top()->workingList.add(itemToAdd);
                        } else {
                            delete itemToAdd->W;
                            delete itemToAdd->cacheSetState;
                            delete itemToAdd;
                        }
                    }
                    DEBUG("Done adding" << endl);

                } else if (curItem->block->oldBB()->isPhony()) {
                    // for phony blocks, simply add the successors
                    DEBUG("is Phony block:" << endl);
                    for (auto sink: curItem->block->outEdges()){
                        DEBUG("- Adding " << sink->oldBB() << endl);
                        todoItemP* itemToAdd = new todoItemP;;
                        itemToAdd->block = sink;
                        itemToAdd->cacheSetState = newState->clone();
                        itemToAdd->W = cloneW(WIPEOUT(curItem->block));
                        if (!todo.top()->workingList.contains(itemToAdd)){
                            todo.top()->workingList.add(itemToAdd);
                        } else {
                            delete itemToAdd->W;
                            delete itemToAdd->cacheSetState;
                            delete itemToAdd;
                        }
                    }

                } else if (curItem->block->oldBB()->isUnknown()) {
                    // Unknown blocks should ideally generate "TOP" (undefined behaviour)
                } else {
                    ASSERTP(false,"Unexpected block type");
                }
            } else {
                // if the newly cloned state couldn't be added, immediately delete it, and attempt a bypass to exit if the conditions are met
                delete newState;
                DEBUG("Not adding, bypass to exit" << endl);
                // a bypass can only be executed once per cfg (-> once per outer WL item), but only if said cfg has been entirely explored at least once
                if ((todo.top()->exitBypass == false) && (completedCfg & (1 << curItem->block->oldBB()->cfg()->index())) ) {
                    // Once the exit BBP is found, add the content of its CSSaver to the exitCS
                    auto exitbb = pColl->graphOfSet(set)->get(curItem->block->oldBB()->cfg())->exit();
                    DEBUG("Exitbb found : " << exitbb->oldBB()->cfg() << endl);
                    CacheSetsSaver* sState = SAVEDP(exitbb);
                    for (auto* s: *sState->getSavedCacheSets()){
                        todo.top()->exitCS.add(s);
                    }
                    todo.top()->exitBypass = true;
                }
            }
            // it is safe to delete the curItem.cacheSetState because only clones have been stored or passed to successors
            delete curItem->W;
            delete curItem->cacheSetState;
            delete curItem;
        }
    }
    // cout << "computing done" << endl;
}






/**
 * @fn getStats
 * Called by makeStats to gather data for non-projection analysis
 * 
 * @param mins int* (table) in which to store the minimum entry states based on set
 * @param maxs int* (table) in which to store the maximum entry states based on set
 * @param moys float* (table) in which to store the average entry states based on set
 * @param bbCount int* (table) in which to store the total number of Block in the CFG based on set
 * @param totalStates MultipleSetsSaver* used to gather all existing entry states while disregarding duplicates
 * @warning can only be executed for non-projection analysis 
 */
void CacheMissProcessor::getStats(int *mins, int *maxs, float *moys, int* bbCount, MultipleSetsSaver* totalStates) {
    for(auto v: cfgs().blocks()){
        if(v->isBasic() || v->isExit()) {
            MultipleSetsSaver* sState = *SAVED(v);
            int* listSizes = sState->getSaversSizes();
            
            for (int i = 0; i < icache->setCount(); i++){
                mins[i] = min(mins[i],listSizes[i]);
                maxs[i] = max(maxs[i],listSizes[i]);
                if (listSizes[i] != 0){
                    moys[i] += listSizes[i];
                    bbCount[i]++;
                }
                for (auto* s: *sState->getSaver(i)->getSavedCacheSets()){
                    totalStates->add(s,i);
                }
            }
        }
    }
}


/**
 * @fn getStatsP
 * Called by makeStats to gather data for projected analysis
 * 
 * @param mins int* (table) in which to store the minimum entry states based on set
 * @param maxs int* (table) in which to store the maximum entry states based on set
 * @param moys float* (table) in which to store the average entry states based on set
 * @param bbCount int* (table) in which to store the total number of Block in the CFG based on set
 * @param usedBbCount int* (table) in which to store the number of actually involved BBP in the CFGP based on set
 * @param totalStates MultipleSetsSaver* used to gather all existing entry states while disregarding duplicates
 * @warning can only be executed for projected analysis 
 */
void CacheMissProcessor::getStatsP(int *mins, int *maxs, float *moys, int* bbCount, int* usedBbCount, MultipleSetsSaver* totalStates) {

    for (int i=0; i<icache->setCount(); i++){
        for (auto c: pColl->graphOfSet(i)->CFGPs()) {
            for (auto bbp : *c->BBPs()){
                if (bbp != nullptr) {
                    CacheSetsSaver* sState = SAVEDP(bbp);
                    auto listSize = sState->getCacheSetCount();

                    mins[i] = min(mins[i],listSize);
                    maxs[i] = max(maxs[i],listSize);
                    if (listSize != 0){
                        moys[i] += listSize;
                        bbCount[i]++;
                        if (bbp->tags().count() > 0){
                            usedBbCount[i]++;
                        }
                    }
                    for (auto* s: *sState->getSavedCacheSets()){
                        totalStates->add(s,i);
                    }
                }
            }
        }
        //cout << bbpcpt << ",";
    }
    //cout << endl;
}




/**
 * @fn makeStats
 * Produces stats based on the results of the computeAnalysis functions
 * 
 * @param output elm::io::Output&
 */
void CacheMissProcessor::makeStats(elm::io::Output &output) {
    int nbSets = icache->setCount();
    int mins[nbSets];
    int maxs[nbSets];
    float moys[nbSets];
    int bbCount[nbSets];
    int usedBbCount[nbSets];

    int wmins[nbSets];
    int wmaxs[nbSets];
    float wmoys[nbSets];
    for (int i = 0; i < nbSets; i++){
        mins[i] = type_info<int>::max;
        maxs[i] = 0;
        moys[i] = 0;
        wmins[i] = type_info<int>::max;
        wmaxs[i] = 0;
        wmoys[i] = 0;
        bbCount[i] = 0;
        usedBbCount[i] = 0;
    }

    MultipleSetsSaver* totalStates = new MultipleSetsSaver;
    totalStates->setupMSS(icache->setCount(),icache->wayCount());

    if (projection) {
        getStatsP(mins, maxs, moys, bbCount, usedBbCount, totalStates);
    } else {
        getStats(mins, maxs, moys, bbCount, totalStates);
    }

    for (int i = 0; i < nbSets; i++){ if (mins[i] == type_info<int>::max){ mins[i] = 0; } }

    
    for (int i = 0; i < nbSets; i++){
        bool begin = true;
        int diffStates;
        int csCount;
        AbstractCacheSetState* curCs;
        for (auto cs : *totalStates->getSaver(i)->getSavedCacheSets()){
            if (begin){
                begin = false;
                curCs = cs;
                csCount = 1;
                diffStates = 1;
            } else {
                if (cs->sameCs(*curCs) == 0) {
                    csCount++;
                } else {
                    diffStates++;
                    wmins[i] = min(wmins[i],csCount);
                    wmaxs[i] = max(wmaxs[i],csCount);
                    wmoys[i] += csCount;
                    csCount = 1;
                    curCs = cs;
                }
            }
        }
        wmins[i] = min(wmins[i],csCount);
        wmaxs[i] = max(wmaxs[i],csCount);
        wmoys[i] += csCount;
        wmoys[i] /= diffStates;
    }


    output << "\t\"bb_count\" : [";
    output << bbCount[0];
    for (int i = 1; i < nbSets; i++){
        output << "," << bbCount[i];
    }
    output << "],\n";

    output << "\t\"used_bb_count\" : [";
    output << usedBbCount[0];
    for (int i = 1; i < nbSets; i++){
        output << "," << usedBbCount[i];
    }
    output << "],\n";

    output << "\t\"total_bb\" : [";
    output << cfgs().countBlocks();
    output << "],\n";


    output << "\t\"state_mins\" : [";
    output << mins[0];
    for (int i = 1; i < nbSets; i++){
        output << "," << mins[i];
    }
    output << "],\n";

    output << "\t\"state_maxs\" : [";
    output << maxs[0];
    for (int i = 1; i < nbSets; i++){
        output << "," << maxs[i];
    }
    output << "],\n";

    output << "\t\"state_moys\" : [";
    moys[0] /= max(bbCount[0],1);
    output << moys[0];
    for (int i = 1; i < nbSets; i++){
        moys[i] /= max(bbCount[i],1);
        output << "," << moys[i];
    }
    output << "],\n";

    output << "\t\"state_total\" : [";
    auto totalList = totalStates->getSaversSizes();
    output << totalList[0];
    for (int i = 1; i < nbSets; i++){
        output << "," << totalList[i];
    }
    output << "],\n";


    output << "\t\"w_mins\" : [";
    output << wmins[0];
    for (int i = 1; i < nbSets; i++){
        output << "," << wmins[i];
    }
    output << "],\n";

    output << "\t\"w_maxs\" : [";
    output << wmaxs[0];
    for (int i = 1; i < nbSets; i++){
        output << "," << wmaxs[i];
    }
    output << "],\n";

    output << "\t\"w_moys\" : [";
    output << wmoys[0];
    for (int i = 1; i < nbSets; i++){
        output << "," << wmoys[i];
    }
    output << "],\n";

    //delete(totalStates);
}





p::feature CACHE_MISS_FEATURE("otawa::hard::CACHE_MISS_FEATURE", p::make<CacheMissProcessor>());

CacheMissProcessor::CacheMissProcessor(): CFGProcessor(reg) {}

p::declare CacheMissProcessor::reg = p::init("CacheMissProcessor", Version(1, 0, 0))
    .make<CacheMissProcessor>()
    .extend<CFGProcessor>()
    .provide(CACHE_MISS_FEATURE)
    .require(DECODED_TEXT)
    .require(COLLECTED_CFG_FEATURE)
    .require(otawa::hard::CACHE_CONFIGURATION_FEATURE)
    .require(EXTENDED_LOOP_FEATURE)
    .require(ipet::ASSIGNED_VARS_FEATURE)
    .require(CFG_SET_PROJECTOR_FEATURE);


void CacheMissProcessor::processAll(WorkSpace *ws) {  
	sys::StopWatch mySW;
	
    maincfg = taskCFG();
    icache = hard::CACHE_CONFIGURATION_FEATURE.get(workspace())->instCache();


    // Projected data
    pColl = CFG_SET_PROJECTOR_FEATURE.get(workspace());

    // Timer start
    mySW.start();

    // Associativity initialisation (must be called once)
    CacheSetState::initAssociativity(icache->wayBits());
    if (icache->replacementPolicy() == hard::Cache::PLRU){
        CacheSetStatePLRU::initTables();
    }

    // Non-projection is not (yet?) compatible with compoundStates?
    if (projection) {
        mycache = new CompoundCacheSetState(icache->replacementPolicy());
        initStateP();
        computeProjectedAnalysis(mycache, mySW);
    } else {
        mycache = new ConcreteCacheSetState(icache->replacementPolicy());
        initState();
        computeAnalysis(mycache, mySW);
    }


    // Computation is over, stop the timer
    mySW.stop();
    exec_time = mySW.delay().micros();


    if (projection) {
        kickedByP();
    } else {
        // Non-projection does not have a kick analysis function
    }

#ifdef newKickers
    cout << "\nNew Kickers edition" << endl;
#else
    cout << "\nbase edition" << endl;
#endif

}

void CacheMissProcessor::configure(const PropList& props) {
    // Retrieve the "-p" argument value from the CL
    CFGProcessor::configure(props);
    projection = PROJECTION(props);
}

void CacheMissProcessor::destroy(WorkSpace *ws) {
    if (projection) {
        for (int i=0; i<icache->setCount(); i++){
            for (auto c: pColl->graphOfSet(i)->CFGPs()) {
                for (auto bbp : *c->BBPs()){
                    if (bbp != nullptr) {
                        CacheSetsSaver* ss = SAVEDP(bbp);
                        delete ss;
                    }
                }
            }
        }
    } else {
        for(auto v: cfgs().blocks()){
            if(v->isBasic() || v->isExit()) {
                MultipleSetsSaver* mss = SAVED(v);
                delete mss;
            }
        }
    }
    CFGProcessor::destroy(ws);
}

void CacheMissProcessor::dump(WorkSpace *ws, Output &out) {
    out << "\t\"file\" : \"" << workspace()->process()->program()->name() << "\",\n";  // name of the input file
    out << "\t\"projection\" : \"" << projection << "\",\n";
    out << "\t\"task\" : \"" << taskCFG() << "\",\n"; // analysed task
    out << "\t\"policy\" : \"" << icache->replacementPolicy() << "\",\n";

    //out << "\t\"bsize\" : " << icache->blockCount() << ",\n";
    out << "\t\"associativity\" : " << (1 << icache->wayBits()) << ",\n";
    out << "\t\"set_count\" : " << icache->setCount() << ",\n";

    out << "\t\"exec_time\" : " << exec_time << ",\n";
    out << "\t\"exit_value\" : " << exit_value << ",\n";

    makeStats(out);

    out << "\t\"_ahcpt\" : " << _ahcpt << ",\n";
    out << "\t\"_amcpt\" : " << _amcpt << ",\n";
    out << "\t\"_nccpt\" : " << _nccpt << ",\n";
    out << "\t\"_fmcpt\" : " << _fmcpt << ",\n";

}