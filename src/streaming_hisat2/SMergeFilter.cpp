#include <vector>
#include <iostream>

#include "mrnet/Packet.h"
#include "mrnet/NetworkTopology.h"

#include "StreamingHISAT2MRNet.h"

#include <unistd.h>
#include <sys/wait.h>

#include "../Executables.h"
#include "../Utils.h"

using namespace MRN;

extern "C"
{
    // Current filter state.
    struct M_filter_state
    {
        int num_children;        // Number of child nodes to this filter.
        int num_children_exited; // Number of child nodes that have reported that they are finished.

        std::vector<std::string> buffer;            // Current buffer of integer arrays (to be merged).
    };

    int merge_bam_files(std::vector<std::string> input_files, std::string out_file)
    {
        int pid;
        pid = fork();
        if(pid == -1)
        {
            std::cerr << "Error in fork() call on backend." << std::endl;
            return -1;
        }

        if(pid == 0)
        {
            // In child
            // lol
            std::cout << "Merging in child." << std::endl;
            char *argv[MAX_CHILDREN_PER_NODE];
            for(uint i = 0; i < MAX_CHILDREN_PER_NODE - 1; ++i)
            {
                argv[i] = (char *) malloc(sizeof(char) * (PATH_MAX + 1));
            }
            bcopy(SAMTOOOLS_PATH, argv[0], PATH_MAX + 1);
            bcopy("cat", argv[1], PATH_MAX + 1);
            bcopy("-o", argv[2], PATH_MAX + 1);
            bcopy(out_file.c_str(), argv[3], PATH_MAX + 1);
            
            int i = 4;
            for(const auto &input_file : input_files)
            {
               bcopy(input_file.c_str(), argv[i], PATH_MAX + 1);
               ++i;
            }
            argv[i] = NULL;

            i = 0;
            while(argv[i] != NULL)
            {
                std::cout << "Argument: " << argv[i++] << std::endl;
            }
            // std::cout << "Input file: " << input_file << std::endl;
            // std::cout << "Output file: " << output_file << std::endl;
            execvp(argv[0], argv);
        }
        else
        {
            // In Parent.
            // TODO: Possibly look into wait_status to see if HISAT2 executed properly.
            int status;
            wait(&status);
        }

        return 0;    
    }

    // Must declare the format of data expected by the filter
    const char * AppendFilter_format_string = "%s";
    void AppendFilter(
        std::vector<PacketPtr> &packets_in,
        std::vector<PacketPtr> &packets_out,
        std::vector<PacketPtr> /* packets_out_reverse */,
        void ** filter_state,
        PacketPtr& /* params */,
        const TopologyLocalInfo* topologyInfo
    )
    {
        // Taken from https://stackoverflow.com/questions/15347123/how-to-construct-a-stdstring-from-a-stdvectorstring/18703743
        std::string s;
        std::string s_piece;
	    char *unpack_ptr;

        struct M_filter_state *state = (struct M_filter_state*) *filter_state;
        if(*filter_state == NULL)
        {
            std::cout << "Making a new filter state" << std::endl;
            state = (struct M_filter_state *) malloc(sizeof(M_filter_state));            
            
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

        for(unsigned int i = 0; i < packets_in.size(); i++)
        {
            cur_packet = packets_in[i];
            stream_id = cur_packet->get_StreamId();

            tag = cur_packet->get_Tag();
            switch(tag)
            {
                // Case - integers to merge. Add them to the stored state buffer.
                case PROT_MERGE:
                    cur_packet->unpack("%s", &unpack_ptr);
                    filter_state->buffer.push_back(std::string(unpack_ptr));
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
        char string_char_ptr [PATH_MAX + 1];

        if(state->buffer.size() > 1)
        {
            s = concat_chunk_filenames(state->buffer);
            merge_bam_files(state->buffer, s);

            state->buffer.clear();

            bcopy(s.c_str(), string_char_ptr, PATH_MAX + 1);
            p = new PacketPtr(new Packet(packets_in[0]->get_StreamId(), PROT_MERGE, "%s", string_char_ptr));
            packets_out.push_back(p);
        }

        // 2. Check for exit conditions.
        if(state->num_children_exited == state->num_children)
        {
            std::cout << "Filter Pushing back new packets." << std::endl;
            // a. Check if we need to flush buffer.
            if(state->curr_buffer_size > 0)
            {
                s = concat_chunk_filenames(state->buffer);
                merge_bam_files(state->buffer, s);

                state->buffer.clear();

                bcopy(s.c_str(), string_char_ptr, PATH_MAX + 1);
                p = new PacketPtr(new Packet(packets_in[0]->get_StreamId(), PROT_MERGE, "%s", string_char_ptr));
                packets_out.push_back(p);
            }

            // b. Send an exit packet.
            PacketPtr exit_p (new Packet(packets_in[0]->get_StreamId(), PROT_SUBTREE_EXIT, ""));
            packets_out.push_back(exit_p);
        }
    }

    const char * AppendFilterTop_format_string = "%s";
    void AppendFilterTop(
        std::vector<PacketPtr> &packets_in,
        std::vector<PacketPtr> &packets_out,
        std::vector<PacketPtr> /* packets_out_reverse */,
        void ** filter_state,
        PacketPtr& /* params */,
        const TopologyLocalInfo* topologyInfo
    )
    {
        // Taken from https://stackoverflow.com/questions/15347123/how-to-construct-a-stdstring-from-a-stdvectorstring/18703743
        std::string s;
        std::string s_piece;
	    char *unpack_ptr;

        struct M_filter_state *state = (struct M_filter_state*) *filter_state;
        if(*filter_state == NULL)
        {
            std::cout << "Making a new filter state" << std::endl;
            state = (struct M_filter_state *) malloc(sizeof(M_filter_state));            
            
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

        for(unsigned int i = 0; i < packets_in.size(); i++)
        {
            cur_packet = packets_in[i];
            stream_id = cur_packet->get_StreamId();

            tag = cur_packet->get_Tag();
            switch(tag)
            {
                // Case - integers to merge. Add them to the stored state buffer.
                case PROT_MERGE:
                    cur_packet->unpack("%s", &unpack_ptr);
                    filter_state->buffer.push_back(std::string(unpack_ptr));
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

        // Check for exit conditions.
        if(state->num_children_exited == state->num_children)
        {
            std::cout << "Filter Pushing back new packets." << std::endl;
            // a. Check if we need to flush buffer.
            if(state->curr_buffer_size > 0)
            {
                s = concat_chunk_filenames(state->buffer);
                merge_bam_files(state->buffer, s);

                state->buffer.clear();

                bcopy(s.c_str(), string_char_ptr, PATH_MAX + 1);
                p = new PacketPtr(new Packet(packets_in[0]->get_StreamId(), PROT_MERGE, "%s", string_char_ptr));
                packets_out.push_back(p);
            }

            // b. Send an exit packet.
            PacketPtr exit_p (new Packet(packets_in[0]->get_StreamId(), PROT_SUBTREE_EXIT, ""));
            packets_out.push_back(exit_p);
        }
    }
}


 
    // Must declare the format of data expected by the filter
    // const char * IntegerMerge_format_string = ""; // No format; can recieve multiple types.
    // void IntegerMerge(
    //     std::vector<PacketPtr> &packets_in,
    //     std::vector<PacketPtr> &packets_out,
    //     std::vector<PacketPtr> /* packets_out_reverse */,
    //     void ** filter_state,
    //     PacketPtr& /* params */,
    //     const TopologyLocalInfo* topologyInfo
    // )
    // {
    //     struct IM_filter_state *state = (struct IM_filter_state*) *filter_state;
    //     if(*filter_state == NULL)
    //     {
    //         std::cout << "Making a new filter state" << std::endl;
    //         state = (struct IM_filter_state *) malloc(sizeof(IM_filter_state));
    //         state->buffer = new int*[MAX_NUMBER_CHUNKS];
    //         state->array_lens = new int[MAX_NUMBER_CHUNKS];
            
    //         state->curr_buffer_size = 0;
            
    //         state->num_children = topologyInfo->get_NumChildren();
    //         if(state->num_children == 0)
    //         {
    //             state->num_children = 1;
    //         }
            
    //         state->num_children_exited = 0; 

    //         *filter_state = (void *) state;
    //         // std::cout << "Filter Num Children: " << state->num_children << std::endl;
    //         // std::cout << "Filter Num Children Exited: " << state->num_children_exited << std::endl;
        
    //         // std::cout << "Filter Topology Information: " << std::endl;
    //         // std::cout << "Rank: " <<  topologyInfo->get_Rank() << std::endl;
    //         // std::cout << "Dist to Root: " <<  topologyInfo->get_RootDistance() << std::endl;
    //         // std::cout << "Dist to leaves: " << topologyInfo->get_MaxLeafDistance() << std::endl;

    //         // std::cout << "Num Descendants: " << topologyInfo->get_NumDescendants() << std::endl;
    //         // std::cout << "Num Leaf Descendants" << topologyInfo->get_NumLeafDescendants() << std::endl;

    //     }

    //     // std:: cout << "Filter finished creating state." << std::endl;

    //     // Iterate over all awaiting packets.
    //     int tag;

    //     int *new_array;
    //     int array_length;
    //     uint stream_id;

    //     PacketPtr cur_packet;
    //     for(unsigned int i = 0; i < packets_in.size(); i++)
    //     {
    //         cur_packet = packets_in[i];
    //         stream_id = cur_packet->get_StreamId();

    //         tag = cur_packet->get_Tag();
    //         switch(tag)
    //         {
    //             // Case - integers to merge. Add them to the stored state buffer.
    //             case PROT_MERGE:
    //                 cur_packet->unpack("%ad", &new_array, &array_length);
    //                 std::cout << "Filter - unpacked array of len : " << array_length << std::endl;
                    
    //                 state->buffer[state->curr_buffer_size] = new_array;
    //                 state->array_lens[state->curr_buffer_size] = array_length;
    //                 state->curr_buffer_size++;
    //                 break;
    //             // Case - a child has reported that it has exited.
    //             case PROT_SUBTREE_EXIT:
    //                 state->num_children_exited++;
    //                 std::cout << "Exit packet observed at filter." << std::endl;
    //                 break;
    //             default:
    //                 std::cerr << "Packet tag unrecognized at filter" << std::endl;
    //                 return;
    //         }
    //     }

    //     // Perform new operations based on filter state:
    //     // 1. If there are >2 elements in the buffer, always merge the whole buffer & send as a packet.
    //     // 2. If # Children exited == # Children,
    //     //     2a. Flush the buffer of any remaining packets & send.
    //     //     2b. Send a 'PROT SUBTREE EXIT' packet up the chain.

    //     // 1. - Flush buffer if needed.
    //     PacketPtr p;
    //     if(state->curr_buffer_size > 1)
    //     {
    //         p = merge_buffer(state, stream_id);
    //         packets_out.push_back(p);
    //     }

    //     // 2. Check for exit conditions.
    //     if(state->num_children_exited == state->num_children)
    //     {
    //         std::cout << "Filter Pushing back new packets." << std::endl;
    //         // a. Check if we need to flush buffer.
    //         if(state->curr_buffer_size > 0)
    //         {
    //             p = merge_buffer(state, stream_id);
    //             packets_out.push_back(p);
    //         }

    //         // b. Send an exit packet.
    //         PacketPtr exit_p (new Packet(stream_id, PROT_SUBTREE_EXIT, ""));
    //         packets_out.push_back(exit_p);
    //     }
    // }
    */
