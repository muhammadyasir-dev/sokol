#include "lwip/udp.h"
#include "lwip/ip.h"
#include "lwip/ip_addr.h"
#include "lwip/mem.h"
#include "lwip/pbuf.h"

void send_udp_packet(ip_addr_t *dest_ip, u16_t dest_port, const char *data, u16_t data_len) {
    struct udp_pcb *pcb;
    struct pbuf *p;

    // Create new UDP PCB
    pcb = udp_new();
    if (pcb == NULL) {
        // Handle error
        return;
    }

    // Allocate pbuf
    p = pbuf_alloc(PBUF_TRANSPORT, data_len, PBUF_RAM);
    if (p == NULL) {
        // Handle error
        udp_remove(pcb);
        return;
    }

    // Copy data into pbuf
    memcpy(p->payload, data, data_len);

    // Send UDP packet
    udp_sendto(pcb, p, dest_ip, dest_port);

    // Free pbuf
    pbuf_free(p);

    // Remove PCB
    udp_remove(pcb);
}

