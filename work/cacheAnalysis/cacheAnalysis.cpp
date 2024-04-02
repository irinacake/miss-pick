#include <elm/io.h>
#include <otawa/otawa.h>
#include <otawa/app/Application.h>
#include <otawa/prog/TextDecoder.h>
#include <otawa/cfg/features.h>
#include <otawa/cfg/Loop.h>
#include <otawa/hard/features.h>
#include <otawa/hard/CacheConfiguration.h>

#include <elm/assert.h>
#include <elm/sys/FileItem.h>
#include <elm/util/BitVector.h>

using namespace elm;
using namespace otawa;


/**
 * @fn
 * 
 * @param
 * @return  
 */

/**
 * @class State
 * A simplistic representation of the state of a cache set
 * 
 * @param isize the size of the cache set to initialise
 */
class State {
  public:
  State(int isize): size(isize) {
    state = new otawa::hard::Cache::block_t[size];
  }

  /**
   * @fn getState
   * @return otawa::hard::Cache::block_t* state
   */
  inline otawa::hard::Cache::block_t* getState(){
    return state;
  }

  /**
   * @fn setValue
   * Sets the value of an element of the state table.
   * @param pos int
   * @param value otawa::hard::Cache::block_t (elm::t::uint32)
   */
  inline void setValue(int pos, otawa::hard::Cache::block_t value){
    ASSERTP(pos >= 0 && pos < size, "In State.setValue() : argument 'pos', index out of bound.");
    state[pos] = value;
  }

  /**
   * @fn equals
   * Tests the equality between this state and a given state
   * 
   * @param state2 the state to compare to
   * @return true if both states are identical, false otherwise
   */
  bool equals(State* state2){
    if (this->size == state2->size) {
      for (int i = 0; i < this->size; i++) {
        if (this->state[i] != state2->state[i]) return false;
      }
      return true;
    } else {
      return false;
    }
  }

  friend elm::io::Output &operator<<(elm::io::Output &output, const State &state);
  
  private:
  int size;
  otawa::hard::Cache::block_t* state;
};

// Redefinition of the << operator for the State class
elm::io::Output &operator<<(elm::io::Output &output, const State &state) {
  output << "( ";
  for (int i = 0; i < state.size ; i++){
    output << state.state[i] << " ";
  }
  output << ") ";
  return output;
}

// Redefinition of the equivalence between two State* for the
// List.contains() method
namespace elm {
  template<>
  class Equiv<State *> {
  public:
    static inline bool isEqual(State *state1, State *state2) {
      return state1->equals(state2);
    }
  };
}




/**
 * @class SaveState
 * A SaveState is a data structure that store multiple cache states
 * for multiple cache sets. It uses the State class as well as the
 * elm List structure to store cache states.
 * 
 * The default constructor does not initialise the cache structure.
 * To initiliase it, first create a class, then call the setCache()
 * method :
 * @code
 * SaveState* newSaveState = new SaveState;
 * newSaveState->setCache(size);
 * @endcode
 * 
 * The class was implemented this way to allow the use of properties.
 * 
 */
class SaveState {
  public:
  SaveState(): saved(nullptr), size(0) {}
  
  void setCache(const otawa::hard::Cache* icache){
    size = icache->setCount();
    saved = new List<State *> [size];
    listSizes = new int[size];
    for (int i = 0; i < size; i++){
      listSizes[i] = 0;
    }
  }
  void setCache(int setCount){
    size = setCount;
    saved = new List<State *> [size];
    listSizes = new int[size];
    for (int i = 0; i < size; i++){
      listSizes[i] = 0;
    }
  }

  void add(State *newState, int set) {
    ASSERTP(set >= 0 && set < size, "In SaveState.add() : argument 'set', index out of bound.");
    if (!(saved[set].contains(newState))){
      listSizes[set]++;
      saved[set].add(newState);
    }
  }

  int* getListSizes(){
    return listSizes;
  }

  friend elm::io::Output &operator<<(elm::io::Output &output, const SaveState &saveState);
  
  private:
  int size;
  int* listSizes;
  List<State *> *saved;
};

elm::io::Output &operator<<(elm::io::Output &output, const SaveState &saveState) {
  output << "[ ";
  for (int i = 0; i < saveState.size ; i++){
    output << "{ ";
    for (int j = 0; j < saveState.listSizes[i]; j++){
      output << *saveState.saved[i][j];
    }
    output << "}";
  }
  output << "]";
  return output;
}



/**
 * @class CacheState
 * A CacheState is an encapsulation of the otawa::hard::Cache class.
 * It stores the current cache state using an AllocArray and provides
 * various update methods based on the update policy.
 * 
 * @param icache the otawa::hard::Cache object to encapsulate.
 */
class CacheState {
public:
  CacheState(const otawa::hard::Cache* icache): cache(icache), nbSets(icache->setCount()), nbWays((int)pow(2,icache->wayBits())) { 
    state.allocate(nbSets * nbWays);
    for (int e=0; e < (nbSets * nbWays); e++) {
      state[e] = otawa::hard::Cache::block_t(-1);
    } 
  }

