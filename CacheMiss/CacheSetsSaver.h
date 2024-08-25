#ifndef OTAWA_CACHEMISS_CACHE_SETS_SAVER_H
#define OTAWA_CACHEMISS_CACHE_SETS_SAVER_H


#include <elm/io.h>
#include <otawa/otawa.h>
#include <otawa/hard/CacheConfiguration.h>
#include <elm/avl/Set.h>

#include "CacheMissDebug.h"
#include "CacheSetState.h"
#include "AbstractCacheSetState.h"

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


class CacheSetsSaver {

public:
    CacheSetsSaver(){
        cacheSetCount = 0;
    }

    ~CacheSetsSaver(){
        for (auto cs: savedCacheSets){
            delete cs;
        }
    }

    CacheSetsSaver(const CacheSetsSaver& other){ // Copy Constructor
        cacheSetCount = other.cacheSetCount;
        for (auto cs: other.savedCacheSets){
            add(cs->clone());
        }
    }

    CacheSetsSaver& operator=(const CacheSetsSaver& other){ // Copy assignment
        if (this == &other)
            return *this;
        savedCacheSets.clear();

        cacheSetCount = other.cacheSetCount;
        for (auto cs: other.savedCacheSets){
            add(cs->clone());
        }
        return *this;
    }

    bool add(AbstractCacheSetState *stateToAdd) {
        int precount = savedCacheSets.count();
        savedCacheSets.insert(stateToAdd);
        return precount != savedCacheSets.count();
        /*
        if (!(savedCacheSets.contains(stateToAdd))){
            cacheSetCount++;
            savedCacheSets.add(stateToAdd->clone());
        }
        */
    }

    inline avl::Set<AbstractCacheSetState*,AbstractCacheSetStateComparator>* getSavedCacheSets(){ // Iteratable
        return &savedCacheSets;
    }
    inline int getCacheSetCount(){
        return savedCacheSets.count();
    }

    bool contains(AbstractCacheSetState *stateToCheck){
        if ((savedCacheSets.contains(stateToCheck))){
            return true;
        } 
        return false;
    }



    friend elm::io::Output &operator<<(elm::io::Output &output, const CacheSetsSaver &csSaver);

private:
    int cacheSetCount;
    avl::Set<AbstractCacheSetState*,AbstractCacheSetStateComparator> savedCacheSets;
};



#endif // OTAWA_CACHEMISS_CACHE_SETS_SAVER_H