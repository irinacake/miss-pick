#include <elm/io.h>
#include <otawa/otawa.h>
#include <otawa/app/Application.h>
#include <otawa/prog/TextDecoder.h>
#include <otawa/cfg/features.h>
#include <otawa/cfg/Loop.h>
#include <otawa/hard/features.h>
#include <otawa/hard/CacheConfiguration.h>

#include <elm/assert.h>
#include <elm/array.h>
#include <elm/sys/FileItem.h>
#include <elm/util/BitVector.h>

#include <elm/sys/StopWatch.h>

#include <elm/options.h>


#include "cacheAnalysisUtils.hpp"


using namespace elm;
using namespace otawa;





p::id<SaveState*> SAVED("SAVED");



void CacheState::updateLRU(otawa::address_t toAdd){
  auto toAddSet = cache->set(toAdd);
  auto toAddTag = cache->block(toAdd);

  // position variable
  int pos = 0;
  
  // search loop : break before incrementing if the same tag is found
  while (pos < nbWays - 1){ 
    if (toAddTag == state[toAddSet]->getValue(pos)) break;
    pos++;
  }
  
  // reverse loop : overwrite the current tag (pos) with the 
  // immediately younger tag (pos-1)
  while (pos > 0){
    state[toAddSet]->setValue(pos,state[toAddSet]->getValue(pos-1));
    //state[toAddSet * nbWays + pos] = state[toAddSet * nbWays + pos-1];
    pos--;
  }

  // set age 0 with the new tag
  state[toAddSet]->setValue(pos,toAddTag);
  
  //displayState();
  //cout << endl;
}

void CacheState::updateFIFO(otawa::address_t toAdd){
  auto toAddSet = cache->set(toAdd);
  auto toAddTag = cache->block(toAdd);

  // position variable
  int pos = 0;
  bool found = false;
  
  // search loop : break before incrementing if the same tag is found
  while (pos < nbWays && !found){
    if (toAddTag == state[toAddSet]->getValue(pos)) {
      found = true;
      break;
    }
    pos++;
  }

  if (!found) {
    state[toAddSet]->setValue(currIndexFIFO[toAddSet],toAddTag);
    currIndexFIFO[toAddSet] = (currIndexFIFO[toAddSet]+1) % nbWays;
  }

  //displayState();
  //cout << endl;
}

void CacheState::updatePLRU(otawa::address_t toAdd){
  auto toAddSet = cache->set(toAdd);
  auto toAddTag = cache->block(toAdd);


  // position variable
  int pos = 0;
  bool found = false;
  
  // search loop : break before incrementing if the same tag is found
  while (pos < nbWays && !found){ 
    if (toAddTag == state[toAddSet]->getValue(pos)) {
      found = true;
      break;
    }
    pos++;
  }

  if (found){
    int i = 0;
    int look = 0;
    int bit = 0;
    while (i < logNbWays) { // 3 - 0 - 1
      bit = 1 << (logNbWays-i-1);
      if ( (pos) & ( 1 << (logNbWays-i-1)) ) { // clear it
        accessBitsPLRU[toAddSet] = accessBitsPLRU[toAddSet] & ~(1 << look);
        look += nbWays / (1 << (i+1)) ;
      } else { // set it
        accessBitsPLRU[toAddSet] = accessBitsPLRU[toAddSet] | (1 << look);
        look += 1;
      }
      i++;
    }
  } else {
    int look = 0;
    int access = 0;

    int i = 0;
    while(i < logNbWays){
      access <<= 1;
      if (accessBitsPLRU[toAddSet] & (1 << look)) {
        access++;
      }

      accessBitsPLRU[toAddSet] = accessBitsPLRU[toAddSet] ^ (1 << look);
      if (accessBitsPLRU[toAddSet] & (1 << look)){
        look += 1;
      } else {
        look += nbWays / (1 << (i+1)) ;
      }
      i++;
    }
  
    // set new tag at access
    state[toAddSet]->setValue(access,toAddTag);
  }

  //displayState();
  //cout << endl;
}


