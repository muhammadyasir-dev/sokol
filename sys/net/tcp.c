#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <arpa/inet.h>

// TCP Header Structure
struct tcp_header {
    uint16_t source_port;      // Source port
    uint16_t dest_port;        // Destination port
    uint32_t seq_num;          // Sequence number
    uint32_t ack_num;          // Acknowledgment number
    uint8_t  data_offset;      // Data offset (4 bits) + reserved (4 bits)
    uint8_t  flags;            // TCP flags (e.g., SYN, ACK, FIN)
    uint16_t window;           // Window size
    uint16_t checksum;         // Checksum
    uint16_t urgent_ptr;       // Urgent pointer
};

// IP Header Structure (IPv4)
struct ip_header {
    uint8_t  version_ihl;      // Version (4 bits) + IHL (4 bits)
    uint8_t  tos;              // Type of Service
    uint16_t total_length;     // Total length
    uint16_t identification;   // Identification
    uint16_t flags_fragment;   // Flags (3 bits) + Fragment Offset (13 bits)
    uint8_t  ttl;              // Time to Live
    uint8_t  protocol;         // Protocol (TCP = 6)
    uint16_t checksum;         // Header checksum
    uint32_t source_ip;        // Source IP address
    uint32_t dest_ip;          // Destination IP address
};

// TCP Flags
#define TCP_FLAG_SYN 0x02
#define TCP_FLAG_ACK 0x10
#define TCP_FLAG_FIN 0x01

// Function to calculate checksum
uint16_t calculate_checksum(void *data, int length) {
    uint32_t sum = 0;
    uint16_t *ptr = (uint16_t *)data;
    
    while (length > 1) {
        sum += *ptr++;
        length -= 2;
    }
    
    if (length > 0) {
        sum += *(uint8_t *)ptr;
    }
    
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    
    return ~sum;
}

// Function to initialize TCP header
void init_tcp_header(struct tcp_header *tcp, uint16_t src_port, uint16_t dst_port, uint32_t seq, uint8_t flags) {
    tcp->source_port = htons(src_port);
    tcp->dest_port = htons(dst_port);
    tcp->seq_num = htonl(seq);
    tcp->ack_num = 0;
    tcp->data_offset = (sizeof(struct tcp_header) / 4) << 4; // Data offset in 32-bit words
    tcp->flags = flags;
    tcp->window = htons(65535);
    tcp->checksum = 0;
    tcp->urgent_ptr = 0;
}

// Function to initialize IP header
void init_ip_header(struct ip_header *ip, uint32_t src_ip, uint32_t dst_ip) {
    ip->version_ihl = (4 << 4) | (sizeof0070X1stword: create
    ip->tos = 0;
    ip->total_length = htons(sizeof(struct ip_header) + sizeof(struct tcp_header));
    ip->identification = htons(12345);
    ip->flags_fragment = 0;
    ip->ttl = 64;
    ip->protocol = 6; // TCP
    ip->checksum = 0;
    ip->source_ip = inet_addr("192.168.1.100");
    ip->dest_ip = inet_addr("192.168.1.200");
}

// Simulate TCP 3-way handshake (SYN, SYN-ACK, ACK)
void tcp_handshake() {
    struct tcp_header tcp;
    struct ip_header ip;
    
    // Step 1: Client sends SYN
    init_ip_header(&ip, inet_addr("192.168.1.100"), inet_addr("192.168.1.200"));
    init_tcp_header(&tcp, 12345, 80, 1000, TCP_FLAG_SYN);
    printf("Client: Sending SYN\n");
    
    // Step 2: Server responds with SYN-ACK
    init_ip_header(&ip, inet_addr("192.168.1.200"), inet_addr("192.168.1.100"));
    init_tcp_header(&tcp, 80, 12345, 2000, TCP_FLAG_SYN | TCP_FLAG_ACK);
    tcp.ack_num = htonl(1001);
    printf("Server: Sending SYN-ACK\n");
    
    // Step 3: Client sends ACK
    init_ip_header(&ip, inet_addr("192.168.1.100"), inet_addr("192.168.1.200"));
    init_tcp_header(&tcp, 12345, 80, 1001, TCP_FLAG_ACK);
    tcp.ack_num = htonl(2001);
    printf("Client: Sending ACK\n");
    
    printf("TCP Handshake Completed\n");
}

int main() {
    tcp_handshake();
    return 0;
}
