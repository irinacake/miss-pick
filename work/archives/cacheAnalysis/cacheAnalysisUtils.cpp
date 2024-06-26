#include "cacheAnalysisUtils.hpp"



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





CacheState::CacheState(const otawa::hard::Cache* icache): cache(icache), nbSets(icache->setCount()), nbWays((int)pow(2,icache->wayBits())), logNbWays(icache->wayBits()) { 
  ASSERTP(logNbWays < 7, "CacheState: cache way limit is 2^6");
  state.allocate(nbSets);

  currIndexFIFO.allocate(nbSets);
  accessBitsPLRU.allocate(nbSets);
  for (int e=0; e < nbSets; e++) {
    currIndexFIFO[e] = 0;
    accessBitsPLRU[e] = 0;
  } 

  for (int e=0; e < (nbSets); e++) {
    state[e] = new State(nbWays);
  } 
}

CacheState::CacheState(const CacheState& oldCacheState) :
  cache(oldCacheState.cache),
  nbSets(oldCacheState.nbSets),
  nbWays(oldCacheState.nbWays),
  logNbWays(oldCacheState.logNbWays),

  state(oldCacheState.state),
  currIndexFIFO(oldCacheState.currIndexFIFO),
  accessBitsPLRU(oldCacheState.accessBitsPLRU) {}



void CacheState::update(otawa::address_t toAdd){
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


void CacheState::displayState(elm::io::Output &output){
  for (int i=0; i < nbSets ; i++) {
    output << i << "\t:\t";
    output << *state[i] << endl;
  }
}

