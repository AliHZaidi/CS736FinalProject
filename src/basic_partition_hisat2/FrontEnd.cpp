#include "mrnet/MRNet.h"
#include "IntegerAddition.h"

#include <map>

using namespace MRN;

int main(int argc, char **argv)
{
    int tag;
    PacketPtr p;
    
    if (argc != 4)
    {
        printf("Error !!!\n");
        exit(-1);
    }

    const char * topology_file = argv[1];
    const char * be_exe = argv[2];
    const char * so_file = argv[3];
    
    // Back end will need a filepath for the index of the reference.
    // Such that the BE can load this upon process creation.
    const char * be_argv = NULL;

    // Instantiate the MRNet internal nodes, using the organization
    // in the topology file, and the specified back-end application.
    printf("Creating Network.\n");
    // Network * network = Network::CreateNetworkFE(topology_file, be_exe, &fe_argv, &attrs);
    Network * network = Network::CreateNetworkFE(topology_file, be_exe, &be_argv);
    printf("Created Network.\n");

    // Make sure path to "so_file" is in LD_LIBRARY_PATH
    int filter_id = network->load_FilterFunc( so_file, "AppendSortedFiles" );
    if(filter_id == -1)
    {
        printf( "Network::load_FilterFunc() failure\n");
        // Network destruction will exit all processes.
        delete network;
        return -1;
    }

    // Get a vector of communicators to communicate with all backends
    // Communicator * comm_BC = network->get_BroadcastCommunicator();

    // Create a Stream that uses AppendSortedFiles filter for aggregation
    Stream *stream = network->new_Stream(comm_BC, filter_id, SFILTER_WAITFORALL);

    // Broadcast a control message to back-ends to send us "num_iters"
    // waves of integers
    tag = PROT_SUM;
    unsigned int num_iters = 5;

    if( stream->send( tag, "%s %d", send_val, num_iters) == -1 )
    {
        printf("Steam send failure.\n");
        return -1;
    }
    if( stream-> flush() == -1 )
    {
        printf("Steam flush failure.\n");
        return -1;
    }

    // We expect "num_iters" aggregated responses from all back-ends.
    for(unsigned int i = 0; i < num_iters; ++i)
    {
        retval = stream->recv(&tag, p);
        if( retval == -1)
        {
            // recv error
            printf("Stream recv error.\n");
            return -1;
        }       
        if (p->unpack("%d", &recv_val) == -1)
        {
            printf("Stream unpack failure.\n");
            return -1;
        }
        else
        {
            printf("Iteration %d: Success! recv_val(%d) == %d\n", i, recv_val, send_val * i * num_backends);
        }
    }
    printf("Back end is ... backing out!!!\n");

    // Network Destruction will exit all processes
    delete network;
    return 0;
}