
#include <kern/e1000.h>
#include <kern/pmap.h>
#include <inc/string.h>


// LAB 6: Your driver code here

//store MMIO base 0
volatile uint32_t *nic_mmio_base0;

// base of Tx Descriptor array must be 16B aligned
// (page aligned satisfies this)
volatile physaddr_t tx_desc_base_pa; //TODO Check if needs to be volaite

// tr desc array
volatile struct tx_desc *ptr_tx_desc_base_va;
// stores VA of buffers
volatile struct tx_buff *buffers [NUMBER_OF_TX_DESC];
/*
 *
 * RX vars
 */
volatile physaddr_t rx_desc_base_pa; //TODO Check if needs to be volaite

// tr desc array
volatile struct e1000_rx_desc *ptr_rx_desc_base_va;
volatile struct rx_buff *rx_buffers [NUMBER_OF_RX_DESC]; //VA of buffers

//attach function for PCI init call
volatile uint32_t *lapic;
int nic_init(struct pci_func *pcif)
{
	// enable the dev
	// negotiate the MMIO BAR 0 (i.e. reg_base[0], reg_size[0])
	pci_func_enable (pcif);

	//map the MMIO region
	nic_mmio_base0 = (uint32_t*) mmio_map_region (pcif->reg_base[0], pcif->reg_size[0]);
	//panic ("nic_init(): Status of NIC %x ", *(nic_mmio_base0+2));

	//Tx Initialization See intel manual 14.5

	alloc_desc_buff();
	init_tx();


	init_rx();
	return 0;
}

// send a physical page.
// caller function MUST check permissions on physical page
//drops a packet if hardware buffer is full
// returns
// 0 if successful
// -E_BUF_FULL if hardware buffer is empty
int try_transmit(struct PageInfo *pp_user, size_t len)
{
	if(len > BUFF_SIZE)
		panic("try_transmit(): len> BUF_SIZE - the system call made a mistake?");

	// tail is an index into ring
	int tail = *( nic_mmio_base0 + E1000_TDT/4 );
	volatile struct tx_desc *ptr_tx_desc_tail = ptr_tx_desc_base_va + tail;

	//tail should point to the next free element. Does it?
	if ( 0 == (ptr_tx_desc_tail->status & E1000_TXD_STAT_DD))
	{
		
		//Hardware didnt set DD bit, means not transmitted yet
		return -E_BUF_FULL ; //drop packet
	}

	//desc pointed by tail is free
	//copy over buffer
	memcpy ( (void * ) buffers[tail] , page2kva(pp_user), len );

	ptr_tx_desc_tail->addr =  PADDR ((void *)buffers[tail]);

	// ask the card to set DD once it transmits, set 3rd bit in cmd
	ptr_tx_desc_tail->cmd = 0x9;
	ptr_tx_desc_tail->status &= ~E1000_TXD_STAT_DD; //reset the DD bit
	ptr_tx_desc_tail->length = len;
	ptr_tx_desc_tail->cso = 0; //for compatibility

	*( nic_mmio_base0 + E1000_TDT/4 ) = (tail+1) % NUMBER_OF_TX_DESC;

	return 0;
}

// returns
// -E_BUF_FULL if there are no packets that were received
// number of bytes copied on success
int try_receive(struct PageInfo *pp_user)
{
	int len;
	int next_available_desc = 	((*( nic_mmio_base0 + E1000_RDT/4 ) + 1) % NUMBER_OF_RX_DESC);
	volatile struct e1000_rx_desc *ptr_rx_desc_tail = (ptr_rx_desc_base_va + next_available_desc);
	//check for DD bit
	//DD bit is set if the packet is succesfully transmitted.
	//If it isnt set then return to try again
	if (!(ptr_rx_desc_tail->status & E1000_RXD_STAT_DD))
		return -E_BUF_FULL;
	//When the DD bit is set,
	//Obtain length from the length field of the descriptor.
	len = ptr_rx_desc_tail->length;
	//Copy the packet data out of that descriptor's packet buffer
	memcpy ( page2kva(pp_user), (void *)rx_buffers[next_available_desc],  len );
	//Data is copied, clear the DD bit.
	ptr_rx_desc_tail->status &= ~E1000_RXD_STAT_DD;
	//Move the RDT to the next position
	*( nic_mmio_base0 + E1000_RDT/4 ) = next_available_desc;
	return len;
}

