#include "CacheSetState.h"


bool CacheSetState::isInit = false;
int CacheSetState::associativity = 0;

bool CacheSetState::equals(CacheSetState& other){
    for (int i = 0; i < associativity; i++) {
        if (savedState[i] != other.savedState[i]){
            return false;
        }
    }
    return true;
}

elm::io::Output &operator<<(elm::io::Output &output, const CacheSetState &state) {
    // Redefinition of the << operator for the CacheSetState class
    output << "( ";
    for (int i = 0; i < state.associativity ; i++){
        output << state.savedState[i] << " ";
    }
    output << ") ";
    return output;
}