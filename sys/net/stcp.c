#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/time.h>
#include <errno.h>

// TCP Header Structure - this is the real deal, byte-perfect
typedef struct {
    uint16_t src_port;      // Source port
    uint16_t dst_port;      // Destination port
    uint32_t seq_num;       // Sequence number
    uint32_t ack_num;       // Acknowledgment number
    uint8_t  data_offset:4; // Data offset (header length in 32-bit words)
    uint8_t  reserved:3;    // Reserved bits
    uint8_t  ns:1;          // ECN-nonce concealment protection
    uint8_t  cwr:1;         // Congestion Window Reduced
    uint8_t  ece:1;         // ECN-Echo
    uint8_t  urg:1;         // Urgent pointer field significant
    uint8_t  ack:1;         // Acknowledgment field significant
    uint8_t  psh:1;         // Push function
    uint8_t  rst:1;         // Reset the connection
    uint8_t  syn:1;         // Synchronize sequence numbers
    uint8_t  fin:1;         // No more data from sender
    uint16_t window;        // Window size
    uint16_t checksum;      // Checksum
    uint16_t urgent_ptr;    // Urgent pointer
} __attribute__((packed)) tcp_header_t;

// IP Header - we need this for the pseudo header in checksum calculation
typedef struct {
    uint8_t  version:4;     // IP version
    uint8_t  ihl:4;         // Internet header length
    uint8_t  tos;           // Type of service
    uint16_t tot_len;       // Total length
    uint16_t id;            // Identification
    uint16_t frag_off;      // Fragment offset
    uint8_t  ttl;           // Time to live
    uint8_t  protocol;      // Protocol
    uint16_t check;         // Header checksum
    uint32_t saddr;         // Source address
    uint32_t daddr;         // Destination address
} __attribute__((packed)) ip_header_t;

// TCP Connection State - the heart of TCP's state machine
typedef enum {
    TCP_CLOSED = 0,
    TCP_LISTEN,
    TCP_SYN_SENT,
    TCP_SYN_RECV,
    TCP_ESTABLISHED,
    TCP_FIN_WAIT1,
    TCP_FIN_WAIT2,
    TCP_CLOSE_WAIT,
    TCP_CLOSING,
    TCP_LAST_ACK,
    TCP_TIME_WAIT
} tcp_state_t;

// Our TCP Connection Control Block - this is where the magic happens
typedef struct {
    int raw_socket;         // Raw socket for sending packets
    uint32_t local_ip;      // Local IP address
    uint32_t remote_ip;     // Remote IP address
    uint16_t local_port;    // Local port
    uint16_t remote_port;   // Remote port
    
    // Sequence number management - the soul of TCP reliability
    uint32_t send_seq;      // Next sequence number to send
    uint32_t send_ack;      // Next acknowledgment number to send
    uint32_t recv_seq;      // Next sequence number expected to receive
    uint32_t recv_ack;      // Last acknowledgment received
    
    // Window management - flow control
    uint16_t send_window;   // Send window size
    uint16_t recv_window;   // Receive window size
    
    tcp_state_t state;      // Current connection state
    
    // Timers and retransmission - the pain of reliable delivery
    struct timeval last_sent;
    int retransmit_count;
} tcp_connection_t;

// Internet checksum calculation - this algorithm is older than most programmers
uint16_t calculate_checksum(void *data, int len) {
    uint16_t *buf = (uint16_t *)data;
    uint32_t sum = 0;
    
    // Sum all 16-bit words
    while (len > 1) {
        sum += *buf++;
        len -= 2;
    }
    
    // Add left-over byte, if any
    if (len == 1) {
        sum += *(uint8_t*)buf << 8;
    }
    
    // Add carry bits and fold to 16 bits
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    
    return ~sum;
}

