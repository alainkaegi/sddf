#pragma once

// Requires:
// - stdint.h (self, ip_addr.h)
// - inet.h (ip_addr.h)
// - ip_addr.h

/**
 * A socket defines a network transmission endpoint.
 */
struct socket {
    ip_addr ip_addr; ///< A host IP address
    uint16_t port;   ///< A host port
};
