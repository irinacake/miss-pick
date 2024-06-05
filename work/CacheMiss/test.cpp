#include <elm/io.h>
#include <otawa/otawa.h>
#include <otawa/app/Application.h>
#include <otawa/app/CFGApplication.h>
#include <elm/options.h>


#include "CacheMissFeature.h"



using namespace elm;
using namespace otawa;






class test: public CFGApplication {
public:
  test(void): CFGApplication(Make("test", Version(1, 0, 0))),
  cacheXml(option::ValueOption<string>::Make(*this).cmd("-c").cmd("--cache").help("Cache configuration xml file").usage(option::arg_required)),
  projection(option::SwitchOption::Make(*this).cmd("-p").cmd("--projection").help("Use to set projection to true"))

{ }

protected:
  void processTask(const CFGCollection& coll, PropList &props) override {


    
    otawa::CACHE_CONFIG_PATH(props) = *cacheXml;
  
    if (projection) {
        PROJECTION(props) = true;
    } else {
        PROJECTION(props) = false;
    }

    

    require(CACHE_MISS_FEATURE);
    
  }

private:
  option::ValueOption<string> cacheXml;
  option::Switch projection;
};



OTAWA_RUN(test)
