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



class CacheState {
public:
  CacheState(const otawa::hard::Cache* icache): cache(icache), nbSets(icache->setCount()), nbWays((int)pow(2,icache->wayBits())) { 
  
    state = AllocArray<otawa::hard::Cache::block_t>(nbSets * nbWays);

    

    for (int e=0; e < (nbSets * nbWays); e++) {
      state[e] = otawa::hard::Cache::block_t(-1);
    } 
 
    displayState();
  }

  ~CacheState(){ }

  void updateLRU(otawa::address_t toAdd, string indent){
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
    cout << endl;
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


  void getSubState(){
    //todo : make a "new" item
  }

private:
  int nbWays;
  int nbSets;
  AllocArray<otawa::hard::Cache::block_t> state;
  //otawa::hard::Cache::block_t* state;
  const otawa::hard::Cache* cache;
};



class State {
  public:
  State(int size) {
    state = new otawa::hard::Cache::block_t[size];
  }
  inline otawa::hard::Cache::block_t* getState(){
    return state;
  }

  private:
  otawa::hard::Cache::block_t* state;
};



class SaveState {
  public:
  SaveState(const otawa::hard::Cache* icache) { 
    saved = new List<State *> [icache->setCount()];
  }

  void add (State *newState) {
    // TODO check if newState not already in saved
    saved->add(newState);
  }

  private:
  List<State *> *saved;
};


//p::id<SaveState> SAVED("SAVED", -1L);
//List<int>






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
          mycache->updateLRU(inst->address(), indent);
          currTag = mycache->getTag(inst->address());
          currSet = mycache->getSet(inst->address());
        } else {
          cout << "cache hit" << endl << endl;
        }
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

    //icache->cacheSize()

    cout << icache->setCount() << endl;

    CacheState mycache(icache);

    //cout << "waybits : " << icache->wayBits() << endl;
    
    statetest(maincfg, &mycache);
    
    

    
    /*

    initDom(maincfg);

    printDom(maincfg, "", false);

    computeDom(maincfg);
    
    cout << io::endl << "after compute" << io::endl;
    
    printDom(maincfg, "", false);
    */

  }

private:
  int i=0;
};



//OTAWA_RUN(Attempt)
int main(int argc, char **argv) {
  return Attempt().manage(argc, argv);
}
