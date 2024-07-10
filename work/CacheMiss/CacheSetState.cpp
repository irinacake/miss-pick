#include "CacheSetState.h"


bool CacheSetState::isInit = false;
int CacheSetState::associativity = 0;
int CacheSetState::logAssociativity = 0;
int* CacheSetStatePLRU::swapTables = nullptr;
static int swapTables4[16] = {0,1,2,3,
                            1,0,2,3,
                            2,3,0,1,
                            3,2,1,0};
static int swapTables4[64] = {0,1,2,3,4,5,6,7,
                            1,0,2,3,
                            2,3,0,1,
                            3,2,1,0,
                            0,1,2,3,
                            1,0,2,3,
                            2,3,0,1,
                            3,2,1,0};



elm::io::Output &operator<<(elm::io::Output &output, const CacheSetState &state) {
    // Redefinition of the << operator for the CacheSetState class
    output << "( ";
    for (int i = 0; i < state.associativity ; i++){
        output << state.savedState[i] << " ";
    }
    output << ") ";
    return output;
}


int CacheSetStateLRU::update(int toAddTag){

    // position variable
    int pos = 0;
    int kicked = -1;

    // search loop : break before incrementing if the same tag is found
    while (pos < associativity - 1){ 
        if (toAddTag == savedState[pos]) break;
        pos++;
    }


    if (pos == associativity - 1) {
        kicked = savedState[pos];
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

    return kicked;
}

CacheSetState* CacheSetStateLRU::clone(){
    return new CacheSetStateLRU(*this);
}

int CacheSetStateLRU::compare(const CacheSetState& other) const {
    auto& castedOther = static_cast<const CacheSetStateLRU&>(other);

    int i = 0;
    while (savedState[i] == castedOther.savedState[i] && i < associativity-1) {
        i++;
    }
    return savedState[i] - castedOther.savedState[i];
}







int CacheSetStateFIFO::update(int toAddTag){

    // position variable
    int pos = 0;
    bool found = false;
    int kicked = -1;

    // search loop : break before incrementing if the same tag is found
    while (pos < associativity && !found){
        if (toAddTag == savedState[pos]) {
            found = true;
            break;
        }
        pos++;
    }

    if (!found) {
        kicked = savedState[index];
        savedState[index] = toAddTag;
        index = (index + 1) % associativity;
    }
    return kicked;
}

CacheSetState* CacheSetStateFIFO::clone(){
    return new CacheSetStateFIFO(*this);
}

int CacheSetStateFIFO::compare(const CacheSetState& other) const {
    auto& castedOther = static_cast<const CacheSetStateFIFO&>(other);

    int i = 0;
    while (savedState[i] == castedOther.savedState[i] && i < associativity-1) {
        i++;
    }
    if (savedState[i] == castedOther.savedState[i]){
        return index - castedOther.index;
    } else {
        return savedState[i] - castedOther.savedState[i];
    }
}




int CacheSetStatePLRU::update(int toAddTag){

    // position variable
    int pos = 0;
    bool found = false;
    int kicked = -1;

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
        kicked = savedState[access];
        savedState[access] = toAddTag;
    }
    return kicked;
}

CacheSetState* CacheSetStatePLRU::clone(){
    return new CacheSetStatePLRU(*this);
}


int CacheSetStatePLRU::compare(const CacheSetState& other) const {
    auto& castedOther = static_cast<const CacheSetStatePLRU&>(other);

    int i = 0;
    while (savedState[i] == castedOther.savedState[i] && i < associativity-1) {
        i++;
    }
    if (savedState[i] == castedOther.savedState[i]){
        return accessBits - castedOther.accessBits;
    } else {
        return savedState[i] - castedOther.savedState[i];
    }
    
}



