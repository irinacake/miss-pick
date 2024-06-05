#include "MultipleSetsSaver.h"


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