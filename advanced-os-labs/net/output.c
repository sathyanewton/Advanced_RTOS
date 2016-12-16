#include "ns.h"

extern union Nsipc nsipcbuf;

void
output(envid_t ns_envid)
{
	binaryname = "ns_output";

	// LAB 6: Your code here:
	// 	- read a packet from the network server
	//	- send the packet to the device driver

	int r;


	int rec_val;
	envid_t from;
	struct jif_pkt *send_jif_pkt;

	while (1)
	{
		rec_val=ipc_recv( &from, (void *)UTEMP, NULL);

		if ( NSREQ_OUTPUT != rec_val)
			panic ("Got srong IPC message");

		send_jif_pkt = (struct jif_pkt *) UTEMP;
		sys_try_transmit(( void * ) send_jif_pkt->jp_data, send_jif_pkt->jp_len );

	}
}
