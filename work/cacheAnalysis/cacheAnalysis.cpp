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

void printbits(elm::t::uint64 n){
  auto i = 1UL << 7;
  auto pcpt = 1;
  while(i>0){
    if(n&i)
      cout << 1;
    else 
      cout << 0;
    i >>= 1;
    if (pcpt % 8 == 0)
      cout << " ";
    pcpt++;
  }
  cout << io::endl;
}




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
  

  /**
   * @fn setCache
   * Initiliases the private attributes required by the class
   * 
   * @param icache const otawa::hard::Cache* 
   * or
   * @param setCount int
   */
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

  /**
   * @fn add 
   * Adds a new cache State in the saved List 
   * to the given set 
   * 
   * @param newState the new State*
   * @param set the set to add State to (0 <= set < size)
   */
  void add(State *newState, int set) {
    ASSERTP(set >= 0 && set < size, "In SaveState.add() : argument 'set', index out of bound.");
    if (!(saved[set].contains(newState))){
      listSizes[set]++;
      saved[set].add(newState);
    }
  }

  /**
   * @fn getListSizes
   * Mostly used for stats purposes
   * @return int*
   */
  int* getListSizes(){
    return listSizes;
  }

  friend elm::io::Output &operator<<(elm::io::Output &output, const SaveState &saveState);
  
  private:
  int size;
  int* listSizes;
  List<State *> *saved;
};

