#include <stdbool.h>
#include <stdint.h>
#include <microkit.h>
#include <sddf/util/util.h>
#include <sddf/util/printf.h>
#include <sddf/network/queue.h>

// Channel used to notified the receiver that the sender has completed
// its initialization (and therefore the transmit queue is ready to be
// used.
const microkit_channel TO_RECV = 2;

uintptr_t tx_free;
uintptr_t tx_active;
uintptr_t tx_buffer_data;
uintptr_t uart_base;

typedef struct state {
    net_queue_handle_t tx_queue;
} state_t;

state_t state;

static void send(int i)
{
    net_buff_desc_t buffer;

    // Wait for a free buffer to become available.
    while (net_queue_empty_free(&state.tx_queue)) {
        // nothing
    }

    assert(!net_queue_empty_free(&state.tx_queue));

    int err = net_dequeue_free(&state.tx_queue, &buffer);
    assert(!err);

    *(int *)(buffer.io_or_offset + tx_buffer_data) = i;
    buffer.len = sizeof(int);

    err = net_enqueue_active(&state.tx_queue, buffer);
    assert(!err);
}

void transmit() {
    for (int i = 0; i < 10; ++i) {
        send(i);
    }
}

void init(void)
{
    sddf_dprintf("send: init\r\n");

    // Initialize our view of the queue (both sides must do this).
    net_queue_init(&state.tx_queue,
                   (net_queue_t *)tx_free,
                   (net_queue_t *)tx_active,
                   /* size */512);

    // Fill the free queue with buffers (only one participant should
    // do this).
    net_buffers_init(&state.tx_queue, 0);

    // Notify the receiver that initialization has completed.
    microkit_notify(TO_RECV);

    transmit();
}

void notified(microkit_channel ch)
{
    switch (ch) {
    default:
        sddf_dprintf("send: received notification on unexpected channel: %u\r\n", ch);
        break;
    }
}
