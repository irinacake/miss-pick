#include <elm/io.h>
#include <otawa/otawa.h>
#include <otawa/app/Application.h>
#include <otawa/app/CFGApplication.h>
#include <elm/options.h>
#include <otawa/trivial/features.h>
#include <otawa/display/ILPSystemDisplayer.h>

#include "CacheMissFeature.h"



using namespace elm;
using namespace otawa;






class test: public CFGApplication {
public:
  test(void): CFGApplication(Make("test", Version(1, 0, 0))),
  cacheXml(option::ValueOption<string>::Make(*this).cmd("-c").cmd("--cache").help("Cache configuration xml file").usage(option::arg_required)),
  outputfile(option::ValueOption<string>::Make(*this).cmd("-o").cmd("--outputfile").help("Output file").usage(option::arg_required)),
  projection(option::SwitchOption::Make(*this).cmd("-p").cmd("--projection").help("Use to set projection to true"))

{ }

protected:
  void processTask(const CFGCollection& coll, PropList &props) override {


    trivial::INSTRUCTION_TIME(props) = 1;
    
    otawa::CACHE_CONFIG_PATH(props) = *cacheXml;
    ipet::EXPLICIT(props) = true;

    if (projection) {
        PROJECTION(props) = true;
    } else {
        PROJECTION(props) = false;
    }

    


    auto cmProc = new CacheMissProcessor();
    run(cmProc);


    require(trivial::EVENT_ADDING_FEATURE);
    require(ipet::WCET_FEATURE);

    if (outputfile == nullptr) {
      cout << "{\n";
      cmProc->dumpStats(workspace(), cout);
      cout << "\t\"WCET\" : " << ipet::WCET(workspace()) << ",\n";
      cout << "}" << endl;
    } else {
      auto output = io::Output(*sys::Path(outputfile).write());
      output << "{\n";
      cmProc->dumpStats(workspace(), output);
      output << "\t\"WCET\" : " << ipet::WCET(workspace()) << "\n";
      output << "}" << endl;
      delete &output.stream();
    }


    
    //cout << "WCET:" << ipet::WCET(workspace()) << endl;

    //run(new otawa::display::ILPSystemDisplayer);
  }

private:
  option::ValueOption<string> cacheXml;
  option::ValueOption<string> outputfile;
  option::Switch projection;
};



OTAWA_RUN(test)
