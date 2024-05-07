#ifndef OTAWA_CACHEFAULT_MULTIPLE_SETS_SAVER_H
#define OTAWA_CACHEFAULT_MULTIPLE_SETS_SAVER_H


#include "CacheFaultDebug.h"
#include "CacheSetsSaver.h"



#include <elm/io.h>
#include <otawa/otawa.h>
#include <otawa/hard/CacheConfiguration.h>


using namespace elm;
using namespace otawa;

class MultipleSetsSaver {

public:
    MultipleSetsSaver() = default;

    void setupMWS(int cacheSetCount, int cacheWayCount){
        setCount = cacheSetCount;
        wayCount = cacheWayCount;
        savedSavers.allocate(setCount);
    }

    ~MultipleSetsSaver() = default;

    MultipleSetsSaver(const MultipleSetsSaver& other) = delete;

    MultipleSetsSaver& operator=(const MultipleSetsSaver& other) = delete;



    int* getSaversSizes(){
        int *listSizes = new int[setCount];
        for (int i = 0; i < setCount; i++){
            listSizes[i] = savedSavers[i].getCacheSetCount();
        }
        return listSizes;
    }


    inline int getSetCount(){
        return setCount;
    }
    inline int getWayCount(){
        return wayCount;
    }


    bool contains(CacheSetState *stateToCheck, int set){
        if ((savedSavers[set].contains(stateToCheck))){
            return true;
        } 
        return false;
    }

    void add(CacheSetState *newState, int set) {
        ASSERTP(set >= 0 && set < setCount, "In CacheSetState.add() : argument 'set', index out of bound.");
        if (!(savedSavers[set].contains(newState))){
            DEBUG("Adding new state" << endl);
            savedSavers[set].add(newState);
        } else {
            DEBUG("not Adding new state" << endl);
        }
    }

    CacheSetsSaver* getSaver(int set) {
        ASSERTP(set >= 0 && set < setCount, "In CacheSetState.add() : argument 'set', index out of bound.");
        return &savedSavers[set];
    }


    friend elm::io::Output &operator<<(elm::io::Output &output, const MultipleSetsSaver &msSaver);

private:
    int setCount;
    int wayCount;
    AllocArray<CacheSetsSaver> savedSavers;
};



#endif // OTAWA_CACHEFAULT_MULTIPLE_SETS_SAVER_H