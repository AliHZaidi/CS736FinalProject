#include <vector>
#include <iostream>

#include "mrnet/MRNet.h"
#include "IntegerSort.h"

using namespace MRN;

/**
 * Generate array of given size.
 * Should be deleted with a delete[] array statement later on.
 */
int *generate_array(int size)
{
    if(size <= 0)
    {
        fprintf(stderr, "Error - attempting to create array with len < 1\n");
        return NULL;
    }
    int *array = new int[size];
    return array;
}

int main(int argc, char **argv)
{
    Stream *stream         = NULL; // Used to read in the stream value from each recv call.
    Stream *control_stream = NULL; // For sending control packets directly to/from parent & backends
    Stream *data_stream    = NULL; // For sending data up and down through filters.

    PacketPtr p;
    int tag = 0;
    int read_value; // Used to read from packets.
    int chunks_to_make = 0;

    bool block_recv;           // Whether or not we block on a receive op.
    bool end_signaled = false; // Set to true when the Front End signals the end of communication.
    bool fin_pkt_sent = false; // Sent after the fin packet for this node has been sent. 
    
    int *data_array;

    std::cout << "Making BE Network" << std::endl;
    Network * network = Network::CreateNetworkBE(argc, argv);
    std::cout << "Made BE Network" << std::endl;
    
    // Main Execution loop.
    // Alternates between attempting to flush the RCV buffer,
    // and Generating and sending a packet.
    do
    {
        // See if we block on next receive
        // (If we have more input to process, do we not do block.)
        block_recv = (chunks_to_make == 0 || control_stream == NULL);
        int ret;

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
                case PROT_FIRST_CHUNK_GEN:
                    std::cout << "Backend received message - make multiple chunks" << std::endl;
                    data_stream = stream;
                    p->unpack("%d", &read_value);
                    chunks_to_make += read_value;
                    std::cout << "Making " << chunks_to_make << " chunks." << std::endl;
                    break;
                case PROT_CHUNK_GEN:
                    data_stream = stream;
                    chunks_to_make++;
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
                    case PROT_FIRST_CHUNK_GEN:
                        std::cout << "Backend received message - make multiple chunks" << std::endl;
                        data_stream = stream;
                        p->unpack("%d", &read_value);
                        chunks_to_make += read_value;
                        break;
                    case PROT_CHUNK_GEN:
                        data_stream = stream;
                        chunks_to_make++;
                        break;
                    case PROT_EXIT:
                        std::cout << "Backend received new message - end exeuction." << std::endl;
                        control_stream = stream;
                        end_signaled = true;
                        break;
                    case PROT_INIT_CONTROL_STREAM:
                        control_stream = stream;
                        break;
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
        if(chunks_to_make > 0 && control_stream != NULL)
        {
            // Generate & Send a packet.
            if(data_stream == NULL || control_stream == NULL)
            {
                fprintf(stderr, "Stream is null on chunck send\n");
                return -1;
            }

            // Generate a data array & send it out the data stream.
            data_array = generate_array(INTS_PER_CHUNK);
            std::cout << "Sending a chunk of length: " << INTS_PER_CHUNK << std::endl;
            if(data_stream->send(PROT_MERGE, "%ad", data_array, INTS_PER_CHUNK) == -1)
            {
                fprintf(stderr, "Error in stream send.\n");
            }
            std::cout << "BE Flushing outstream after sending chunk." << std::endl;
            if(data_stream->flush() == -1)
            {
                std::cerr << "Error in flushing stream." << std::endl;
            }
            std::cout << "Chunk sent." << std::endl;
            delete[] data_array;

            // Request FE for another chunk.
            if(control_stream->send(PROT_CHUNK_REQUEST, "") == -1)
            {
                std::cerr << "Error in sending to control stream." << std::endl;
            }
            if(control_stream->flush() == -1)
            {
                std::cerr << "Error in flushing control stream." << std::endl;
            }
        
            chunks_to_make--;
        }

        // Send a 'fin' packet if conditions are met.
        if(end_signaled && (chunks_to_make == 0))
        {
            std::cout << "BE signaling the end." << std::endl;
            data_stream->send(PROT_SUBTREE_EXIT, "");
            data_stream->flush();
            fin_pkt_sent = true;
        }
    } while (!fin_pkt_sent);
    
    network->waitfor_ShutDown();
    delete network;
    return 0;
}