// TCP checksum with pseudo header - because TCP lives on top of IP
uint16_t tcp_checksum(tcp_header_t *tcp_hdr, uint32_t src_ip, uint32_t dst_ip, 
                      uint8_t *data, int data_len) {
    // Pseudo header for checksum calculation
    struct {
        uint32_t src_ip;
        uint32_t dst_ip;
        uint8_t  zero;
        uint8_t  protocol;
        uint16_t tcp_len;
    } __attribute__((packed)) pseudo_hdr;
    
    pseudo_hdr.src_ip = src_ip;
    pseudo_hdr.dst_ip = dst_ip;
    pseudo_hdr.zero = 0;
    pseudo_hdr.protocol = IPPROTO_TCP;
    pseudo_hdr.tcp_len = htons(sizeof(tcp_header_t) + data_len);
    
    // Calculate checksum over pseudo header + TCP header + data
    uint32_t sum = 0;
    uint16_t *buf;
    int len;
    
    // Pseudo header
    buf = (uint16_t *)&pseudo_hdr;
    len = sizeof(pseudo_hdr);
    while (len > 1) {
        sum += *buf++;
        len -= 2;
    }
    
    // TCP header (with checksum field zeroed)
    tcp_hdr->checksum = 0;
    buf = (uint16_t *)tcp_hdr;
    len = sizeof(tcp_header_t);
    while (len > 1) {
        sum += *buf++;
        len -= 2;
    }
    
    // Data
    if (data && data_len > 0) {
        buf = (uint16_t *)data;
        len = data_len;
        while (len > 1) {
            sum += *buf++;
            len -= 2;
        }
        if (len == 1) {
            sum += *(uint8_t*)buf << 8;
        }
    }
    
    // Fold and complement
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    
    return ~sum;
}

// Create a raw socket - this is where we bypass the kernel's TCP stack
int create_raw_socket() {
    int sock = socket(AF_INET, SOCK_RAW, IPPROTO_TCP);
    if (sock < 0) {
        perror("Failed to create raw socket (run as root!)");
        return -1;
    }
    
    // Tell the kernel we'll handle IP headers ourselves
    int one = 1;
    if (setsockopt(sock, IPPROTO_IP, IP_HDRINCL, &one, sizeof(one)) < 0) {
        perror("Failed to set IP_HDRINCL");
        close(sock);
        return -1;
    }
    
    return sock;
}

// Send a TCP packet - the fundamental operation
int send_tcp_packet(tcp_connection_t *conn, uint8_t flags, uint8_t *data, int data_len) {
    char packet[4096];  // Should be enough for most packets
    ip_header_t *ip_hdr = (ip_header_t *)packet;
    tcp_header_t *tcp_hdr = (tcp_header_t *)(packet + sizeof(ip_header_t));
    
    // Build IP header - the envelope for our TCP letter
    memset(ip_hdr, 0, sizeof(ip_header_t));
    ip_hdr->version = 4;
    ip_hdr->ihl = 5;  // 20 bytes
    ip_hdr->tos = 0;
    ip_hdr->tot_len = htons(sizeof(ip_header_t) + sizeof(tcp_header_t) + data_len);
    ip_hdr->id = htons(rand());
    ip_hdr->frag_off = 0;
    ip_hdr->ttl = 64;
    ip_hdr->protocol = IPPROTO_TCP;
    ip_hdr->saddr = conn->local_ip;
    ip_hdr->daddr = conn->remote_ip;
    ip_hdr->check = 0;
    ip_hdr->check = calculate_checksum(ip_hdr, sizeof(ip_header_t));
    
    // Build TCP header - this is where the TCP magic happens
    memset(tcp_hdr, 0, sizeof(tcp_header_t));
    tcp_hdr->src_port = htons(conn->local_port);
    tcp_hdr->dst_port = htons(conn->remote_port);
    tcp_hdr->seq_num = htonl(conn->send_seq);
    tcp_hdr->ack_num = htonl(conn->send_ack);
    tcp_hdr->data_offset = 5;  // 20 bytes, no options
    
    // Set flags - the control bits that make TCP work
    tcp_hdr->syn = (flags & 0x02) ? 1 : 0;
    tcp_hdr->ack = (flags & 0x10) ? 1 : 0;
    tcp_hdr->fin = (flags & 0x01) ? 1 : 0;
    tcp_hdr->rst = (flags & 0x04) ? 1 : 0;
    tcp_hdr->psh = (flags & 0x08) ? 1 : 0;
    
    tcp_hdr->window = htons(conn->recv_window);
    tcp_hdr->urgent_ptr = 0;
    
    // Copy data if any
    if (data && data_len > 0) {
        memcpy(packet + sizeof(ip_header_t) + sizeof(tcp_header_t), data, data_len);
    }
    
    // Calculate TCP checksum
    tcp_hdr->checksum = tcp_checksum(tcp_hdr, conn->local_ip, conn->remote_ip, data, data_len);
    
    // Send the packet
    struct sockaddr_in dest;
    dest.sin_family = AF_INET;
    dest.sin_addr.s_addr = conn->remote_ip;
    dest.sin_port = 0;  // Ignored for raw sockets
    
    int packet_len = sizeof(ip_header_t) + sizeof(tcp_header_t) + data_len;
    int sent = sendto(conn->raw_socket, packet, packet_len, 0, 
                      (struct sockaddr *)&dest, sizeof(dest));
    
    if (sent < 0) {
        perror("Failed to send packet");
        return -1;
    }
    
    // Update sequence number if we sent data or SYN/FIN
    if (data_len > 0 || (flags & 0x03)) {
        conn->send_seq += data_len + ((flags & 0x03) ? 1 : 0);
    }
    
    gettimeofday(&conn->last_sent, NULL);
    
    printf("Sent TCP packet: seq=%u, ack=%u, flags=0x%02x, len=%d\n",
           ntohl(tcp_hdr->seq_num), ntohl(tcp_hdr->ack_num), flags, data_len);
    
    return sent;
}

