#ifndef OTAWA_CACHEMISS_ABS_CACHE_SET_H
#define OTAWA_CACHEMISS_ABS_CACHE_SET_H

#include <elm/io.h>
#include <elm/data/ListMap.h>
#include <otawa/otawa.h>
#include <otawa/hard/CacheConfiguration.h>
#include <elm/data/ListSet.h>
#include <otawa/cfg/Loop.h>

#include "CacheMissDebug.h"
#include "CacheSetState.h"
//#include "CacheMissFeature.h"

#include "CFGSetProjector.h"




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
#ifdef newKickers
    virtual int update(int toAddTag, BBP* b) = 0;
    virtual void reduce(otawa::Loop* l) = 0;
#endif
    virtual AbstractCacheSetState* clone() = 0;
    virtual int compare(const AbstractCacheSetState& other) const = 0;
    virtual int sameCs(const AbstractCacheSetState& other) const = 0;
    virtual void print(elm::io::Output &output) = 0;
    friend elm::io::Output &operator<<(elm::io::Output &output, AbstractCacheSetState &state);

};

class AbstractCacheSetStateComparator {
    public:
    int doCompare(const AbstractCacheSetState *object1, const AbstractCacheSetState *object2) const {
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
#ifdef newKickers
    int update(int toAddTag, BBP* b) override { return cs->update(toAddTag); }
    void reduce(otawa::Loop* l) override {}
#endif
    AbstractCacheSetState* clone() override { return new ConcreteCacheSetState(*this); }
    int compare(const AbstractCacheSetState& other) const override {
        auto& castedOther = static_cast<const ConcreteCacheSetState&>(other);
        return cs->compare(*castedOther.cs);
    }
    int sameCs(const AbstractCacheSetState& other) const override {
        auto& castedOther = static_cast<const ConcreteCacheSetState&>(other);
        return cs->compare(*castedOther.cs);
    }
    void print(elm::io::Output &output) override { output << *cs; }

private:
    CacheSetState* cs;
};



#ifdef newKickers

enum LoopOrBlock {LOOP, BLOCK};

struct LoopBlock {
    union lbUnion
    {
        otawa::Loop* loop; 
        Block* block; 
    };
    LoopBlock(LoopOrBlock t, Block* v) {
        type = t;
        lob.block = v;
    }
    LoopBlock(LoopOrBlock t, otawa::Loop* v) {
        type = t;
        lob.loop = v;
    }
    LoopBlock* clone(){
        if (type == LoopOrBlock::BLOCK) {
            return new LoopBlock(type, lob.block);
        } else {
            return new LoopBlock(type, lob.loop);
        }
    }
    LoopOrBlock type;
    lbUnion lob;
};

class LoopBlockComparator {
    public:
    int doCompare(const LoopBlock *object1, const LoopBlock *object2) const {
        //cout << "compare called" << endl;
        if (object1->type == object2->type) {
            if (object1->type == LoopOrBlock::BLOCK){
                return object1->lob.block - object2->lob.block;
            } else {
                return object1->lob.loop->header() - object2->lob.loop->header();
            }
        } else {
            return object1->type - object2->type;
        }
    }
};


// extern p::id<ListSet<LoopBlock*,LoopBlockComparator>> KICKERS;


class CompoundCacheSetState: public AbstractCacheSetState {
public:
    CompoundCacheSetState(otawa::hard::Cache::replace_policy_t policy);
    ~CompoundCacheSetState() { delete cs; }
    CompoundCacheSetState(const CompoundCacheSetState& other) {
        cs = other.cs->clone();
        W = new ListMap<int,LoopBlock*>();
        auto w = other.W->pairs().begin();
        for (; w != other.W->pairs().end(); ++w) {
            W->put((*w).fst,(*w).snd);
        }
    }

    inline void setStateValue(int position, int value) override { cs->setStateValue(position,value); }
    inline int getStateValue(int position) override { return cs->getStateValue(position); }
    inline int* getState() override { return cs->getState(); }

    int update(int toAddTag, Block* b) override;
    int update(int toAddTag, BBP* b) override;
    void reduce(otawa::Loop* l) override {
        auto w = W->pairs().begin();
        for (; w != W->pairs().end(); ++w) {
            if ( (*w).snd->type == LoopOrBlock::LOOP ){
                if ((*w).snd->lob.loop->parent() == l){
                    auto tag = (*w).fst;
                    W->put(tag, new LoopBlock(LoopOrBlock::LOOP, l));
                }
            } else {
                if (Loop::of((*w).snd->lob.block) == l){
                    auto tag = (*w).fst;
                    W->put(tag,new LoopBlock(LoopOrBlock::LOOP, l));
                }
            }
        }
    }

    AbstractCacheSetState* clone() override { return new CompoundCacheSetState(*this); }
    
    int compare(const AbstractCacheSetState& other) const override;
    
    int sameCs(const AbstractCacheSetState& other) const override {
        auto& castedOther = static_cast<const CompoundCacheSetState&>(other);
        return cs->compare(*castedOther.cs);
    }

    void print(elm::io::Output &output) override;
    
    inline ListMap<int,LoopBlock*>* getW(){ return W; }
private:
    CacheSetState* cs;
    ListMap<int,LoopBlock*>* W;
};


#else

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

#endif














#endif // OTAWA_CACHEMISS_ABS_CACHE_SET_H



