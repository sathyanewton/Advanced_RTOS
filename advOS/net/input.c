#include "ns.h"

extern union Nsipc nsipcbuf;

void
input(envid_t ns_envid)
{
	binaryname = "ns_input";

	// code for lab 6-M.G
	// 	- read a packet from the device driver
	//	- send it to the network server
	// Hint: When you IPC a page to the network server, it will be
	// reading from it for a while, so don't immediately receive
	// another packet in to the same physical page.

    int return_value;
	int num_bytes;

	while(1)
    {
		if ((return_value = sys_page_alloc(sys_getenvid(), (void *)&nsipcbuf, PTE_P | PTE_W | PTE_U)) < 0) 
        {
			panic("Error in sys_page_alloc!!");
		}

		while ((num_bytes = sys_E1000_rx_packet((char *)&nsipcbuf, PGSIZE)) < 0) 
        {
            ;
		}

		memcpy(nsipcbuf.pkt.jp_data, (char *)&nsipcbuf, num_bytes);
		nsipcbuf.pkt.jp_len = num_bytes;

		// send IPC to network server
		ipc_send(ns_envid, NSREQ_INPUT, &(nsipcbuf.pkt), PTE_P | PTE_W | PTE_U);
	}

}
