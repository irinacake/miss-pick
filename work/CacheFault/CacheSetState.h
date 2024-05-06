#ifndef OTAWA_CACHEFAULT_CACHESET_H
#define OTAWA_CACHEFAULT_CACHESET_H


#include <elm/io.h>
#include <otawa/otawa.h>

using namespace elm;
using namespace otawa;

class CacheSetState {

public:
    CacheSetState(){ // Constructor
        ASSERTP(isInit, "initAssociativity has not been called yet");
        savedState = new int[associativity];
        for (int i=0; i<associativity; i++){
            savedState[i] = 0;
        }
    }

    virtual ~CacheSetState(){ // Destructor
        delete[]savedState;
    }

    CacheSetState(const CacheSetState& other){ // Copy Constructor
        ASSERTP(isInit, "initAssociativity has not been called yet");
        savedState = new int[associativity];
        for (int i=0; i<associativity; i++){
            savedState[i] = other.savedState[i];
        }
    }


    CacheSetState& operator=(const CacheSetState& other){ // Copy assignment
        if (this == &other)
            return *this;
        delete[]savedState;
        savedState = new int[associativity];
        for (int i=0; i<associativity; i++){
            savedState[i] = other.savedState[i];
        }
        return *this;
    }


    inline void setStateValue(int position, int value) {
        ASSERTP(position >= 0 && position < associativity, "In CacheSetState.setStateValue() : argument 'position', index out of bound.");
        savedState[position] = value;
    }

    inline int getStateValue(int position){
        ASSERTP(position >= 0 && position < associativity, "In CacheSetState.getStateValue() : argument 'position', index out of bound.");
        return savedState[position];
    }

    inline int* getState(){
        return savedState;
    }


    /**
     * @fn initAssociativity
     * this method must be called first
    */
    static inline void initAssociativity(int associativityValue){
        if (!isInit){
            associativity = associativityValue;
            isInit = true;
        }
    }



    bool equals(CacheSetState& other);

    friend elm::io::Output &operator<<(elm::io::Output &output, const CacheSetState &state);

private:
    static int associativity;
    static bool isInit;
    int* savedState;
};


namespace elm {
  template<>
  class Equiv<CacheSetState *> {
  public:
    static inline bool isEqual(CacheSetState *state1, CacheSetState *state2) {
      return state1->equals(*state2);
    }
  };
}






class CacheSetStateLRU: public CacheSetState {
public:
    CacheSetStateLRU(): CacheSetState() {}

    ~CacheSetStateLRU() {}

    CacheSetStateLRU(const CacheSetStateLRU& other): CacheSetState(other) {}
    
    CacheSetStateLRU& operator=(const CacheSetStateLRU& other){ // Copy assignment
        if (this == &other){ return *this; }
        CacheSetState::operator=(other);
        return *this;
    }
};


class CacheSetStateFIFO: public CacheSetState {
public:
    int index;

    CacheSetStateFIFO(): CacheSetState(), index(0) {}

    ~CacheSetStateFIFO() {}

    CacheSetStateFIFO(const CacheSetStateFIFO& other): CacheSetState(other), index(other.index) {}

    CacheSetStateFIFO& operator=(const CacheSetStateFIFO& other){ // Copy assignment
        if (this == &other){ return *this; }
        CacheSetState::operator=(other);
        index = other.index;
        return *this;
    }
};


class CacheSetStatePLRU: public CacheSetState {
public:
    t::uint64 accessBits;

    CacheSetStatePLRU(): CacheSetState(), accessBits(0) {}

    ~CacheSetStatePLRU() {}

    CacheSetStatePLRU(const CacheSetStatePLRU& other): CacheSetState(other), accessBits(other.accessBits) {}

    CacheSetStatePLRU& operator=(const CacheSetStatePLRU& other){ // Copy assignment
        if (this == &other){ return *this; }
        CacheSetState::operator=(other);
        accessBits = other.accessBits;
        return *this;
    }
};




















#endif // OTAWA_CACHEFAULT_CACHESET_H



