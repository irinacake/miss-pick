#include <elm/io.h>
#include <otawa/otawa.h>
#include <otawa/app/Application.h>
#include <otawa/prog/TextDecoder.h>
#include <otawa/cfg/features.h>
#include <otawa/cfg/Loop.h>



#include <elm/sys/FileItem.h>


using namespace elm;
using namespace otawa;



void dumpCall(CFG *g, string indent) {
	cout << g << io::endl;
	indent = indent + "\t";
	for(auto v: *g)
		if(v->isSynth()) {
			cout << indent << "-> ";
			dumpCall(v->toSynth()->callee(), indent);
		}
}


class Supercfg: public Application {
public:
  Supercfg(void): Application("Supercfg", Version(1, 0, 0)) { }

protected:
  void work(const string &entry, PropList &props) override {
    
    require(DECODED_TEXT);
    require(COLLECTED_CFG_FEATURE);
    require(LOOP_INFO_FEATURE);

    //const CFGCollection *cfgs = COLLECTED_CFG_FEATURE.get(workspace());
    auto cfgs = COLLECTED_CFG_FEATURE.get(workspace());

    auto cfg = cfgs->get(1);
    

    
    auto top_loop = Loop::top(cfg);



    cout << top_loop->header() << io::endl;
    
    /*
    cout << "count ins : " << b->countIns() << io::endl;
    cout << "count outs: " << b->countOuts() << io::endl;

    for (auto e: b->outEdges()){
      cout << e->isCall() << io::endl;
      cout << e->sink() << io::endl;
      auto s = e->sink();
      cout << "count ins : " << s->countIns() << io::endl;
      cout << "count outs: " << s->countOuts() << io::endl;

      auto bb = s->toBasic();

      //for (auto b: *bb){
      //  cout << b << io::endl;
      //}
    }
    
    dumpCall(cfgs->get(0), "");
    */
    
  }

private:
  int i=0;
};



//OTAWA_RUN(Supercfg)
int main(int argc, char **argv) {
  return Supercfg().manage(argc, argv);
}
