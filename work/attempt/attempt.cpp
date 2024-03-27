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


p::id<elm::t::uint64> KICK("KICK", -1L);


class CacheState {
public:
  CacheState(const otawa::hard::Cache* icache): cache(icache)
    { state = new otawa::hard::Cache::block_t[cache->blockBits()]; }

  ~CacheState(){ delete [] state; }


  void updateLRU(otawa::address_t toAdd){
    auto tag = cache->block(toAdd);

    // démarrer à zéro
    // vérifier l'existence progressive
    // si trouvé, alors faire des xch progressifs
    int pos = 0;
    
    // there is no need to check the last entry of the state, it either gets deleted, or shifted to age 0
    // if the tag is found in the middle, break the search loop
    while (pos < cache->blockBits() - 1){ 
      if (tag == state[pos]) break;
      pos++;
    }
    
    // overwrite to simulate elements switch
    while (pos > 0){
      state[pos] = state[pos-1];
      pos--;
    }

    // new tag has age 0
    state[pos] = tag;

  }


  const otawa::hard::Cache* getCache(){
    return cache;
  }


private:
  otawa::hard::Cache::block_t* state;
  const otawa::hard::Cache* cache;
};






otawa::Block* cache[3] = {NULL,NULL,NULL};
int cacheSize = 3;
int currElem = 0;


void printbits(elm::t::uint64 n){
  auto i = 1UL << 63;
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



void printDom(CFG *g, string indent, bool dive) {
	//cout << g << "(" << g->count() << ")" << io::endl;
	for(auto v: *g){
		if(v->isSynth() && dive) {
			printDom(v->toSynth()->callee(), indent + "\t", dive);
    }
    cout << indent << "-> BB " << v->id() << " : ";
    printbits(KICK(v));
  }
}


void initDom(CFG *g) {
  auto nb_bb = g->count();
  for(auto v: *g){
		if(v->isSynth()) {
			initDom(v->toSynth()->callee());
		}
    KICK(v) = 0UL;
  }
}


void computeDom(CFG *g) {

  // compute KICK so long as there are changes
  bool change = true;

  while (change) {
    // By default, there are no changes
    change = false;

    // start at the entry
    auto currB = g->entry();
    // reset the cache every time
    for (int i = 0; i < cacheSize; i++)
      cache[i] = NULL;

    for (auto inst: *currB->toBasic()){
      inst->address();
    }

    while (!currB->isExit()){

      if (cache[currElem] == NULL){
        cache[currElem] = currB;
        currElem = (currElem + 1) % cacheSize;
      } else {
        if (KICK(cache[currElem]) != (KICK(cache[currElem]) | (1 << currB->id()))){
          change = true;
        }
        KICK(cache[currElem]) = (KICK(cache[currElem]) | (1 << currB->id()) );
        cache[currElem] = currB;
        currElem = (currElem + 1) % cacheSize;
      }

      // comment sélectionner le prochain block
      if (currB->countOuts() == 1) {
        for (auto e: currB->outEdges())
          currB = e->sink();
      } else {
        for (auto e: currB->outEdges()){
          if (e->isTaken()) 
            currB = e->sink();
        }
      }

      if(currB->isSynth()) {
        currB = currB->toSynth()->callee()->entry();
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

    CacheState mycache(icache);

    cout << "size : " << mycache.getCache()->blockBits() << endl;
    
    
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
