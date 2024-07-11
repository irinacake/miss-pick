#ifndef OTAWA_CACHEMISS_CACHE_SET_H
#define OTAWA_CACHEMISS_CACHE_SET_H

#include <elm/io.h>
#include <otawa/otawa.h>

#include "CacheMissDebug.h"


#define newPLRU

using namespace elm;
using namespace otawa;


/**
 * @class CacheSetState
 * 
 * composition of AbstractCacheSetState
 * 
 * @ingroup cachemiss
 */

/**
 * @fn
 * 
 * @param
 * @return
 * @warning
 */


class CacheSetState {

public:
    CacheSetState(){ // Constructor
        ASSERTP(isInit, "initAssociativity has not been called yet");
        savedState = new int[associativity];
        for (int i=0; i<associativity; i++){
            savedState[i] = -1;
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
     * 
     * @param associativityBits is the number of bits used
     * for the associativity and not the associativity itself
    */
    static inline void initAssociativity(int associativityBits){
        if (!isInit){
            logAssociativity = associativityBits;
            associativity = (1 << associativityBits);
            isInit = true;
        }
    }

    virtual int update(int toAddTag) = 0;

    virtual CacheSetState* clone() = 0;

    virtual int compare(const CacheSetState& other) const = 0;


    friend elm::io::Output &operator<<(elm::io::Output &output, const CacheSetState &state);

    inline static bool isInitialised(){
        return isInit;
    }
private:
    static bool isInit;
    
protected:
    int* savedState;
    static int associativity;
    static int logAssociativity;
};



class CacheSetStateComparator {
    public:
    int doCompare(const CacheSetState *object1, const CacheSetState *object2) {
        return object1->compare(*object2);
    }
    static int compare(const CacheSetState *object1, const CacheSetState *object2) {
        return object1->compare(*object2);
    }
};





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

    int update(int toAddTag) override;

    CacheSetState* clone() override;

    int compare(const CacheSetState& other) const override;
};






class CacheSetStateFIFO: public CacheSetState {
public:

    CacheSetStateFIFO(): CacheSetState(), index(0) {}

    ~CacheSetStateFIFO() {}

    CacheSetStateFIFO(const CacheSetStateFIFO& other): CacheSetState(other), index(other.index) {}

    CacheSetStateFIFO& operator=(const CacheSetStateFIFO& other){ // Copy assignment
        if (this == &other){ return *this; }
        CacheSetState::operator=(other);
        index = other.index;
        return *this;
    }

    int update(int toAddTag) override;

    CacheSetState* clone() override;

    int compare(const CacheSetState& other) const override;
private:
    int index;
};








class CacheSetStatePLRU: public CacheSetState {
public:

#ifdef newPLRU
    CacheSetStatePLRU(): CacheSetState() {}
#else
    CacheSetStatePLRU(): CacheSetState(), accessBits(0) {}
#endif

    ~CacheSetStatePLRU() {}

#ifdef newPLRU
    CacheSetStatePLRU(const CacheSetStatePLRU& other): CacheSetState(other) {}
#else
    CacheSetStatePLRU(const CacheSetStatePLRU& other): CacheSetState(other)
    ,accessBits(other.accessBits) {}
#endif

    CacheSetStatePLRU& operator=(const CacheSetStatePLRU& other){ // Copy assignment
        if (this == &other){ return *this; }
        CacheSetState::operator=(other);
#ifndef newPLRU
        accessBits = other.accessBits;
#endif
        return *this;
    }

    static void initTables(){
        swapTables = new int[associativity*associativity];
        ASSERTP(associativity == 4 || associativity == 8,"InitTables: associativity must be 4 or 8");
        if (associativity == 4) {
            swapTables = swapTables4;
        } else {
            swapTables = swapTables8;
        }
    }

    int update(int toAddTag) override;

    CacheSetState* clone() override;

    int compare(const CacheSetState& other) const override;
private:
#ifndef newPLRU
    t::uint64 accessBits;
#endif
    static int* swapTables;
    static int swapTables4[16];
    static int swapTables8[64];
};



















#endif // OTAWA_CACHEMISS_CACHE_SET_H



