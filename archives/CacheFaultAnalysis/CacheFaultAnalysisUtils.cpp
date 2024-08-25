#include "CacheFaultAnalysisFeature.h"



void printbits(elm::t::uint64 n, elm::io::Output &output){
  auto i = 1UL << 7;
  auto pcpt = 1;
  while(i>0){
    if(n&i)
      output << 1;
    else 
      output << 0;
    i >>= 1;
    if (pcpt % 8 == 0)
      output << " ";
    pcpt++;
  }
  output << io::endl;
}


State::State(int isize): size(isize) {
  state = new otawa::hard::Cache::block_t[size];
}

bool State::equals(State* state2){
  if (this->size == state2->size) {
    for (int i = 0; i < this->size; i++) {
      if (this->state[i] != state2->state[i]){
        return false;
      }
    }
    return true;
  } else {
    return false;
  }
}

// Redefinition of the << operator for the State class
elm::io::Output &operator<<(elm::io::Output &output, const State &state) {
  output << "( ";
  for (int i = 0; i < state.size ; i++){
    output << state.state[i] << " ";
  }
  output << ") ";
  return output;
}






SaveState::SaveState(): saved(nullptr), size(0) {}

void SaveState::setCache(const otawa::hard::Cache* icache){
    size = icache->setCount();
    saved = new List<State *> [size];
    listSizes = new int[size];
    for (int i = 0; i < size; i++){
        listSizes[i] = 0;
    }
}

void SaveState::setCache(int setCount){
  size = setCount;
  saved = new List<State *> [size];
  listSizes = new int[size];
  for (int i = 0; i < size; i++){
    listSizes[i] = 0;
  }
}

void SaveState::add(State *newState, int set) {
  ASSERTP(set >= 0 && set < size, "In SaveState.add() : argument 'set', index out of bound.");
  if (!(saved[set].contains(newState))){
    DEBUG("Adding new state" << endl);
    listSizes[set]++;
    saved[set].add(newState);
  } else {
    DEBUG("not Adding new state" << endl);
  }
}

bool SaveState::contains(State *newState, int set){
    if ((saved[set].contains(newState))){
        return true;
    } 
    return false;
}


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
 * @fn update (LRU)
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
 *
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
 **/
void CacheStateLRU::update(otawa::address_t toAdd){
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
    pos--;
  }

  // set age 0 with the new tag
  state[toAddSet]->setValue(pos,toAddTag);
}


/**
 * @fn update (FIFO)
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
 *
 * Algorithm details :
 * 1. retrieve the set to add the instruction to
 * 2. retrieve the tag that will represent the cache block
 * 3. initialise the "found" position to 0
 * 4. iterate through the corresponding set
 * 4.1. if the same tag is found, stop everything
 * 4.2. otherwise set the currIndex to the new tag
 *      and increment it by 1
 **/
void CacheStateFIFO::update(otawa::address_t toAdd){
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
}


/**
 * @fn updatePLRU
 * Updates the current cache state with a new cache block according
 * to the PseudoLRU policy : https://en.wikipedia.org/wiki/Pseudo-LRU
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
 **/
void CacheStatePLRU::update(otawa::address_t toAdd){
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
    while (i < logNbWays) {
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
}
