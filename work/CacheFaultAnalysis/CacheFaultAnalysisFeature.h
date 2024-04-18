#include <elm/io.h>
#include <otawa/otawa.h>
#include <otawa/hard/CacheConfiguration.h>
#include <otawa/proc.h>
#include <otawa/proc/CFGProcessor.h>


using namespace elm;
using namespace otawa;

#ifndef NDEBUG
#define DEBUG(x) //cout << x
#define SPEDEBUG(x) //x
#else
#define DEBUG(x)
#define SPEDEBUG
#endif




extern p::feature CACHE_FAULT_ANALYSIS_FEATURE;


/**
 * @fn printbits
 * A function that prints a uint64 in binary form,
 * 8 bits by 8 bits
 * 
 * @param n the uint64 to print
*/
void printbits(elm::t::uint64 n, elm::io::Output &output = cout);


/**
 * @class State
 * A simplistic representation of the state of a cache set
 * 
 * @param isize the size of the cache set to initialise
 */
class State {
  public:
  State(int isize);

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
   * @fn getValue
   * Returns the value of an element of the state table.
   * 
   * @param pos int
   * @return value store at pos
   */
  inline otawa::hard::Cache::block_t getValue(int pos) {
    ASSERTP(pos >= 0 && pos < size, "In State.getValue() : argument 'pos', index out of bound.");
    return state[pos]; 
    }


  /**
   * @fn equals
   * Tests the equality between this state and a given state
   * 
   * @param state2 the state to compare to
   * @return true if both states are identical, false otherwise
   */
  bool equals(State* state2);

  friend elm::io::Output &operator<<(elm::io::Output &output, const State &state);

private:
  int size;
  otawa::hard::Cache::block_t* state;
};


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
  SaveState();

  /**
   * @fn setCache
   * Initiliases the private attributes required by the class
   * 
   * @param icache const otawa::hard::Cache* 
   * or
   * @param setCount int
   */
  void setCache(const otawa::hard::Cache* icache);
  void setCache(int setCount);

  /**
   * @fn add 
   * Adds a new cache State in the saved List 
   * to the given set 
   * 
   * @param newState the new State*
   * @param set the set to add State to (0 <= set < size)
   */
  void add(State *newState, int set);


  /**
   * @fn contains 
   * Checks wether or not the given State exists in
   * the stored States of the given set
   * 
   * @param newState the State to check the existence of
   * @param set the set to check
   */
  bool contains(State *newState, int set);

  /**
   * @fn getListSizes
   * Mostly used for stats purposes
   * @return int*
   */
  int* getListSizes() { return listSizes; }

  const List<State *>& getList(int set) { return saved[set]; }

  friend elm::io::Output &operator<<(elm::io::Output &output, const SaveState &saveState);
  
  private:
  int size;
  int* listSizes;
  List<State *> *saved;

};




/**
 * @class CacheState
 * A CacheState is an encapsulation of the otawa::hard::Cache class.
 * It stores the current cache state using an AllocArray and provides
 * various update methods based on the update policy.
 * 
 * @param icache the otawa::hard::Cache object to encapsulate.
 */
class AbstractCacheState {
public:
    AbstractCacheState(const otawa::hard::Cache* icache): cache(icache), nbSets(icache->setCount()), nbWays((int)pow(2,icache->wayBits())), logNbWays(icache->wayBits()) { 
    ASSERTP(logNbWays < 7, "CacheState: cache way limit is 2^6");
    state.allocate(nbSets);
    for (int e=0; e < (nbSets); e++) {
      state[e] = new State(nbWays);
    } 
  }

  AbstractCacheState(const AbstractCacheState& oldCacheState) :
    cache(oldCacheState.cache),
    nbSets(oldCacheState.nbSets),
    nbWays(oldCacheState.nbWays),
    logNbWays(oldCacheState.logNbWays),
    state(oldCacheState.state)
  {}

  virtual ~AbstractCacheState() {}


