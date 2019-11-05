#if !defined(hisat2_mrnet_h )
#define hisat2_mrnet_h 1

#include "mrnet/Types.h"

typedef enum { PROT_EXIT=FirstApplicationTag, 
               PROT_APPEND,
               PROT_MERGESORT } Protocol;

// TODO: Make this a param @ FrontEnd
const unsigned NUM_CHUNKS = 5;

#endif /* hisat2_mrnet_h */
