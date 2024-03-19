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


p::id<elm::t::uint64> DOM("DOM", -1L);



void dumpCall(CFG *g, string indent) {
  // g is a CFG, not a collection of CFG
  // printing g prints its name
  // count() gives the amount of blocks contained,
  // including entry and exit which are empty
	cout << g << "(" << g->count() << ")" << io::endl;
	indent = indent + "\t";

  // for every block, v, in this CFG
	for(auto v: *g)

    // if it is a synth block (ie: function call)
		if(v->isSynth()) {
      // increase the indent
			cout << indent << "-> ";
      // recursive call to dump to the called cfg
			dumpCall(v->toSynth()->callee(), indent);

    // if it is a basic block
		} else if (v->isBasic()) {
      // check how many instructions it has
      cout << indent << "-> BB :" << DOM(v) << io::endl;
      // for each instruction it has
      /*
      for (auto inst: *v->toBasic()){
        // print themp
        cout << indent << "-----> ";
			  cout << inst << io::endl;
      }
      */
    }
}





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


void printDom(CFG *g, string indent) {
	//cout << g << "(" << g->count() << ")" << io::endl;
	for(auto v: *g){
		if(v->isSynth()) {
			printDom(v->toSynth()->callee(), indent + "\t");
    }
    cout << indent << "-> BB " << v->index() << " : ";
    printbits(DOM(v));
  }
}

void initDom(CFG *g) {
  auto nb_bb = g->count();
  for(auto v: *g){
		if(v->isSynth()) {
			initDom(v->toSynth()->callee());
		}
    DOM(v) = (1UL << nb_bb) - 1; // set the nb_bb first bits to 1, others to 0
  }
}


void computeDom(CFG *g) {

  bool change = true;

  while (change) {
    for(auto v: *g){
      if(v->isSynth()) {
        initDom(v->toSynth()->callee());
      }

      int newdom = -1L;
      // newdom = newdom intersection 
      if (v->isEntry()){
        newdom = newdom & DOM(v);
      } else {
        // for p in pred(v)
          // newdom = newdom . dom(p)
      }

      DOM(v) = v->index() | newdom;
    }
  }
}


class Dominator: public Application {
public:
  Dominator(void): Application("Dominator", Version(1, 0, 0)) { }

protected:
  void work(const string &entry, PropList &props) override {
    
    require(DECODED_TEXT);
    require(COLLECTED_CFG_FEATURE);
    require(LOOP_INFO_FEATURE);

    auto cfgs = COLLECTED_CFG_FEATURE.get(workspace());

    auto maincfg = cfgs->entry();
    

    initDom(maincfg);

    printDom(maincfg, "");

  }

private:
  int i=0;
};



//OTAWA_RUN(Dominator)
int main(int argc, char **argv) {
  return Dominator().manage(argc, argv);
}
