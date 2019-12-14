
#include <map>
#include <string>
#include <vector>

#include <iostream> // For Debug
#include <sstream>

#include <unistd.h> // For testing sleep

#include "mrnet/MRNet.h"

#include "StreamingHISAT2MRNet.h"

#include "Utils.h"
#include "Executables.h"

using namespace MRN;

// Function lifted from the HeterogenousFilters source code
// given with MRNet
bool assign_filters( NetworkTopology* nettop, 
                     int be_filter, int cp_filter, int fe_filter, 
                     std::string& up )
{
    std::ostringstream assignment;

    // assign FE
    NetworkTopology::Node* root = nettop->get_Root();
    assignment << fe_filter << " => " << root->get_Rank() << " ; ";

    // assign BEs
    std::set< NetworkTopology::Node * > bes;
    nettop->get_BackEndNodes(bes);
    std::set< NetworkTopology::Node* >::iterator niter = bes.begin();
    for( ; niter != bes.end(); niter++ ) {
        assignment << be_filter << " => " << (*niter)->get_Rank() << " ; ";
    }

    // assign CPs (if any) using '*', which means everyone not already assigned
    assignment << cp_filter << " => * ; ";

    up = assignment.str();
    
    return true;
}

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
    int num_chunks = 0;
    ss >> num_chunks;

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

    NetworkTopology *nettop = network->get_NetworkTopology();

    // Make sure path to "so_file" is in LD_LIBRARY_PATH
    int filter_id_top = network->load_FilterFunc( STREAMING_MERGE_FILTER_SO, "MergeFilterTop" );
    if(filter_id_top == -1)
    {
        printf( "Network::load_FilterFunc() failure\n");
        // Network destruction will exit all processes.
        delete network;
        return -1;
    }

    int filter_id_normal = network->load_FilterFunc( STREAMING_MERGE_FILTER_SO, "MergeFilter" );
    if(filter_id_normal == -1)
    {
        printf( "Network::load_FilterFunc() failure\n");
        // Network destruction will exit all processes.
        delete network;
        return -1;
    }

    int filter_id_bottom = network->load_FilterFunc( STREAMING_MERGE_FILTER_SO, "SortFilter" );
    if(filter_id_normal == -1)
    {
        printf( "Network::load_FilterFunc() failure\n");
        // Network destruction will exit all processes.
        delete network;
        return -1;
    }

    // Assign string filters to up/sync/down for filter assignment later on.

    std::string down = ""; //TFILTER_NULL

    // use default (SFILTER_DONTWAIT) filter for upstream synchronization
    char assign[16];
    sprintf(assign, "%d => *;", SFILTER_DONTWAIT);
    std::string sync = assign;
    
    std::string up; // Assign filters as per normal to BE/Mid, Top gets special fitler.
    assign_filters(nettop, filter_id_bottom, filter_id_normal, filter_id_top, up);


    // A Broadcast communicator contains all the back-ends
    Communicator * comm_BC = network->get_BroadcastCommunicator();

    // Create a Stream that uses Integer_Add filter for aggregation
    Stream *data_stream    = network->new_Stream(comm_BC, up, sync, down);
    Stream *control_stream = network->new_Stream(comm_BC, TFILTER_NULL, SFILTER_DONTWAIT, TFILTER_NULL);
    Stream *stream;

    std::set<CommunicationNode *> end_points = comm_BC->get_EndPoints();

    // Init the Control Stream.
    if(control_stream->send(PROT_INIT_CONTROL_STREAM, "") == -1)
    {
        printf("Steam send failure.\n");
        return -1;        
    }
    if(control_stream-> flush() == -1 )
    {
        printf("Steam flush failure.\n");
        return -1;
    }

    /**
     * END SECTION
     */

    std::vector<std::string> chunk_paths        = get_chunk_filenames(std::string(input_file), num_chunks);
    std::vector<std::string> output_chunk_paths = get_chunk_filenames(std::string(output_file), num_chunks);
    
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

    // Iterate over endpoints and chunk filenames.
    // Distribute chunks to each backend

    int chunk_counter = 0;
    std::vector<std::string>::iterator filename_in_iterator = chunk_paths.begin();
    std::vector<std::string>::iterator filename_out_iterator = output_chunk_paths.begin();
    std::set<CommunicationNode *>::iterator endpoint_iterator = end_points.begin();
    
    PacketPtr send_packet;
    Rank backend_rank;
    Rank srcRank;

    tag = PROT_ALIGN;

    while(endpoint_iterator != end_points.end())
    {
        backend_rank = (*endpoint_iterator)->get_Rank();

        // Distribute K Chunks for each backend to start.

        for(uint i = 0; i < INIT_CHUNKS_PER_BACKEND; ++i){
            if(filename_in_iterator == chunk_paths.end())
            {
                break;
            }

            std::cout << "Sending the filename: " << *filename_in_iterator << std::endl;

            send_packet = PacketPtr(new Packet(data_stream->get_Id(), tag, "%s %s",
                (*filename_in_iterator).c_str(),
                (*filename_out_iterator).c_str()));
            
            send_packet->set_Destinations(&backend_rank, 1);

            if( data_stream->send(send_packet) == -1 )
            {
                printf("Steam send failure.\n");
                return -1;
            }
            if( data_stream-> flush() == -1 )
            {
                printf("Steam flush failure.\n");
                return -1;
            }

            ++filename_in_iterator;
            ++filename_out_iterator;

            chunk_counter++;
        }
        ++endpoint_iterator;
    }

    // BELOW: Receive loop for frontend.
    // Continously grab packets and send down responses.
    char *filename_ptr;
    while(true)
    {
        retval = network->recv(&tag, p, &stream, true);
        if( retval == -1)
        {
            // recv error
            printf("Stream recv error.\n");
            return -1;
        }
        srcRank = p->get_SourceRank();     

        switch(tag){
            // Case - a BE has completed a chunk and now requests a new one.
            // We have 2 cases here - 
            // 1. If there are more chunks to send down, then send down another one.
            // 2. Else, send down a PROT_EXIT message.
            case PROT_CHUNK_REQUEST:
                if(chunk_counter < num_chunks)
                {
                    std::cout << "FE Sending down a packet." << std::endl;
                    send_packet = PacketPtr(new Packet(data_stream->get_Id(), PROT_ALIGN, "%s %s",
                    (*filename_in_iterator).c_str(),
                    (*filename_out_iterator).c_str()));
                    send_packet->set_Destinations(&srcRank, 1);
                
                    data_stream->send(send_packet);
                    data_stream->flush();

                    chunk_counter++;
                    filename_in_iterator++;
                    filename_out_iterator++;

                    std::cout << "Current number of chunks to go: " << chunk_counter << std::endl;
                }
                else
                {
                    send_packet = PacketPtr(new Packet(control_stream->get_Id(), PROT_EXIT, ""));
                    send_packet->set_Destinations(&srcRank, 1);
                    
                    control_stream->send(send_packet);
                    control_stream->flush();
                }
                break;
            case PROT_MERGE:
                p->unpack("%s", &filename_ptr);
                // printf("Get final array of length: %d\n", final_arr_len);
                break;
            case PROT_SUBTREE_EXIT:
                printf("Got exit message. Breaking.\n");
                break;
        }

        if(tag == PROT_SUBTREE_EXIT)
        {
            break;
        }
    }

    
    // Network Destruction will exit all processes
    delete network;

    /**
     * END SECTION
     */

    return 0;
}
