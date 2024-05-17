#include <elm/io.h>
#include <otawa/otawa.h>
#include <otawa/app/Application.h>
#include <otawa/app/CFGApplication.h>


#include "CacheMissFeature.h"



using namespace elm;
using namespace otawa;






class test: public CFGApplication {
public:
test(void): CFGApplication(Make("test", Version(1, 0, 0))),
cacheXml(option::ValueOption<string>::Make(*this).cmd("-c").cmd("--cache").help("Cache configuration xml file").usage(option::arg_required))

{ }

protected:
  void processTask(const CFGCollection& coll, PropList &props) override {


    
    otawa::CACHE_CONFIG_PATH(props) = *cacheXml;

    

    require(CACHE_MISS_FEATURE);
    
  }

private:
  option::ValueOption<string> cacheXml;
};



OTAWA_RUN(test)
