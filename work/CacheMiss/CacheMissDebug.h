
#ifndef OTAWA_CACHEMISS_CACHE_MISS_DEBUG_H
#define OTAWA_CACHEMISS_CACHE_MISS_DEBUG_H

#ifndef NDEBUG
#define DEBUG(x) //cout << x
#define SPEDEBUG(x) //x
#else
#define DEBUG(x)
#define SPEDEBUG
#endif


#endif // OTAWA_CACHEMISS_CACHE_MISS_DEBUG_H