#include "CacheSetState.h"


bool CacheSetState::isInit = false;
int CacheSetState::associativity = 0;
int CacheSetState::logAssociativity = 0;

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



void CacheSetStateLRU::update(int toAddTag){

    // position variable
    int pos = 0;

    // search loop : break before incrementing if the same tag is found
    while (pos < associativity - 1){ 
        if (toAddTag == savedState[pos]) break;
        pos++;
    }

    // reverse loop : overwrite the current tag (pos) with the 
    // immediately younger tag (pos-1)
    while (pos > 0){
        //state[toAddSet]->setValue(pos,state[toAddSet]->getValue(pos-1));
        savedState[pos] = savedState[pos-1];
        pos--;
    }

    // set age 0 with the new tag
    //state[toAddSet]->setValue(pos,toAddTag);
    savedState[pos] = toAddTag;
}

CacheSetState* CacheSetStateLRU::clone(){
    return new CacheSetStateLRU(*this);
}





void CacheSetStateFIFO::update(int toAddTag){

    // position variable
    int pos = 0;
    bool found = false;

    // search loop : break before incrementing if the same tag is found
    while (pos < associativity && !found){
        if (toAddTag == savedState[pos]) {
            found = true;
            break;
        }
        pos++;
    }

    if (!found) {
        savedState[index] = toAddTag;
        index = (index + 1) % associativity;
    }
}

CacheSetState* CacheSetStateFIFO::clone(){
    return new CacheSetStateFIFO(*this);
}



void CacheSetStatePLRU::update(int toAddTag){

    // position variable
    int pos = 0;
    bool found = false;

    // search loop : break before incrementing if the same tag is found
    while (pos < associativity && !found){ 
        if (toAddTag == savedState[pos]) {
            found = true;
            break;
        }
        pos++;
    }

    if (found){
    int i = 0;
    int look = 0;
    int bit = 0;
    while (i < logAssociativity) {
        bit = 1 << (logAssociativity-i-1);
        if ( (pos) & ( 1 << (logAssociativity-i-1)) ) { // clear it
            accessBits = accessBits & ~(1 << look);
            look += associativity / (1 << (i+1)) ;
        } else { // set it
            accessBits = accessBits | (1 << look);
            look += 1;
        }
        i++;
    }
    } else {
    int look = 0;
    int access = 0;

    int i = 0;
    while(i < logAssociativity){
        access <<= 1;
        if (accessBits & (1 << look)) {
        access++;
        }

        accessBits = accessBits ^ (1 << look);
        if (accessBits & (1 << look)){
        look += 1;
        } else {
        look += associativity / (1 << (i+1)) ;
        }
        i++;
    }

    // set new tag at access
    savedState[access] = toAddTag;
    }
}

CacheSetState* CacheSetStatePLRU::clone(){
    return new CacheSetStatePLRU(*this);
}