  void updateLRU(otawa::address_t toAdd){
    auto toAddSet = cache->set(toAdd);
    auto toAddTag = cache->block(toAdd);

    // démarrer à zéro
    // vérifier l'existence progressive
    // si trouvé, alors faire des xch progressifs
    int pos = 0;
    
    // there is no need to check the last entry of the state, it either gets deleted, or shifted to age 0
    // if the tag is found in the middle, break the search loop
    while (pos < nbWays - 1){ 
      if (toAddTag == state[toAddSet * nbWays + pos]) break;
      pos++;
    }
    
    // overwrite to simulate elements switch
    while (pos > 0){
      state[toAddSet * nbWays + pos] = state[toAddSet * nbWays + pos-1];
      pos--;
    }

    // new tag has age 0
    state[toAddSet * nbWays + pos] = toAddTag;
    
    //displayState();
  }

  void displayState(){
    for (int i=0; i < nbSets ; i++) {
      cout << i << " : ";
      for (int j=0; j < nbWays; j++){
        cout << state[i * nbWays + j] << "  ";
      }
      cout << endl;
    }
  }

  inline const otawa::hard::Cache* getCache(){
    return cache;
  }

  inline AllocArray<otawa::hard::Cache::block_t>* getState(){
    return &state;
  }

  inline otawa::hard::Cache::block_t getTag(otawa::address_t toAdd) {
    return cache->block(toAdd);
  }

  inline otawa::hard::Cache::set_t getSet(otawa::address_t toAdd) {
    return cache->set(toAdd);
  }

  State* getSubState(otawa::address_t toGet){
    auto toGetSet = cache->set(toGet);
    State* newState = new State(nbWays);
    int pos = 0;
    while (pos < nbWays){ 
      newState->setValue(pos,state[toGetSet * nbWays + pos]);
      pos++;
    }
    return newState;
  }

private:
  int nbWays;
  int nbSets;
  AllocArray<otawa::hard::Cache::block_t> state;
  const otawa::hard::Cache* cache;
};




p::id<SaveState*> SAVED("SAVED");



void printStates(CFG *g, CacheState *mycache, string indent = "") {
  for(auto v: *g){
		if(v->isSynth()) {
			printStates(v->toSynth()->callee(), mycache, indent + "\t");
    } else if (v->isBasic()) {
      cout << indent << **SAVED(v) << endl;
    }
  }
}

void initState(CFG *g, CacheState *mycache, string indent = "") {
  for(auto v: *g){
		if(v->isSynth()) {
			initState(v->toSynth()->callee(), mycache, indent + "\t");
    } else if (v->isBasic()) {
      SaveState* newSaveState = new SaveState;
      newSaveState->setCache(mycache->getCache());
      SAVED(v) = newSaveState;
    }
  }
}

void statetest(CFG *g, CacheState *mycache, string indent = "") {

  int currTag;
  int currSet;
	for(auto v: *g){
		if(v->isSynth()) {
			statetest(v->toSynth()->callee(), mycache, indent + "\t");
    } else if (v->isBasic()) {
      currTag = -1;
      currSet = -1;
      for (auto inst : *v->toBasic()){
        if (currTag != mycache->getTag(inst->address())
            || currSet != mycache->getSet(inst->address()) ){

          currTag = mycache->getTag(inst->address());
          currSet = mycache->getSet(inst->address());

          State* newState = mycache->getSubState(inst->address());
          SAVED(v)->add(newState, currSet);

          mycache->updateLRU(inst->address());

        }
      }
    }
  }
}



void getStats(CFG *g, int *mins, int *maxs, float *moys, int* bbCount, int waysCount) {
  for(auto v: *g){
		if(v->isSynth()) {
			getStats(v->toSynth()->callee(), mins, maxs, moys, bbCount, waysCount);
    } else if (v->isBasic()) {
      SaveState* sState = *SAVED(v);
      int* listSizes = sState->getListSizes();
      for (int i = 0; i < waysCount; i++){
        mins[i] = min(mins[i],listSizes[i]);
        maxs[i] = max(maxs[i],listSizes[i]);
        moys[i] += listSizes[i];
        (*bbCount)++;
      }
    }
  }
}

void makeStats(CFG *g, CacheState *mycache) {
  int waysCount = mycache->getCache()->setCount();
  int mins[waysCount];
  int maxs[waysCount];
  float moys[waysCount];
  for (int i = 0; i < waysCount; i++){
    mins[i] = type_info<int>::max;
    maxs[i] = 0;
    moys[i] = 0;
  }
  int bbCount = 0;


  getStats(g, mins, maxs, moys, &bbCount, waysCount);

  cout << "bbcount : " << bbCount << endl;

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
    moys[i] /= bbCount;
    cout << moys[i] << " ";
  }
  cout << endl;

}





class CacheAnalysis: public Application {
public:
  CacheAnalysis(void): Application("CacheAnalysis", Version(1, 0, 0)) { }

protected:
  void work(const string &entry, PropList &props) override {
    
    //otawa::VERBOSE(props) = true;
    otawa::CACHE_CONFIG_PATH(props) = "mycache.xml";

    require(DECODED_TEXT);
    require(COLLECTED_CFG_FEATURE);
    require(otawa::hard::CACHE_CONFIGURATION_FEATURE);

    auto cfgs = COLLECTED_CFG_FEATURE.get(workspace());
    auto maincfg = cfgs->entry();
    
    auto confs = hard::CACHE_CONFIGURATION_FEATURE.get(workspace());
    auto icache = confs->instCache();
    CacheState mycache(icache);

    initState(maincfg, &mycache);
    statetest(maincfg, &mycache);
    //printStates(maincfg, &mycache);

    makeStats(maincfg, &mycache);
    
  }

private:
};



//OTAWA_RUN(CacheAnalysis)
int main(int argc, char **argv) {
  return CacheAnalysis().manage(argc, argv);
}
