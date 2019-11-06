#include <vector>
#include <iostream>

#include "mrnet/Packet.h"
#include "mrnet/NetworkTopology.h"

#include "BasicHISAT2MRNet.h"


using namespace MRN;

extern "C"
{
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
        for (const auto &packet : packets_in)
        {
            packet->unpack("%s", &unpack_ptr);
	    s_piece = std::string(unpack_ptr);
	    std::cout << "Read string at filter : " << s_piece << std::endl;
	    s += s_piece;
        }
        // TODO: Run the append operation on the file.

        PacketPtr new_packet( new Packet(packets_in[0]->get_StreamId(),
                                         packets_in[0]->get_Tag(), "%s", s));
        packets_out.push_back(new_packet);
    }
}
