#ifndef OTAWA_CACHEMISS_ABS_CACHE_SET_H
#define OTAWA_CACHEMISS_ABS_CACHE_SET_H

#include <elm/io.h>
#include <elm/data/ListMap.h>
#include <otawa/otawa.h>
#include <otawa/hard/CacheConfiguration.h>

#include "CacheMissDebug.h"
#include "CacheSetState.h"


using namespace elm;
using namespace otawa;


/**
 * @class
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

class AbstractCacheSetState {

public:
    virtual ~AbstractCacheSetState() {} // Destructor

    virtual inline void setStateValue(int position, int value) = 0;
    virtual inline int getStateValue(int position) = 0;
    virtual inline int* getState() = 0;
    static inline void initAssociativity(int associativityBits){
        CacheSetState::initAssociativity(associativityBits);}

    virtual int update(int toAddTag, Block* b) = 0;
    virtual AbstractCacheSetState* clone() = 0;
    virtual int compare(const AbstractCacheSetState& other) const = 0;
    virtual void print(elm::io::Output &output) = 0;
    friend elm::io::Output &operator<<(elm::io::Output &output, AbstractCacheSetState &state);

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
    ConcreteCacheSetState(otawa::hard::Cache::replace_policy_t policy);
    ~ConcreteCacheSetState() { delete cs; }
    ConcreteCacheSetState(const ConcreteCacheSetState& other) { cs = other.cs->clone(); }

    inline void setStateValue(int position, int value) override { cs->setStateValue(position,value); }
    inline int getStateValue(int position) override { return cs->getStateValue(position); }
    inline int* getState() override { return cs->getState(); }


    int update(int toAddTag, Block* b) override { return cs->update(toAddTag); }
    AbstractCacheSetState* clone() override { return new ConcreteCacheSetState(*this); }
    int compare(const AbstractCacheSetState& other) const override {
        auto castedOther = static_cast<const ConcreteCacheSetState&>(other);
        return cs->compare(*castedOther.cs);
    }
    void print(elm::io::Output &output) override { output << *cs; }

private:
    CacheSetState* cs;
};




class CompoundCacheSetState: public AbstractCacheSetState {
public:
    CompoundCacheSetState(otawa::hard::Cache::replace_policy_t policy);
    ~CompoundCacheSetState() { delete cs; }
    CompoundCacheSetState(const CompoundCacheSetState& other) {
        cs = other.cs->clone();
        W = ListMap<int,Block*>(other.W);
    }

    inline void setStateValue(int position, int value) override { cs->setStateValue(position,value); }
    inline int getStateValue(int position) override { return cs->getStateValue(position); }
    inline int* getState() override { return cs->getState(); }

    int update(int toAddTag, Block* b) override;

    AbstractCacheSetState* clone() override { return new CompoundCacheSetState(*this); }
    
    int compare(const AbstractCacheSetState& other) const override;
    
    void print(elm::io::Output &output) override;
    
    inline ListMap<int,Block*>* getW(){ return &W; }
private:
    CacheSetState* cs;
    ListMap<int,Block*> W;
};

















#endif // OTAWA_CACHEMISS_ABS_CACHE_SET_H



