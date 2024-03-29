#include <elm/io.h>
#include <otawa/otawa.h>
#include <otawa/app/Application.h>
#include <otawa/prog/TextDecoder.h>
#include <otawa/cfg/features.h>
#include <otawa/cfg/Loop.h>
#include <otawa/hard/features.h>
#include <otawa/hard/CacheConfiguration.h>



#include <elm/sys/FileItem.h>
#include <elm/util/BitVector.h>


using namespace elm;
using namespace otawa;




class State {
  public:
  State(int isize): size(isize) {
    state = new otawa::hard::Cache::block_t[size];
  }
  inline otawa::hard::Cache::block_t* getState(){
    return state;
  }
  inline void setValue(int pos, otawa::hard::Cache::block_t value){
    state[pos] = value;
  }
  friend elm::io::Output &operator<<(elm::io::Output &output, const State &state);

  private:
  int size;
  otawa::hard::Cache::block_t* state;
};

elm::io::Output &operator<<(elm::io::Output &output, const State &state) {
  cout << "state ? size : " << state.size << endl;

  for (int i = 0; i < state.size ; i++){
    output << state.state[i] << " ";
  }
  return output;
}




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

  void add(State *newState, int set) {
    // TODO check if newState not already in saved    
    listSizes[set]++;
    saved[set].add(newState);
  }

  friend elm::io::Output &operator<<(elm::io::Output &output, const SaveState &saveState);
  
  private:
  int size;
  int* listSizes;
  List<State *> *saved;
};

elm::io::Output &operator<<(elm::io::Output &output, const SaveState &saveState) {
  cout << "save state printer, size: " << saveState.size << endl;

  for (int i = 0; i < saveState.size ; i++){
    for (int j = 0; j < saveState.listSizes[i]; j++){
      cout << "i : " << i << ", j : " << j << endl;
      if (saveState.saved[i][j] == nullptr) {
        output << "nullptr" << endl;
      } else {
        output << *saveState.saved[i][j];
      }
    }
  }
  return output;
}





class CacheState {
public:
  CacheState(const otawa::hard::Cache* icache): cache(icache), nbSets(icache->setCount()), nbWays((int)pow(2,icache->wayBits())) { 
  
    state.allocate(nbSets * nbWays);

    

    for (int e=0; e < (nbSets * nbWays); e++) {
      state[e] = otawa::hard::Cache::block_t(-1);
    } 
 
    displayState();
  }

  ~CacheState(){ }

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
    
    displayState();
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
    //todo : make a "new" item
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
  //otawa::hard::Cache::block_t* state;
  const otawa::hard::Cache* cache;
};



p::id<SaveState*> SAVED("SAVED");
//List<int>



void printStates(CFG *g, CacheState *mycache,
              int currTag = -1, int currSet = -1, string indent = "") {

  for(auto v: *g){
		if(v->isSynth()) {
			//printStates(v->toSynth()->callee(), mycache, currTag, currSet, indent + "\t");
    } else if (v->isBasic()) {
      for (auto inst : *v->toBasic()){
        if (currTag != mycache->getTag(inst->address())
            || currSet != mycache->getSet(inst->address()) ){
          currTag = mycache->getTag(inst->address());
          currSet = mycache->getSet(inst->address());
          


          SaveState* currSaveState = SAVED(inst);

          cout << " curr save state " << currSaveState << " " << *currSaveState << endl;
          //cout << indent << *(SAVED(inst)) << endl;
        }
      }
    }
  }
}

void initState(CFG *g, CacheState *mycache,
              int currTag = -1, int currSet = -1, string indent = "") {
  for(auto v: *g){
		if(v->isSynth()) {
			initState(v->toSynth()->callee(), mycache, currTag, currSet, indent + "\t");
    } else if (v->isBasic()) {
      for (auto inst : *v->toBasic()){
        if (currTag != mycache->getTag(inst->address())
            || currSet != mycache->getSet(inst->address()) ){
          currTag = mycache->getTag(inst->address());
          currSet = mycache->getSet(inst->address());

          SaveState* newSaveState = new SaveState;
          
          newSaveState->setCache(mycache->getCache());
          
          SAVED(inst) = newSaveState;
        }
      }
    }
  }
}

void statetest(CFG *g, CacheState *mycache,
              int currTag = -1, int currSet = -1, string indent = "") {

	for(auto v: *g){
		if(v->isSynth()) {
			//statetest(v->toSynth()->callee(), mycache, currTag, currSet, indent + "\t");
    } else if (v->isBasic()) {
      for (auto inst : *v->toBasic()){

        cout << indent << "inst adr : " << inst->address() << endl;
        cout << indent << "inst tag : " << mycache->getTag(inst->address()) << endl;
        cout << indent << "inst set : " << mycache->getSet(inst->address()) << endl;

        if (currTag != mycache->getTag(inst->address())
            || currSet != mycache->getSet(inst->address()) ){
          mycache->updateLRU(inst->address());

          currTag = mycache->getTag(inst->address());
          currSet = mycache->getSet(inst->address());

          State* newState = mycache->getSubState(inst->address());
          cout << indent << "new state : " << newState << endl;
          //cout << indent << "SaveState of current inst : " << currSaveState << endl;
          SAVED(inst)->add(newState, currSet);


          //SaveState* currSaveState = SAVED(inst);
          //cout << indent << "current SaveState of inst : " << *currSaveState << endl;

        } else {
          cout << indent << "-> cache hit!" << endl;
        }
        cout << endl << endl;
      }
    }
  }
}


class Attempt: public Application {
public:
  Attempt(void): Application("Attempt", Version(1, 0, 0)) { }

protected:
  void work(const string &entry, PropList &props) override {
    
    //otawa::VERBOSE(props) = true;
    otawa::CACHE_CONFIG_PATH(props) = "mycache.xml";


    require(DECODED_TEXT);
    require(COLLECTED_CFG_FEATURE);
    require(otawa::hard::CACHE_CONFIGURATION_FEATURE);

    auto cfgs = COLLECTED_CFG_FEATURE.get(workspace());

    auto maincfg = cfgs->entry();
    


    //auto *mycache = new CacheState();


    auto confs = hard::CACHE_CONFIGURATION_FEATURE.get(workspace());

    auto icache = confs->instCache();

    cout << icache->replacementPolicy() << io::endl;

    cout << icache->setCount() << endl;

    CacheState mycache(icache);

    

    initState(maincfg, &mycache);
    
    statetest(maincfg, &mycache);

    printStates(maincfg, &mycache);
    
  }

private:
  int i=0;
};



//OTAWA_RUN(Attempt)
int main(int argc, char **argv) {
  return Attempt().manage(argc, argv);
}
