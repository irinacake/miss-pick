#include "AbstractCacheSetState.h"


// #ifdef newKickers
// p::id<ListSet<LoopBlock*,LoopBlockComparator>> KICKERS("KICKERS");
// #endif


elm::io::Output &operator<<(elm::io::Output &output, AbstractCacheSetState &state) {
    // Redefinition of the << operator for the AbstractCacheSetState class
    state.print(output);
    return output;
}


ConcreteCacheSetState::ConcreteCacheSetState(otawa::hard::Cache::replace_policy_t policy) {
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





CompoundCacheSetState::CompoundCacheSetState(otawa::hard::Cache::replace_policy_t policy) {
#ifdef newKickers
    W = new ListMap<int,LoopBlock*>();
#endif
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



#ifdef newKickers
int CompoundCacheSetState::update(int toAddTag, Block* b) {
    int k = cs->update(toAddTag);
    return k;
}


int CompoundCacheSetState::update(int toAddTag, BBP* b) {

    int k = cs->update(toAddTag);

    if (k != -1){
        // someone was kicked, add it to W
        W->put(k, new LoopBlock(LoopOrBlock::BLOCK,b->oldBB()));// when optimising with merge,
        // there needs to be a list initialisation here 
    }
    if (W->hasKey(toAddTag)){
        // if the key is in the W, then it has been kicked
        //(*KICKERS(b)).insert(W->get(toAddTag));
        // since update() will load it back in, remove it from the W
        W->remove(toAddTag);
    }
    return k;
}

int CompoundCacheSetState::compare(const AbstractCacheSetState& other) const {
    auto& castedOther = static_cast<const CompoundCacheSetState&>(other);
    int cscmp = cs->compare(*castedOther.cs);
    if (cscmp == 0) {
        auto a = W->pairs().begin();
        auto b = castedOther.W->pairs().begin();
        for (; a != W->pairs().end() && b != castedOther.W->pairs().end(); ++a,++b) {
            if ((*a).fst != (*b).fst){
                return (*a).fst - (*b).fst;
            }
            if ((*a).snd->type == (*b).snd->type){
                if ((*a).snd->type == LoopOrBlock::BLOCK){
                    if ((*a).snd->lob.block->index() != (*b).snd->lob.block->index()){
                        return (*a).snd->lob.block->index() - (*b).snd->lob.block->index();
                    }
                } else {
                    if ((*a).snd->lob.loop != (*b).snd->lob.loop){
                        return (*a).snd->lob.loop->address() - (*b).snd->lob.loop->address();
                    }
                }
            } else {
                return (*a).snd->type - (*b).snd->type;
            }
        }
        if (a == W->pairs().end() && b == castedOther.W->pairs().end()){
            return 0;
        } else {
            if (a != W->pairs().end()){
                return -1;
            } else {
                return 1;
            }
        }
    }
    return cscmp;
}

void CompoundCacheSetState::print(elm::io::Output &output) {
    output << *cs;
    output << "<";
    for (auto a : W->pairs()){
        output << "(" << a.fst << "," << a.snd << ")";
    }
    output << ">";
}

#else

int CompoundCacheSetState::update(int toAddTag, Block* b) {
    if (W.hasKey(toAddTag)){
        // if the key is in the W, then it has been kicked. But since update() will load it back in, remove it from the W
        W.remove(toAddTag);
    }
    int k = cs->update(toAddTag);
    if (k != -1){
        // someone was kicked, add it to W
        W.put(k,b);// when optimising with merge,
        // there needs to be a list initialisation here 
    }
    return k;
}


int CompoundCacheSetState::compare(const AbstractCacheSetState& other) const {
    auto& castedOther = static_cast<const CompoundCacheSetState&>(other);
    int cscmp = cs->compare(*castedOther.cs);
    if (cscmp == 0) {
        auto a = W.pairs().begin();
        auto b = castedOther.W.pairs().begin();
        for (; a != W.pairs().end() && b != castedOther.W.pairs().end(); ++a,++b) {
            if ((*a).fst != (*b).fst){
                return (*a).fst - (*b).fst;
            }
            if ((*a).snd->index() != (*b).snd->index()){
                return (*a).snd->index() - (*b).snd->index();
            }
        }
        if (a == W.pairs().end() && b == castedOther.W.pairs().end()){
            return 0;
        } else {
            if (a != W.pairs().end()){
                return -1;
            } else {
                return 1;
            }
        }
    }
    return cscmp;
}

void CompoundCacheSetState::print(elm::io::Output &output) {
    output << *cs;
    output << "<";
    for (auto a : W.pairs()){
        output << "(" << a.fst << "," << a.snd << ")";
    }
    output << ">";
}


#endif

