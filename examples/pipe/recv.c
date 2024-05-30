#include <stdbool.h>
#include <stdint.h>
#include <microkit.h>
#include <sddf/util/util.h>
#include <sddf/util/printf.h>
#include <sddf/network/queue.h>

const microkit_channel FROM_SEND = 2;

uintptr_t tx_free;
uintptr_t tx_active;
uintptr_t tx_buffer_data;
uintptr_t uart_base;

volatile bool send_init_done = false;

typedef struct state {
    net_queue_handle_t tx_queue;
} state_t;

state_t state;

void receive(void)
{
    net_buff_desc_t buffer;

    for (int j = 0; j < 10; ++j) {
        // Wait for the queue to become non-empty.
        while (net_queue_empty_active(&state.tx_queue)) {
            // nothing
        }

        assert(!net_queue_empty_active(&state.tx_queue));

        int err = net_dequeue_active(&state.tx_queue, &buffer);
        assert(!err);

        int i = *(int *)(buffer.io_or_offset + tx_buffer_data);
        sddf_dprintf("Received %d\r\n", i);
        buffer.len = 0;

        err = net_enqueue_free(&state.tx_queue, buffer);
        assert(!err);
    }
}

void init(void)
{
    sddf_dprintf("recv: init\r\n");

    // Initialize our view of the queue (both sides must do this).
    net_queue_init(&state.tx_queue,
                   (net_queue_t *)tx_free,
                   (net_queue_t *)tx_active,
                   /* size */512);

    // The following code does not work. We never receive the
    // notification.
    //while (!send_init_done) {
    //    // nothing
    //}
    //sddf_dprintf("recv: done waiting\r\n");
    //receive();
}

void notified(microkit_channel ch)
{
    switch (ch) {
    case FROM_SEND:
        send_init_done = true;
        receive();
        break;
    default:
        sddf_dprintf("recv: received notification on unexpected channel: %u\r\n", ch);
        break;
    }
}
