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
