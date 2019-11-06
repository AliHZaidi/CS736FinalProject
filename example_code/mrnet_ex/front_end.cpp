#include "mrnet/MRNet.h"
#include "IntegerAddition.h"

#include <map>

using namespace MRN;

int main(int argc, char **argv)
{
    int send_val = 32, recv_val = 0;
    int tag, retval;
    PacketPtr p;
    if (argc != 4)
    {
        printf("Error !!!\n");
        exit(-1);
    }

    const char * topology_file = argv[1];
    const char * be_exe = argv[2];
    const char * so_file = argv[3];
    const char * fe_argv = NULL;


    // std::map< std::string, std::string> attrs;
    // attrs.insert("XPLAT_RSH", "ssh");

    // Instantiate the MRNet internal nodes, using the organization
    // in the topology file, and the specified back-end application.
    printf("Creating Network.\n");
    // Network * network = Network::CreateNetworkFE(topology_file, be_exe, &fe_argv, &attrs);
    Network * network = Network::CreateNetworkFE(topology_file, be_exe, &fe_argv);
    printf("Created Network.\n");

    // Make sure path to "so_file" is in LD_LIBRARY_PATH
    int filter_id = network->load_FilterFunc( so_file, "IntegerAdd" );
    if(filter_id == -1)
    {
        printf( "Network::load_FilterFunc() failure\n");
        // Network destruction will exit all processes.
        delete network;
        return -1;
    }

    // A Broadcast communicator contains all the back-ends
    Communicator * comm_BC = network->get_BroadcastCommunicator();

    // Create a Stream that uses Integer_Add filter for aggregation
    Stream *stream = network->new_Stream(comm_BC, filter_id, SFILTER_WAITFORALL);

    int num_backends = comm_BC->get_EndPoints().size();

    // // Broadcast a control message to back-ends to send us "num_iters"
    // waves of integers
    tag = PROT_SUM;
    unsigned int num_iters = 5;

    if( stream->send( tag, "%d %d", send_val, num_iters) == -1 )
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
    stream->send(PROT_EXIT);
    printf("Back end is ... backing out!!!\n");

    // Network Destruction will exit all processes
    delete network;
    return 0;
}