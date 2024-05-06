
#ifndef OTAWA_CACHEFAULT_CACHE_FAULT_DEBUG_H
#define OTAWA_CACHEFAULT_CACHE_FAULT_DEBUG_H

#ifndef NDEBUG
#define DEBUG(x) //cout << x
#define SPEDEBUG(x) //x
#else
#define DEBUG(x)
#define SPEDEBUG
#endif


#endif // OTAWA_CACHEFAULT_CACHE_FAULT_DEBUG_H