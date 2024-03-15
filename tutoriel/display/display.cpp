#include <elm/io.h>
#include <otawa/otawa.h>
#include <otawa/app/Application.h>
#include <otawa/prog/TextDecoder.h>
#include <otawa/cfg/features.h>

#include <elm/sys/FileItem.h>

using namespace elm;
using namespace otawa;


p::id<bool> MARK("MARK", false);



class Display: public Application {
public:
  Display(void): Application("display", Version(1, 0, 0)) { }

protected:
  void work(const string &entry, PropList &props) override {
    
    // enable instructions analysis
    require(DECODED_TEXT);
    //require(COLLECTED_CFG_FEATURE);

    // open (overwrite) 2 files
    // part 1 : nodes
    auto p1 = io::FileOutput("p1.dot");
    // part 2 : paths
    auto p2 = io::FileOutput("p2.dot");

    // basic initialisation
    p1 << "digraph \"main\" {" << io::endl;
    p1 << "\tnode [ shape=box ];\n" << io::endl;
    p2 << io::endl;

    // get the first instruction
    auto inst = workspace()->process()->findInstAt(entry);

    // declare a todo vector
    Vector<Inst *> todo;
    // add the first int to it
    todo.add(inst);

    while (!todo.isEmpty() && i < 1000){
      // pop the next inst in todo
      auto curInst = todo.pop();

      // only work on it if not already marked
      if (!MARK(curInst)){
        // work to do varies based on inst type
        if ( curInst->isReturn() ){
          // add nothing in todo
          // add no path

          // debug
          i++;cout<<"[ret:"<<i<<"]("<<curInst->address()<<") "<<curInst<<io::endl;;
        } else if (curInst->isBranch()) {
          // add both next and branch
          // todo : if "always", then skip adding nextinst
          todo.add(curInst->target());
          todo.add(curInst->nextInst());

          // add path to target
          p2 << "\t\"" << curInst->address() << "\" -> \"" << curInst->target()->address() << "\" [ label = \"taken\" ];" << io::endl;

          //add path to next
          p2 << "\t\"" << curInst->address() << "\" -> \"" << curInst->nextInst()->address() << "\";" << io::endl;

          // debug
          i++;cout<<"[brch:"<<i<<"]("<<curInst->address()<<") "<<curInst<<io::endl;;
        } else if (curInst->isCall()) {
          // add both next and branch
          // todo : if "always", then skip adding nextinst
          todo.add(curInst->target());
          todo.add(curInst->nextInst());

          // add path to target (call)
          p2 << "\t\"" << curInst->address() << "\" -> \"" << curInst->target()->address() << "\" [ label = \"call\" ];" << io::endl;
          //simplified
          //p2 << "\t\"" << curInst->address() << "\" -> \"func\" [ label = \"call\" ];" << io::endl;

          // add path to next
          p2 << "\t\"" << curInst->address() << "\" -> \"" << curInst->nextInst()->address() << "\";" << io::endl;

          // debug
          i++;cout<<"[call:"<<i<<"]("<<curInst->address()<<") "<<curInst<<io::endl;;
        }
        
        else {
          // default, add next
          todo.add(curInst->nextInst());

          // add path to next
          p2 << "\t\"" << curInst->address() << "\" -> \"" << curInst->nextInst()->address() << "\";" << io::endl;

          // debug
          i++;cout<<"[:"<<i<<"]("<<curInst->address()<<") "<<curInst<<io::endl;;
        }

        // append node to the .dot in every case
        p1 << "\t\"" << curInst->address() << "\" [ label = \"" << curInst->address() << " " << curInst << "\" ];" << io::endl;

        // mark current as done
        MARK(curInst) = true;

      }
    }


    auto p2b = io::FileInput("p2.dot");
    for(auto line: p2b.lines()){
      p1 << line;
    }
    p1 << "}" << io::endl;
  }

private:
  int i=0;
};



//OTAWA_RUN(Display)
int main(int argc, char **argv) {
  return Display().manage(argc, argv);
}
