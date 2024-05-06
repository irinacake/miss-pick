#ifndef OTAWA_CACHEFAULT_CACHE_SETS_SAVER_H
#define OTAWA_CACHEFAULT_CACHE_SETS_SAVER_H


#include <elm/io.h>
#include <otawa/otawa.h>
#include <otawa/hard/CacheConfiguration.h>

#include "CacheFaultDebug.h"
#include "CacheSetState.h"

using namespace elm;
using namespace otawa;

class CacheSetsSaver {

public:
    CacheSetsSaver(){
        cacheSetCount = 0;
    }

    ~CacheSetsSaver(){
        for (auto cs: savedCacheSets){
            delete(cs);
        }
    }

    CacheSetsSaver(const CacheSetsSaver& other){ // Copy Constructor
        cacheSetCount = other.cacheSetCount;
        for (auto cs: other.savedCacheSets){
            add(cs);
        }
    }

    CacheSetsSaver& operator=(const CacheSetsSaver& other){ // Copy assignment
        if (this == &other)
            return *this;
        savedCacheSets.clear();

        cacheSetCount = other.cacheSetCount;
        for (auto cs: other.savedCacheSets){
            add(cs);
        }
        return *this;
    }

    void add(CacheSetState *stateToAdd) {
        if (!(savedCacheSets.contains(stateToAdd))){
            cacheSetCount++;
            savedCacheSets.add(stateToAdd->clone());
        }
    }

    inline List<CacheSetState*> getSavedCacheSets(){ // Iteratable
        return savedCacheSets;
    }
    inline int getCacheSetCount(){
        return cacheSetCount;
    }

    bool contains(CacheSetState *stateToCheck){
        if ((savedCacheSets.contains(stateToCheck))){
            return true;
        } 
        return false;
    }



    friend elm::io::Output &operator<<(elm::io::Output &output, const CacheSetsSaver &csSaver);

private:
    int cacheSetCount;
    List<CacheSetState*> savedCacheSets;
};



#endif // OTAWA_CACHEFAULT_CACHE_SETS_SAVER_H