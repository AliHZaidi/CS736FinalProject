#include "mrnet/MRNet.h"
#include "IntegerSort.h"

#include <map>

#include <iostream>
#include <sstream>
#include <string>

using namespace MRN;

const uint TOTAL_NUM_CHUNKS = 6;
const uint INIT_CHUNKS_PER_BE = 4;

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

    NetworkTopology *nettop = network->get_NetworkTopology();

    // Make sure path to "so_file" is in LD_LIBRARY_PATH
    int filter_id_top = network->load_FilterFunc( so_file, "IntegerMergeTop" );
    if(filter_id_top == -1)
    {
        printf( "Network::load_FilterFunc() failure\n");
        // Network destruction will exit all processes.
        delete network;
        return -1;
    }

    int filter_id_normal = network->load_FilterFunc( so_file, "IntegerMerge" );
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
    assign_filters(nettop, filter_id_normal, filter_id_normal, filter_id_top, up);



    // A Broadcast communicator contains all the back-ends
    Communicator * comm_BC = network->get_BroadcastCommunicator();

    // Create a Stream that uses Integer_Add filter for aggregation
    Stream *data_stream    = network->new_Stream(comm_BC, up, sync, down);
    Stream *control_stream = network->new_Stream(comm_BC, TFILTER_NULL, SFILTER_DONTWAIT, TFILTER_NULL);
    Stream *stream;

    int num_backends = comm_BC->get_EndPoints().size();

    // First message on control stream.
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

    // Send first chunks.
    if(data_stream->send(PROT_FIRST_CHUNK_GEN, "%d", INIT_CHUNKS_PER_BE) == -1 )
    {
        printf("Steam send failure.\n");
        return -1;
    }
    if(data_stream-> flush() == -1 )
    {
        printf("Steam flush failure.\n");
        return -1;
    }

    chunk_counter += num_backends * INIT_CHUNKS_PER_BE;

    // We expect "num_iters" aggregated responses from all back-ends.
    PacketPtr send_packet;
    Rank srcRank;

    int *final_arr;
    int final_arr_len;

    bool continue_loop = true;
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
                if(chunk_counter < TOTAL_NUM_CHUNKS)
                {
                    std::cout << "FE Sending down a packet." << std::endl;
                    send_packet = PacketPtr(new Packet(data_stream->get_Id(), PROT_CHUNK_GEN, ""));
                    send_packet->set_Destinations(&srcRank, 1);
                
                    data_stream->send(send_packet);
                    data_stream->flush();

                    chunk_counter++;
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
                p->unpack("%ad", &final_arr, &final_arr_len);
                printf("Get final array of length: %d\n", final_arr_len);
                break;
            case PROT_SUBTREE_EXIT:
                continue_loop = false;
                printf("Got exit message. Breaking.\n");
                break;
        }

        if(tag == PROT_SUBTREE_EXIT)
        {
            break;
        }
    }
    printf("Front end is ... backing out!!!\n");

    // Network Destruction will exit all processes
    delete network;

    sleep(5);
    return 0;
}