// allocate space for descriptors and buffers
static void
alloc_desc_buff()
{
	int i;

	struct PageInfo *pp_desc = 	page_alloc (ALLOC_ZERO);
	if ( NULL == pp_desc )
		panic ("alloc_desc_buff(): page_alloc says no physical pages!");
	tx_desc_base_pa = page2pa (pp_desc);
	ptr_tx_desc_base_va = KADDR (tx_desc_base_pa); //holds base VA of desc
	
	// set the DD bit to 1 as now all desc are free
	for (i=0; i<NUMBER_OF_TX_DESC; i++)
	{
		(ptr_tx_desc_base_va+i)->status = E1000_TXD_STAT_DD;
	}

	struct PageInfo *pp;
	//allocate space for tx buffer
	for (i=0; i<NUMBER_OF_TX_DESC; i++)
	{
		if ( NULL == (pp = 	page_alloc (!ALLOC_ZERO)) )
			panic ("alloc_desc_buff(): page_alloc says no physical pages!");
		buffers[i] = page2kva (pp);	
	}

	/*
	 *
	 * RX Allocation
	 */
	//TX descriptors used 64 * 16 bytes = 1024B. So we use the rest for rx desc array
	rx_desc_base_pa = page2pa (pp_desc) + LENGTH_OF_TX_RING;
	//sanity check
	if (0 !=  rx_desc_base_pa%16)
		panic("rx desc array not aligned!");
	ptr_rx_desc_base_va = KADDR (rx_desc_base_pa); //holds base VA of rx desc



	//allocate space for rx buffer
	for (i=0; i<NUMBER_OF_RX_DESC; i++)
	{
		if ( NULL == (pp = 	page_alloc (!ALLOC_ZERO)) )
			panic ("alloc_desc_buff(): page_alloc says no physical pages!");
		rx_buffers[i] = page2kva (pp);

		(ptr_rx_desc_base_va+i)->status &= ~E1000_RXD_STAT_DD; //set status as empty
		(ptr_rx_desc_base_va+i)->buffer_addr = (uint32_t) PADDR ((void *)rx_buffers[i]);
	}

}
static void
	init_tx()
{
	// set the TDBAL register,
		// and set TDBAH as zero as we use only 32 bits
		*( nic_mmio_base0 + E1000_TDBAL/4) = tx_desc_base_pa;
		*( nic_mmio_base0 + E1000_TDBAH/4) = 0;

		//set length of Tx Ring
		// must be 128B aligned
		*( nic_mmio_base0 + E1000_TDLEN/4) = LENGTH_OF_TX_RING;

		//write 0 to tx desc head and tx desc tail registers
		*( nic_mmio_base0 + E1000_TDH/4 ) = 0;
		*( nic_mmio_base0 + E1000_TDT/4 ) = 0;

		*( nic_mmio_base0 + E1000_TCTL/4 ) = E1000_TCTL_EN | E1000_TCTL_PSP;
		*( nic_mmio_base0 + E1000_TCTL/4 ) |= 0xf << 4; //CT bits
		*( nic_mmio_base0 + E1000_TCTL/4 ) |=  (0x40 << 12); //E1000_TCTL_COLD

		// page 313 TIPG register
		*( nic_mmio_base0 + E1000_TIPG/4 ) = 10 /*IPGT */
				| ( 8 <<10) /*IPGR1 is significant only in half-duplex mode of operation.*/
				| (6 <<20) /*IPGR2 is significant only in half-duplex mode of operation.*/;
}

static void
	init_rx()
{
	// see sec. 14.4
	//set MAC address
	// volatile char *hwaddr = (char *)(nic_mmio_base0 + E1000_RA/4);
	// cprintf ("%x", hwaddr);
	// hwaddr[0] = 0x12;
	// hwaddr[1] = 0x00;
	// hwaddr[2] = 0x54;
	// hwaddr[3] = 0x52;
	// hwaddr[4] = 0x56;
	// hwaddr[5] = 0x34;
	// hwaddr[6] = 0x00;
	// hwaddr[7] = 0x80;
	uint8_t mac[6] = {0x52, 0x54, 0x00, 0x12, 0x34, 0x56};
	*( nic_mmio_base0 + E1000_RAL/4 ) = mac[3] << 24 | mac[2] << 16 | mac[1] << 8 | mac[0];
	*( nic_mmio_base0 + E1000_RAH/4 ) = mac[5]<<8 | mac[4] | E1000_RAH_AV;

	//LAN Multicast to 0
	*( nic_mmio_base0 + E1000_MTA/4 ) = 0;
	//Disable all interrupts to 0
	*( nic_mmio_base0 + E1000_IMS/4 ) = 0;

	// and set RDBAH as zero as we use only 32 bits
	*( nic_mmio_base0 + E1000_RDBAL/4) = rx_desc_base_pa;
	*( nic_mmio_base0 + E1000_RDBAH/4) = 0;

	//set length of rx Ring
	// must be 128B aligned
	*( nic_mmio_base0 + E1000_RDLEN/4) = LENGTH_OF_RX_RING;


	//write 0 to rx desc head and rx desc tail registers
	*( nic_mmio_base0 + E1000_RDH/4 ) = 0;
	*( nic_mmio_base0 + E1000_RDT/4 ) = NUMBER_OF_RX_DESC-1;

	// init to 0
	*( nic_mmio_base0 + E1000_RCTL/4 ) = 0;
	// disable jumbo packets
	*( nic_mmio_base0 + E1000_RCTL/4 ) &= ~E1000_RCTL_LPE;

	// no loop back mode
	*( nic_mmio_base0 + E1000_RCTL/4 ) |= E1000_RCTL_LBM_NO;
	//enable broadcast
	*( nic_mmio_base0 + E1000_RCTL/4 ) |= E1000_RCTL_BAM;
	//set buffer size
	*( nic_mmio_base0 + E1000_RCTL/4 ) |= E1000_RCTL_BSEX;
	*( nic_mmio_base0 + E1000_RCTL/4 ) |= E1000_RCTL_SZ_4096;
	//strip crc
	*( nic_mmio_base0 + E1000_RCTL/4 ) |= E1000_RCTL_SECRC;

	//finally, enable the RX
	*( nic_mmio_base0 + E1000_RCTL/4 ) |= E1000_RCTL_EN;

}

void e1000_disp_tail()
{
	cprintf ("tail=%d\n", *( nic_mmio_base0 + E1000_RDT/4 ));
	cprintf ("head=%d\n", *( nic_mmio_base0 + E1000_RDH/4 ));
	cprintf ("data in buf0 = %x %x %x %x %x\n", rx_buffers[0]->data[0], rx_buffers[0]->data[1], rx_buffers[0]->data[2]
		, rx_buffers[0]->data[3], rx_buffers[0]->data[4]);
	struct PageInfo* pp = page_alloc(!ALLOC_ZERO);
	int r;
	r = try_receive(pp);
	char * d = (char *) page2kva ( pp );
	cprintf ("length returned is %d, data is %x %x %x %x %x\n", r, d[0], d[1], d[2], d[3], d[4]);
}
