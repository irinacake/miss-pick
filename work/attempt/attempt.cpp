#include <elm/io.h>
#include <otawa/otawa.h>
#include <otawa/app/Application.h>
#include <otawa/prog/TextDecoder.h>
#include <otawa/cfg/features.h>
#include <otawa/cfg/Loop.h>

#include <elm/sys/FileItem.h>
#include <elm/util/BitVector.h>


using namespace elm;
using namespace otawa;


p::id<elm::t::uint64> KICK("KICK", -1L);

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
    
    require(DECODED_TEXT);
    require(COLLECTED_CFG_FEATURE);
    require(LOOP_INFO_FEATURE);

    auto cfgs = COLLECTED_CFG_FEATURE.get(workspace());

    auto maincfg = cfgs->entry();
    

    initDom(maincfg);

    printDom(maincfg, "", true);

    computeDom(maincfg);
    
    cout << io::endl << "after compute" << io::endl;
    
    printDom(maincfg, "", true);

  }

private:
  int i=0;
};



//OTAWA_RUN(Attempt)
int main(int argc, char **argv) {
  return Attempt().manage(argc, argv);
}
