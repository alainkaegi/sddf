#include <stdbool.h>
#include <stdint.h>
#include <microkit.h>
#include <sddf/util/util.h>
#include <sddf/network/queue.h>
#include <ethernet_config.h>

#define DRIVER_CH 0

uintptr_t rx_free;
uintptr_t rx_active;

uintptr_t rx_buffer_data_vaddr;
uintptr_t rx_buffer_data_paddr;

uintptr_t uart_base;

typedef struct state {
    net_queue_handle_t rx_queue;
} state_t;

state_t state;

static void put_uint(unsigned int v) {
    if (v == 0) {
        microkit_dbg_putc('0');
        return;
    }

    int l = 0;
    unsigned vv = v;
    while (vv != 0) {
        vv = vv/10;
        l++;
    }

    char buf[10];
    char *p = buf + l - 1;
    
    while (v != 0) {
        *p-- = "0123456789"[v % 10];
        v = v/10;
    }

    for (int i = 0; i < l; ++i) {
        microkit_dbg_putc(buf[i]);
    }
}

static void put_byte(uint8_t b) {
    microkit_dbg_putc("0123456789ABCDEF"[(b & 0xF0) >> 4]);
    microkit_dbg_putc("0123456789ABCDEF"[b & 0x0F]);
    return;
}

static void dumpp(void *packet, unsigned int packet_len) {
    unsigned char *data = (unsigned char *) packet;
    for (int i = 0; i < packet_len; ++i) {
        if (i % 16 == 0) {
            put_byte(i);
            microkit_dbg_putc('\t');
        } else if (i % 4 == 0) {
            microkit_dbg_putc(' ');
        }
        put_byte(data[i]);
        if (i % 16 == 15) {
            microkit_dbg_puts("\r\n");
        }
    }

    microkit_dbg_puts("\r\n");
}

void receive(void)
{
    int err;
    net_buff_desc_t buffer;

    unsigned int n = net_queue_size(state.rx_queue.active);
    microkit_dbg_puts("Receiving...(");
    put_uint(n);
    microkit_dbg_puts(")\r\n");
    if (n == 0) {
        // Require signaling from the producer
        net_request_signal_active(&state.rx_queue);

        microkit_dbg_puts("Done receiving...\r\n");
        return;
    }

    while (1) {
        // Dequeue a buffer; return if unsuccessful.
        if (net_dequeue_active(&state.rx_queue, &buffer) == -1) {
            // Require signaling from the producer
            net_request_signal_active(&state.rx_queue);

            microkit_dbg_puts("Done receiving...\r\n");
            return;
        }

        // Flush stale cache entries corresponding to the addresses of
        // the buffer content before reading the content of said
        // buffer.  It seems that what is needed is just an
        // "invalidate".  Unfortunately on ARM, user-space code can
        // only do "clean and invalidate" (i.e., write back dirty
        // cache lines which would overwrite data written by the DMA
        // engine).  At this stage, the consumer (this file) is not
        // modifying data so it should never have any dirty cache line
        // corresponding to the content of the buffer.  In other
        // words, for now, "clean" is safe.  The sddf developers
        // mention other ways to perform this task (see comments in
        // virt_rx.c), but they found this way to be fastest.
        cache_clean_and_invalidate(rx_buffer_data_vaddr - rx_buffer_data_paddr + buffer.io_or_offset,
                                   rx_buffer_data_vaddr - rx_buffer_data_paddr + buffer.io_or_offset + ROUND_UP(buffer.len, 1 << CONFIG_L1_CACHE_LINE_SIZE_BITS));

        // Dump the packet
        //dumpp(rx_buffer_data_vaddr + buffer.io_or_offset, buffer.len);
        dumpp(rx_buffer_data_vaddr - rx_buffer_data_paddr + buffer.io_or_offset, buffer.len);
        buffer.len = 0;

        // Return the buffer to the free queue.  Should succeed.
        err = net_enqueue_free(&state.rx_queue, buffer);
        assert(!err);
    }
}

void init(void)
{
    microkit_dbg_puts("recv: init\r\n");

    // Initialize our view of the queue (both sides must do this).
    net_queue_init(&state.rx_queue,
                   (net_queue_t *)rx_free,
                   (net_queue_t *)rx_active,
                   RX_QUEUE_SIZE_DRIV);
    net_buffers_init(&state.rx_queue, rx_buffer_data_paddr);

    microkit_notify(DRIVER_CH);
}

void notified(microkit_channel ch)
{
    switch (ch) {
    case DRIVER_CH:
        receive();
        break;
    default:
        microkit_dbg_puts("recv: received notification on unexpected channel: ");
        put_uint(ch);
        microkit_dbg_puts("\r\n");
        break;
    }
}
