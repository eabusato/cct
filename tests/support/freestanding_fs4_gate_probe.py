#!/usr/bin/env python3

import argparse
import socket
import struct
import subprocess
import sys
import time
from pathlib import Path

ETH_TYPE_ARP = 0x0806
ETH_TYPE_IP = 0x0800
IP_PROTO_ICMP = 1
IP_PROTO_TCP = 6

TCP_FIN = 0x01
TCP_SYN = 0x02
TCP_PSH = 0x08
TCP_ACK = 0x10

HOST_MAC = bytes.fromhex("525400123457")
HOST_IP = bytes([10, 0, 2, 2])
GUEST_IP = bytes([10, 0, 2, 15])
BROADCAST_MAC = b"\xff" * 6


def mac_text(mac: bytes) -> str:
    return ":".join(f"{b:02x}" for b in mac)


def ip_text(ip: bytes) -> str:
    return ".".join(str(b) for b in ip)


def checksum(data: bytes) -> int:
    if len(data) & 1:
        data += b"\x00"
    total = 0
    for i in range(0, len(data), 2):
        total += (data[i] << 8) | data[i + 1]
    while total >> 16:
        total = (total & 0xFFFF) + (total >> 16)
    return (~total) & 0xFFFF


def ethernet_frame(dst_mac: bytes, src_mac: bytes, eth_type: int, payload: bytes) -> bytes:
    return dst_mac + src_mac + struct.pack("!H", eth_type) + payload


def arp_request(src_mac: bytes, src_ip: bytes, target_ip: bytes) -> bytes:
    payload = struct.pack(
        "!HHBBH6s4s6s4s",
        1,
        ETH_TYPE_IP,
        6,
        4,
        1,
        src_mac,
        src_ip,
        b"\x00" * 6,
        target_ip,
    )
    return ethernet_frame(BROADCAST_MAC, src_mac, ETH_TYPE_ARP, payload)


def ipv4_packet(src_ip: bytes, dst_ip: bytes, protocol: int, payload: bytes, ident: int = 0x1337) -> bytes:
    version_ihl = 0x45
    total_len = 20 + len(payload)
    header = struct.pack(
        "!BBHHHBBH4s4s",
        version_ihl,
        0,
        total_len,
        ident,
        0,
        64,
        protocol,
        0,
        src_ip,
        dst_ip,
    )
    header = header[:10] + struct.pack("!H", checksum(header)) + header[12:]
    return header + payload


def icmp_echo_request(identifier: int, sequence: int, payload: bytes) -> bytes:
    header = struct.pack("!BBHHH", 8, 0, 0, identifier, sequence)
    full = header + payload
    full = full[:2] + struct.pack("!H", checksum(full)) + full[4:]
    return full


def tcp_segment(src_ip: bytes, dst_ip: bytes, src_port: int, dst_port: int, seq: int, ack: int, flags: int, payload: bytes) -> bytes:
    data_offset = 5 << 4
    header = struct.pack(
        "!HHLLBBHHH",
        src_port,
        dst_port,
        seq,
        ack,
        data_offset,
        flags,
        4096,
        0,
        0,
    )
    tcp_len = len(header) + len(payload)
    pseudo = struct.pack("!4s4sBBH", src_ip, dst_ip, 0, IP_PROTO_TCP, tcp_len)
    full = header + payload
    csum = checksum(pseudo + full)
    header = header[:16] + struct.pack("!H", csum) + header[18:]
    return header + payload


def parse_ethernet(frame: bytes):
    if len(frame) < 14:
        return None
    return {
        "dst": frame[0:6],
        "src": frame[6:12],
        "type": struct.unpack("!H", frame[12:14])[0],
        "payload": frame[14:],
    }


def parse_arp(payload: bytes):
    if len(payload) < 28:
        return None
    vals = struct.unpack("!HHBBH6s4s6s4s", payload[:28])
    return {
        "htype": vals[0],
        "ptype": vals[1],
        "hlen": vals[2],
        "plen": vals[3],
        "oper": vals[4],
        "sha": vals[5],
        "spa": vals[6],
        "tha": vals[7],
        "tpa": vals[8],
    }


def parse_ipv4(payload: bytes):
    if len(payload) < 20:
        return None
    version_ihl = payload[0]
    ihl = (version_ihl & 0x0F) * 4
    if ihl < 20 or len(payload) < ihl:
        return None
    total_len = struct.unpack("!H", payload[2:4])[0]
    if total_len < ihl or len(payload) < total_len:
        return None
    return {
        "protocol": payload[9],
        "src_ip": payload[12:16],
        "dst_ip": payload[16:20],
        "payload": payload[ihl:total_len],
    }


def parse_icmp(payload: bytes):
    if len(payload) < 8:
        return None
    return {
        "type": payload[0],
        "code": payload[1],
        "identifier": struct.unpack("!H", payload[4:6])[0],
        "sequence": struct.unpack("!H", payload[6:8])[0],
        "payload": payload[8:],
    }


