# Network Notes

## Observations

- Router-assigned device IP observed: `192.168.31.176`
- Ping successful from Ubuntu host
- Low-latency local connectivity confirmed

## Port checks observed

Closed/refused:
- 22 (SSH)
- 23 (Telnet)
- 80 (HTTP)
- 443 (HTTPS)
- 5555 (ADB over TCP)
- 8080 (alternate HTTP)
- 8710 (possible HDC-related check)

Open:
- 53 (DNS)


## Additional observation

- Plugging into the other Ethernet port produced a different DHCP lease: `192.168.31.196`
- This suggests both physical ETH ports may be active/enumerated, or port-to-interface mapping differs by jack
- Need to later confirm:
  - whether both ports are independently usable
  - interface naming / MAC mapping
  - whether one port is WAN/LAN differentiated in software
