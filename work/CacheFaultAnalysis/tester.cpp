#include <elm/io.h>
#include <otawa/otawa.h>
#include <otawa/app/Application.h>

#include "CacheFaultAnalysisFeature.h"


using namespace elm;
using namespace otawa;






class tester: public Application {
public:
  tester(void): Application("tester", Version(1, 0, 0)),
  cacheXml(option::ValueOption<string>::Make(*this).cmd("-c").cmd("--cache").help("Cache configuration xml file").usage(option::arg_required))
  
   { }

protected:
  void work(const string &entry, PropList &props) override {

    
    otawa::CACHE_CONFIG_PATH(props) = *cacheXml;


    require(CACHE_FAULT_ANALYSIS_FEATURE);

  }

private:
  option::ValueOption<string> cacheXml;
};



OTAWA_RUN(tester)
