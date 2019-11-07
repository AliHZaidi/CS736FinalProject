#include <vector>
#include <iostream>

#include "mrnet/Packet.h"
#include "mrnet/NetworkTopology.h"

#include "BasicHISAT2MRNet.h"

#include <unistd.h>
#include <sys/wait.h>

#include "../Executables.h"
#include "../Utils.h"

using namespace MRN;

extern "C"
{
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
	    char *argv[64];
	    for(int i = 0; i < 63; ++i)
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
        void ** /* client data */,
        PacketPtr& /* params */,
        const TopologyLocalInfo*
    )
    {
        // Taken from https://stackoverflow.com/questions/15347123/how-to-construct-a-stdstring-from-a-stdvectorstring/18703743
        std::string s;
        std::string s_piece;
	    char *unpack_ptr;

        std::vector<std::string> filename_strings;

        for (const auto &packet : packets_in)
        {
            packet->unpack("%s", &unpack_ptr);

            // s_piece = std::string(unpack_ptr);
            // std::cout << "Read string at filter : " << s_piece << std::endl;
            // s += s_piece;
        
            filename_strings.push_back(std::string(unpack_ptr));
        }

        s = concat_chunk_filenames(filename_strings);
	std::cout << "Merging bams." << std::endl;
	if(packets_in.size() > 1)
	{
	    merge_bam_files(filename_strings, s);
	}
	std::cout << "Writing packet with string: " << s << " at filter." <<std::endl;

        // Write the string to a buffer to be placed in the packet.
        // Apparently, if we try to feed s in directly the C++ deconstructor will
        // Delete it before we can send it back; but feeding in a static buffer works.
        char string_char_ptr [PATH_MAX + 1];
        bcopy(s.c_str(), string_char_ptr, PATH_MAX + 1);

        PacketPtr new_packet( new Packet(packets_in[0]->get_StreamId(),
                                         packets_in[0]->get_Tag(), "%s", string_char_ptr));

        packets_out.push_back(new_packet);
    }
}
