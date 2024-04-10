#include <elm/io.h>
#include <otawa/otawa.h>
#include <otawa/hard/CacheConfiguration.h>


using namespace elm;
using namespace otawa;

#ifndef NDEBUG
#define DEBUG(x) cout << x
#define SPEDEBUG(x) x
#else
#define DEBUG(x)
#define SPEDEBUG
#endif


void printbits(elm::t::uint64 n);


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
   * @fn equals
   * Tests the equality between this state and a given state
   * 
   * @param state2 the state to compare to
   * @return true if both states are identical, false otherwise
   */
  bool equals(State* state2);

  inline otawa::hard::Cache::block_t getValue(int pos) {
    ASSERTP(pos >= 0 && pos < size, "In State.getValue() : argument 'pos', index out of bound.");
    return state[pos]; 
    }

  friend elm::io::Output &operator<<(elm::io::Output &output, const State &state);

private:
  int size;
  otawa::hard::Cache::block_t* state;

};






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

  bool contains(State *newState, int set);

  /**
   * @fn getListSizes
   * Mostly used for stats purposes
   * @return int*
   */
  int* getListSizes() { return listSizes; }

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
class CacheState {
public:
  CacheState(const otawa::hard::Cache* icache);
  CacheState(const CacheState& oldCacheState);

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
  void updateLRU(otawa::address_t toAdd);
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
  void updateFIFO(otawa::address_t toAdd);
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
  void updatePLRU(otawa::address_t toAdd);

  void update(otawa::address_t toAdd);

  /**
   * @fn displayState
   * prints to cout the current state of the cache
  */
  void displayState();


  virtual CacheState* copy() { return new CacheState(*this); }

  //bool existsIn(otawa::Block* blockCheck);
  //bool existsIn(otawa::Block* blockCheck, int set);


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

  State* newSubState(int set);

private:
  int nbWays;
  int logNbWays;
  int nbSets;
  AllocArray<int> currIndexFIFO;
  AllocArray<elm::t::uint64> accessBitsPLRU;
  AllocArray<State *> state;
  const otawa::hard::Cache* cache;

};