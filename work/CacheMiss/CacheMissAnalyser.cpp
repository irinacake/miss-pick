#include "CacheMissFeature.h"
#include <otawa/cfg/Loop.h>


p::id<MultipleSetsSaver*> SAVED("SAVED");
p::id<CacheSetsSaver*> SAVEDP("SAVEDP");
p::id<bool> PROJECTION("PROJECTION");

void CacheMissProcessor::printStates() {
    for(auto v: cfgs().blocks()){
        if(v->isSynth()) {
            cout << v << endl;
        } else if (v->isBasic()) {
            cout << v << **SAVED(v) << endl;
        }
    }
}


void CacheMissProcessor::initState() {
    for(auto v: cfgs().blocks()){
        if(v->isBasic() || v->isExit()) {
            MultipleSetsSaver* newSetsSaver = new MultipleSetsSaver;
            newSetsSaver->setupMWS(icache->setCount(),icache->wayCount());
            SAVED(v) = newSetsSaver;
        }
    }
}
void CacheMissProcessor::initStateP() {
    int cp;
    for (int i=0; i<icache->setCount(); i++){
        cp = 0;
        for (auto c: pColl->graphOfSet(i)->CFGPs()) {
            for (auto bbp : *c->BBPs()){
                if (bbp != nullptr) {
                    cp++;
                    CacheSetsSaver* newSetsSaver = new CacheSetsSaver();
                    SAVEDP(bbp) = newSetsSaver;
                }
            }
        }
        //cout << "for i=" << i << ", cp=" << cp << endl;
    }
}



struct todoItem {
    Block* block;
    CacheSetState* cacheSetState;
};

struct callStack {
    Block* caller;
    Vector<CacheSetState*> exitCS;
    Vector<todoItem> workingList;
    bool exitBypass = false;
};


