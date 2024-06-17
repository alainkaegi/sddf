#pragma once

#include <microkit.h>
#include <sddf/network/queue.h>

#define NUM_CLIENTS 1

#define ETH_NAME "eth"

#define DATA_REGION_SIZE                    0x200000
#define HW_REGION_SIZE                      0x10000

#define MAC_ADDR_ARP                        0xFFFFFFFFFFFF

#if defined(CONFIG_PLAT_ODROIDC4)
#define MAC_ADDR                            0x525401000003
#else
#error "Must define MAC addresses for clients in ethernet config"
#endif

#define TX_QUEUE_SIZE_DRIV                   512
#define RX_QUEUE_SIZE_DRIV                   512
#define RX_DATA_REGION_SIZE_DRIV            DATA_REGION_SIZE

_Static_assert(RX_DATA_REGION_SIZE_DRIV >= RX_QUEUE_SIZE_DRIV *NET_BUFFER_SIZE,
               "Driver RX data region size must fit Driver RX buffers");

#define MAX(a,b) (((a) > (b)) ? (a) : (b))

#define ETH_MAX_QUEUE_SIZE MAX(TX_QUEUE_SIZE_DRIV, RX_QUEUE_SIZE_DRIV)
_Static_assert(sizeof(net_queue_t) + ETH_MAX_QUEUE_SIZE *sizeof(net_buff_desc_t) <= DATA_REGION_SIZE,
               "net_queue_t must fit into a single data region.");

static inline bool __str_match(const char *s0, const char *s1)
{
    while (*s0 != '\0' && *s1 != '\0' && *s0 == *s1) {
        s0++, s1++;
    }
    return *s0 == *s1;
}

static void __set_mac_addr(uint8_t *mac, uint64_t val)
{
    mac[0] = val >> 40 & 0xff;
    mac[1] = val >> 32 & 0xff;
    mac[2] = val >> 24 & 0xff;
    mac[3] = val >> 16 & 0xff;
    mac[4] = val >> 8 & 0xff;
    mac[5] = val & 0xff;
}

static inline void mem_region_init_sys(char *pd_name, uintptr_t *mem_regions, uintptr_t start_region)
{
    mem_regions[0] = start_region;
    mem_regions[1] = start_region + DATA_REGION_SIZE;
}
