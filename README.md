# CSE406 Network Security Project

## Overview

This project demonstrates various network security concepts through practical implementations of network testing and attack simulation tools. The project consists of three main components designed for educational purposes in controlled environments.

⚠️ **IMPORTANT DISCLAIMER**: These tools are for educational and authorized testing purposes only. Unauthorized use against systems you do not own or have explicit permission to test is illegal and unethical.

## Project Components

### 1. Packet Flooding Tool (`pf.cpp`)

A C++ implementation that performs network stress testing by sending a large volume of packets to a target.

**Features:**

- High-speed packet generation
- Configurable target IP and parameters
- Network stress testing capabilities

**Compilation & Usage:**

```bash
# Compile
g++ -o pf pf.cpp

# Run (requires root privileges)
sudo ./pf
```

### 2. Ping of Death Attack (`pod.cpp` & `pod2.py`)

Implementation of the classic Ping of Death attack that sends oversized ICMP packets.

#### C++ Version (`pod.cpp`)

```bash
# Compile
g++ -o pod pod.cpp

# Run
sudo ./pod
```

#### Python Version (`pod2.py`)

**Dependencies:**

- Python 3.x
- Scapy library

**Installation:**

```bash
# Install Scapy
pip3 install scapy

# For system-wide installation (if needed)
sudo pip3 install scapy --break-system-packages
```

**Usage:**

```bash
sudo python3 pod2.py
```

**Configuration:**

- Target IP: `10.13.23.144` (modify in script)
- Payload size: ~165KB fragmented packets
- Fragment size: 1480 bytes

### 3. Network Monitoring Tool (`log.py`)

Real-time network monitoring tool that tracks ping response times and visualizes network performance.

**Features:**

- Real-time RTT monitoring
- Live graph visualization
- Statistical analysis (avg, max, min RTT)
- Plot export functionality

**Dependencies:**

- Python 3.x
- matplotlib
- numpy
- ping3 (or system ping alternative)

**Installation Options:**

**Option 1: Install ping3 (if system allows)**

```bash
pip3 install ping3 matplotlib numpy
```

**Option 2: Use system ping (recommended for externally managed environments)**
If you encounter "externally-managed-environment" error, replace the ping3 import in `log.py`:

```python
# Replace this line:
from ping3 import ping

# With this function:
import subprocess
import re

def ping(host, timeout=1):
    """System ping replacement for ping3"""
    try:
        result = subprocess.run(['ping', '-c', '1', '-W', str(timeout), host],
                              capture_output=True, text=True, timeout=timeout+2)
        if result.returncode == 0:
            match = re.search(r'time=(\d+\.?\d*)', result.stdout)
            if match:
                return float(match.group(1)) / 1000  # Convert ms to seconds
        return None
    except:
        return None
```

**Usage:**

```bash
# Try without sudo first (for GUI display)
python3 log.py

# If sudo needed for network access
sudo -E python3 log.py

# For display issues with sudo
xhost +local:
sudo python3 log.py
```

**Configuration:**

- Target host: `10.13.23.144`
- Ping interval: 0.5 seconds
- Run duration: 10 seconds
- Max data points: 100

## Technical Requirements

### System Requirements

- Linux operating system
- Root/sudo privileges for network operations
- Python 3.x
- GCC compiler for C++ components

### Network Requirements

- Access to target network (10.13.23.144)
- ICMP traffic allowed
- Raw socket permissions

### Python Dependencies

```bash
# Core dependencies
pip3 install matplotlib numpy

# For ping3 approach
pip3 install ping3

# Alternative: use system ping (no additional dependencies)
```

## Security Considerations

### ⚠️ Legal and Ethical Warnings

1. **Authorization Required**: Only use these tools on networks and systems you own or have explicit written permission to test
2. **Educational Purpose**: These tools are designed for learning network security concepts
3. **Controlled Environment**: Use only in isolated lab environments
4. **Legal Compliance**: Ensure compliance with local laws and regulations
5. **Responsible Disclosure**: If vulnerabilities are discovered, follow responsible disclosure practices

### Attack Implications

- **Packet Flooding**: Can cause network congestion and service degradation
- **Ping of Death**: May crash or destabilize vulnerable systems
- **Network Monitoring**: Generally benign but may trigger security alerts

## Troubleshooting

### Common Issues

1. **Permission Denied**

   ```bash
   sudo python3 script.py
   ```

2. **ping3 Module Not Found**

   - Install: `pip3 install ping3`
   - Or use system ping alternative (see above)

3. **Externally Managed Environment Error**

   ```bash
   sudo pip3 install package --break-system-packages
   ```

   Or use virtual environment:

   ```bash
   python3 -m venv venv
   source venv/bin/activate
   pip install ping3
   ```

4. **GUI Display Issues with sudo**

   ```bash
   xhost +local:
   sudo -E python3 log.py
   ```

5. **Scapy Import Errors**
   ```bash
   pip3 install scapy
   ```

## Educational Objectives

This project demonstrates:

1. **Network Protocol Understanding**: Low-level packet manipulation and ICMP protocol
2. **Security Testing Methodologies**: Stress testing and vulnerability assessment
3. **Network Monitoring**: Real-time performance analysis and visualization
4. **Attack Vectors**: Understanding how network attacks work and their impact
5. **Defense Strategies**: Recognizing attack patterns and implementing countermeasures

## File Structure

```
CSE406-Project-1/
├── README.md           # This documentation
├── pf.cpp              # Packet flooding tool (C++)
├── pod.cpp             # Ping of Death (C++)
├── pod2.py             # Ping of Death (Python/Scapy)
└── log.py              # Network monitoring tool
```

## Output Files

- `ping_plot.png`: Generated graph from log.py showing RTT over time
- Compiled executables: `pf`, `pod`

## Academic Context

This project is part of CSE406 (Network Security) coursework, focusing on:

- Understanding network vulnerabilities
- Implementing security testing tools
- Analyzing network behavior under stress
- Learning ethical hacking principles

---

**Remember**: With great power comes great responsibility. Use these tools wisely and ethically.
