#include <string>

#include <iostream>
#include <fstream>
#include <sstream>

// For fork-exec-wait
#include <unistd.h>
#include <sys/wait.h>

#include "mrnet/MRNet.h"

#include "BasicHISAT2MRNet.h"

#include "../Executables.h"
#include "../Utils.h"

using namespace MRN;

// For testing/debug
void write_empty_file(const char *f_name)
{
    std::string filename_out;    
    std::cout << "Filename given to backend node : " << f_name << std::endl;

    filename_out = std::string(f_name) + ".out";
    std::cout << "Filename for the output of the program: " << filename_out << std::endl;

    std::ofstream outfile (filename_out);
}

/*
 * Run HISAT2 on the given input file.
 */
int compress_file(char *input_file, char*output_file)
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
        char exe_path[PATH_MAX + 1];

        bcopy(SAMTOOOLS_PATH, exe_path, PATH_MAX + 1);
        
        // Args for execution.
        // Fixed size vector at this point; we can screw around with this if we want to make it more flexible.
        char *argv[7];
	argv[0] = exe_path;
        argv[1] = "view";
        argv[2] = "-b";
        argv[3] = "-o";
        argv[4] = output_file;
        argv[5] = input_file;
        argv[6] = NULL;
	std::cout << "Input file: " << input_file << std::endl;
	std::cout << "Output file: " << output_file << std::endl;
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

/**
 * Compress the given input .sam file into a .bam files by callign samtools merge
 */
int align_file(char *input_file, char* output_file)
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
        char exe_path[PATH_MAX + 1];
        char index_path[PATH_MAX + 1];

        bcopy(HISAT2_SMALL_PATH, exe_path, PATH_MAX + 1);
        bcopy(EXAMPLE_INDEX_PATH, index_path, PATH_MAX + 1);
        
        // Args for execution.
        // Fixed size vector at this point; we can screw around with this if we want to make it more flexible.
        char *argv[9];
        argv[0] = exe_path;
        argv[1] = "-f";
        argv[2] = "-x";
        argv[3] = index_path;
        argv[4] = "-U";
        argv[5] = input_file;
        argv[6] = "-S";
        argv[7] = output_file;
        argv[8] = NULL;

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

int main(int argc, char **argv)
{
    Stream *stream = NULL;
    PacketPtr p;
    int tag = 0;
    std::cout << "Creating backend network object." << std::endl;
    Network * network = Network::CreateNetworkBE(argc, argv);
    std::cout << "Created backend network object." << std::endl;

    char *filename_ptr_in;
    char *filename_ptr_out;
    char filename_ptr_bam[PATH_MAX + 1];

    std::string filename_in;
    std::string filename_out;
    std::string filename_out_bam;

    // Main Execution loop.
    do
    {
        // Check for network recv failure.
        if(network->recv(&tag, p, &stream) != 1)
        {
            fprintf(stderr, "stream::recv() failure\n");
            return -1;
        }
        std::cout << "Backend node received a packet." << std::endl;

        // Switch on Network Tech.
        switch(tag)
        {
            case PROT_ALIGN:
                p->unpack("%s %s", &filename_ptr_in, &filename_ptr_out);
		        filename_in  = std::string(filename_ptr_in);
                filename_out = std::string(filename_ptr_out);
                filename_out_bam = filename_sam_to_bam(filename_ptr_out);

                std::cout << "Received the file " << filename_in << std::endl;
                std::cout << "Received the file " << filename_out << " to output" << std::endl;

                // Inject the operation to write the alignment file here.
                // For now, let's write out a dummy file just to make sure we are making it here.
                // In the future, we will want to to fork/exec/wait hisat2 witht the appropriate
                // arguments
                
                // write_empty_file(filename_in.c_str());
                // Align the file.
		std::cout << "ALigning file." << std::endl;
		align_file(filename_ptr_in, filename_ptr_out);
		std::cout << "File aligned." << std::endl;


                // Compress the output .sam file to a .bam file.
                bcopy(filename_out_bam.c_str(), filename_ptr_bam, PATH_MAX + 1);
		compress_file(filename_ptr_out, filename_ptr_bam);

                if(stream->send(tag, "%s", filename_out_bam.c_str()) == -1)
                {
                    printf("Stream send failure.\n");
                    return -1;
                }
                if(stream->flush() == -1)
                {
                    printf("stream::flush() failure\n");
                }
                break;
            case PROT_EXIT:
                printf("Processing a PROT_EXIT ... \n");
                break;
            default:
                printf("Unknown protocol %d\n", tag);
                break;
        }
        /* code */
    } while (tag != PROT_EXIT);
    
    network->waitfor_ShutDown();
    delete network;
    return 0;
}
