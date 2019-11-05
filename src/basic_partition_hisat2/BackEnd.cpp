#include <string>

#include "mrnet/MRNet.h"
#include "BasicHISAT2MRNet.h"

using namespace MRN;

int main(int argc, char **argv)
{
    Stream *stream = NULL;
    PacketPtr p;
    int tag = 0;
    Network * network = Network::CreateNetworkBE(argc, argv);
    
    std::string filename_in;
    std::string filename_out;

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
            case PROT_APPEND:
                p->unpack("%s", &filename_in);


                if(stream->send(tag, "%s", filename_in) == -1)
                {
                    printf("Stream send failure.\n");
                    return -1;
                }
                if(stream->flush() == -1)
                {
                    printf("stream::flush() failure\n");
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