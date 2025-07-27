# CSE406 Network Security Project 1

## Overview

This project implements various network security tools and attack simulations for educational purposes in cybersecurity research. The project consists of three main components: a packet flooding tool, a Ping of Death (PoD) attack implementation, and a network monitoring/logging utility.

⚠️ **DISCLAIMER: This project is for educational and research purposes only. Use these tools only on networks you own or have explicit permission to test. Unauthorized network attacks are illegal and unethical.**

## Components

### 1. Packet Flooding Tool (`pf.cpp`)
A high-performance network flooding tool that can generate massive amounts of network traffic for stress testing.

**Features:**
- Multi-threaded packet generation for maximum throughput
- Pre-generated packet pools for efficiency
- Support for various packet types (ICMP, TCP, UDP)
- Real-time statistics and bandwidth monitoring
- Configurable thread count and packet batching
- Aggressive and extreme flooding modes

**Usage:**
```bash
# Compile the tool
g++ -o pf pf.cpp -lpthread

# Run with root privileges (required for raw sockets)
sudo ./pf <target_ip>
```

### 2. Ping of Death (PoD) Attack (`pod.cpp`)
An implementation of the classic Ping of Death attack that sends oversized ICMP packets to potentially crash vulnerable systems.

**Features:**
- Generates fragmented ICMP packets larger than the maximum IP packet size
- Uses IP fragmentation to bypass size restrictions
- Configurable packet sizes (default ~128KB)
- Spoofed source IP addresses
- Real-time fragment transmission monitoring

**Usage:**
```bash
# Compile the tool
g++ -o pod pod.cpp

# Run with root privileges
sudo ./pod <target_ip>
```

### 3. Network Monitoring and Logging Tool (`log.py`)
A Python-based network monitoring tool that performs continuous ping tests and visualizes network latency in real-time.

**Features:**
- Real-time ping monitoring with configurable intervals
- Live graphical visualization using matplotlib
- Statistical analysis (min, max, average RTT)
- Automatic plot saving and data logging
- Graceful handling of timeouts and network errors
- Customizable monitoring duration and target hosts

**Usage:**
```bash
# Install required dependencies
pip install ping3 matplotlib numpy

# Run the monitoring tool
python3 log.py
```

## Project Structure

```
CSE406-Project-1/
├── pf.cpp          # Packet flooding tool source code
├── pf              # Compiled packet flooding executable
├── pod.cpp         # Ping of Death attack source code
├── pod             # Compiled PoD executable
├── log.py          # Network monitoring and visualization tool
├── README.md       # This documentation file
└── .gitignore      # Git ignore file
```

## Technical Details

### Compilation Requirements

**C++ Tools (pf.cpp, pod.cpp):**
- GCC compiler with C++11 support or later
- Linux operating system (uses Linux-specific networking APIs)
- Root privileges for raw socket operations

**Python Tool (log.py):**
- Python 3.6 or later
- Required packages: `ping3`, `matplotlib`, `numpy`

### Security Considerations

These tools implement actual network attack techniques and should be handled with extreme care:

1. **Legal Compliance**: Only use on networks you own or have written permission to test
2. **Ethical Use**: These tools are for education and authorized security testing only
3. **Network Impact**: Packet flooding can cause network congestion and service disruption
4. **System Vulnerabilities**: PoD attacks may crash vulnerable systems

### Configuration Options

**Packet Flooding Tool:**
- `BATCH_SIZE`: Maximum packets per batch (default: 1000)
- `PACKET_POOL_SIZE`: Pre-generated packet pool size (default: 5000)
- `NUM_THREADS`: Number of concurrent threads (default: 8x CPU cores)

**Ping of Death Tool:**
- `POD_PACKET_SIZE`: Size of oversized packet (default: ~128KB)
- `FRAG_SIZE`: Fragment size for IP fragmentation (default: 1472 bytes)

**Monitoring Tool:**
- `HOST`: Target IP address to monitor (default: "10.13.23.144")
- `PING_INTERVAL`: Time between pings in seconds (default: 1)
- `RUN_DURATION`: Total monitoring duration (default: 10 seconds)

## Educational Objectives

This project demonstrates:
1. **Network Protocol Understanding**: Low-level IP and ICMP packet manipulation
2. **Socket Programming**: Raw socket creation and management in C++
3. **Multi-threading**: Concurrent programming for high-performance networking
4. **Network Security**: Common attack vectors and their implementations
5. **Monitoring and Analysis**: Real-time network performance visualization

## License

This project is developed for educational purposes as part of CSE406 coursework. Please ensure compliance with your institution's policies and local laws when using these tools.

## Contributing

This is an academic project. If you're a student working on similar coursework, please ensure you follow your institution's academic integrity policies.

## Support

For questions related to this project, please consult your course materials or contact your instructor.