void CacheMissProcessor::computeAnalysis(CacheSetState *initState, sys::StopWatch& mySW){
    int currTag;

    t::uint64 completedCfg;


    Vector<callStack> todo;

    int i = 0;

    //cout << "computing CacheMissProcessor" << endl;
    for (int set = 0; set < icache->setCount(); set++) {
        cout << "computing new set : " << set << endl;
        DEBUG("computing new set : " << set << endl);
        completedCfg = 0;
        currTag = 0;

        todoItem initItem;
        initItem.block = maincfg->entry();
        initItem.cacheSetState = initState->clone();

        callStack initCallStack;
        initCallStack.caller = nullptr;
        initCallStack.workingList.add(initItem);

        todo.add(initCallStack);




        while (!todo.isEmpty()){
            i++;
            if (i%1000000 == 0 && mySW.currentDelay().mins() > 30){
                exit_value = 1 + set;
                break;
            }


            if (todo.top().workingList.isEmpty()){
                DEBUG("\nCurrent callStack is done:" << endl);
                auto callstack = todo.pop();
                for (auto cs: callstack.exitCS){
                    DEBUG("- Adding exitCS item " << *cs << ", to :" << endl);
                    for (auto e: callstack.caller->outEdges()){
                        auto sink = e->sink();
                        DEBUG("- - " << sink << endl);
                        todoItem itemToAdd;
                        itemToAdd.block = sink;
                        itemToAdd.cacheSetState = cs->clone();
                        todo.top().workingList.add(itemToAdd);
                    }
                    delete(cs);
                }
                if (callstack.caller != nullptr) {
                    DEBUG("Marked cfg" << endl);
                    completedCfg = completedCfg | (1 << callstack.caller->toSynth()->callee()->index());
                }
                continue;
            }



            auto curItem = todo.top().workingList.pop();

            DEBUG("\nTodo: " << curItem.block << endl);
            DEBUG("From CFG: " << curItem.block->cfg() << endl);
            DEBUG("Initial State :" << endl);
            DEBUG(*curItem.cacheSetState << endl);


            if (curItem.block->isEntry()) {
                DEBUG("is Entry block:" << endl);
                bool addition = false;
                for (auto e: curItem.block->outEdges()){
                    auto sink = e->sink();
                    if(!sink->isBasic() || !SAVED(sink)->contains(curItem.cacheSetState,set)){
                        addition = true;
                        DEBUG("- Adding " << sink << endl);
                        todoItem itemToAdd;
                        itemToAdd.block = sink;
                        itemToAdd.cacheSetState = curItem.cacheSetState->clone();
                        todo.top().workingList.add(itemToAdd);
                    }
                }
                if (!addition){
                    DEBUG("Not adding, bypass to exit" << endl);
                    if ((todo.top().exitBypass == false) && (completedCfg & (1 << curItem.block->cfg()->index())) ) {
                        auto exitbb = curItem.block->cfg()->exit();
                        DEBUG("Exitbb found : " << exitbb->cfg() << endl);
                        MultipleSetsSaver* sState = *SAVED(exitbb);
                        for (auto* s: *sState->getSaver(set)->getSavedCacheSets()){
                            todo.top().exitCS.add(s->clone());
                        }
                        todo.top().exitBypass = true;
                    }
                }

                delete(curItem.cacheSetState);

            } else if(curItem.block->isExit()) {
                DEBUG("is Exit block:" << endl);
                CacheSetState* newState = curItem.cacheSetState->clone();
                if (SAVED(curItem.block)->add(newState, set)) {
                    if (todo.top().caller != nullptr){
                        DEBUG("- Adding "<< *curItem.cacheSetState << " to exitCS" << endl);
                        todo.top().exitCS.add(curItem.cacheSetState);
                    }
                } else {
                    DEBUG("Not adding, bypass to exit" << endl);
                    if ((todo.top().exitBypass == false) && (completedCfg & (1 << curItem.block->cfg()->index())) ) {
                        auto exitbb = curItem.block->cfg()->exit();
                        DEBUG("Exitbb found : " << exitbb->cfg() << endl);
                        MultipleSetsSaver* sState = *SAVED(exitbb);
                        for (auto* s: *sState->getSaver(set)->getSavedCacheSets()){
                            todo.top().exitCS.add(s->clone());
                        }
                        todo.top().exitBypass = true;
                    }
                }

            } else if(curItem.block->isSynth()) {
                DEBUG("is Synth block:" << endl);
                if ( curItem.block->toSynth()->callee() != nullptr ){
                    DEBUG("- Adding " << curItem.block->toSynth()->callee()->entry() << endl);
                    todoItem itemToAdd;
                    itemToAdd.block = curItem.block->toSynth()->callee()->entry();
                    itemToAdd.cacheSetState = curItem.cacheSetState;

                    callStack newCallStack;
                    newCallStack.caller = curItem.block;
                    newCallStack.workingList.add(itemToAdd);

                    todo.add(newCallStack);
                } else { // L'ajout des todos suivant devrait avoir des TOP partout (undefined behaviour)
                    for (auto e: curItem.block->outEdges()){
                        auto sink = e->sink();
                        if(!sink->isBasic() || !SAVED(sink)->contains(curItem.cacheSetState,set)){
                            DEBUG("- Adding " << sink << endl);
                            todoItem itemToAdd;
                            itemToAdd.block = sink;
                            itemToAdd.cacheSetState = curItem.cacheSetState->clone();
                            todo.top().workingList.add(itemToAdd);
                        }
                    }
                    delete(curItem.cacheSetState);
                }

            } else if (curItem.block->isBasic()) {
                currTag = -1;

                DEBUG("is Basic block:" << endl);
                DEBUG("Before : " << **SAVED(curItem.block) << endl);

                CacheSetState* newState = curItem.cacheSetState->clone();
                if (SAVED(curItem.block)->add(newState, set)){

                    DEBUG("After : " << **SAVED(curItem.block) << endl);

                    for (auto inst : *curItem.block->toBasic()){
                        //DEBUG(icache->block(inst->address()) << endl);
                        if (currTag != icache->block(inst->address())){ // TAG
                            DEBUG(" - new tag" << endl;)
                            currTag = icache->block(inst->address());
                            if (set == icache->set(inst->address())){
                                DEBUG("   - matches set" << endl;)
                                curItem.cacheSetState->update(icache->block(inst->address()));
                            }
                        }
                    }

                    DEBUG("Final State :" << endl);
                    DEBUG(*curItem.cacheSetState << endl);

                    for (auto e: curItem.block->outEdges()){
                        auto sink = e->sink();
                        DEBUG("Verifying exist (Basic) : " << sink << endl);
                        if(!sink->isBasic() || !SAVED(sink)->contains(curItem.cacheSetState,set)){
                            DEBUG("- Adding " << sink << endl);
                            todoItem itemToAdd;
                            itemToAdd.block = sink;
                            itemToAdd.cacheSetState = curItem.cacheSetState->clone();
                            todo.top().workingList.add(itemToAdd);
                        }
                    }
                } else {
                    DEBUG("Not adding, bypass to exit" << endl);
                    if ((todo.top().exitBypass == false) && (completedCfg & (1 << curItem.block->cfg()->index())) ) {
                        auto exitbb = curItem.block->cfg()->exit();
                        DEBUG("Exitbb found : " << exitbb->cfg() << endl);
                        MultipleSetsSaver* sState = *SAVED(exitbb);
                        for (auto* s: *sState->getSaver(set)->getSavedCacheSets()){
                            todo.top().exitCS.add(s->clone());
                        }
                        todo.top().exitBypass = true;
                    }
                }

                delete(curItem.cacheSetState);
            } else if (curItem.block->isPhony()) {
                DEBUG("is Phony block:" << endl);
                for (auto e: curItem.block->outEdges()){
                    auto sink = e->sink();
                    if(!sink->isBasic() || !SAVED(sink)->contains(curItem.cacheSetState,set)){
                        DEBUG("- Adding " << sink << endl);
                        todoItem itemToAdd;
                        itemToAdd.block = sink;
                        itemToAdd.cacheSetState = curItem.cacheSetState->clone();
                        todo.top().workingList.add(itemToAdd);
                    }
                }

                delete(curItem.cacheSetState);
            } else if (curItem.block->isUnknown()) {
            } else {
                ASSERTP(false,"Unexpected block type");
            }
        }
    }
    exit_value = 0;
}



