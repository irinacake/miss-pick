#include "MultipleSetsSaver.h"


/**
 * @class MultipleSetsSaver
 * Composition of the CacheSetsSaver class.
 * The constructor was made empty, and the initialisation of the
 * class moved to the setupMSS function in order to make the use
 * of otawa properties possible
 * 
 * @ingroup cachemiss
 */

/**
 * @fn setupMSS
 * Initialises the private attributes of the MSS class.
 * 
 * @param cacheSetCount, cacheWayCount int  
 */
void MultipleSetsSaver::setupMSS(int cacheSetCount, int cacheWayCount){
    setCount = cacheSetCount;
    wayCount = cacheWayCount;
    savedSavers.allocate(setCount);
}


/**
 * @fn getSaversSizes
 * 
 * @return an int* that contains the return values of the
 * getCacheSetCount method for all the CacheSetsSaver of
 * the savedSavers attribute
 */
int* MultipleSetsSaver::getSaversSizes(){
    int *listSizes = new int[setCount];
    for (int i = 0; i < setCount; i++){
        listSizes[i] = savedSavers[i].getCacheSetCount();
    }
    return listSizes;
}


/**
 * @fn contains
 * Checks whether or not the given state is contained in
 * the CacheSetsSaver of the given set.
 * 
 * @param stateToCheck AbstractCacheSetState*
 * @param set int
 * @return true if contained, false if not
 * @warning the value of set must be greater or equal than 0 and lower than setCount
 */


/**
 * @fn add
 * Adds the given state to the CacheSetsSaver of the given set.
 * @param newState AbstractCacheSetState*
 * @param set int
 * @return true if the newState was effectively added, false if not
 * @warning the value of set must be greater or equal than 0 and lower than setCount
 */


/**
 * @fn getSaver
 * 
 * @param set in
 * @return the CacheSetsSaver of the given set
 * @warning the value of set must be greater or equal than 0 and lower than setCount
 */


elm::io::Output &operator<<(elm::io::Output &output, const MultipleSetsSaver &msSaver) {
    // Redefinition of the << operator for the MultipleSetsSaver class
    output << "{\n";
    for (auto saver: msSaver.savedSavers){
        if (saver.getCacheSetCount() > 0)
            output << "\t" << saver << endl;
    }
    output << "}";
    return output;
}