#ifndef OTAWA_CACHEMISS_MULTIPLE_SETS_SAVER_H
#define OTAWA_CACHEMISS_MULTIPLE_SETS_SAVER_H

#include "CacheMissDebug.h"
#include "CacheSetsSaver.h"

#include <elm/io.h>
#include <otawa/otawa.h>
#include <otawa/hard/CacheConfiguration.h>


using namespace elm;
using namespace otawa;


class MultipleSetsSaver {

public:
    MultipleSetsSaver() = default;
    void setupMSS(int cacheSetCount, int cacheWayCount);
    ~MultipleSetsSaver() = default;
    MultipleSetsSaver(const MultipleSetsSaver& other) = delete;
    MultipleSetsSaver& operator=(const MultipleSetsSaver& other) = delete;

    int* getSaversSizes();
    inline int getSetCount(){ return setCount; }
    inline int getWayCount(){ return wayCount; }

    inline bool contains(AbstractCacheSetState *stateToCheck, int set){
        ASSERTP(set >= 0 && set < setCount, "In MultipleSetsSaver.contains() : argument 'set', index out of bound.");
        if ((savedSavers[set].contains(stateToCheck))){ return true; } 
        return false;
    }
    inline bool add(AbstractCacheSetState *newState, int set) {
        ASSERTP(set >= 0 && set < setCount, "In MultipleSetsSaver.add() : argument 'set', index out of bound.");
        return savedSavers[set].add(newState);
    }
    inline CacheSetsSaver* getSaver(int set) {
        ASSERTP(set >= 0 && set < setCount, "In MultipleSetsSaver.getSaver() : argument 'set', index out of bound.");
        return &savedSavers[set];
    }

    friend elm::io::Output &operator<<(elm::io::Output &output, const MultipleSetsSaver &msSaver);

private:
    int setCount;
    int wayCount;
    AllocArray<CacheSetsSaver> savedSavers;
};



#endif // OTAWA_CACHEMISS_MULTIPLE_SETS_SAVER_H