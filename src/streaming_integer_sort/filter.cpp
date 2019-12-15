#include <vector>
#include <set>
#include <iostream>

#include "mrnet/Packet.h"
#include "mrnet/NetworkTopology.h"

#include "IntegerSort.h"

using namespace MRN;

extern "C"
{
    // Current filter state.
    struct IM_filter_state
    {
        int num_children;        // Number of child nodes to this filter.
        int num_children_exited; // Number of child nodes that have reported that they are finished.

        int **buffer;            // Current buffer of integer arrays (to be merged).
        int *array_lens;         // Int lens of all packets in the buffer.
        int curr_buffer_size;    // Number of valid arrays that occupy the buffer.
    };

    // Merge the packets held in the state buffer and return a packet containing this information.
    // Also, flush out the state buffer information.
    PacketPtr merge_buffer(struct IM_filter_state *state, uint stream_id)
    {
        int arr_len;
        int *buffer;
        
        int new_arr_size = 0;
        std::cout << "Merging buffer with " << state->curr_buffer_size << " packets." << std::endl;
        for(int i = 0; i < state->curr_buffer_size; ++i)
        {
            arr_len = state->array_lens[i];
            new_arr_size += arr_len;
        }

        std::cout << "New arr size: " << new_arr_size <<std::endl;
        int *concat_arr = new int[new_arr_size];

        // Copy over values to new buffer
        int *curr_buffer;
        int curr_ind = 0;
        for(int arr_ind = 0; arr_ind < state->curr_buffer_size; ++arr_ind)
        {
            curr_buffer = state->buffer[arr_ind];
            arr_len = state->array_lens[arr_ind];

            for(int integer_ind = 0; integer_ind < arr_len; ++integer_ind)
            {
                concat_arr[curr_ind] = integer_ind;
                curr_ind++;
            }
        }

        // Reset state information.
        for(int i = 0; i < state->curr_buffer_size; ++i)
        {
            free(state->buffer[i]);
            state->buffer[i] = NULL;
            state->array_lens[i] = 0;
        }
        state->curr_buffer_size = 0;

        PacketPtr packet( new Packet(
            stream_id, PROT_MERGE, "%ad",
            concat_arr, new_arr_size
        ));

        return packet;
    }

    const char * IntegerMergeTop_format_string = ""; // No format; can recieve multiple types.
    void IntegerMergeTop(
        std::vector<PacketPtr> &packets_in,
        std::vector<PacketPtr> &packets_out,
        std::vector<PacketPtr> /* packets_out_reverse */,
        void ** filter_state,
        PacketPtr& /* params */,
        const TopologyLocalInfo* topologyInfo
    )
    {
        // std::cout << "Calling filter!" << std::endl;

        struct IM_filter_state *state = (struct IM_filter_state*) *filter_state;
        if(*filter_state == NULL)
        {
            std::cout << "Making a new filter state" << std::endl;
            state = (struct IM_filter_state *) malloc(sizeof(IM_filter_state));
            state->buffer = new int*[MAX_NUMBER_CHUNKS];
            state->array_lens = new int[MAX_NUMBER_CHUNKS];
            
            state->curr_buffer_size = 0;
            
            state->num_children = topologyInfo->get_NumChildren();
            if(state->num_children == 0)
            {
                state->num_children = 1;
            }
            
            state->num_children_exited = 0; 

            *filter_state = (void *) state;
            // std::cout << "Filter Num Children: " << state->num_children << std::endl;
            // std::cout << "Filter Num Children Exited: " << state->num_children_exited << std::endl;
        
            // std::cout << "Filter Topology Information: " << std::endl;
            // std::cout << "Rank: " <<  topologyInfo->get_Rank() << std::endl;
            // std::cout << "Dist to Root: " <<  topologyInfo->get_RootDistance() << std::endl;
            // std::cout << "Dist to leaves: " << topologyInfo->get_MaxLeafDistance() << std::endl;

            // std::cout << "Num Descendants: " << topologyInfo->get_NumDescendants() << std::endl;
            // std::cout << "Num Leaf Descendants" << topologyInfo->get_NumLeafDescendants() << std::endl;

        }

        // std:: cout << "Filter finished creating state." << std::endl;

        // Iterate over all awaiting packets.
        int tag;

        int *new_array;
        int array_length;
        uint stream_id;

        PacketPtr cur_packet;
        for(unsigned int i = 0; i < packets_in.size(); i++)
        {
            cur_packet = packets_in[i];
            stream_id = cur_packet->get_StreamId();

            tag = cur_packet->get_Tag();
            switch(tag)
            {
                // Case - integers to merge. Add them to the stored state buffer.
                case PROT_MERGE:
                    cur_packet->unpack("%ad", &new_array, &array_length);
                    std::cout << "Filter - unpacked array of len : " << array_length << std::endl;
                    
                    state->buffer[state->curr_buffer_size] = new_array;
                    state->array_lens[state->curr_buffer_size] = array_length;
                    state->curr_buffer_size++;
                    break;
                // Case - a child has reported that it has exited.
                case PROT_SUBTREE_EXIT:
                    state->num_children_exited++;
                    std::cout << "Exit packet observed at filter." << std::endl;
                    break;
                default:
                    std::cerr << "Packet tag unrecognized at filter" << std::endl;
                    return;
            }
        }

        // Perform new operations based on filter state:
        // If # Children exited == # Children,
        //     a. Flush the buffer of any remaining packets & send.
        //     b. Send a 'PROT SUBTREE EXIT' packet up the chain.
        // DO NOT send packets in any other condition.

        // Check for exit conditions.
        PacketPtr p;
        if(state->num_children_exited == state->num_children)
        {
            std::cout << "Filter Pushing back new packets." << std::endl;
            // a. Check if we need to flush buffer.
            if(state->curr_buffer_size > 0)
            {
                p = merge_buffer(state, stream_id);
                packets_out.push_back(p);
            }

            // b. Send an exit packet.
            PacketPtr exit_p (new Packet(stream_id, PROT_SUBTREE_EXIT, ""));
            packets_out.push_back(exit_p);
        }
        else
        {
            std::cout << "Filter ending without sending packets." << std::endl;
        }
        
    }

    // Must declare the format of data expected by the filter
    const char * IntegerMerge_format_string = ""; // No format; can recieve multiple types.
    void IntegerMerge(
        std::vector<PacketPtr> &packets_in,
        std::vector<PacketPtr> &packets_out,
        std::vector<PacketPtr> /* packets_out_reverse */,
        void ** filter_state,
        PacketPtr& /* params */,
        const TopologyLocalInfo* topologyInfo
    )
    {
        struct IM_filter_state *state = (struct IM_filter_state*) *filter_state;
        if(*filter_state == NULL)
        {
            std::cout << "Making a new filter state" << std::endl;
            state = (struct IM_filter_state *) malloc(sizeof(IM_filter_state));
            state->buffer = new int*[MAX_NUMBER_CHUNKS];
            state->array_lens = new int[MAX_NUMBER_CHUNKS];
            
            state->curr_buffer_size = 0;
            
            state->num_children = topologyInfo->get_NumChildren();
            if(state->num_children == 0)
            {
                state->num_children = 1;
            }
            
            state->num_children_exited = 0; 

            *filter_state = (void *) state;
            // std::cout << "Filter Num Children: " << state->num_children << std::endl;
            // std::cout << "Filter Num Children Exited: " << state->num_children_exited << std::endl;
        
            // std::cout << "Filter Topology Information: " << std::endl;
            // std::cout << "Rank: " <<  topologyInfo->get_Rank() << std::endl;
            // std::cout << "Dist to Root: " <<  topologyInfo->get_RootDistance() << std::endl;
            // std::cout << "Dist to leaves: " << topologyInfo->get_MaxLeafDistance() << std::endl;

            // std::cout << "Num Descendants: " << topologyInfo->get_NumDescendants() << std::endl;
            // std::cout << "Num Leaf Descendants" << topologyInfo->get_NumLeafDescendants() << std::endl;

        }

        // std:: cout << "Filter finished creating state." << std::endl;

        // Iterate over all awaiting packets.
        int tag;

        int *new_array;
        int array_length;
        uint stream_id;

        PacketPtr cur_packet;
        for(unsigned int i = 0; i < packets_in.size(); i++)
        {
            cur_packet = packets_in[i];
            stream_id = cur_packet->get_StreamId();

            tag = cur_packet->get_Tag();
            switch(tag)
            {
                // Case - integers to merge. Add them to the stored state buffer.
                case PROT_MERGE:
                    cur_packet->unpack("%ad", &new_array, &array_length);
                    std::cout << "Filter - unpacked array of len : " << array_length << std::endl;
                    
                    state->buffer[state->curr_buffer_size] = new_array;
                    state->array_lens[state->curr_buffer_size] = array_length;
                    state->curr_buffer_size++;
                    break;
                // Case - a child has reported that it has exited.
                case PROT_SUBTREE_EXIT:
                    state->num_children_exited++;
                    std::cout << "Exit packet observed at filter." << std::endl;
                    break;
                default:
                    std::cerr << "Packet tag unrecognized at filter" << std::endl;
                    return;
            }
        }

        // Perform new operations based on filter state:
        // 1. If there are >2 elements in the buffer, always merge the whole buffer & send as a packet.
        // 2. If # Children exited == # Children,
        //     2a. Flush the buffer of any remaining packets & send.
        //     2b. Send a 'PROT SUBTREE EXIT' packet up the chain.

        // 1. - Flush buffer if needed.
        PacketPtr p;
        if(state->curr_buffer_size > 1)
        {
            p = merge_buffer(state, stream_id);
            packets_out.push_back(p);
        }

        // 2. Check for exit conditions.
        if(state->num_children_exited == state->num_children)
        {
            std::cout << "Filter Pushing back new packets." << std::endl;
            // a. Check if we need to flush buffer.
            if(state->curr_buffer_size > 0)
            {
                p = merge_buffer(state, stream_id);
                packets_out.push_back(p);
            }

            // b. Send an exit packet.
            PacketPtr exit_p (new Packet(stream_id, PROT_SUBTREE_EXIT, ""));
            packets_out.push_back(exit_p);
        }
    }
}