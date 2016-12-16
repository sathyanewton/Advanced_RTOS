#include "ns.h"

extern union Nsipc nsipcbuf;

void
input(envid_t ns_envid)
{
	binaryname = "ns_input";

	// LAB 6: Your code here:
	// 	- read a packet from the device driver
	//	- send it to the network server
	// Hint: When you IPC a page to the network server, it will be
	// reading from it for a while, so don't immediately receive
	// another packet in to the same physical page.
	int r;
	r = sys_page_alloc(0, (void *) UTEMP, PTE_U|PTE_W|PTE_P);
	if (r <0)
		panic ("net/input: error in sys page alloc");

	while (1)
	{

		r = sys_page_alloc(0, (void *) &nsipcbuf, PTE_U|PTE_W|PTE_P);
		if (r <0)
		panic ("net/input: error in sys page alloc");

		r = sys_try_receive((void *) UTEMP);
		if (-E_BUF_FULL != r)
		{
			// if a page is recieved
			//read a page
			//IPC it to core network
			nsipcbuf.pkt.jp_len=r;
			memcpy ( nsipcbuf.pkt.jp_data, (void *) UTEMP, r);

			ipc_send(ns_envid,  NSREQ_INPUT, &nsipcbuf, PTE_P | PTE_U);
		}
		else
		{
			//just try later for a packet
			sys_yield();
		}
	}
}
