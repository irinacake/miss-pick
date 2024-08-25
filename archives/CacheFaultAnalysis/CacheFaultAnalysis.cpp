#include <elm/assert.h>
#include <elm/array.h>
#include <elm/sys/FileItem.h>
#include <elm/util/BitVector.h>
#include <elm/sys/StopWatch.h>
#include <elm/options.h>
#include <elm/sys/FileItem.h>
#include <elm/sys/System.h>

#include <otawa/proc/Processor.h>
#include <otawa/prog/Manager.h>
#include <otawa/prog/WorkSpace.h>
#include <otawa/prog/TextDecoder.h>

#include <otawa/cfg/features.h>
#include <otawa/cfg/Loop.h>

#include <otawa/hard/features.h>
#include <otawa/hard/CacheConfiguration.h>

#include <otawa/app/Application.h>

#include "CacheFaultAnalysisFeature.h"


using namespace elm;
using namespace otawa;



p::id<SaveState*> SAVED("SAVED");



p::id<bool> MARKPRINT("MARKPRINT", false);

void printStates(CFG *g, AbstractCacheState *mycache, string indent = "") {
  if (g == nullptr) {
    return;
  }
  for(auto v: *g){
    if (!MARKPRINT(v)){
      MARKPRINT(v) = true;
      if(v->isSynth()) {
        cout << indent << v << endl;
        printStates(v->toSynth()->callee(), mycache, indent + "\t");
      } else if (v->isBasic()) {
        cout << indent << v << **SAVED(v) << endl;
      }
    }
  }
}



p::id<bool> MARKINIT("MARKINIT", false);

void initState(CFG *g, AbstractCacheState *mycache, string indent = "") {
  if (g == nullptr) {
    return;
  }
  for(auto v: *g){
    if (!MARKINIT(v)){
      MARKINIT(v) = true;
      if(v->isSynth()) {
        initState(v->toSynth()->callee(), mycache, indent + "\t");
      } else if (v->isBasic()) {
        SaveState* newSaveState = new SaveState;
        newSaveState->setCache(mycache->getCache());
        SAVED(v) = newSaveState;
      }
    }
  }
}



