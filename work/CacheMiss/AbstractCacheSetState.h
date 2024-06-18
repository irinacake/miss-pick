#ifndef OTAWA_CACHEMISS_ABS_CACHE_SET_H
#define OTAWA_CACHEMISS_ABS_CACHE_SET_H

#include <elm/io.h>
#include <otawa/otawa.h>

#include "CacheMissDebug.h"
#include "CacheSetState.h"
#include "CFGSetProjector.h"


using namespace elm;
using namespace otawa;

class AbstractCacheSetState {

public:
    //AbstractCacheSetState(otawa::hard::Cache::replace_policy_t policy);

    virtual ~AbstractCacheSetState() = 0; // Destructor

    //AbstractCacheSetState(const AbstractCacheSetState& other);// Copy Constructor


    //AbstractCacheSetState& operator=(const AbstractCacheSetState& other);// Copy assignment


    virtual inline void setStateValue(int position, int value) = 0;
    virtual inline int getStateValue(int position) = 0;
    virtual inline int* getState() = 0;
    static inline void initAssociativity(int associativityBits){
        CacheSetState::initAssociativity(associativityBits);
    }

    virtual int update(int toAddTag, BBP* bbp) = 0;
    virtual AbstractCacheSetState* clone() = 0;
    virtual int compare(const AbstractCacheSetState& other) const = 0;

private:
protected:
};



class AbstractCacheSetStateComparator {
    public:
    int doCompare(const AbstractCacheSetState *object1, const AbstractCacheSetState *object2) {
        return object1->compare(*object2);
    }
    static int compare(const AbstractCacheSetState *object1, const AbstractCacheSetState *object2) {
        return object1->compare(*object2);
    }
};





class ConcreteCacheSetState: public AbstractCacheSetState {
public:
    ConcreteCacheSetState(otawa::hard::Cache::replace_policy_t policy) {
        switch (policy)
        {
        case otawa::hard::Cache::LRU:
            cs = new CacheSetStateLRU();
            break;
        case otawa::hard::Cache::FIFO:
            cs = new CacheSetStateFIFO();
            break;
        case otawa::hard::Cache::PLRU:
            cs = new CacheSetStatePLRU();
            break;
        default:
            ASSERTP(false, "Unsupported policy");
            break;
        }
    }

    ~ConcreteCacheSetState() {
        delete cs;
    }

    ConcreteCacheSetState(const ConcreteCacheSetState& other) {
        cs = other.cs->clone();
    }
    
    /*
    ConcreteCacheSetState& operator=(const ConcreteCacheSetState& other){ // Copy assignment
        if (this == &other){ return *this; }
        // TODO ??
        //AbstractCacheSetState::operator=(other);
        return *this;
    }
    */

    inline void setStateValue(int position, int value) override {
        cs->setStateValue(position,value);
    }
    inline int getStateValue(int position) override {
        return cs->getStateValue(position);
    }
    inline int* getState() override {
        return cs->getState();
    }


    int update(int toAddTag, BBP* bbp) override {
        return cs->update(toAddTag);
    }

    AbstractCacheSetState* clone() override {
        return new ConcreteCacheSetState(*this);
    }

    int compare(const AbstractCacheSetState& other) const override {
        auto castedOther = static_cast<const ConcreteCacheSetState&>(other);
        return cs->compare(*castedOther.cs);
    }

private:
    CacheSetState* cs;
};




class CompoundCacheSetState: public AbstractCacheSetState {
public:
    CompoundCacheSetState(otawa::hard::Cache::replace_policy_t policy) {
        switch (policy)
        {
        case otawa::hard::Cache::LRU:
            cs = new CacheSetStateLRU();
            break;
        case otawa::hard::Cache::FIFO:
            cs = new CacheSetStateFIFO();
            break;
        case otawa::hard::Cache::PLRU:
            cs = new CacheSetStatePLRU();
            break;
        default:
            ASSERTP(false, "Unsupported policy");
            break;
        }
    }

    ~CompoundCacheSetState() {
        delete cs;
        //TODO : delete map
    }

    CompoundCacheSetState(const CompoundCacheSetState& other) {
        cs = other.cs->clone();
        //TODO : clone map
    }
    
    /*
    CompoundCacheSetState& operator=(const CompoundCacheSetState& other){ // Copy assignment
        if (this == &other){ return *this; }
        AbstractCacheSetState::operator=(other);
        return *this;
    }
    */

    inline void setStateValue(int position, int value) override {
        cs->setStateValue(position,value);
    }
    inline int getStateValue(int position) override {
        return cs->getStateValue(position);
    }
    inline int* getState() override {
        return cs->getState();
    }

    int update(int toAddTag, BBP* bbp) override {
        //TODO implement the new update functions
        return cs->update(toAddTag);
    }

    AbstractCacheSetState* clone() override {
        return new CompoundCacheSetState(*this);
    }

    int compare(const AbstractCacheSetState& other) const override {
        auto castedOther = static_cast<const CompoundCacheSetState&>(other);
        //TODO : take MAP into account for the compare
        return cs->compare(*castedOther.cs);
    }

private:
    CacheSetState* cs;
    ListMap<int,BBP*> W;
};

















#endif // OTAWA_CACHEMISS_ABS_CACHE_SET_H



