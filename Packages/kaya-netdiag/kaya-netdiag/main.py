#!/usr/bin/env python3
import argparse
import ipaddress
import shutil
import socket
import struct
import subprocess
import sys


def info():
    print('Network interfaces:')
    found = False
    try:
        import fcntl
        for _, name in socket.if_nameindex():
            try:
                sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
                request = struct.pack('256s', name.encode()[:15])
                raw = fcntl.ioctl(sock.fileno(), 0x8915, request)
                address = socket.inet_ntoa(raw[20:24])
                if address != '0.0.0.0':
                    print(f'  {name}: {address}')
                    found = True
            except OSError:
                pass
        if not found:
            print('  No IPv4 addresses found.')
    except (ImportError, OSError) as exc:
        print(f'  IPv4 lookup unavailable: {exc}')

    try:
        with open('/proc/net/if_inet6', encoding='ascii') as addresses:
            for row in addresses:
                cols = row.split()
                if len(cols) == 6:
                    address = ipaddress.IPv6Address(bytes.fromhex(cols[0]))
                    if not address.is_link_local:
                        print(f'  {cols[5]}: {address}')
    except (OSError, ValueError):
        pass

    route_file = '/proc/net/route'
    try:
        with open(route_file, encoding='ascii') as routes:
            next(routes, None)
            for row in routes:
                cols = row.split()
                if len(cols) > 2 and cols[1] == '00000000':
                    gateway = socket.inet_ntoa(struct.pack('<I', int(cols[2], 16)))
                    print(f'Default gateway: {gateway} via {cols[0]}')
                    break
    except OSError:
        print('Default gateway: unavailable (Linux route table not present)')

    try:
        with open('/etc/resolv.conf', encoding='utf-8') as resolv:
            dns = [line.split()[1] for line in resolv if line.strip().startswith('nameserver ')]
        print('DNS servers: ' + (', '.join(dns) if dns else 'not listed'))
    except OSError:
        print('DNS servers: unavailable')


def dns(host):
    try:
        results = sorted({item[4][0] for item in socket.getaddrinfo(host, None)})
        print('\n'.join(results))
    except socket.gaierror as exc:
        print(f'DNS lookup failed: {exc}', file=sys.stderr)
        return 1
    return 0


def ping(host, count, timeout):
    binary = shutil.which('ping')
    if not binary:
        print('System ping is missing. Install iputils-ping with Ubuntu APT.', file=sys.stderr)
        return 127
    try:
        return subprocess.run([binary, '-c', str(count), '-W', str(timeout), host], check=False).returncode
    except OSError as exc:
        print(f'Could not run ping: {exc}', file=sys.stderr)
        return 1


def port(host, number, timeout):
    try:
        addresses = socket.getaddrinfo(host, number, type=socket.SOCK_STREAM)
    except socket.gaierror as exc:
        print(f'DNS lookup failed: {exc}', file=sys.stderr)
        return 1
    for family, socktype, proto, _, address in addresses:
        try:
            with socket.socket(family, socktype, proto) as sock:
                sock.settimeout(timeout)
                sock.connect(address)
            print(f'OPEN: {host}:{number}')
            return 0
        except OSError:
            continue
    print(f'CLOSED or unreachable: {host}:{number}')
    return 1


def trace(host, hops, timeout):
    binary = shutil.which('tracepath')
    args = [binary, '-m', str(hops), host] if binary else None
    if not binary:
        binary = shutil.which('traceroute')
        args = [binary, '-m', str(hops), '-w', str(timeout), host] if binary else None
    if not binary:
        print('Neither tracepath nor traceroute is installed on the host.', file=sys.stderr)
        return 127
    try:
        return subprocess.run(args, check=False).returncode
    except OSError as exc:
        print(f'Could not run route trace: {exc}', file=sys.stderr)
        return 1


def main():
    parser = argparse.ArgumentParser(description='Small, bounded network diagnostics for Linux/WSL.')
    commands = parser.add_subparsers(dest='command', required=True)
    commands.add_parser('info', help='show interface, default route, and DNS details')
    resolve = commands.add_parser('dns', help='resolve a hostname')
    resolve.add_argument('host')
    check_ping = commands.add_parser('ping', help='ping one host (1–10 packets)')
    check_ping.add_argument('host')
    check_ping.add_argument('-c', '--count', type=int, choices=range(1, 11), default=4)
    check_ping.add_argument('-W', '--timeout', type=int, choices=range(1, 11), default=2)
    check_port = commands.add_parser('port', help='check one TCP port (no range scanning)')
    check_port.add_argument('host')
    check_port.add_argument('port', type=int, choices=range(1, 65536))
    check_port.add_argument('--timeout', type=float, choices=[0.5, 1, 2, 3, 5, 10], default=3)
    check_trace = commands.add_parser('trace', help='trace one host with a bounded hop count')
    check_trace.add_argument('host')
    check_trace.add_argument('--hops', type=int, choices=range(5, 31), default=15)
    check_trace.add_argument('--timeout', type=int, choices=range(1, 6), default=2)
    args = parser.parse_args()
    if args.command == 'info':
        info()
        return 0
    if args.command == 'dns':
        return dns(args.host)
    if args.command == 'ping':
        return ping(args.host, args.count, args.timeout)
    if args.command == 'port':
        return port(args.host, args.port, args.timeout)
    return trace(args.host, args.hops, args.timeout)


if __name__ == '__main__':
    sys.exit(main())
