#include <vector>
#include <iostream>
#include <string>

#include <stdio.h>
#include <unistd.h>

#include "mrnet/Packet.h"
#include "mrnet/NetworkTopology.h"

#include "StreamingHISAT2MRNet.h"

#include <sys/wait.h>

#include "Executables.h"
#include "Utils.h"

using namespace MRN;

extern "C"
{
    // Current filter state.
    struct M_filter_state
    {
        int num_children;        // Number of child nodes to this filter.
        int num_children_exited; // Number of child nodes that have reported that they are finished.

        char *buffer[MAX_NUMBER_CHUNKS];            // Current buffer of integer arrays (to be merged).
        char *full_aligned_file_name;
        int curr_buffer_size;
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
            std::cout << "Merging in child." << std::endl;
            char *argv[MAX_NUMBER_CHUNKS + 5];
            for(uint i = 0; i < MAX_CHILDREN_PER_NODE - 1; ++i)
            {
                argv[i] = (char *) malloc(sizeof(char) * (PATH_MAX + 1));
            }
            bcopy(SAMTOOLS_PATH, argv[0], PATH_MAX + 1);
            bcopy("merge", argv[1], PATH_MAX + 1);
            // bcopy("-o", argv[2], PATH_MAX + 1);
            bcopy(out_file.c_str(), argv[2], PATH_MAX + 1);
            
            int i = 3;
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
            execvp(argv[0], argv);
        }
        else
        {
            // In Parent.
            // TODO: Possibly look into wait_status to see if HISAT2 executed properly.
            int status;
            wait(&status);
            for(const auto &input_file : input_files)
            {
               remove(input_file.c_str());
            }
        }

        return 0;    
    }

    int merge_bam_files_sequence(std::vector<std::string> input_files, std::string out_file, std::string out_tmp_file)
    {
        int pid;
        for(const auto &input_file : input_files)
        {
            pid = fork();
            if(pid == -1)
            {
                std::cerr << "Error in fork() call on backend." << std::endl;
                return -1;
            }

            if(pid == 0)
            {
                // In child
                std::cout << "Merging in child." << std::endl;
                char *argv[MAX_NUMBER_CHUNKS + 5];
                for(uint i = 0; i < MAX_NUMBER_CHUNKS - 1; ++i)
                {
                    argv[i] = (char *) malloc(sizeof(char) * (PATH_MAX + 1));
                }
                bcopy(SAMTOOLS_PATH, argv[0], PATH_MAX + 1);
                bcopy("merge", argv[1], PATH_MAX + 1);
                bcopy(out_tmp_file.c_str(), argv[2], PATH_MAX + 1);
                
                int i = 3;
                bcopy(input_file.c_str(), argv[i], PATH_MAX + 1);
                ++i;

                bcopy(out_file.c_str(), argv[i], PATH_MAX + 1);
                ++i;

                argv[i] = NULL;

                i = 0;
                while(argv[i] != NULL)
                {
                    std::cout << "Argument: " << argv[i++] << std::endl;
                }
                execvp(argv[0], argv);
            }
            else
            {
                // In Parent.
                int status;
                wait(&status);

                remove(out_file.c_str());
                rename(out_tmp_file.c_str(), out_file.c_str());
                remove(input_file.c_str());
                
            }
        }

        return 0;    
    }

    int sort_bam_file(const char *in_file, const char *out_file)
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
            char *argv[6];

            for(uint i = 0; i < 6; ++i)
            {
                argv[i] = (char *) malloc(sizeof(char) * (PATH_MAX + 1));
            }

            bcopy(SAMTOOLS_PATH, argv[0], PATH_MAX + 1);
            bcopy("sort", argv[1], PATH_MAX + 1);
            bcopy("-o", argv[2], PATH_MAX + 1);
            bcopy(out_file, argv[3], PATH_MAX + 1);
            bcopy(in_file, argv[4], PATH_MAX + 1);
            argv[5] = NULL;

            int i = 0;
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
            
            remove(in_file);
            rename(out_file, in_file);
        }

        return 0;           
    }

    std::vector<std::string> filenames_vector(M_filter_state *state)
    {
        std::vector<std::string> ret;
        for(int i = 0; i < state->curr_buffer_size; ++i)
        {
            ret.push_back(std::string(state->buffer[i]));
        }

        return ret;
    }

    // Must declare the format of data expected by the filter
    const char * MergeFilter_format_string = "%s";
    void MergeFilter(
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
            state->curr_buffer_size = 0;

            *filter_state = (void *) state;
            std::cout << "Made a new filter state." << std::endl;
        }

        PacketPtr cur_packet;
        uint stream_id;
        int tag;
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
                    std::cout << "Unpacked filename " << unpack_ptr << std::endl;
                    state->buffer[state->curr_buffer_size] = unpack_ptr;
                    state->curr_buffer_size++;
                    std::cout << "File added to buffer." << std::endl;
                    break;
                // Case - a child has reported that it has exited.
                case PROT_SUBTREE_EXIT:
                    state->num_children_exited++;
                    std::cout << "Exit packet observed at intermediate filter." << std::endl;
                    break;
                default:
                    std::cerr << "Packet tag unrecognized at filter" << std::endl;
                    return;
            }
        }
        std::cout << "Finished unpacking packets" << std::endl;

        // Perform new operations based on filter state:
        // 1. If there are >2 elements in the buffer, always merge the whole buffer & send as a packet.
        // 2. If # Children exited == # Children,
        //     2a. Flush the buffer of any remaining packets & send.
        //     2b. Send a 'PROT SUBTREE EXIT' packet up the chain.

        // 1. - Flush buffer if needed.
        std::cout << "Flushing packet buffer." << std::endl;
        char string_char_ptr [PATH_MAX + 1];

        if(state->curr_buffer_size > 1)
        {
            // Get vector of filenames.
            std::vector<std::string> fname_vec = filenames_vector(state);

            std::cerr << "getting chunk filesnames for vec of size: " << fname_vec.size() << std::endl;
            s = concat_chunk_filenames(fname_vec);
            std::cout << "Merging bam files for: " << s << std::endl;
            merge_bam_files(fname_vec, s);

            for(int i = 0; i < state->curr_buffer_size; i++)
            {
                free(state->buffer[i]);
                state->buffer[i] = NULL;
            }
            state->curr_buffer_size = 0;

            bcopy(s.c_str(), string_char_ptr, PATH_MAX + 1);
            PacketPtr p(new Packet(packets_in[0]->get_StreamId(), PROT_MERGE, "%s", string_char_ptr));
            packets_out.push_back(p);
        }

        std::cout << "Finished flushing buffer." << std::endl;

        std::cout << "Checking exit conditions" << std::endl;

        // 2. Check for exit conditions.
        if(state->num_children_exited == state->num_children)
        {
            std::cout << "Filter Pushing back new packets." << std::endl;
            // a. Check if we need to flush buffer.
            if(state->curr_buffer_size == 1)
            {
                PacketPtr p(new Packet(packets_in[0]->get_StreamId(), PROT_MERGE, "%s", state->buffer[0]));
                packets_out.push_back(p);
            }

            // b. Send an exit packet.
            PacketPtr exit_p (new Packet(packets_in[0]->get_StreamId(), PROT_SUBTREE_EXIT, ""));
            packets_out.push_back(exit_p);
        }
    }

    const char *SortFilter_format_string = "";
    /**
     * Bottom level filter for sotring packets.
     */
    void SortFilter(
        std::vector<PacketPtr> &packets_in,
        std::vector<PacketPtr> &packets_out,
        std::vector<PacketPtr> /* packets_out_reverse */,
        void ** filter_state,
        PacketPtr& /* params */,
        const TopologyLocalInfo* topologyInfo
    )
    {
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

            state->curr_buffer_size = 0;

            *filter_state = (void *) state;
            std::cout << "Made a new filter state." << std::endl;
        }

        PacketPtr cur_packet;
        uint stream_id;
        int tag;
        char *unpack_ptr;
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
                    std::cout << "Unpacked filename " << unpack_ptr << std::endl;
                    state->buffer[state->curr_buffer_size] = unpack_ptr;
                    state->curr_buffer_size++;
                    std::cout << "File added to buffer." << std::endl;
                    break;
                // Case - a child has reported that it has exited.
                case PROT_SUBTREE_EXIT:
                    state->num_children_exited++;
                    std::cout << "Exit packet observed at sort filter." << std::endl;
                    break;
                default:
                    std::cerr << "Packet tag unrecognized at filter" << std::endl;
                    return;
            }
        }

        const char *in_filename;
        std::string out_str;
        const char *out_filename;
        for(int i = 0; i < state->curr_buffer_size; ++i)
        {
            // Sort File
            in_filename = state->buffer[i];
            out_str = filename_sorted_bam(in_filename);
            out_filename = out_str.c_str();
            std::cout << "From unsorted input file" << in_filename << std::endl;
            std::cout << "Generating sorted file: " << out_str << std::endl;
            std::cout << "Also, " << out_filename << std::endl;
            
            sort_bam_file(in_filename, out_filename);
            
            std::cout << "Sorted file generated" << std::endl;
        
            // Send file
            PacketPtr p(new Packet(packets_in[0]->get_StreamId(), PROT_MERGE, "%s", in_filename));
            packets_out.push_back(p);
        }
        state->curr_buffer_size = 0;

        // Send exit packet if necessary
        if(state->num_children == state->num_children_exited){
            PacketPtr exit_p (new Packet(packets_in[0]->get_StreamId(), PROT_SUBTREE_EXIT, ""));
            packets_out.push_back(exit_p);
        }
    }

    const char * MergeFilterTop_format_string = "%s";
    void MergeFilterTop(
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
            state->curr_buffer_size = 0;

            state->full_aligned_file_name = NULL;

            *filter_state = (void *) state;
            std::cout << "Made a new filter state." << std::endl;
        }

        PacketPtr cur_packet;
        uint stream_id;
        int tag;
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
                    std::cout << "Unpacked filename " << unpack_ptr << std::endl;
                    state->buffer[state->curr_buffer_size] = unpack_ptr;
                    state->curr_buffer_size++;
                    std::cout << "File added to buffer." << std::endl;
                    break;
                // Case - a child has reported that it has exited.
                case PROT_SUBTREE_EXIT:
                    state->num_children_exited++;
                    std::cout << "Exit packet observed at top level filter filter." << std::endl;
                    break;
                default:
                    std::cerr << "Packet tag unrecognized at filter" << std::endl;
                    return;
            }
        }
        std::cout << "Finished unpacking packets" << std::endl;

        // If we can merge - do so.
        if(state->curr_buffer_size > 0)
        {
            std::vector<std::string> fname_vec = filenames_vector(state);
            std::string tmp_fname;
            s = get_final_filename(fname_vec);

            // Get initial file used to contain the entire alignment array.
            // Done by popping an element out of the fname vector and setting it to the output file.
            if(state->full_aligned_file_name == NULL)
            {
                state->full_aligned_file_name = (char *) malloc(sizeof(char) * PATH_MAX);
                bcopy(s.c_str(), state->full_aligned_file_name, PATH_MAX);
                tmp_fname = fname_vec.back();
                fname_vec.pop_back();
                rename(tmp_fname.c_str(), state->full_aligned_file_name);
            }

            std::cout << "Merging bam files for: " << s << std::endl;
            tmp_fname = filename_temp(s.c_str());

            merge_bam_files_sequence(fname_vec, s, tmp_fname);
            state->curr_buffer_size = 0;
        }

        // Perform new operations based on filter state:
        // If # Children exited == # Children,
        //     a. Flush the buffer of any remaining packets & send.
        //     b. Send a 'PROT SUBTREE EXIT' packet up the chain.
        char string_char_ptr[PATH_MAX + 1];
        std::cout << "Checking exit conditions" << std::endl;
        std::cout << "Num Children exited: " << state->num_children_exited << std::endl;
        std::cout << "Num Children total: " << state->num_children << std::endl;

        // Check for exit conditions.
        if(state->num_children_exited == state->num_children)
        {
            std::cout << "Filter Pushing back new packets." << std::endl;
            std::cout << "This is the TOP LEVEL Filter" << std::endl;
            // a. Send alignment packet.
            if(state->full_aligned_file_name != NULL)
            {
                PacketPtr p(new Packet(packets_in[0]->get_StreamId(), PROT_MERGE, "%s", state->full_aligned_file_name));
                packets_out.push_back(p);
            }
            // b. Send an exit packet.
            PacketPtr exit_p (new Packet(packets_in[0]->get_StreamId(), PROT_SUBTREE_EXIT, ""));
            packets_out.push_back(exit_p);
        }
    }
}
