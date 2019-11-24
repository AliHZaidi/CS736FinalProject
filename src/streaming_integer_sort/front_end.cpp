#include "mrnet/MRNet.h"
#include "IntegerSort.h"

#include <map>

using namespace MRN;

const uint TOTAL_NUM_CHUNKS = 16;
const uint INIT_CHUNKS_PER_BE = 4;
const uint INTS_PER_CHUNK = 100;

int main(int argc, char **argv)
{
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

    uint chunk_counter = 0;

    // Instantiate the MRNet internal nodes, using the organization
    // in the topology file, and the specified back-end application.
    printf("Creating Network.\n");
    Network * network = Network::CreateNetworkFE(topology_file, be_exe, &fe_argv);
    printf("Created Network.\n");

    // Make sure path to "so_file" is in LD_LIBRARY_PATH
    int filter_id = network->load_FilterFunc( so_file, "IntegerMerge" );
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
    Stream *data_stream = network->new_Stream(comm_BC, filter_id, SFILTER_DONTWAIT);
    Stream *control_stream = network->new_Stream(comm_BC, TFILTER_NULL, SFILTER_DONTWAIT, TFILTER_NULL);

    int num_backends = comm_BC->get_EndPoints().size();

    if(control_stream->send(PROT_FIRST_GEN, "%d", send_val) == -1 )
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
    while(true)
    {
        retval = stream->recv(&tag, p);
        if( retval == -1)
        {
            // recv error
            printf("Stream recv error.\n");
            return -1;
        }     

        switch(tag){
            case 
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
    control_stream->send(PROT_EXIT);
    printf("Back end is ... backing out!!!\n");

    // Network Destruction will exit all processes
    delete network;
    return 0;
}