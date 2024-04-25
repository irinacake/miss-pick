#include <elm/io.h>
#include <otawa/otawa.h>
#include <otawa/app/Application.h>
#include <otawa/app/CFGApplication.h>
#include <otawa/prog/TextDecoder.h>


#include "MultipleSetsSaver.h"



using namespace elm;
using namespace otawa;




//p::id<SavedCacheSetsArrays*> SAVED("SAVED");



class test: public CFGApplication {
public:
  test(void): CFGApplication(Make("test", Version(1, 0, 0))),
  cacheXml(option::ValueOption<string>::Make(*this).cmd("-c").cmd("--cache").help("Cache configuration xml file").usage(option::arg_required))
  
   { }

protected:
  void processTask(const CFGCollection& coll, PropList &props) override {

    otawa::CACHE_CONFIG_PATH(props) = *cacheXml;

    require(DECODED_TEXT);
    require(COLLECTED_CFG_FEATURE);
    require(otawa::hard::CACHE_CONFIGURATION_FEATURE);

    auto cfgs = COLLECTED_CFG_FEATURE.get(workspace());
    auto maincfg = cfgs->entry();
    auto icache = hard::CACHE_CONFIGURATION_FEATURE.get(workspace())->instCache();
    



    auto msSaver = new MultipleSetsSaver();
    msSaver->setupMWS(icache->setCount(),icache->wayCount());

    cout << "setcount, waycount : " << msSaver->getSetCount() << ", " << msSaver->getWayCount() << endl;

    auto s = new CacheSet(5);

    s->setStateValue(0,1);
    s->setStateValue(1,2);
    s->setStateValue(2,3);


    int *listSizes = msSaver->getSaversSizes();
    for (int i = 0; i < msSaver->getSetCount(); i++){
      cout << listSizes[i] << " ";
    }
    cout << endl;
  }

private:
  option::ValueOption<string> cacheXml;
};



OTAWA_RUN(test)
