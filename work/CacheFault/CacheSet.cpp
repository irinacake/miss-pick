#include "CacheSet.h"


bool CacheSet::equals(const CacheSet& other){
    if (associativity == other.associativity) {
        for (int i = 0; i < associativity; i++) {
            if (savedState[i] != other.savedState[i]){
                return false;
            }
        }
        return true;
    }
    return false;
}

elm::io::Output &operator<<(elm::io::Output &output, const CacheSet &state) {
    // Redefinition of the << operator for the CacheSet class
    output << "( ";
    for (int i = 0; i < state.associativity ; i++){
        output << state.savedState[i] << " ";
    }
    output << ") ";
    return output;
}