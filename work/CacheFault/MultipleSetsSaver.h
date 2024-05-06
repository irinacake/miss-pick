#ifndef OTAWA_CACHEFAULT_MULTIPLESETSSAVER_H
#define OTAWA_CACHEFAULT_MULTIPLESETSSAVER_H

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



    friend elm::io::Output &operator<<(elm::io::Output &output, const MultipleSetsSaver &msSaver);

private:
    int setCount;
    int wayCount;
    AllocArray<CacheSetsSaver> savedSavers;
};



#endif // OTAWA_CACHEFAULT_MULTIPLESETSSAVER_H