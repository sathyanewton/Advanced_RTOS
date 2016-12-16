#ifndef JOS_KERN_E1000_H
#define JOS_KERN_E1000_H
#include <inc/memlayout.h>
#include <kern/pci.h>

#define PCI_VENDOR_ID (0x8086)
#define PCI_DEVICE_ID (0x100E)

/*
 *
 * Function Declarations
 */
// allocate space for descriptor ring and buffer
static void alloc_desc_buff();

// initialize registers
static void init_tx();
static void init_rx();
// attach function for PCI init call
int nic_init(struct pci_func *pcif);

//transmit a physical page 
int try_transmit(struct PageInfo *pp_user, size_t len);
int try_receive(struct PageInfo *pp_user);

void e1000_disp_tail();
// MAX buffer size possible as we need contigeous memory for 1 buffer
// as we can not use boot_alloc(), we can get only contigeous memory
// in multiples of PGSIZE
#define BUFF_SIZE 4096

#define NUMBER_OF_TX_DESC (64)
#define SIZE_OF_TX_DESC (16)
#define LENGTH_OF_TX_RING (NUMBER_OF_TX_DESC * SIZE_OF_TX_DESC)

#define NUMBER_OF_RX_DESC (128)
#define SIZE_OF_RX_DESC (16)
#define LENGTH_OF_RX_RING (NUMBER_OF_RX_DESC * SIZE_OF_RX_DESC)

//tx_desc
struct tx_desc
{
	uint64_t addr;
	uint16_t length;
	uint8_t cso;
	uint8_t cmd;
	uint8_t status;
	uint8_t css;
	uint16_t special;
};//size 16B

struct tx_buff
{
	char data[BUFF_SIZE];
};

/* Receive Descriptor */
struct e1000_rx_desc {
    uint64_t buffer_addr; /* Address of the descriptor's data buffer */
    uint16_t length;     /* Length of data DMAed into data buffer */
    uint16_t csum;       /* Packet checksum */
    uint8_t status;      /* Descriptor status */
    uint8_t errors;      /* Descriptor Errors */
    uint16_t special;
};

struct rx_buff
{
	char data[BUFF_SIZE];
};
/*
 * Macros for NIC
 */
//offests into the Main register
#define E1000_TDBAL    0x03800  /* TX Descriptor Base Address Low - RW */
#define E1000_TDBAH    0x03804  /* TX Descriptor Base Address High - RW */
#define E1000_TDLEN    0x03808  /* TX Descriptor Length - RW */
#define E1000_TDH      0x03810  /* TX Descriptor Head - RW */
#define E1000_TDT      0x03818  /* TX Descripotr Tail - RW */
#define E1000_TCTL     0x00400  /* TX Control - RW */
#define E1000_TIPG     0x00410  /* TX Inter-packet gap -RW */


//recieve
#define E1000_RDBAL    0x02800  /* RX Descriptor Base Address Low - RW */
#define E1000_RDBAH    0x02804  /* RX Descriptor Base Address High - RW */
#define E1000_RDLEN    0x02808  /* RX Descriptor Length - RW */
#define E1000_RA       0x05400  /* Receive Address - RW Array */
#define E1000_MTA      0x05200  /* Multicast Table Array - RW Array */
#define E1000_IMS      0x000D0  /* Interrupt Mask Set - RW */
#define E1000_RDH      0x02810  /* RX Descriptor Head - RW */
#define E1000_RDT      0x02818  /* RX Descriptor Tail - RW */
#define E1000_RCTL     0x00100  /* RX Control - RW */
#define E1000_RCTL_EN             0x00000002    /* enable */
#define E1000_RCTL_LPE            0x00000020    /* long packet enable */
#define E1000_RCTL_LBM_NO         0x00000000    /* no loopback mode */
#define E1000_RCTL_BAM            0x00008000    /* broadcast enable */
#define E1000_RCTL_BSEX           0x02000000    /* Buffer size extension */
#define E1000_RCTL_SZ_4096        0x00030000    /* rx buffer size 4096 */
#define E1000_RCTL_SECRC          0x04000000    /* Strip Ethernet CRC */
#define E1000_RAH_AV 0x80000000
#define E1000_RAH			 0x05404
#define E1000_RAL			 0x05400




#define E1000_RXD_STAT_DD       0x01    /* Descriptor Done */

/* Transmit Control E1000_TCTL bits*/
#define E1000_TCTL_RST    0x00000001    /* software reset */
#define E1000_TCTL_EN     0x00000002    /* enable tx */
#define E1000_TCTL_BCE    0x00000004    /* busy check enable */
#define E1000_TCTL_PSP    0x00000008    /* pad short packets */
#define E1000_TCTL_CT     0x00000ff0    /* collision threshold */
#define E1000_TCTL_COLD   0x003ff000    /* collision distance */
#define E1000_TCTL_SWXOFF 0x00400000    /* SW Xoff transmission */
#define E1000_TCTL_PBE    0x00800000    /* Packet Burst Enable */
#define E1000_TCTL_RTLC   0x01000000    /* Re-transmit on late collision */
#define E1000_TCTL_NRTU   0x02000000    /* No Re-transmit on underrun */
#define E1000_TCTL_MULR   0x10000000    /* Multiple request support */

/* Transmit Descriptor bit definitions */
#define E1000_TXD_STAT_DD    0x00000001 /* Descriptor Done */
#define E1000_TXD_CMD_RS     0x08000000 /* Report Status */


// error code for e1000 driver
#define E_BUF_FULL (1)

#endif	// JOS_KERN_E1000_H
