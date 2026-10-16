#!/usr/bin/env python3
import argparse
import ipaddress


def main():
    parser = argparse.ArgumentParser(description='Calculate network details from an IPv4 or IPv6 CIDR.')
    parser.add_argument('cidr', help='for example 192.168.1.12/24 or 2001:db8::1/64')
    parser.add_argument('--strict', action='store_true', help='reject host bits instead of normalizing')
    args = parser.parse_args()
    try:
        network = ipaddress.ip_network(args.cidr, strict=args.strict)
    except ValueError as exc:
        parser.error(str(exc))
    print(f'Network:       {network.network_address}')
    print(f'Prefix:        /{network.prefixlen}')
    print(f'Netmask:       {network.netmask}')
    print(f'Broadcast:     {network.broadcast_address}' if network.version == 4 else 'Broadcast:     N/A (IPv6)')
    print(f'Address count: {network.num_addresses}')
    if network.version == 4 and network.num_addresses > 2:
        print(f'Usable range:  {network.network_address + 1} - {network.broadcast_address - 1}')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