struct todoItemP {
    BBP* block;
    CacheSetState* cacheSetState;
};

struct callStackP {
    BBP* caller;
    Vector<CacheSetState*> exitCS;
    Vector<todoItemP> workingList;
    bool exitBypass = false;
};


void CacheMissProcessor::computeProjectedAnalysis(CacheSetState *initState, sys::StopWatch& mySW){
    Vector<callStackP> todo;

    int i = 0;

    //cout << "computing CacheMissProcessor" << endl;
    for (int set = 0; set < icache->setCount(); set++) {
        cout << "computing new set : " << set << endl;
        DEBUG("computing new set : " << set << endl);

        t::uint64 completedCfg = 0;

        todoItemP initItem;
        if (pColl->graphOfSet(set)->get(maincfg) == nullptr)
            continue;
        initItem.block = pColl->graphOfSet(set)->get(maincfg)->entry();
        initItem.cacheSetState = initState->clone();

        callStackP initCallStack;
        initCallStack.caller = nullptr;
        initCallStack.workingList.add(initItem);

        todo.add(initCallStack);

        while (!todo.isEmpty()){
            i++;
            if (i%1000000 == 0 && mySW.currentDelay().mins() > 30){
                exit_value = 1 + set;
                break;
            }

            if (todo.top().workingList.isEmpty()){
                DEBUG("\nCurrent callStack is done:" << endl);
                auto callstack = todo.pop();
                for (auto cs: callstack.exitCS){
                    DEBUG("- Adding exitCS item " << *cs << ", to :" << endl);
                    for (auto sink: callstack.caller->outEdges()){
                        DEBUG("- - " << sink->oldBB() << endl);
                        todoItemP itemToAdd;
                        itemToAdd.block = sink;
                        itemToAdd.cacheSetState = cs->clone();
                        todo.top().workingList.add(itemToAdd);
                    }
                    //delete cs;
                }
                if (callstack.caller != nullptr) {
                    DEBUG("Marked cfg" << endl);
                    completedCfg = completedCfg | (1 << callstack.caller->oldBB()->toSynth()->callee()->index());
                }
                continue;
            }

            auto curItem = todo.top().workingList.pop();
            

            DEBUG("\nTodo: " << curItem.block->oldBB() << endl);
            DEBUG("From CFG: " << curItem.block->oldBB()->cfg() << endl);
            DEBUG("Initial State :" << endl);
            DEBUG(*curItem.cacheSetState << endl);


            DEBUG("Before : " << **SAVEDP(curItem.block) << endl);

            CacheSetState* newState = curItem.cacheSetState->clone();

            if (SAVEDP(curItem.block)->add(newState)){

                DEBUG("After : " << **SAVEDP(curItem.block) << endl);

                if (curItem.block->oldBB()->isEntry()) {
                    DEBUG("is Entry block:" << endl);
                    for (auto sink: curItem.block->outEdges()){
                        DEBUG("- Adding " << sink->oldBB() << endl);
                        todoItemP itemToAdd;
                        itemToAdd.block = sink;
                        itemToAdd.cacheSetState = newState->clone();
                        todo.top().workingList.add(itemToAdd);
                    }

                } else if(curItem.block->oldBB()->isExit()) {
                    DEBUG("is Exit block:" << endl);
                    if (todo.top().caller != nullptr){
                        DEBUG("- Adding "<< *newState << " to exitCS" << endl);
                        todo.top().exitCS.add(newState);
                    }
                    

                } else if(curItem.block->oldBB()->isSynth()) {
                    DEBUG("is Synth block:" << endl);
                    
                    if ( curItem.block->toSynth()->callee() != nullptr ){
                        DEBUG("- Adding " << curItem.block->toSynth()->callee()->entry()->oldBB() << endl);
                        todoItemP itemToAdd;
                        itemToAdd.block = curItem.block->toSynth()->callee()->entry();
                        itemToAdd.cacheSetState = newState->clone();

                        callStackP newCallStack;
                        newCallStack.caller = curItem.block;
                        newCallStack.workingList.add(itemToAdd);

                        todo.add(newCallStack);
                        
                    } //else { // L'ajout des todos suivant devrait avoir des TOP partout (undefined behaviour)
                    //}

                } else if (curItem.block->oldBB()->isBasic()) {
                    DEBUG("is Basic block: (updating curCS) -> ");

                    for (auto inst : curItem.block->tags()){
                        curItem.cacheSetState->update(inst);
                    }

                    DEBUG(*curItem.cacheSetState << endl);

                    for (auto sink: curItem.block->outEdges()){
                        DEBUG("- Adding " << sink->oldBB() << endl);
                        todoItemP itemToAdd;
                        itemToAdd.block = sink;
                        itemToAdd.cacheSetState = curItem.cacheSetState->clone();
                        todo.top().workingList.add(itemToAdd);
                    }

                } else if (curItem.block->oldBB()->isPhony()) {
                    DEBUG("is Phony block:" << endl);
                    for (auto sink: curItem.block->outEdges()){
                        DEBUG("- Adding " << sink->oldBB() << endl);
                        todoItemP itemToAdd;
                        itemToAdd.block = sink;
                        itemToAdd.cacheSetState = newState->clone();
                        todo.top().workingList.add(itemToAdd);
                    }

                } else if (curItem.block->oldBB()->isUnknown()) {
                } else {
                    ASSERTP(false,"Unexpected block type");
                }
            } else {
                delete newState;
                DEBUG("Not adding, bypass to exit" << endl);
                if ((todo.top().exitBypass == false) && (completedCfg & (1 << curItem.block->oldBB()->cfg()->index())) ) {
                    auto exitbb = pColl->graphOfSet(set)->get(curItem.block->oldBB()->cfg())->get(curItem.block->oldBB()->cfg()->exit()->index());
                    DEBUG("Exitbb found : " << exitbb->oldBB()->cfg() << endl);
                    CacheSetsSaver* sState = SAVEDP(exitbb);
                    for (auto* s: *sState->getSavedCacheSets()){
                        todo.top().exitCS.add(s);
                    }
                    todo.top().exitBypass = true;
                }
            }

            delete curItem.cacheSetState;
        }
    }
    exit_value = 0;
    cout << "computing done" << endl;
}



