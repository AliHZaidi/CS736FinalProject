#if !defined(hisat2_mrnet_h )
#define hisat2_mrnet_h 1

#include "mrnet/Types.h"

typedef enum { PROT_EXIT=FirstApplicationTag,
               PROT_ALIGN, // Packets to be aligned - have format string "%s %s"; first is input, second is output 
               PROT_APPEND, // Return - please append these files "%s"
               PROT_MERGESORT // Return - please mergesort these files. "%s"
               } Protocol;

// TODO: Make this a param @ FrontEnd
const unsigned NUM_CHUNKS = 5;
const unsigned MAX_CHILDREN_PER_NODE = 64;

const unsigned BUFFER_FLUSH_SIZE = 0;

#endif /* hisat2_mrnet_h */