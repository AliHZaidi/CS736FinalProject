
#include <map>
#include <string>
#include <vector>

#include <iostream> // For Debug
#include <sstream>

#include <unistd.h> // For testing sleep

#include "mrnet/MRNet.h"

#include "StreamingHISAT2MRNet.h"

#include "../Utils.h"
#include "../Executables.h"

using namespace MRN;

int main(int argc, char **argv)
{
    int tag, retval;
    PacketPtr p;
    
    if (argc != 5)
    {
        printf("Error !!!\n");
        exit(-1);
    }

    const char *topology_file = argv[1];
    const char *input_file = argv[2];
    const char *output_file = argv[3];
    const char *num_chunks_str = argv[4];

    std::stringstream ss(num_chunks_str);
    int num_chunks << ss;

    // Back end will need a filepath for the index of the reference.
    // Such that the BE can load this upon process creation.
    const char * be_argv = NULL;

    /**
     * BELOW: Network code for setting up NW.
     */

    // Instantiate the MRNet internal nodes, using the organization
    // in the topology file, and the specified back-end application.
    printf("Creating Network.\n");
    Network * network = Network::CreateNetworkFE(topology_file, STREAMING_BACKEND_EXEC, &be_argv);
    printf("Created Network.\n");

    // Make sure path to "so_file" is in LD_LIBRARY_PATH
    int filter_id = network->load_FilterFunc(STREAMING_APPEND_FILTER_EXEC, "AppendFilter" );
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
    Stream *stream = network->new_Stream(comm_BC, filter_id, SFILTER_DONTWAIT);


    // TODO: Modify this code snippet.
    // Basically, set to send a packet to ONE backend with a given chunk.
    std::set<CommunicationNode *> end_points = comm_BC->get_EndPoints();

    /**
     * END SECTION
     */

    std::vector<std::string> chunk_paths = get_chunk_filenames(std::string(input_file), NUM_CHUNKS);
    std::vector<std::string> output_chunk_paths = get_chunk_filenames(std::string(output_file), NUM_CHUNKS);
    
    // Some sanity checking.
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

    for(auto i = output_chunk_paths.begin(); i != output_chunk_paths.end(); ++i)
    {
        std::cout << "Will create out-chunk of " << *i << std::endl;
    }

    /**
     * BELOW: Sending filenames to backends.
     */

    tag = PROT_ALIGN;

    unsigned stream_id = stream->get_Id();

    // Iterate over endpoints and chunk filenames.
    // Distribute one chunk to each backend
    // TODO: Make sure these have the same length. 
    std::vector<std::string>::iterator filename_in_iterator = chunk_paths.begin();
    std::vector<std::string>::iterator filename_out_iterator = output_chunk_paths.begin();
    std::set<CommunicationNode *>::iterator endpoint_iterator = end_points.begin();
    
    PacketPtr send_packet;
    Rank backend_rank;

    while(endpoint_iterator != end_points.end())
    {
        backend_rank = (*endpoint_iterator)->get_Rank();

        std::cout << "Sending the filename: " << *filename_in_iterator << std::endl;

        send_packet = PacketPtr(new Packet(stream_id, tag, "%s %s",
            (*filename_in_iterator).c_str(),
            (*filename_out_iterator).c_str()));
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

        ++filename_in_iterator;
        ++filename_out_iterator;
        ++endpoint_iterator;
    }
    char *char_ptr;
    tag = PROT_APPEND;

    retval = stream->recv(&tag, p);
    if(retval == -1)
    {
        std::cout << "Uh oh detected on FE" << std::endl;
    }
    std::cout << "Received a packet at FE" << std::endl;
    if(p->unpack("%s", &char_ptr) == -1)
    {
        std::cout << "Error in unpacking packet." << std::endl;
    }

    std::cout << "Payload: " << std::string(char_ptr) << std::endl;
    printf("Front end is ... backing out!!!\n");

    // TODO: Code here for cleanup.

    
    // Network Destruction will exit all processes
    delete network;

    /**
     * END SECTION
     */

    return 0;
}
