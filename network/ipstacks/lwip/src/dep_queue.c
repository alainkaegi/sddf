#include <stdint.h>
#include "dep_queue.h"

void dep_queue_init(struct dep_queue *q, unsigned int capacity) {
    q->capacity = capacity;
    q->count    = 0;
    q->in       = 0;
    q->out      = 0;
}

unsigned int increment_index(struct dep_queue *q, unsigned int i) {
    if (i + 1 == q->capacity) {
        return 0;
    } else {
        return i + 1;
    }
}
void dep_queue_enqueue(struct dep_queue *q, uint64_t e) {
    // wait for queue to become non-full
    while (q->capacity == q->count) {
        // do nothing
    }
    q->queue[q->in] = e;
    q->in           = increment_index(q, q->in);
    q->count++;
}

uint64_t dep_queue_dequeue(struct dep_queue *q) {
    // wait for queue to become non-empty
    while (q->count == 0) {
        // do nothing
    }
    uint64_t v = q->queue[q->out];
    q->out   = increment_index(q, q->out);
    q->count--;
    return v;
}

// TBD: use bool instead?
uint8_t dep_queue_is_empty(struct dep_queue *q) { return q->count == 0; }
