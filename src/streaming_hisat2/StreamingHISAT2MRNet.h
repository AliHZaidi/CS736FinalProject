#if !defined(hisat2_mrnet_h )
#define hisat2_mrnet_h 1

#include "mrnet/Types.h"

typedef enum { 
    PROT_EXIT = FirstApplicationTag, // Message sent from FE to BE to indicate an exit. Sent after a request for another chunk, when there are no more chunks to give.
    PROT_SUBTREE_EXIT,               // Message sent up the data stream to indicate that the given subtree has finished and is exiting.
    PROT_ALIGN,
    PROT_CHUNK_REQUEST,              // Request from BE to FE to get one more chunk. 
    PROT_MERGE,                      // Packet sent up the hierarchy - to be merged at intermediate nodes.
    PROT_INIT_CONTROL_STREAM         // First message sent from FE to BE in control stream. Used for BE to learn control stream for sending messages.
} Protocol;
               
const unsigned INIT_CHUNKS_PER_BACKEND = 4;
const unsigned MAX_NUMBER_CHUNKS = 4096;

#endif /* hisat2_mrnet_h */
