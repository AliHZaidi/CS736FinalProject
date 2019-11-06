#include <string>

#include <iostream>
#include <fstream>
#include <sstream>

#include "mrnet/MRNet.h"
#include "BasicHISAT2MRNet.h"

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

int main(int argc, char **argv)
{
    Stream *stream = NULL;
    PacketPtr p;
    int tag = 0;
    std::cout << "Creating backend network object." << std::endl;
    Network * network = Network::CreateNetworkBE(argc, argv);
    std::cout << "Created backend network object." << std::endl;

    std::string filename_in;
    char *filename_ptr;
    std::string filename_out;

    // Main Execution loop.
    do
    {
        // Check for network recv failure.
        if(network->recv(&tag, p, &stream) != 1)
        {
            fprintf(stderr, "stream::recv() failure\n");
            return -1;
        }

        // Switch on Network Tech.
        switch(tag)
        {
            case PROT_APPEND:
                p->unpack("%s", &filename_ptr);
		filename_in = std::string(filename_ptr);
                std::cout << "Received the file " << filename_in << std::endl;

                // Inject the operation to write the alignment file here.
                // For now, let's write out a dummy file just to make sure we are making it here.
                // In the future, we will want to to fork/exec/wait hisat2 witht the appropriate
                // arguments
                write_empty_file(filename_in.c_str());

                if(stream->send(tag, "%s", filename_in.c_str()) == -1)
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