def parse_tcp(payload: bytes):
    if len(payload) < 20:
        return None
    src_port, dst_port, seq, ack, data_offset, flags, window, csum, urgent = struct.unpack("!HHLLBBHHH", payload[:20])
    header_len = (data_offset >> 4) * 4
    if header_len < 20 or len(payload) < header_len:
        return None
    return {
        "src_port": src_port,
        "dst_port": dst_port,
        "seq": seq,
        "ack": ack,
        "flags": flags,
        "payload": payload[header_len:],
    }


class Fs4GateProbe:
    def __init__(self, args):
        self.args = args
        self.log_dir = Path(args.log_dir)
        self.log_dir.mkdir(parents=True, exist_ok=True)
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sock.bind((args.peer_host, args.peer_port))
        self.sock.settimeout(0.25)
        self.qemu_proc = None
        self.guest_mac = None

    def launch_qemu(self):
        qemu_log = self.log_dir / "qemu_gate_probe.log"
        qemu_out = qemu_log.open("w", encoding="utf-8")
        cmd = [
            self.args.qemu_bin,
            "-cdrom",
            self.args.iso,
            "-display",
            "none",
            "-no-reboot",
            "-no-shutdown",
            "-serial",
            "none",
            "-parallel",
            "none",
            "-monitor",
            "none",
            "-netdev",
            f"socket,id=net0,udp={self.args.peer_host}:{self.args.peer_port},localaddr={self.args.peer_host}:{self.args.qemu_port}",
            "-device",
            "rtl8139,netdev=net0",
        ]
        self.qemu_proc = subprocess.Popen(cmd, stdout=qemu_out, stderr=subprocess.STDOUT)
        time.sleep(self.args.boot_delay)

    def close(self):
        if self.qemu_proc is not None and self.qemu_proc.poll() is None:
            self.qemu_proc.terminate()
            try:
                self.qemu_proc.wait(timeout=5)
            except subprocess.TimeoutExpired:
                self.qemu_proc.kill()
                self.qemu_proc.wait(timeout=5)
        self.sock.close()

    def send_frame(self, frame: bytes):
        self.sock.sendto(frame, (self.args.peer_host, self.args.qemu_port))

    def recv_frame(self, timeout: float):
        deadline = time.time() + timeout
        while time.time() < deadline:
            if self.qemu_proc is not None and self.qemu_proc.poll() is not None:
                raise RuntimeError(f"QEMU exited early with status {self.qemu_proc.returncode}")
            try:
                data, _addr = self.sock.recvfrom(65535)
                return data
            except socket.timeout:
                continue
        return None

    def resolve_guest_mac(self):
        request = arp_request(HOST_MAC, HOST_IP, GUEST_IP)
        for _ in range(8):
            self.send_frame(request)
            deadline = time.time() + 1.0
            while time.time() < deadline:
                frame = self.recv_frame(0.25)
                if frame is None:
                    continue
                eth = parse_ethernet(frame)
                if not eth or eth["type"] != ETH_TYPE_ARP:
                    continue
                arp = parse_arp(eth["payload"])
                if not arp:
                    continue
                if arp["oper"] == 2 and arp["spa"] == GUEST_IP and arp["tpa"] == HOST_IP:
                    self.guest_mac = arp["sha"]
                    return
        raise RuntimeError("did not receive ARP reply from guest")

    def probe_icmp(self):
        ident = 0x4242
        seq = 1
        payload = b"CCT-FS4-PING"
        icmp = icmp_echo_request(ident, seq, payload)
        frame = ethernet_frame(self.guest_mac, HOST_MAC, ETH_TYPE_IP, ipv4_packet(HOST_IP, GUEST_IP, IP_PROTO_ICMP, icmp))
        self.send_frame(frame)

        deadline = time.time() + self.args.timeout
        while time.time() < deadline:
            rx = self.recv_frame(0.25)
            if rx is None:
                continue
            eth = parse_ethernet(rx)
            if not eth or eth["type"] != ETH_TYPE_IP:
                continue
            ip = parse_ipv4(eth["payload"])
            if not ip or ip["protocol"] != IP_PROTO_ICMP:
                continue
            icmp_rx = parse_icmp(ip["payload"])
            if not icmp_rx:
                continue
            if (
                ip["src_ip"] == GUEST_IP
                and ip["dst_ip"] == HOST_IP
                and icmp_rx["type"] == 0
                and icmp_rx["identifier"] == ident
                and icmp_rx["sequence"] == seq
                and icmp_rx["payload"] == payload
            ):
                return
        raise RuntimeError("did not receive ICMP echo reply from guest")

    def probe_tcp(self):
        src_port = 40080
        dst_port = 80
        client_seq = 0x10203040
        guest_seq = None
        banner = bytearray()
        probe_payload = b"hello from fs4 gate probe"

        syn = tcp_segment(HOST_IP, GUEST_IP, src_port, dst_port, client_seq, 0, TCP_SYN, b"")
        self.send_frame(ethernet_frame(self.guest_mac, HOST_MAC, ETH_TYPE_IP, ipv4_packet(HOST_IP, GUEST_IP, IP_PROTO_TCP, syn, 0x2001)))
        client_seq += 1

        deadline = time.time() + self.args.timeout
        while time.time() < deadline:
            rx = self.recv_frame(0.25)
            if rx is None:
                continue
            eth = parse_ethernet(rx)
            if not eth or eth["type"] != ETH_TYPE_IP:
                continue
            ip = parse_ipv4(eth["payload"])
            if not ip or ip["protocol"] != IP_PROTO_TCP or ip["src_ip"] != GUEST_IP or ip["dst_ip"] != HOST_IP:
                continue
            tcp = parse_tcp(ip["payload"])
            if not tcp or tcp["src_port"] != dst_port or tcp["dst_port"] != src_port:
                continue
            if (tcp["flags"] & (TCP_SYN | TCP_ACK)) == (TCP_SYN | TCP_ACK) and tcp["ack"] == client_seq:
                guest_seq = tcp["seq"] + 1
                ack = tcp_segment(HOST_IP, GUEST_IP, src_port, dst_port, client_seq, guest_seq, TCP_ACK, b"")
                self.send_frame(ethernet_frame(self.guest_mac, HOST_MAC, ETH_TYPE_IP, ipv4_packet(HOST_IP, GUEST_IP, IP_PROTO_TCP, ack, 0x2002)))
                psh = tcp_segment(HOST_IP, GUEST_IP, src_port, dst_port, client_seq, guest_seq, TCP_PSH | TCP_ACK, probe_payload)
                self.send_frame(ethernet_frame(self.guest_mac, HOST_MAC, ETH_TYPE_IP, ipv4_packet(HOST_IP, GUEST_IP, IP_PROTO_TCP, psh, 0x2003)))
                client_seq += len(probe_payload)
                break
        if guest_seq is None:
            raise RuntimeError("did not receive TCP SYN-ACK from guest")

        deadline = time.time() + self.args.timeout
        while time.time() < deadline:
            rx = self.recv_frame(0.25)
            if rx is None:
                continue
            eth = parse_ethernet(rx)
            if not eth or eth["type"] != ETH_TYPE_IP:
                continue
            ip = parse_ipv4(eth["payload"])
            if not ip or ip["protocol"] != IP_PROTO_TCP or ip["src_ip"] != GUEST_IP or ip["dst_ip"] != HOST_IP:
                continue
            tcp = parse_tcp(ip["payload"])
            if not tcp or tcp["src_port"] != dst_port or tcp["dst_port"] != src_port:
                continue
            if tcp["payload"]:
                if tcp["seq"] == guest_seq:
                    banner.extend(tcp["payload"])
                    guest_seq += len(tcp["payload"])
                    ack = tcp_segment(HOST_IP, GUEST_IP, src_port, dst_port, client_seq, guest_seq, TCP_ACK, b"")
                    self.send_frame(ethernet_frame(self.guest_mac, HOST_MAC, ETH_TYPE_IP, ipv4_packet(HOST_IP, GUEST_IP, IP_PROTO_TCP, ack, 0x2004)))
            if (tcp["flags"] & TCP_FIN) != 0 and tcp["seq"] == guest_seq:
                guest_seq += 1
                ack = tcp_segment(HOST_IP, GUEST_IP, src_port, dst_port, client_seq, guest_seq, TCP_ACK, b"")
                self.send_frame(ethernet_frame(self.guest_mac, HOST_MAC, ETH_TYPE_IP, ipv4_packet(HOST_IP, GUEST_IP, IP_PROTO_TCP, ack, 0x2005)))
            if b"Civitas Freestanding v0.4 - FS-4 Gate" in banner and b"IP: 10.0.2.15 | TCP OK | ICMP OK" in banner:
                return banner.decode("utf-8", errors="replace")
        raise RuntimeError("did not receive expected TCP banner from guest")