/*

bool CacheState::existsIn(otawa::Block* blockCheck){
  cout << "Begin exists in" << endl;
  bool contains = false;
  int currTag = -1;
  int currSet = -1;

  if (blockCheck->isBasic()){
    contains = true;
    cout << "is Basic" << endl;
    for (auto inst : *blockCheck->toBasic()){
      if (currTag != getTag(inst->address())
          || currSet != getSet(inst->address()) ){

        currTag = getTag(inst->address());
        currSet = getSet(inst->address());

        cout << "testing contains" << endl;
        if ((SAVED(blockCheck)->contains(getSubState(inst->address()),currSet))){
          cout << "contains" << endl;
          //contains &= true;
        } else {
          cout << "is not.." << endl;
          contains &= false;
        }
      }
    }
  }
  return contains;
}



bool CacheState::existsIn(otawa::Block* blockCheck, int set){
  cout << "hi" << endl;

  State* newState = new State(nbWays);
  int pos = 0;
  while (pos < nbWays){ 
    newState->setValue(pos,state[set]->getValue(pos));
    pos++;
  }

  if ((SAVED(blockCheck)->contains(newState,set))){
    return true;
  } 
  return false;
}

*/

State* CacheState::newSubState(int set){
  State* newState = new State(nbWays);
  int pos = 0;
  while (pos < nbWays){ 
    newState->setValue(pos,state[set]->getValue(pos));
    pos++;
  }
  return newState;
}







p::id<bool> MARKPRINT("MARKPRINT", false);

void printStates(CFG *g, CacheState *mycache, string indent = "") {
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
        cout << "isExit" << v->isExit() << endl;
        cout << indent << v << **SAVED(v) << endl;
      }
    }
  }
}



p::id<bool> MARKINIT("MARKINIT", false);

void initState(CFG *g, CacheState *mycache, string indent = "") {
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

/*

void computeAnalysis(CFG *g, CacheState *mycache) {

  int icount = 0;
  int currTag;
  int currSet;


  Vector<Pair<Block *,CacheState *>> todo;

  todo.add(pair(g->entry(),mycache));

  
  while (!todo.isEmpty()){
    icount++;
    auto curPair = todo.pop();
    auto curBlock = curPair.fst;
    auto curCacheState = curPair.snd;

    cout << "\nTodo: " << curBlock << endl;
    cout << "Initial State :" << endl;
    curCacheState->displayState();

    if (curBlock->isEntry()) {
      cout << "is Entry block:" << endl;
      for (auto e: curBlock->outEdges()){
        auto sink = e->sink();
        if(!curCacheState->existsIn(sink)){
          cout << "- Adding " << sink << endl;
          todo.add(pair(sink,curCacheState->copy()));
        }
      }
    } else if(curBlock->isExit()) {
      cout << "is Exit block:" << endl;
      for (auto caller: curBlock->cfg()->callers()){
        for (auto e: caller->outEdges()){
          auto sink = e->sink();
          if(!curCacheState->existsIn(sink)){
            cout << "- Adding " << sink << endl;
            todo.add(pair(sink,curCacheState->copy()));
          }
        }
      }

    } else if(curBlock->isSynth()) {
      cout << "is Synth block:" << endl;
      if ( curBlock->toSynth()->callee() != nullptr ){
        cout << "- Adding " << curBlock->toSynth()->callee()->entry() << endl;
        todo.add(pair(curBlock->toSynth()->callee()->entry(),curCacheState));
      }

    } else if (curBlock->isBasic()) {

      cout << "is Basic block:" << endl;

      currTag = -1;
      currSet = -1;

      for (auto inst : *curBlock->toBasic()){
        if (currTag != curCacheState->getTag(inst->address())
            || currSet != curCacheState->getSet(inst->address()) ){

          currTag = curCacheState->getTag(inst->address());
          currSet = curCacheState->getSet(inst->address());

          cout << "Getting new substate" << endl;
          State* newState = curCacheState->getSubState(inst->address());
          cout << "Adding to SAVED" << endl;
          SAVED(curBlock)->add(newState, currSet);

          cout << "Updating cache" << endl;
          cout << "before (" << curBlock->index() << ") :" << endl;
          curCacheState->displayState();
          curCacheState->update(inst->address());
          cout << "after :" << endl;
          curCacheState->displayState();
          cout << endl;

        }
      }

      for (auto e: curBlock->outEdges()){
        auto sink = e->sink();
        cout << "Verifying exist" << endl;
        if(!curCacheState->existsIn(sink)){
          cout << "- Adding " << sink << endl;
          todo.add(pair(sink,curCacheState->copy()));
        }
      }
    }
  }
  cout << icount << " iterations" << endl;
}

*/


void computeAnalysisSetBySet(CFG *g, CacheState *mycache) {

  int icount = 0;

  int currSet = 0;
  int currTag = 0;

  bool updated = false;
  

  Vector<Pair<Block *,CacheState *>> todo;

  for (int set = 0; set < mycache->getNbSets(); set++) {

    DEBUG("computing new set : " << set << endl);


    todo.add(pair(g->entry(),mycache->copy()));

    
    while (!todo.isEmpty()){
      icount++;
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

      } else if(curBlock->isSynth()) {
        DEBUG("is Synth block:" << endl);
        if ( curBlock->toSynth()->callee() != nullptr ){
          DEBUG("- Adding " << curBlock->toSynth()->callee()->entry() << endl);
          todo.add(pair(curBlock->toSynth()->callee()->entry(),curCacheState));
        }

      } else if (curBlock->isBasic()) {
        
        currTag = -1;

        DEBUG("is Basic block:" << endl);
        DEBUG("Before : " << **SAVED(curBlock) << endl);
              
        State* newState = curCacheState->newSubState(set);
        SAVED(curBlock)->add(newState, set);

        DEBUG("After : " << **SAVED(curBlock) << endl);

        for (auto inst : *curBlock->toBasic()){
          //DEBUG("set1 : " << set << endl);
          //DEBUG("set2 : " << curCacheState->getSet(inst->address()) << endl);
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

        // SaveState should have null ?
        // how to not add a block that is not concerned
        // but sometimes you still want to add a BB that
        // will then forward
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
      }
    }
  }
  cout << icount << " iterations" << endl;
}




p::id<bool> MARKSTATS("MARKSTATS", false);

void getStats(CFG *g, int *mins, int *maxs, float *moys, int* bbCount, int waysCount) {
  if (g == nullptr) {
    return;
  }
  for(auto v: *g){
    if (!MARKSTATS(v)) {
      MARKSTATS(v) = true;
      if(v->isSynth()) {
        getStats(v->toSynth()->callee(), mins, maxs, moys, bbCount, waysCount);
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
        }
      }
    }
  }
}


