
#include <map>
#include <string>
#include <vector>
#include <iostream> // For Debug

#include "mrnet/MRNet.h"
#include "BasicHISAT2MRNet.h"
#include "../Utils.h"

using namespace MRN;

// Filenames for BE Executable Path ; Filter SO Path.
// We can change these out to be arguments, or to for other
// users as needed.
const char *be_exe = "~/736/CS736FinalProject/src/BackEnd";
const char *append_filter = "~/736/CS736FinalProject/src/AppendFilter.so";

int main(int argc, char **argv)
{
    int tag;
    PacketPtr p;
    
    if (argc != 3)
    {
        printf("Error !!!\n");
        exit(-1);
    }

    const char * topology_file = argv[1];
    const char * target_file = argv[2];
    
    // Back end will need a filepath for the index of the reference.
    // Such that the BE can load this upon process creation.
    const char * be_argv = NULL;

    /**
     * BELOW: Network code for setting up NW.
     */

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
    Communicator * comm_BC = network->get_BroadcastCommunicator();

    // Create a Stream that uses AppendSortedFiles filter for aggregation
    Stream *stream = network->new_Stream(comm_BC, filter_id, SFILTER_WAITFORALL);


    // TODO: Modify this code snippet.
    // Basically, set to send a packet to ONE backend with a given chunk.
    std::set<CommunicationNode *> end_points = comm_BC->get_EndPoints();

    /**
     * END SECTION
     */

    std::vector<std::string> chunk_paths = get_chunk_filenames(std::string(target_file), NUM_CHUNKS);
    for(auto i = chunk_paths.begin(); i != chunk_paths.end(); ++i)
    {
        if(!fexists( (*i).c_str() ))
        {
            std::cerr << "Error: Filename " << *i << " for chunk does not exist." << std::endl;
            return -1;
        }
        else
        {
            std::cout << "Found Chunk: " << *i << std::endl;
        }
    }

    /**
     * BELOW: Sending filenames to backends.
     */

    tag = PROT_APPEND;

    unsigned stream_id = stream->get_Id();
    std::vector<std::string>::iterator filename_iterator = chunk_paths.begin();
    std::set<CommunicationNode *>::iterator endpoint_iterator = end_points.begin();
    
    PacketPtr send_packet;
    Rank backend_rank;

    while(endpoint_iterator != end_points.end())
    {
        backend_rank = (*endpoint_iterator)->get_Rank();

        send_packet = PacketPtr(new Packet(stream_id, tag, "%s", *filename_iterator));
        send_packet->set_Destinations(&backend_rank, 1);

        if( stream->send(send_packet) == -1 )
        {
            printf("Steam send failure.\n");
            return -1;
        }
        if( stream-> flush() == -1 )
        {
            printf("Steam flush failure.\n");
            return -1;
        }
    }

    // We expect "num_iters" aggregated responses from all back-ends.
    // for(unsigned int i = 0; i < num_iters; ++i)
    // {
    //     retval = stream->recv(&tag, p);
    //     if( retval == -1)
    //     {
    //         // recv error
    //         printf("Stream recv error.\n");
    //         return -1;
    //     }       
    //     if (p->unpack("%d", &recv_val) == -1)
    //     {
    //         printf("Stream unpack failure.\n");
    //         return -1;
    //     }
    //     else
    //     {
    //         printf("Iteration %d: Success! recv_val(%d) == %d\n", i, recv_val, send_val * i * num_backends);
    //     }
    // }
    printf("Back end is ... backing out!!!\n");

    // Network Destruction will exit all processes
    delete network;

    /**
     * END SECTION
     */

    return 0;
}