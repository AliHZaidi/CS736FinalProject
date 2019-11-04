#include "mrnet/MRNet.h"
#include "IntegerAddition.h"

using namespace MRN;

int main(int argc, char **argv)
{
    Stream *stream = NULL;
    PacketPtr p;
    int tag = 0, recv_val = 0, num_iters = 0;
    Network * network = Network::CreateNetworkBE(argc, argv);
    
    // Main Execution loop.
    do
    {
        // Check for network recv failure.
        if(network->recv(&tag, p, &stream) != 1)
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
                break;
            default:
                printf("Unknown protocol %d\n", tag);
                break;
        }
        /* code */
    } while (tag != PROT_EXIT);
    
    network->waitfor_ShutDown();
    delete network;
    return 0;
}