void makeStats(CFG *g, CacheState *mycache) {
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


  getStats(g, mins, maxs, moys, bbCount, waysCount);

  
  cout << "bbcount : ";
  for (int i = 0; i < waysCount; i++){
    cout << bbCount[i] << " ";
  }
  cout << endl;

  cout << "mins : ";
  for (int i = 0; i < waysCount; i++){
    cout << mins[i] << " ";
  }
  cout << endl;

  cout << "maxs : ";
  for (int i = 0; i < waysCount; i++){
    cout << maxs[i] << " ";
  }
  cout << endl;

  cout << "moys : ";
  for (int i = 0; i < waysCount; i++){
    moys[i] /= bbCount[i];
    cout << moys[i] << " ";
  }
  cout << endl;
}





class CacheAnalysis: public Application {
public:
  CacheAnalysis(void): Application("CacheAnalysis", Version(1, 0, 0)),
    //opt(option::SwitchOption::Make(*this).cmd("-o").cmd("--com").help("option 1")),
    cacheXml(option::ValueOption<string>::Make(*this).cmd("-c").cmd("--cache").help("Cache configuration xml file").usage(option::arg_required))
  
   { }

protected:
  void work(const string &entry, PropList &props) override {
    
    sys::StopWatch mySW;

    //otawa::VERBOSE(props) = true;
    otawa::CACHE_CONFIG_PATH(props) = *cacheXml;

    require(DECODED_TEXT);
    require(COLLECTED_CFG_FEATURE);
    require(otawa::hard::CACHE_CONFIGURATION_FEATURE);

    auto cfgs = COLLECTED_CFG_FEATURE.get(workspace());
    auto maincfg = cfgs->entry();
    
    auto confs = hard::CACHE_CONFIGURATION_FEATURE.get(workspace());
    auto icache = confs->instCache();
    CacheState mycache(icache);


    mySW.start();

    cout << "init" << endl;
    initState(maincfg, &mycache);
    cout << "init done\n --------------- \nwork" << endl;
    //mycache.displayState();
    //computeAnalysis(maincfg, &mycache);
    computeAnalysisSetBySet(maincfg, &mycache);

    mySW.stop();

    //SPEDEBUG(printStates(maincfg, &mycache);)
    printStates(maincfg, &mycache);

    cout << "Policy : " << icache->replacementPolicy() << endl;
    //mycache.displayState();
    cout << endl;

    makeStats(maincfg, &mycache);
    
    cout << "time : " << mySW.delay() << endl;
    
  }

private:
  //option::SwitchOption opt;
  option::ValueOption<string> cacheXml;
};



//OTAWA_RUN(CacheAnalysis)
int main(int argc, char **argv) {
  return CacheAnalysis().manage(argc, argv);
}
