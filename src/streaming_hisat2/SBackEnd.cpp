#include <iostream>
#include <fstream>
#include <sstream>

#include <vector>

// For fork-exec-wait
#include <unistd.h>
#include <sys/wait.h>

#include <stdio.h>
#include <unistd.h>

#include "mrnet/MRNet.h"

#include "StreamingHISAT2MRNet.h"

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

        bcopy(SAMTOOLS_PATH, exe_path, PATH_MAX + 1);
        
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
        remove(input_file);
    }

    return 0;
}

/**
 * Compress the given input .sam file into a .bam files by callign samtools merge
 */
int align_file(char *input_file, char* output_file)
{
    int pid;
    // Get time est here
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
        // Get time2 est here
        // do t2 - t1
    }

    return 0;
}

int main(int argc, char **argv)
{
    Stream *stream         = NULL; // Used to read in the stream value from each recv call.
    Stream *control_stream = NULL; // For sending control packets directly to/from parent & backends
    Stream *data_stream    = NULL; // For sending data up and down through filters.

    PacketPtr p;
    int tag = 0;

    bool block_recv;           // Whether or not we block on a receive op.
    bool end_signaled = false; // Set to true when the Front End signals the end of communication.
    bool fin_pkt_sent = false; // Sent after the fin packet for this node has been sent. 

    std::cout << "Creating backend network object." << std::endl;
    Network * network = Network::CreateNetworkBE(argc, argv);
    std::cout << "Created backend network object." << std::endl;

    char *filename_ptr_in;
    char *filename_ptr_out;
    char filename_ptr_bam[PATH_MAX + 1];

    std::string filename_in;
    std::string filename_out;
    std::string filename_out_bam;

    int ret;
    std::vector<std::string> chunks_to_make_inputs;
    std::vector<std::string> chunks_to_make_outputs;

    // Main Execution loop.
    do
    {
        // See if we block on next receive
        // (If we have more input to process, do we not do block.)
        block_recv = (chunks_to_make_inputs.size() == 0 || control_stream == NULL);
        // If we block for receive - do a standard receive operation.
        // Then, flush out the rest of the buffer.

        // If we don't block for receive - skip right to flushing out the buffer.
        std::cout << "BE Recv calling." << std::endl;
        if(block_recv)
        {
            std::cout << "Blocking on receive" << std::endl;
            if(network->recv(&tag, p, &stream, true) != 1)
            {
                fprintf(stderr, "stream::recv() failure\n");
                return -1;
            }

            switch(tag)
            {
                case PROT_ALIGN:
                    std::cout << "Backend received align message" << std::endl;
                    data_stream = stream;
                    p->unpack("%s %s", &filename_ptr_in, &filename_ptr_out);

                    // Will this change as char *s change?
                    chunks_to_make_inputs.push_back(std::string(filename_ptr_in));
                    chunks_to_make_outputs.push_back(std::string(filename_ptr_out));

                    break;
                case PROT_EXIT:
                    std::cout << "Backend received new message - end exeuction." << std::endl;
                    control_stream = stream;
                    end_signaled = true;
                    break;
                case PROT_INIT_CONTROL_STREAM:
                    control_stream = stream;
                    break;
                default:
                    std::cerr << "Error - tag not recognized (Backend)." << std::endl;
                    return -1;
            }
        }

        // Loop - Receive and process all remaining packets in the buffer.
        while(true)
        {
            ret = network->recv(&tag, p, &stream, false);
            
            // Case - packet read
            if(ret == 1)
            {
                // Unpack and process packet.
                switch(tag)
                {
                    case PROT_ALIGN:
                        std::cout << "Backend received align message" << std::endl;
                        data_stream = stream;
                        p->unpack("%s %s", &filename_ptr_in, &filename_ptr_out);

                        // Will this change as char *s change?
                        chunks_to_make_inputs.push_back(std::string(filename_ptr_in));
                        chunks_to_make_outputs.push_back(std::string(filename_ptr_out));

                        break;
                    case PROT_EXIT:
                        std::cout << "Backend received new message - end exeuction." << std::endl;
                        control_stream = stream;
                        end_signaled = true;
                        break;
                    case PROT_INIT_CONTROL_STREAM:
                        control_stream = stream;
                        break;
                    default:
                        std::cerr << "Error - tag not recognized (Backend)." << std::endl;
                        return -1;
                }

                continue;
            }
            // Case - no more packets to read.
            else if(ret == 0)
            {
                break;
            }
            // Case - Error
            else
            {
                fprintf(stderr, "stream::recv() failure\n");
                return -1;                    
            }
        }
        std::cout << "Finished Receiving" << std::endl;

        // Generate and send a packet.
        if(chunks_to_make_inputs.size() > 0 && control_stream != NULL)
        {
            // Generate & Send a packet.
            if(data_stream == NULL || control_stream == NULL)
            {
                fprintf(stderr, "Stream is null on chunck send\n");
                return -1;
            }

            // Generate a data array & send it out the data stream.
            filename_in = chunks_to_make_inputs.back();
            chunks_to_make_inputs.pop_back();
            filename_out = chunks_to_make_outputs.back();
            chunks_to_make_outputs.pop_back();
            filename_out_bam = filename_sam_to_bam(filename_out.c_str());

            // Alignment OP.
            align_file(filename_ptr_in, filename_ptr_out);

            bcopy(filename_out_bam.c_str(), filename_ptr_bam, PATH_MAX + 1);
            compress_file(filename_ptr_out, filename_ptr_bam);

            // Possibly commands for SAMTOOLS INDEX, SAMTOOLS SORT?

            if(data_stream->send(PROT_MERGE, "%s",filename_out_bam.c_str()) == -1)
            {
                fprintf(stderr, "Error in stream send.\n");
            }
            if(data_stream->flush() == -1)
            {
                std::cerr << "Error in flushing stream." << std::endl;
            }

            // Request FE for another chunk.
            if(control_stream->send(PROT_CHUNK_REQUEST, "") == -1)
            {
                std::cerr << "Error in sending to control stream." << std::endl;
            }
            if(control_stream->flush() == -1)
            {
                std::cerr << "Error in flushing control stream." << std::endl;
            }        
        }

        // Send a 'fin' packet if conditions are met.
        if(end_signaled && (chunks_to_make_inputs.size() == 0))
        {
            std::cout << "BE signaling the end." << std::endl;
            data_stream->send(PROT_SUBTREE_EXIT, "");
            data_stream->flush();
            fin_pkt_sent = true;
        }
        /* code */
    } while (!fin_pkt_sent);
    
    network->waitfor_ShutDown();
    delete network;
    return 0;
}
