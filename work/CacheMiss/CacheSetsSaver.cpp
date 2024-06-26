#include "CacheSetsSaver.h"


elm::io::Output &operator<<(elm::io::Output &output, const CacheSetsSaver &csSaver) {
    // Redefinition of the << operator for the MultipleSetsSaver class
    output << "[ ";
    for (auto cs: csSaver.savedCacheSets){
        cs->print(output);
        output << " ";
    }
    output << "]";
    return output;
}