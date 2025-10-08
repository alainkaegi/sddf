#pragma once

// Requires:
// - stdint.h

/**
 * A blocking circular fixed-size queue of 64-bit quantities.
 *
 * This implementation stores the elements of the queue in an array
 * embedded at the end of the queue state.
 *
 * The instance variable {@code capacity} stores the size of the queue
 * as the maximum number of elements it can hold.
 *
 * Elements are inserted in and removed from the queue in order of
 * arrival (first in, first out queuing discipline).  If an element is
 * inserted at slot i in the array, then the next element is inserted
 * at slot (i + 1) modulo {@code capacity}.
 *
 * The variables {@code in} and {@code out} track where the next
 * element should be inserted in and retrieved from, respectively, the
 * array.
 *
 * The instance variable {@code count} tracks the number elements
 * currently in the queue.
 *
 * If {@code in} is equal to {@code out}, the queue is either empty or
 * full.  {@code count} discriminates between the two cases.
 *
 * {@code queue} stores the start of the array holding the content of
 * the queue data.  Starting at that location, there should be enough
 * memory to hold {@code capacity} times 8 bytes.  One possible way to
 * allocate this memory is to invoke {@code malloc} as follows:
 *
 * {@code malloc(sizeof(struct queue) + capacity*8)}
 *
 * There is currently no enforcement of alignment to improve access
 * performance.
 */
struct dep_queue {
    unsigned int capacity;       ///< queue capacity (in number of elements)
    volatile unsigned int count; ///< number of queued elements
    unsigned int in;             ///< enqueue point
    unsigned int out;            ///< dequeue point
    int lock;                    ///< lock to enforce mutual exclusion
    uint64_t queue[0];           ///< beginning of array holding queue data
};

/**
 * Initialize a circular queue.
 *
 * O(1).
 *
 * The allocation of the queue structure must include enough memory
 * beyond the last instance variable to hold the actual queue state.
 * That additional amount of memory must be large enough to hold
 * {@code capacity} times 8 bytes.
 *
 * @param q  pointer to memory area large enough to hold queue state and data
 * @param capacity  the maximum number of elements the queue can hold
 */
void dep_queue_init(struct dep_queue *q, unsigned int capacity);

/**
 * Enqueue an element into the queue.
 *
 * A call to this function will block, until the queue is non-full.
 *
 * O(1).
 *
 * @param q  the pointer to a circular queue structure
 * @param e  the pointer to an element to be inserted in the queue
 */
void dep_queue_enqueue(struct dep_queue *q, uint64_t e);

/**
 * Dequeue an element from the queue.
 *
 * A call to this function will block, until the queue is non-empty.
 *
 * O(1).
 *
 * @param q  the pointer to a circular queue structure
 * @return  the element removed from the queue
 */
uint64_t dep_queue_dequeue(struct dep_queue *q);

/**
 * Query a queue to determine if it is empty. This can be used to avoid blocking
 * on calls to enqueue and dequeue functions.
 *
 * @param q  the pointer to the circular queue structure
 * @return  non-zero if {@code q} is empty
 */
uint8_t dep_queue_is_empty(struct dep_queue *q);
