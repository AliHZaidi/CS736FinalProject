/****************************************************************************
 * Copyright � 2003-2015 Dorian C. Arnold, Philip C. Roth, Barton P. Miller *
 *                  Detailed MRNet usage rights in "LICENSE" file.          *
 ****************************************************************************/

#if !defined(integer_sort_h )
#define integer_sort_h 1

#include "mrnet/Types.h"

typedef enum { 
    PROT_EXIT = FirstApplicationTag, // Message sent from FE to BE to indicate an exit. Sent after a request for another chunk, when there are no more chunks to give.
    PROT_FIRST_CHUNK_GEN,            // First control message sent form FE to BE. Generate N Chunks
    PROT_CHUNK_GEN,                  // Control message sent from FE to BE to generate one more chunk.
    PROT_CHUNK_REQUEST,              // Request from BE to FE to get one more chunk. 
    PROT_MERGE                       // Packet sent up the hierarchy - to be 
} Protocol;

#endif /* integer_addition_h */