  /**
   * @fn update
   * 
   * @param toAdd instruction address to be added to the cache
  */
  virtual void update(otawa::address_t toAdd) = 0;

  /**
   * @fn displayState
   * prints to cout the current state of the cache
  */
  void displayState(elm::io::Output &output = cout) {
    for (int i=0; i < nbSets ; i++) {
      output << i << "\t:\t";
      output << *state[i] << endl;
    }
  }


  /**
   * @fn copy
   * Returns a copy of the current object
  */
  virtual AbstractCacheState* copy() = 0;


  // Various Getters
  inline const otawa::hard::Cache* getCache(){return cache;}

  inline AllocArray<State *>* getState(){return &state;}

  inline otawa::hard::Cache::block_t getTag(otawa::address_t toAdd) {return cache->block(toAdd);}

  inline otawa::hard::Cache::set_t getSet(otawa::address_t toAdd) {return cache->set(toAdd);}

  inline int getNbSets() {return nbSets;}


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
  State* getSubState(int set) {
    ASSERTP(set >= 0 && set < nbSets, "In getSubState() : argument 'set', index out of bound.");
    return state[set];
  }

  /**
   * @fn newSubState
   * Instantiates a new State object, initialises it by copying
   * the current State of the specified set and returns it
   * 
   * @param set the set of the State to copy
   * @return a newly instantiated State object
  */
  State* newSubState(int set){
    State* newState = new State(nbWays);
    int pos = 0;
    while (pos < nbWays){ 
      newState->setValue(pos,state[set]->getValue(pos));
      pos++;
    }
    return newState;
  }

protected:
  int nbWays;
  int logNbWays;
  int nbSets;
  AllocArray<State *> state;
  const otawa::hard::Cache* cache;
};




class CacheFaultAnalysisProcessor: public CFGProcessor {
public:
	static p::declare reg;
	CacheFaultAnalysisProcessor();

protected:
	void processAll(WorkSpace *ws) override;
	void processCFG(WorkSpace *ws, CFG *cfg) override {}
  void dump(WorkSpace *ws, Output &out) override;
	//void configure() override;

private:
  int exec_time;
  AbstractCacheState* mycache;
};


class CacheStateLRU: public AbstractCacheState {
public:
  CacheStateLRU(const otawa::hard::Cache* icache) : AbstractCacheState(icache) {}
  CacheStateLRU(const CacheStateLRU& oldCacheState): AbstractCacheState(oldCacheState){}

  void update(otawa::address_t toAdd);

  CacheStateLRU* copy() { return new CacheStateLRU(*this); }
};

class CacheStateFIFO: public AbstractCacheState {
public:
  CacheStateFIFO(const otawa::hard::Cache* icache) : AbstractCacheState(icache) {
    currIndexFIFO.allocate(nbSets);
    for (int e=0; e < nbSets; e++) {
      currIndexFIFO[e] = 0;
    } 
  }
  CacheStateFIFO(const CacheStateFIFO& oldCacheState): currIndexFIFO(oldCacheState.currIndexFIFO), AbstractCacheState(oldCacheState){
  }

  void update(otawa::address_t toAdd);

  CacheStateFIFO* copy() { return new CacheStateFIFO(*this); }
private:
  AllocArray<int> currIndexFIFO;
};


class CacheStatePLRU: public AbstractCacheState {
public:
  CacheStatePLRU(const otawa::hard::Cache* icache) : AbstractCacheState(icache) {
    accessBitsPLRU.allocate(nbSets);
    for (int e=0; e < nbSets; e++) {
      accessBitsPLRU[e] = 0;
    } 
  }
  CacheStatePLRU(const CacheStatePLRU& oldCacheState): accessBitsPLRU(oldCacheState.accessBitsPLRU), AbstractCacheState(oldCacheState){}

  void update(otawa::address_t toAdd);

  CacheStatePLRU* copy() { return new CacheStatePLRU(*this); }
private:
  AllocArray<elm::t::uint64> accessBitsPLRU;
};