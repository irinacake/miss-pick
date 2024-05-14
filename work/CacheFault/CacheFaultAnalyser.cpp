#include "CacheFaultFeature.h"


p::id<MultipleSetsSaver*> SAVED("SAVED");


void CacheFaultAnalysisProcessor::printStates() {
    for(auto v: cfgs().blocks()){
        if(v->isSynth()) {
            cout << v << endl;
        } else if (v->isBasic()) {
            cout << v << **SAVED(v) << endl;
        }
    }
}


void CacheFaultAnalysisProcessor::initState() {
    for(auto v: cfgs().blocks()){
        if(v->isBasic()) {
            MultipleSetsSaver* newSetsSaver = new MultipleSetsSaver;
            newSetsSaver->setupMWS(icache->setCount(),icache->wayCount());
            SAVED(v) = newSetsSaver;
        }
    }
}



void CacheFaultAnalysisProcessor::computeAnalysis(CFG *g, CacheSetState *initState, sys::StopWatch& mySW){
    cout << "computing (v2)" << endl;

    int currSet = 0;
    int currTag = 0;

    struct todoItem {
        Block* block;
        CacheSetState* cacheSetState;
        Vector<Block*> callStack;
    };

    Vector<todoItem> todo;
    //Vector<Pair<Pair<Block *,CacheSetState *>,Vector<Block *>>> todo;

    int i = 0;
    for (int set = 0; set < icache->setCount(); set++) {
        cout << "computing new set : " << set << endl;
        DEBUG("computing new set : " << set << endl);


        //todo.add(pair(g->entry(), initState->copy()));
        // IS this a proper copy ?
        todoItem initItem;
        initItem.block = g->entry();
        initItem.cacheSetState = initState->clone();
        initItem.callStack = Vector<Block*>();
        todo.add(initItem);




        while (!todo.isEmpty()){
            i++;
            if (i%1000000 == 0){ // timeout
                //cout << "set : " << set << endl;
                if (mySW.currentDelay().mins() > 30){
                    sys::System::exit(10000 + set);
                }
            }

            auto curItem = todo.pop();

            DEBUG("\nTodo: " << curItem.block << endl);
            DEBUG("Initial State :" << endl);
            DEBUG(curItem.cacheSetState);

            if (curItem.block->isEntry()) {
                DEBUG("is Entry block:" << endl);
                for (auto e: curItem.block->outEdges()){
                    auto sink = e->sink();
                    if(!sink->isBasic() || !SAVED(sink)->contains(curItem.cacheSetState,set)){
                        DEBUG("- Adding " << sink << endl);
                        todoItem itemToAdd;
                        itemToAdd.block = sink;
                        itemToAdd.cacheSetState = curItem.cacheSetState->clone();
                        itemToAdd.callStack = curItem.callStack;
                        todo.add(itemToAdd);
                    }
                }

                delete(curItem.cacheSetState);

            } else if(curItem.block->isExit()) {
                DEBUG("is Exit block:" << endl);
                if (!curItem.callStack.isEmpty()){
                    auto caller = curItem.callStack.pop();
                    for (auto e: caller->outEdges()){
                        auto sink = e->sink();
                        if(!sink->isBasic() || !SAVED(sink)->contains(curItem.cacheSetState,set)){
                            DEBUG("- Adding " << sink << endl);
                            todoItem itemToAdd;
                            itemToAdd.block = sink;
                            itemToAdd.cacheSetState = curItem.cacheSetState->clone();
                            itemToAdd.callStack = curItem.callStack;
                            todo.add(itemToAdd);
                        }
                    }
                }

                delete(curItem.cacheSetState);

            } else if(curItem.block->isSynth()) {
                DEBUG("is Synth block:" << endl);
                if ( curItem.block->toSynth()->callee() != nullptr ){
                    DEBUG("- Adding " << curItem.block->toSynth()->callee()->entry() << endl);
                    todoItem itemToAdd;
                    itemToAdd.block = curItem.block->toSynth()->callee()->entry();
                    itemToAdd.cacheSetState = curItem.cacheSetState;
                    curItem.callStack.push(curItem.block);
                    itemToAdd.callStack = curItem.callStack;
                    todo.add(itemToAdd);
                } else { // L'ajout des todos suivant devrait avoir des TOP partout (undefined behaviour)
                    for (auto e: curItem.block->outEdges()){
                        auto sink = e->sink();
                        if(!sink->isBasic() || !SAVED(sink)->contains(curItem.cacheSetState,set)){
                            DEBUG("- Adding " << sink << endl);
                            todoItem itemToAdd;
                            itemToAdd.block = sink;
                            itemToAdd.cacheSetState = curItem.cacheSetState->clone();
                            itemToAdd.callStack = curItem.callStack;
                            todo.add(itemToAdd);
                        }
                    }
                    delete(curItem.cacheSetState);
                }

            } else if (curItem.block->isBasic()) {
                /// TODO I AM HERE

                currTag = -1;

                DEBUG("is Basic block:" << endl);
                DEBUG("Before : " << **SAVED(curItem.block) << endl);

                CacheSetState* newState = curItem.cacheSetState->clone();
                SAVED(curItem.block)->add(newState, set);

                DEBUG("After : " << **SAVED(curItem.block) << endl);

                for (auto inst : *curItem.block->toBasic()){
                    DEBUG(icache->block(inst->address()) << endl);
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
                SPEDEBUG(curCacheState->displayState();)

                for (auto e: curItem.block->outEdges()){
                    auto sink = e->sink();
                    DEBUG("Verifying exist (Basic) : " << sink << endl);
                    if(!sink->isBasic() || !SAVED(sink)->contains(curItem.cacheSetState,set)){
                        DEBUG("- Adding " << sink << endl);
                        todoItem itemToAdd;
                        itemToAdd.block = sink;
                        itemToAdd.cacheSetState = curItem.cacheSetState->clone();
                        itemToAdd.callStack = curItem.callStack;
                        todo.add(itemToAdd);
                    }
                }
                delete(curItem.cacheSetState);
            }
        }
    }
    cout << "computing done" << endl;
}





void CacheFaultAnalysisProcessor::getStats(int *mins, int *maxs, float *moys, int* bbCount, int waysCount, MultipleSetsSaver* totalStates) {
    for(auto v: cfgs().blocks()){
        if(v->isBasic()) {
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



void CacheFaultAnalysisProcessor::makeStats(elm::io::Output &output) {
    int waysCount = icache->setCount();
    int mins[waysCount];
    int maxs[waysCount];
    float moys[waysCount];
    int bbCount[waysCount];
    for (int i = 0; i < waysCount; i++){
        mins[i] = type_info<int>::max;
        maxs[i] = 0;
        moys[i] = 0;
        bbCount[i] = 0;
    }

    MultipleSetsSaver* totalStates = new MultipleSetsSaver;
    totalStates->setupMWS(icache->setCount(),icache->wayCount());

    getStats(mins, maxs, moys, bbCount, waysCount, totalStates);

    output << "\t\"bb_count\" : [";
    output << bbCount[0];
    for (int i = 1; i < waysCount; i++){
        output << "," << bbCount[i];
    }
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
    moys[0] /= bbCount[0];
    output << moys[0];
    for (int i = 1; i < waysCount; i++){
        moys[i] /= bbCount[i];
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
}





p::feature CACHE_FAULT_ANALYSIS_FEATURE("otawa::hard::CACHE_FAULT_ANALYSIS_FEATURE", p::make<CacheFaultAnalysisProcessor>());


CacheFaultAnalysisProcessor::CacheFaultAnalysisProcessor(): CFGProcessor(reg) {
  
}

p::declare CacheFaultAnalysisProcessor::reg = p::init("CacheFaultAnalysisProcessor", Version(1, 0, 0))
    .make<CacheFaultAnalysisProcessor>()
    .extend<CFGProcessor>()
    .provide(CACHE_FAULT_ANALYSIS_FEATURE)
    .require(DECODED_TEXT)
    .require(COLLECTED_CFG_FEATURE)
    .require(otawa::hard::CACHE_CONFIGURATION_FEATURE);


void CacheFaultAnalysisProcessor::processAll(WorkSpace *ws) {  
	sys::StopWatch mySW;
	
    auto maincfg = taskCFG();
    icache = hard::CACHE_CONFIGURATION_FEATURE.get(workspace())->instCache();


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


    initState();
    computeAnalysis(maincfg, mycache, mySW);

    //printStates();


    mySW.stop();


    exec_time = mySW.delay().micros();

}



void CacheFaultAnalysisProcessor::dump(WorkSpace *ws, Output &out) {

  auto icache = hard::CACHE_CONFIGURATION_FEATURE.get(workspace())->instCache();

  out << "{\n";
  out << "\t\"file\" : \"" << workspace()->process()->program()->name() << "\",\n";  //get name of the input file
  out << "\t\"task\" : \"" << taskCFG() << "\",\n";
  out << "\t\"policy\" : \"" << icache->replacementPolicy() << "\",\n";
  
  out << "\t\"bsize\" : " << icache->blockCount() << ",\n";
  out << "\t\"associativity\" : " << (int)pow(2,icache->wayBits()) << ",\n";
  out << "\t\"set_count\" : " << icache->setCount() << ",\n";

  out << "\t\"exec_time\" : " << exec_time << ",\n";

  makeStats(out);

  out << "}" << endl;

}