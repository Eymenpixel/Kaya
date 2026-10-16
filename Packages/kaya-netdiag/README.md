# Kaya Network Diagnostics

- `kaya-netdiag info` shows local interface IPv4 addresses, the default gateway, and configured DNS servers.
- `kaya-netdiag dns example.com` resolves a host.
- `kaya-netdiag ping example.com --count 4` uses the host's `ping` for at most ten packets.
- `kaya-netdiag port example.com 443` checks one TCP port only; it does not scan ranges.
- `kaya-netdiag trace example.com --hops 15` uses a host-provided `tracepath` or `traceroute`, capped at 30 hops.

The package does not bundle system commands. On Ubuntu, install `iputils-ping`
with APT if the host does not already provide `ping`. Interface and route
details are Linux/WSL-specific. The trace command reuses existing host tools and
does not install or bundle them.
