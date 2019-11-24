#include <vector>
#include <set>

#include "mrnet/Packet.h"
#include "mrnet/NetworkTopology.h"

using namespace MRN;

extern "C"
{
    // Must declare the format of data expected by the filter
    const char * IntegerMerge_format_string = "%d";
    void IntegerMerge(
        std::vector<PacketPtr> &packets_in,
        std::vector<PacketPtr> &packets_out,
        std::vector<PacketPtr> /* packets_out_reverse */,
        void ** /* client data */,
        PacketPtr& /* params */,
        const TopologyLocalInfo* topologyInfo
    )
    {
        const NetworkTopology *topology = topologyInfo->get_Topology();
        set<NetowrkTopolgy::Node*> *nodes;
        topolgy->get_BackEndNodes(nodes);

        int sum = 0;

        for(unsigned int i = 0; i < packets_in.size(); i++)
        {
            PacketPtr cur_packet = packets_in[i];
            int val;
            cur_packet->unpack("%d", &val);
            sum += val;
        }

        PacketPtr new_packet( new Packet(packets_in[0]->get_StreamId(),
                                         packets_in[0]->get_Tag(), "%d", sum));
        packets_out.push_back(new_packet);
    }
}