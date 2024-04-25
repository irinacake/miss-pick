#ifndef OTAWA_CACHEFAULT_CACHESET_H
#define OTAWA_CACHEFAULT_CACHESET_H


#include <elm/io.h>
#include <otawa/otawa.h>

using namespace elm;
using namespace otawa;

class CacheSet {

public:
    CacheSet(int stateAssociativity){ // Constructor
        associativity = stateAssociativity;
        savedState = new int[associativity];
        for (int i=0; i<associativity; i++){
            savedState[i] = 0;
        }
    }

    ~CacheSet(){ // Destructor
        delete[]savedState;
    }

    CacheSet(const CacheSet& other){ // Copy Constructor
        associativity = other.associativity;
        savedState = new int[associativity];
        for (int i=0; i<associativity; i++){
            savedState[i] = other.savedState[i];
        }
    }


    CacheSet& operator=(const CacheSet& other){ // Copy assignment
        if (this != &other)
            return *this;
        delete[]savedState;
        associativity = other.associativity;
        savedState = new int[associativity];
        for (int i=0; i<associativity; i++){
            savedState[i] = other.savedState[i];
        }
        return *this;
    }


    inline void setStateValue(int position, int value) {
        ASSERTP(position >= 0 && position < associativity, "In CacheSet.setStateValue() : argument 'position', index out of bound.");
        savedState[position] = value;
    }

    inline int getStateValue(int position){
        ASSERTP(position >= 0 && position < associativity, "In CacheSet.getStateValue() : argument 'position', index out of bound.");
        return savedState[position];
    }

    inline int* getState(){
        return savedState;
    }



    bool equals(const CacheSet& other);

    friend elm::io::Output &operator<<(elm::io::Output &output, const CacheSet &state);

private:
    int associativity;
    int* savedState;
};


namespace elm {
  template<>
  class Equiv<CacheSet *> {
  public:
    static inline bool isEqual(CacheSet *state1, CacheSet *state2) {
      return state1->equals(*state2);
    }
  };
}


#endif // OTAWA_CACHEFAULT_CACHESET_H
