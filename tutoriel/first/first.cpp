#include <elm/io.h>
#include <otawa/otawa.h>
#include <otawa/app/Application.h>
#include <otawa/prog/TextDecoder.h>
#include <otawa/cfg/features.h>

using namespace elm;
using namespace otawa;

class First: public Application {
public:
  First(void): Application("first", Version(1, 0, 0)) { }

protected:
  void work(const string &entry, PropList &props) override {
    
    //Address addr = workspace()->process()->findLabel(entry);
    //cout << entry << " found at " << addr << io::endl;

    require(DECODED_TEXT);
    //require(COLLECTED_CFG_FEATURE);

    //auto inst = workspace()->findInstAt(addr);
    auto inst = workspace()->process()->findInstAt(entry);

    while (!inst->isReturn()){
      i++;
      cout << "[" << i << "](" << inst->address() << ") " << inst;
      cout << io::endl;
      
      inst = inst->nextInst();
    }

    /*
    for(auto file: workspace()->process()->files()){
      for (auto seg: file->segments()){
        
      }
    }*/
  }

private:
  int i=0;
};


int main(int argc, char **argv) {
  return First().manage(argc, argv);
}
