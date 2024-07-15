#include "AbstractCacheSetState.h"


#ifdef newKickers
p::id<ListSet<LoopBlock*,LoopBlockComparator>> KICKERS("KICKERS");
#endif


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
    // //if (k == -1) {
    //     //if (!W->hasKey(toAddTag)){
    //     //} // else cache hit
    // //}
    // if (k != -1){
    //     // someone was kicked, add it to W
    //     W->put(k,new LoopBlock(LoopOrBlock::BLOCK,b));// when optimising with merge,
    //     // there needs to be a list initialisation here 
    //     if (W->hasKey(toAddTag)){
    //         // if the key is in the W, then it was kicked (by the instruction few lines above in a previous call), but is now loaded back in
    //         W->remove(toAddTag);
    //     }
    // }
    return k;
}

int CompoundCacheSetState::update(int toAddTag, BBP* b) {
    int k = cs->update(toAddTag);
    //if (k == -1) {
        //if (!W->hasKey(toAddTag)){
        //} // else cache hit
    //}
    if (k != -1){
        // someone was kicked, add it to W
        W->put(k, new LoopBlock(LoopOrBlock::BLOCK,b->oldBB()));// when optimising with merge,
        // there needs to be a list initialisation here 
        cout << "Kicked someone, adding to W" << endl;
        if (W->hasKey(toAddTag)){
            // if the key is in the W, then it was kicked (by the instruction few lines above in a previous call), but is now loaded back in
            cout << "I was kicked" << endl;
            (*KICKERS(b)).insert(W->get(toAddTag));
            W->remove(toAddTag);
        }
    }
    return k;
}

int CompoundCacheSetState::compare(const AbstractCacheSetState& other) const {
    auto& castedOther = static_cast<const CompoundCacheSetState&>(other);
    // cout << "ccss1 : " << *this->cs << endl;
    // cout << "ccss2 : " << *castedOther.cs << endl;
    // cout << "ccss1 count : " << this->W->count() << endl;
    // cout << "ccss2 count : " << castedOther.W->count() << endl;
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
                        //cout << (*b).snd->lob.loop << endl;
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
    int k = cs->update(toAddTag);
    //if (k == -1) {
        //if (!W.hasKey(toAddTag)){
        //} // else cache hit
    //}
    if (k != -1){
        // someone was kicked, add it to W
        W.put(k,b);// when optimising with merge,
        // there needs to be a list initialisation here 
        cout << "Kicked someone, adding to W" << endl;
        if (W.hasKey(toAddTag)){
            cout << "I was kicked" << endl;
            // if the key is in the W, then it was kicked (by the instruction few lines above in a previous call), but is now loaded back in
            W.remove(toAddTag);
        }
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