def parse_args():
    parser = argparse.ArgumentParser(description="Probe the freestanding FS-4 gate over a socket-backed RTL8139 peer.")
    parser.add_argument("--qemu-bin", required=True, help="Path to qemu-system-i386")
    parser.add_argument("--iso", required=True, help="Path to grub-hello.iso")
    parser.add_argument("--peer-host", default="127.0.0.1")
    parser.add_argument("--peer-port", type=int, default=19046)
    parser.add_argument("--qemu-port", type=int, default=19045)
    parser.add_argument("--boot-delay", type=float, default=7.0)
    parser.add_argument("--timeout", type=float, default=8.0)
    parser.add_argument("--log-dir", default="tests/.tmp/fs4_gate_probe")
    return parser.parse_args()


def main():
    args = parse_args()
    probe = Fs4GateProbe(args)
    try:
        probe.launch_qemu()
        probe.resolve_guest_mac()
        print(f"[OK] ARP reply from guest MAC {mac_text(probe.guest_mac)}")
        probe.probe_icmp()
        print(f"[OK] ICMP echo reply from {ip_text(GUEST_IP)}")
        banner = probe.probe_tcp()
        print(f"[OK] TCP banner from {ip_text(GUEST_IP)}:80")
        print(banner)
        return 0
    except Exception as exc:
        print(f"[ERROR] {exc}", file=sys.stderr)
        return 1
    finally:
        probe.close()


if __name__ == "__main__":
    raise SystemExit(main())
