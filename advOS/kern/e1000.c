#include <kern/e1000.h>
#include <kern/pci.h>
#include <kern/pmap.h>
#include <inc/string.h>
#include <inc/mmu.h>
#include <inc/assert.h>

// code for lab 6-M.G
volatile uint32_t *e1000_bar_base;

struct tx_desc tx_descriptors[E1000_TX_DESCS];
char tx_bufs[E1000_TX_DESCS][E1000_TX_BUF_SIZE];

struct rx_desc rx_descriptors[E1000_RX_DESCS];
char rx_bufs[E1000_RX_DESCS][E1000_TX_BUF_SIZE];
static int head;


int E1000_attach_function(struct pci_func *pcif)
{
    pci_func_enable(pcif);
    e1000_bar_base = mmio_map_region(pcif->reg_base[0], pcif->reg_size[0] );

    /* Debugging*/
    //    cprintf("value of device status register%x\n", *(e1000_bar_address+ 0x00008/4)); 

	int i;
	for (i = 0; i < E1000_TX_DESCS; ++i) 
    {
		tx_descriptors[i].status = E1000_TXD_STAT_DD;
	}

    /* Transmitter registers*/
	e1000_bar_base[E1000_TDH / 4] = 0;
	e1000_bar_base[E1000_TDT / 4] = 0;
    
    e1000_bar_base[E1000_TDBAL / 4] = PADDR(tx_descriptors);
	e1000_bar_base[E1000_TDBAH / 4] = 0;
	e1000_bar_base[E1000_TDLEN / 4] = sizeof(struct tx_desc) * E1000_TX_DESCS;

	e1000_bar_base[E1000_TCTL / 4] |= E1000_TCTL_EN;
	e1000_bar_base[E1000_TCTL / 4] |= E1000_TCTL_PSP;

	e1000_bar_base[E1000_TCTL / 4] &= ~E1000_TCTL_CT;
	e1000_bar_base[E1000_TCTL / 4] |= (E1000_TCTL_CT_VAL << 4);

	e1000_bar_base[E1000_TCTL / 4] &= ~E1000_TCTL_COLD;
	e1000_bar_base[E1000_TCTL / 4] |= (E1000_TCTL_COLD_VAL << 12);

	e1000_bar_base[E1000_TIPG / 4] = (E1000_TIPG_IPGT_VAL ) | (E1000_TIPG_IPGR1_VAL << 10)
	                                      | (E1000_TIPG_IPGR2_VAL << 20) | (E1000_TIPG_RESERVED_VAL << 30);

    /* Receiver registers */
	
  
    e1000_bar_base[E1000_RDBAL / 4] = PADDR(rx_descriptors);
	e1000_bar_base[E1000_RDBAH / 4] = 0;
	e1000_bar_base[E1000_RDLEN / 4] = sizeof(struct rx_desc) * E1000_RX_DESCS;



    for (i = 0; i < E1000_TX_DESCS; ++i) 
    {
		rx_descriptors[i].addr = PADDR(rx_bufs[i]);
	}

    e1000_bar_base[E1000_IMS / 4] = 0;
    for (i = 0; i < E1000_N_MTA_ELEMS; ++i) 
    {
		e1000_bar_base[E1000_MTA / 4] = 0;
	}

    e1000_bar_base[E1000_RDH / 4] = 0;
	e1000_bar_base[E1000_RDT / 4] = E1000_RX_DESCS - 1;
  
    e1000_bar_base[E1000_RCTL / 4] |= E1000_RCTL_EN;
    e1000_bar_base[E1000_RCTL / 4] &= ~E1000_RCTL_LPE;
	e1000_bar_base[E1000_RCTL / 4] |= E1000_RCTL_BAM;
    e1000_bar_base[E1000_RCTL / 4] |= E1000_RCTL_SECRC;


    e1000_bar_base[E1000_RAL(0) / 4] = 0x12005452;
    e1000_bar_base[E1000_RAH(0) / 4] = 0x5634;
    e1000_bar_base[E1000_RAH(0) / 4] |= E1000_RAH_AV;

    /* Test tx_packet transmit*/
    /* Debugging
        char d='a';
        E1000_tx_packet(&d, 1);
    */
    return 0;
}

int
E1000_tx_packet(char *data, int num_bytes)
{
    
    int tail = e1000_bar_base[E1000_TDT / 4];
    
    if (((tx_descriptors[tail].status) & (E1000_TXD_STAT_DD)) == 0) 
    {
		return -1;
	}        
    
    memcpy(tx_bufs[tail], data, num_bytes);

    tx_descriptors[tail].addr = PADDR(tx_bufs[tail]);
	tx_descriptors[tail].length = num_bytes;
	tx_descriptors[tail].cso = 0;
	tx_descriptors[tail].cmd = E1000_TXD_CMD_RS | E1000_TXD_CMD_EOP;    
	tx_descriptors[tail].status = 0;

    e1000_bar_base[E1000_TDT / 4] = (tail+1) % E1000_TX_DESCS;

    return 0;
}

int
E1000_rx_packet(char *buf, size_t max_size)
{
    if (!(rx_descriptors[head].status & E1000_RXD_STAT_DD)) 
    {
		return -1;
	}

	size_t num_bytes = MIN(max_size, rx_descriptors[head].length);

	memcpy(buf, rx_bufs[head], num_bytes);

	head = (head + 1) % E1000_RX_DESCS;

	int tail = (e1000_bar_base[E1000_RDT / 4] + 1) % E1000_RX_DESCS;
	e1000_bar_base[E1000_RDT / 4] = tail;

	return num_bytes;
}
    

