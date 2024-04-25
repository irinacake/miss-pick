#ifndef OTAWA_CACHEFAULT_CACHESETSSAVER_H
#define OTAWA_CACHEFAULT_CACHESETSSAVER_H


#include <elm/io.h>
#include <otawa/otawa.h>
#include <otawa/hard/CacheConfiguration.h>

#include "CacheSet.h"

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
            add(new CacheSet(*cs));
        }
    }

    CacheSetsSaver& operator=(const CacheSetsSaver& other){ // Copy assignment
        if (this != &other)
            return *this;
        for (auto cs: savedCacheSets){
            delete(cs);
        }
        cacheSetCount = other.cacheSetCount;
        for (auto cs: other.savedCacheSets){
            add(new CacheSet(*cs));
        }
        return *this;
    }

    void add(CacheSet *newState) {
        if (!(savedCacheSets.contains(newState))){
            cacheSetCount++;
            savedCacheSets.add(newState);
        }
    }

    inline List<CacheSet *> getSavedCacheSets(){ // Iteratable
        return savedCacheSets;
    }
    inline int getCacheSetCount(){
        return cacheSetCount;
    }



    friend elm::io::Output &operator<<(elm::io::Output &output, const CacheSetsSaver &csSaver);

private:
    int cacheSetCount;
    List<CacheSet *> savedCacheSets;
};



#endif // OTAWA_CACHEFAULT_CACHESETSSAVER_H