void computeAnalysis(CFG *g, AbstractCacheState *mycache, sys::StopWatch& mySW) {
  int currSet = 0;
  int currTag = 0;
  
  Vector<Pair<Block *,AbstractCacheState *>> todo;

  int i = 0;
  for (int set = 0; set < mycache->getNbSets(); set++) {
    cout << "computing new set : " << set << endl;
    DEBUG("computing new set : " << set << endl);


    todo.add(pair(g->entry(),mycache->copy()));

    
    while (!todo.isEmpty()){
      i++;
      if (i%1000000 == 0){
        //cout << "set : " << set << endl;
        if (mySW.currentDelay().mins() > 30){
          sys::System::exit(10000 + set);
        }
      }

      auto curPair = todo.pop();
      auto curBlock = curPair.fst;
      auto curCacheState = curPair.snd;
      
      DEBUG("\nTodo: " << curBlock << endl);
      DEBUG("Initial State :" << endl);
      SPEDEBUG(curCacheState->displayState();)

      if (curBlock->isEntry()) {
        DEBUG("is Entry block:" << endl);
        for (auto e: curBlock->outEdges()){
          auto sink = e->sink();
          if(!sink->isBasic() || !SAVED(sink)->contains(curCacheState->getSubState(set),set)){
            DEBUG("- Adding " << sink << endl);
            todo.add(pair(sink,curCacheState->copy()));
          }
        }

        delete(curCacheState);
      
      } else if(curBlock->isExit()) {
        DEBUG("is Exit block:" << endl);
        for (auto caller: curBlock->cfg()->callers()){
          for (auto e: caller->outEdges()){
            auto sink = e->sink();
            if(!sink->isBasic() || !SAVED(sink)->contains(curCacheState->getSubState(set),set)){
              DEBUG("- Adding " << sink << endl);
              todo.add(pair(sink,curCacheState->copy()));
            }
          }
        }
        
        delete(curCacheState);

      } else if(curBlock->isSynth()) {
        DEBUG("is Synth block:" << endl);
        if ( curBlock->toSynth()->callee() != nullptr ){
          DEBUG("- Adding " << curBlock->toSynth()->callee()->entry() << endl);
          todo.add(pair(curBlock->toSynth()->callee()->entry(),curCacheState));
        } else {
          for (auto e: curBlock->outEdges()){
            auto sink = e->sink();
            if(!sink->isBasic() || !SAVED(sink)->contains(curCacheState->getSubState(set),set)){
              DEBUG("- Adding " << sink << endl);
              // use another cacheState with all element set to "T" (unknown)
              todo.add(pair(sink,curCacheState->copy()));
            }
          }
          delete(curCacheState);
        }

      } else if (curBlock->isBasic()) {
        
        currTag = -1;

        DEBUG("is Basic block:" << endl);
        DEBUG("Before : " << **SAVED(curBlock) << endl);
              
        State* newState = curCacheState->newSubState(set);
        SAVED(curBlock)->add(newState, set);

        DEBUG("After : " << **SAVED(curBlock) << endl);

        for (auto inst : *curBlock->toBasic()){
          DEBUG(curCacheState->getTag(inst->address()) << endl);
          if (currTag != curCacheState->getTag(inst->address())){
            DEBUG(" - new tag" << endl;)
            currTag = curCacheState->getTag(inst->address());
            if (set == curCacheState->getSet(inst->address())){
              DEBUG("   - matches set" << endl;)
              curCacheState->update(inst->address());
            }
          }
        }

        DEBUG("Final State :" << endl);
        SPEDEBUG(curCacheState->displayState();)

        for (auto e: curBlock->outEdges()){
          auto sink = e->sink();
          DEBUG("Verifying exist (Basic) : " << sink << endl);
          if(!sink->isBasic() || !SAVED(sink)->contains(curCacheState->getSubState(set),set)){
            DEBUG("- Adding " << sink << endl);
            todo.add(pair(sink,curCacheState->copy()));
          }
        }
        delete(curCacheState);
      }
    }
  }
}




p::id<bool> MARKSTATS("MARKSTATS", false);

void getStats(CFG *g, int *mins, int *maxs, float *moys, int* bbCount, int waysCount, SaveState* totalStates) {
  if (g == nullptr) {
    return;
  }
  for(auto v: *g){
    if (!MARKSTATS(v)) {
      MARKSTATS(v) = true;
      if(v->isSynth()) {
        getStats(v->toSynth()->callee(), mins, maxs, moys, bbCount, waysCount, totalStates);
      } else if (v->isBasic()) {
        SaveState* sState = *SAVED(v);
        int* listSizes = sState->getListSizes();
        for (int i = 0; i < waysCount; i++){
          mins[i] = min(mins[i],listSizes[i]);
          maxs[i] = max(maxs[i],listSizes[i]);
          if (listSizes[i] != 0){
            moys[i] += listSizes[i];
            bbCount[i]++;
          }
          for (auto s: sState->getList(i)){
            totalStates->add(s,i);
          }
        }
      }
    }
  }
}


void makeStats(CFG *g, AbstractCacheState *mycache, elm::io::Output &output) {
  int waysCount = mycache->getCache()->setCount();
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

  SaveState* totalStates = new SaveState;
  totalStates->setCache(mycache->getCache());

  getStats(g, mins, maxs, moys, bbCount, waysCount, totalStates);

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
  auto totalList = totalStates->getListSizes();
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
  auto icache = hard::CACHE_CONFIGURATION_FEATURE.get(workspace())->instCache();

  switch (icache->replacementPolicy())
  {
  case otawa::hard::Cache::LRU:
    mycache = new CacheStateLRU(icache);
    break;
  case otawa::hard::Cache::FIFO:
    mycache = new CacheStateFIFO(icache);
    break;
  case otawa::hard::Cache::PLRU:
    mycache = new CacheStatePLRU(icache);
    break;
  
  default:
    break;
  }

  mySW.start();

  initState(maincfg, mycache);
  computeAnalysis(maincfg, mycache, mySW);

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

  makeStats(taskCFG(), mycache, out);

  out << "}" << endl;

}