void CacheMissProcessor::getStats(int *mins, int *maxs, float *moys, int* bbCount, int waysCount, MultipleSetsSaver* totalStates) {
    for(auto v: cfgs().blocks()){
        if(v->isBasic() || v->isExit()) {
            MultipleSetsSaver* sState = *SAVED(v);
            int* listSizes = sState->getSaversSizes();
            
            for (int i = 0; i < waysCount; i++){
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

void CacheMissProcessor::getStatsP(int *mins, int *maxs, float *moys, int* bbCount, int* usedBbCount, int waysCount, MultipleSetsSaver* totalStates) {

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



void CacheMissProcessor::makeStats(elm::io::Output &output) {
    int waysCount = icache->setCount();
    int mins[waysCount];
    int maxs[waysCount];
    float moys[waysCount];
    int bbCount[waysCount];
    int usedBbCount[waysCount];
    for (int i = 0; i < waysCount; i++){
        mins[i] = type_info<int>::max;
        maxs[i] = 0;
        moys[i] = 0;
        bbCount[i] = 0;
        usedBbCount[i] = 0;
    }

    MultipleSetsSaver* totalStates = new MultipleSetsSaver;
    totalStates->setupMWS(icache->setCount(),icache->wayCount());

    if (projection) {
        getStatsP(mins, maxs, moys, bbCount, usedBbCount, waysCount, totalStates);
    } else {
        getStats(mins, maxs, moys, bbCount, waysCount, totalStates);
    }

    for (int i = 0; i < waysCount; i++){ if (mins[i] == type_info<int>::max){ mins[i] = 0; } }

    output << "\t\"bb_count\" : [";
    output << bbCount[0];
    for (int i = 1; i < waysCount; i++){
        output << "," << bbCount[i];
    }
    output << "],\n";

    output << "\t\"used_bb_count\" : [";
    output << usedBbCount[0];
    for (int i = 1; i < waysCount; i++){
        output << "," << usedBbCount[i];
    }
    output << "],\n";

    output << "\t\"total_bb\" : [";
    output << cfgs().countBlocks();
    output << "],\n";


    output << "\t\"state_mins\" : [";
    output << mins[0];
    for (int i = 1; i < waysCount; i++){
        output << "," << mins[i];
    }
    output << "],\n";

    output << "\t\"state_maxs\" : [";
    output << maxs[0];
    for (int i = 1; i < waysCount; i++){
        output << "," << maxs[i];
    }
    output << "],\n";

    output << "\t\"state_moys\" : [";
    moys[0] /= max(bbCount[0],1);
    output << moys[0];
    for (int i = 1; i < waysCount; i++){
        moys[i] /= max(bbCount[i],1);
        output << "," << moys[i];
    }
    output << "],\n";

    output << "\t\"state_total\" : [";
    auto totalList = totalStates->getSaversSizes();
    output << totalList[0];
    for (int i = 1; i < waysCount; i++){
        output << "," << totalList[i];
    }
    output << "]\n";

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
    .require(CFG_SET_PROJECTOR_FEATURE);


void CacheMissProcessor::processAll(WorkSpace *ws) {  
	sys::StopWatch mySW;
	
    maincfg = taskCFG();
    icache = hard::CACHE_CONFIGURATION_FEATURE.get(workspace())->instCache();


    // Projected data
    pColl = CFG_SET_PROJECTOR_FEATURE.get(workspace());

    //cout << a << endl;
    //auto a = pColl->graphOfSet(3);
    //cout << "inside cache miss, printing set 3 : \n" << *a << endl;

    //auto b = a->entry();
    //cout << "\n\n\n printing entry cfg of set 3 :\n" << *b << endl;

    //auto c = b->entry();
    //cout << "\n\n\n printing entry BBP of entry cfg of set 3 :\n" << *c << endl;



    mySW.start();

    // must be called
    CacheSetState::initAssociativity(icache->wayBits());

    switch (icache->replacementPolicy())
    {
    case otawa::hard::Cache::LRU:
        mycache = new CacheSetStateLRU();
        break;
    case otawa::hard::Cache::FIFO:
        mycache = new CacheSetStateFIFO();
        break;
    case otawa::hard::Cache::PLRU:
        mycache = new CacheSetStatePLRU();
        break;
    default:
        break;
    }


    if (projection) {
        initStateP();
        computeProjectedAnalysis(mycache, mySW);
    } else {
        initState();
        computeAnalysis(mycache, mySW);
    }

    //printStates();


    mySW.stop();


    exec_time = mySW.delay().micros();

}

void CacheMissProcessor::configure(const PropList& props) {
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

    auto icache = hard::CACHE_CONFIGURATION_FEATURE.get(workspace())->instCache();

    out << "{\n";
    out << "\t\"file\" : \"" << workspace()->process()->program()->name() << "\",\n";  //get name of the input file
    out << "\t\"projection\" : \"" << projection << "\",\n";  //get name of the input file
    out << "\t\"task\" : \"" << taskCFG() << "\",\n";
    out << "\t\"policy\" : \"" << icache->replacementPolicy() << "\",\n";

    out << "\t\"bsize\" : " << icache->blockCount() << ",\n";
    out << "\t\"associativity\" : " << (1 << icache->wayBits()) << ",\n";
    out << "\t\"set_count\" : " << icache->setCount() << ",\n";

    out << "\t\"exec_time\" : " << exec_time << ",\n";
    out << "\t\"exit_value\" : " << exit_value << ",\n";

    makeStats(out);

    out << "}" << endl;

}