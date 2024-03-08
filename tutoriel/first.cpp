#include <elm/io.h>
#include <otawa/otawa.h>
#include <otawa/app/Application.h>


using namespace elm;
using namespace otawa;

class First: public Application {
public:
  First(void): Application("first", Version(1, 0, 0)) { }

protected:
  void work(const string &entry, PropList &props) override {
    i++;
    cout << "[" << i << "]";
    Address addr = workspace()->process()->findLabel(entry);
    cout << entry << " found at " << addr << io::endl;
  }

private:
  int i=0;
};


int main(int argc, char **argv) {
  return First().manage(argc, argv);
}
