#include <vector>

#include "mrnet/MRNet.h"
#include "IntegerAddition.h"

using namespace MRN;

int main(int argc, char **argv)
{
    Stream *stream = NULL;
    std::vector<PacketPtr> p;
    int tag = 0;
    int chunks_to_make = 0;
    bool block_recv; // Whether or not we block on a receive op.
    Network * network = Network::CreateNetworkBE(argc, argv);
    
    // Main Execution loop.
    do
    {
        // Check for network recv failure.
        block_recv = (chunks_to_make == 0);

        // If we block for receive - do a standard receive operation.
        // Then, flush out the rest of the buffer.

        // If we don't block for receive - skip right to flushing out the buffer.
        if(block_recv)
        {
            if(network->recv(&tag, p, &stream, true) != 1)
            {
                fprintf(stderr, "stream::recv() failure\n");
                return -1;
            }            
        }
        else
        {

        }

        if(network->recv(&tag, p, &stream, ) == -1)
        {
            fprintf(stderr, "stream::recv() failure\n");
            return -1;
        }

        // Switch on Network Tech.
        switch(tag)
        {
            case PROT_SUM:
                p->unpack("%d %d", &recv_val, &num_iters);

                // Send Num_Iters waves of integers.
                for(unsigned int i = 0; i < num_iters; i++ )
                {
                    if(stream->send(tag, "%d", recv_val * i) == -1)
                    {
                        printf("Stream send failure.\n");
                        return -1;
                    }
                    if(stream->flush() == -1)
                    {
                        printf("stream::flush() failure\n");
                    }
                }
                break;
            
            case PROT_EXIT:
                printf("Processing a PROT_EXIT ... \n");
                if( stream->send(tag, "%d", 0) == -1 ) {
                    fprintf( stderr, "BE: stream::send(%%s) failure in PROT_EXIT\n" );
                    break;
                }
                if( stream->flush( ) == -1 ) {
                    fprintf( stderr, "BE: stream::flush() failure in PROT_EXIT\n" );
                }
                break;
            default:
                printf("Unknown protocol %d\n", tag);
                break;
        }
    } while (tag != PROT_EXIT);
    
    network->waitfor_ShutDown();
    delete network;
    return 0;
}