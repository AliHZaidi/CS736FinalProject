/****************************************************************************
 * Copyright � 2003-2015 Dorian C. Arnold, Philip C. Roth, Barton P. Miller *
 *                  Detailed MRNet usage rights in "LICENSE" file.          *
 ****************************************************************************/

#if !defined(integer_sort_h )
#define integer_sort_h 1

#include "mrnet/Types.h"

typedef enum { 
    PROT_EXIT = FirstApplicationTag, // Message sent from FE to BE to indicate an exit. Sent after a request for another chunk, when there are no more chunks to give.
    PROT_SUBTREE_EXIT,               // Message sent up the data stream to indicate that the given subtree has finished and is exiting.
    PROT_FIRST_CHUNK_GEN,            // First control message sent form FE to BE. Generate N Chunks
    PROT_CHUNK_GEN,                  // Control message sent from FE to BE to generate one more chunk.
    PROT_CHUNK_REQUEST,              // Request from BE to FE to get one more chunk. 
    PROT_MERGE,                      // Packet sent up the hierarchy - to be merged at intermediate nodes.
    PROT_INIT_CONTROL_STREAM         // First message sent from FE to BE in control stream. Used for BE to learn control stream for sending messages.
} Protocol;

const uint INTS_PER_CHUNK = 100;
const uint MAX_NUMBER_CHUNKS = 100000;

#endif /* integer_addition_h */