// TCP three-way handshake initiation - the beginning of every TCP connection
int tcp_connect(tcp_connection_t *conn, const char *dest_ip, uint16_t dest_port) {
    // Initialize connection
    conn->remote_ip = inet_addr(dest_ip);
    conn->remote_port = dest_port;
    conn->local_port = 12345 + (rand() % 10000);  // Random local port
    conn->send_seq = rand();  // Random initial sequence number
    conn->recv_window = 65535;  // Maximum window size
    conn->state = TCP_SYN_SENT;
    
    printf("Initiating TCP connection to %s:%d\n", dest_ip, dest_port);
    
    // Send SYN packet
    return send_tcp_packet(conn, 0x02, NULL, 0);  // SYN flag
}

// Process received TCP packet - the state machine heart of TCP
int process_tcp_packet(tcp_connection_t *conn, tcp_header_t *tcp_hdr, uint8_t *data, int data_len) {
    uint32_t seq = ntohl(tcp_hdr->seq_num);
    uint32_t ack = ntohl(tcp_hdr->ack_num);
    
    printf("Received TCP packet: seq=%u, ack=%u, flags=0x%02x, len=%d\n",
           seq, ack, *(uint8_t *)&tcp_hdr->fin, data_len);
    
    // State machine - this is where decades of TCP experience shows
    switch (conn->state) {
        case TCP_SYN_SENT:
            if (tcp_hdr->syn && tcp_hdr->ack) {
                // SYN-ACK received, complete handshake
                conn->recv_seq = seq + 1;
                conn->send_ack = seq + 1;
                conn->recv_ack = ack;
                conn->state = TCP_ESTABLISHED;
                
                // Send ACK to complete three-way handshake
                send_tcp_packet(conn, 0x10, NULL, 0);  // ACK flag
                printf("TCP connection established!\n");
            }
            break;
            
        case TCP_ESTABLISHED:
            if (data_len > 0) {
                // Data received, send ACK
                conn->recv_seq = seq + data_len;
                conn->send_ack = seq + data_len;
                send_tcp_packet(conn, 0x10, NULL, 0);  // ACK flag
                
                printf("Received data: %.*s\n", data_len, data);
            }
            
            if (tcp_hdr->fin) {
                // FIN received, start closing
                conn->recv_seq = seq + 1;
                conn->send_ack = seq + 1;
                conn->state = TCP_CLOSE_WAIT;
                
                // Send ACK for FIN
                send_tcp_packet(conn, 0x10, NULL, 0);  // ACK flag
                
                // Send FIN to close our side
                send_tcp_packet(conn, 0x11, NULL, 0);  // FIN+ACK flags
                conn->state = TCP_LAST_ACK;
            }
            break;
            
        case TCP_LAST_ACK:
            if (tcp_hdr->ack) {
                // Final ACK received, connection closed
                conn->state = TCP_CLOSED;
                printf("TCP connection closed\n");
            }
            break;
            
        default:
            break;
    }
    
    return 0;
}

