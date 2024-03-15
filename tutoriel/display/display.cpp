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

    // open (overwrite) main output file
    auto p1 = io::FileOutput("p1.dot");

    // get the first instruction
    auto inst = workspace()->process()->findInstAt(entry);

    // declare todo vectors
    // for each functions called
    Vector<Inst *> fnctodo;
    // for each inst within a function
    Vector<Inst *> todo;
    // add the first int to it
    fnctodo.add(inst);

    while (!fnctodo.isEmpty()){
      // transfer from fnctodo to todo
      todo.add(fnctodo.pop());
      
      // open (overwrite) temporary output file for paths
      auto p2 = io::FileOutput("p2.dot");

      // basic var to give a different name
      i2++;

      // init the function in .dot
      p1 << "digraph \"func" << i2 << "\" {" << io::endl;
      p1 << "\tnode [ shape=box ];\n" << io::endl;
      p2 << io::endl;

      //debug
      i = 0;
      
      while (!todo.isEmpty()){
        // pop the next inst in todo
        auto curInst = todo.pop();

        // only work on the inst if not already marked
        if (!MARK(curInst)){
          // work to do varies based on inst type
          if ( curInst->isReturn() ){
            // add nothing to todo
            // add no path

            // debug
            cout<<"[ret:"<<i<<"]("<<curInst->address()<<") "<<curInst<<io::endl;i++;
          } else if (curInst->isBranch()) {
            // add branch target to todo
            todo.add(curInst->target());

            // add path to target
            p2 << "\t\"" << curInst->address() << "\" -> \"" << curInst->target()->address() << "\" [ label = \"taken\" ];" << io::endl;
            
            // if and only if the branch is conditionnal
            if (curInst->isCond()) {
              // add nextinst to todo
              todo.add(curInst->nextInst());

              // add path to next
              p2 << "\t\"" << curInst->address() << "\" -> \"" << curInst->nextInst()->address() << "\";" << io::endl;
            }
            // else, the nextInst will never be taken

            // debug
            cout<<"[brch:"<<i<<"]("<<curInst->address()<<") "<<curInst<<io::endl;i++;
          } else if (curInst->isCall()) {
            // add both next and call to todo
            fnctodo.add(curInst->target());
            todo.add(curInst->nextInst());

            // add path to target (call)
            //p2 << "\t\"" << curInst->address() << "\" -> \"" << curInst->target()->address() << "\" [ label = \"call\" ];" << io::endl;
            //simplified
            p2 << "\t\"" << curInst->address() << "\" -> \"func\" [ label = \"call\" ];" << io::endl;

            // add path to next
            p2 << "\t\"" << curInst->address() << "\" -> \"" << curInst->nextInst()->address() << "\";" << io::endl;

            // debug
            cout<<"[call:"<<i<<"]("<<curInst->address()<<") "<<curInst<<io::endl;i++;
          }
          
          else {
            // default, add next to todo
            todo.add(curInst->nextInst());

            // add path to next
            p2 << "\t\"" << curInst->address() << "\" -> \"" << curInst->nextInst()->address() << "\";" << io::endl;

            // debug
            cout<<"[:"<<i<<"]("<<curInst->address()<<") "<<curInst<<io::endl;i++;
          }

          // append node to the .dot in every case
          p1 << "\t\"" << curInst->address() << "\" [ label = \"" << curInst->address() << " " << curInst << "\" ];" << io::endl;

          // mark current as done
          MARK(curInst) = true;
        }
      }

      // read the temporary file as input
      auto p2b = io::FileInput("p2.dot");
      // append its lines to the main file
      for(auto line: p2b.lines()){
        p1 << line;
      }
      p1 << "}\n" << io::endl;
      // this simply allows to split the nodes and the paths
    }

  }

private:
  int i=0;
  int i2=0;
};



//OTAWA_RUN(Display)
int main(int argc, char **argv) {
  return Display().manage(argc, argv);
}