// Redefinition of the << operator for the SaveState class
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
  CacheState(const otawa::hard::Cache* icache): cache(icache), nbSets(icache->setCount()), nbWays((int)pow(2,icache->wayBits())), logNbWays(icache->wayBits()) { 
    ASSERTP(logNbWays < 7, "CacheState: cache way limit is 2^6");
    state.allocate(nbSets * nbWays);

    currIndexFIFO = new int[nbSets];
    accessBitsPLRU = new elm::t::uint64[nbSets];
    for (int e=0; e < nbSets; e++) {
      currIndexFIFO[e] = 0;
      accessBitsPLRU[e] = 0;
    } 

    for (int e=0; e < (nbSets * nbWays); e++) {
      state[e] = otawa::hard::Cache::block_t(-1);
    } 
  }

  /**
   * @fn updateLRU
   * Updates the current cache state with a new cache block according
   * to the LRU policy
   * 
   * Cache blocks are identified by the tag of the address of its
   * first instruction. The user is expected to handle which address
   * to give themselves.
   * 
   * This function will determine in which set to add this block based
   * on the value of the address and the methods provided by the 
   * encapsulated Cache item.
   * 
   * @param toAdd instruction address to be added to the cache
   */
  void updateLRU(otawa::address_t toAdd){
    /**
     * Algorithm details :
     * 1. retrieve the set to add the instruction to
     * 2. retrieve the tag that will represent the cache block
     * 3. initialise the "found" position to 0
     * 4. iterate through the corresponding set
     * 4.1. if the same tag is found, stop iterating
     * 4.2. otherwise continue until the end of the set
     * 5. iterate through the corresponding set again in reverse
     *    while shifting the currently stored tags towards the
     *    end of the set (increasing their age)
     * 6. set the first tag of the set (age 0) to the new tag
     * 
     * Notes : 
     * - whether the new tag is already in the cache or not is
     *   essentially treated in the same way, and the updade is
     *   already in O(n)
     * - checking the last entry of the set in unnecessary as it
     *   will either get evicted or "shifted" back to age 0. Instead
     *   of manually shifting it, setting the new tag to age 0 
     *   effectively achieves the same result.
    */

    auto toAddSet = cache->set(toAdd);
    auto toAddTag = cache->block(toAdd);

    // position variable
    int pos = 0;
    
    // search loop : break before incrementing if the same tag is found
    while (pos < nbWays - 1){ 
      if (toAddTag == state[toAddSet * nbWays + pos]) break;
      pos++;
    }
    
    // reverse loop : overwrite the current tag (pos) with the 
    // immediately younger tag (pos-1)
    while (pos > 0){
      state[toAddSet * nbWays + pos] = state[toAddSet * nbWays + pos-1];
      pos--;
    }

    // set age 0 with the new tag
    state[toAddSet * nbWays + pos] = toAddTag;
    
    //displayState();
    //cout << endl;
  }

  /**
   * @fn updateFIFO
   * Updates the current cache state with a new cache block according
   * to the FIFO policy
   * 
   * Cache blocks are identified by the tag of the address of its
   * first instruction. The user is expected to handle which address
   * to give themselves.
   * 
   * This function will determine in which set to add this block based
   * on the value of the address and the methods provided by the 
   * encapsulated Cache item.
   * 
   * @param toAdd instruction address to be added to the cache
   */
  void updateFIFO(otawa::address_t toAdd){
    /**
     * Algorithm details :
     * 1. retrieve the set to add the instruction to
     * 2. retrieve the tag that will represent the cache block
     * 3. initialise the "found" position to 0
     * 4. iterate through the corresponding set
     * 4.1. if the same tag is found, stop everything
     * 4.2. otherwise set the currIndex to the new tag
     *      and increment it by 1
    */

    auto toAddSet = cache->set(toAdd);
    auto toAddTag = cache->block(toAdd);

    // position variable
    int pos = 0;
    bool found = false;
    
    // search loop : break before incrementing if the same tag is found
    while (pos < nbWays && !found){
      if (toAddTag == state[toAddSet * nbWays + pos]) {
        found = true;
        break;
      }
      pos++;
    }

    if (!found) {
      state[toAddSet * nbWays + currIndexFIFO[toAddSet]] = toAddTag;
      currIndexFIFO[toAddSet] = (currIndexFIFO[toAddSet]+1) % nbWays;
    }

    //displayState();
    //cout << endl;
  }

  /**
   * @fn updatePLRU
   * Updates the current cache state with a new cache block according
   * to the PseudoLRU policy
   * 
   * Cache blocks are identified by the tag of the address of its
   * first instruction. The user is expected to handle which address
   * to give themselves.
   * 
   * This function will determine in which set to add this block based
   * on the value of the address and the methods provided by the 
   * encapsulated Cache item.
   * 
   * @param toAdd instruction address to be added to the cache
   */
  void updatePLRU(otawa::address_t toAdd){
    /**

    */

    auto toAddSet = cache->set(toAdd);
    auto toAddTag = cache->block(toAdd);


    // position variable
    int pos = 0;
    bool found = false;
    
    // search loop : break before incrementing if the same tag is found
    while (pos < nbWays && !found){ 
      if (toAddTag == state[toAddSet * nbWays + pos]) {
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
      state[toAddSet * nbWays + access] = toAddTag;
    }

    //displayState();
    //cout << endl;
  }

  void update(otawa::address_t toAdd){
    switch (cache->replacementPolicy())
    {
    case otawa::hard::Cache::LRU:
      updateLRU(toAdd);
      break;
    case otawa::hard::Cache::FIFO:
      updateFIFO(toAdd);
      break;
    case otawa::hard::Cache::PLRU:
      updatePLRU(toAdd);
      break;
    
    default:
      break;
    }
  }

  /**
   * @fn displayState
   * prints to cout the current state of the cache
  */
  void displayState(){
    //TODO use "cout" as an argument (output something)
    for (int i=0; i < nbSets ; i++) {
      cout << i << " : ";
      for (int j=0; j < nbWays; j++){
        cout << state[i * nbWays + j] << "  ";
      }
      cout << endl;
    }
  }

  // Various Getters
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

  /**
   * @fn getSubState
   * Instantiates a new State object, initialises it and
   * returns it
   * 
   * @param toGet otawa::address_t 
   * @return newState a newly instantiated State object 
   * that represents the state of the cache set of the
   * given instruction's address.
  */
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
  int logNbWays;
  int nbSets;
  int *currIndexFIFO;
  elm::t::uint64 *accessBitsPLRU;
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

          mycache->update(inst->address());

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
        if (listSizes[i] != 0){
          moys[i] += listSizes[i];
          bbCount[i]++;
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

    cout << "Policy : " << icache->replacementPolicy() << endl;
    mycache.displayState();
    cout << endl;

    makeStats(maincfg, &mycache);
    
  }

private:
};



//OTAWA_RUN(CacheAnalysis)
int main(int argc, char **argv) {
  return CacheAnalysis().manage(argc, argv);
}
