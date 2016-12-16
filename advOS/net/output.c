#include "ns.h"

extern union Nsipc nsipcbuf;

void
output(envid_t ns_envid)
{
	binaryname = "ns_output";

	// code for lab 6 -M.G
	// 	- read a packet from the network server
	//	- send the packet to the device driver

    envid_t from_env_store;
    int perm_store;
    int return_value;

    while(1)
    {
        return_value = ipc_recv(&from_env_store, &nsipcbuf, &perm_store);
        
        if (!(from_env_store == ns_envid && return_value == NSREQ_OUTPUT))
        {
            continue;
        }

        // Send
        while (sys_E1000_tx_packet(nsipcbuf.pkt.jp_data, nsipcbuf.pkt.jp_len));        
    }
}