// Main TCP event loop - where we wait for packets and handle them
int tcp_run(tcp_connection_t *conn) {
    char buffer[4096];
    struct sockaddr_in src;
    socklen_t src_len = sizeof(src);
    
    while (conn->state != TCP_CLOSED && conn->state != TCP_TIME_WAIT) {
        // Set timeout for receive
        fd_set read_fds;
        struct timeval timeout;
        FD_ZERO(&read_fds);
        FD_SET(conn->raw_socket, &read_fds);
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        
        int ready = select(conn->raw_socket + 1, &read_fds, NULL, NULL, &timeout);
        
        if (ready > 0 && FD_ISSET(conn->raw_socket, &read_fds)) {
            int len = recvfrom(conn->raw_socket, buffer, sizeof(buffer), 0,
                              (struct sockaddr *)&src, &src_len);
            
            if (len > 0) {
                ip_header_t *ip_hdr = (ip_header_t *)buffer;
                
                // Check if this packet is for us
                if (ip_hdr->protocol == IPPROTO_TCP &&
                    ip_hdr->saddr == conn->remote_ip &&
                    ip_hdr->daddr == conn->local_ip) {
                    
                    tcp_header_t *tcp_hdr = (tcp_header_t *)(buffer + (ip_hdr->ihl * 4));
                    
                    if (ntohs(tcp_hdr->src_port) == conn->remote_port &&
                        ntohs(tcp_hdr->dst_port) == conn->local_port) {
                        
                        int tcp_hdr_len = tcp_hdr->data_offset * 4;
                        int data_len = len - (ip_hdr->ihl * 4) - tcp_hdr_len;
                        uint8_t *data = (uint8_t *)tcp_hdr + tcp_hdr_len;
                        
                        process_tcp_packet(conn, tcp_hdr, data, data_len);
                    }
                }
            }
        }
        
        // Handle timeouts and retransmissions here in a real implementation
        // For now, we'll just continue
    }
    
    return 0;
}

// Initialize TCP connection structure
tcp_connection_t *tcp_init() {
    tcp_connection_t *conn = malloc(sizeof(tcp_connection_t));
    if (!conn) return NULL;
    
    memset(conn, 0, sizeof(tcp_connection_t));
    
    conn->raw_socket = create_raw_socket();
    if (conn->raw_socket < 0) {
        free(conn);
        return NULL;
    }
    
    // Get local IP address (simplified - assumes first interface)
    conn->local_ip = inet_addr("127.0.0.1");  // Should be dynamic in real implementation
    conn->state = TCP_CLOSED;
    
    return conn;
}

// Send data over established TCP connection
int tcp_send(tcp_connection_t *conn, uint8_t *data, int len) {
    if (conn->state != TCP_ESTABLISHED) {
        printf("Connection not established\n");
        return -1;
    }
    
    return send_tcp_packet(conn, 0x18, data, len);  // PSH+ACK flags
}

// Clean up TCP connection
void tcp_close(tcp_connection_t *conn) {
    if (conn->state == TCP_ESTABLISHED) {
        // Send FIN to close connection
        send_tcp_packet(conn, 0x11, NULL, 0);  // FIN+ACK flags
        conn->state = TCP_FIN_WAIT1;
        
        // Wait for close handshake to complete
        tcp_run(conn);
    }
    
    if (conn->raw_socket >= 0) {
        close(conn->raw_socket);
    }
    
    free(conn);
}

// Example usage - a simple TCP client
int main() {
    printf("TCP Implementation from Scratch\n");
    printf("===============================\n");
    printf("Note: This requires root privileges to create raw sockets\n\n");
    
    tcp_connection_t *conn = tcp_init();
    if (!conn) {
        printf("Failed to initialize TCP\n");
        return 1;
    }
    
    // Connect to a server (change IP as needed)
    if (tcp_connect(conn, "127.0.0.1", 80) < 0) {
        printf("Failed to initiate connection\n");
        tcp_close(conn);
        return 1;
    }
    
    // Run the TCP state machine
    tcp_run(conn);
    
    // Send some data if connected
    if (conn->state == TCP_ESTABLISHED) {
        char *http_request = "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n";
        tcp_send(conn, (uint8_t *)http_request, strlen(http_request));
        
        // Wait for response
        sleep(2);
    }
    
    // Clean up
    tcp_close(conn);
    
    return 0;
}
