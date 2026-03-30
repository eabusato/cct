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
IP_PROTO_TCP = 6

TCP_FIN = 0x01
TCP_SYN = 0x02
TCP_PSH = 0x08
TCP_ACK = 0x10

HOST_MAC = bytes.fromhex("525400123457")
HOST_IP = bytes([10, 0, 2, 2])
GUEST_IP = bytes([10, 0, 2, 15])
BROADCAST_MAC = b"\xff" * 6


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


def ipv4_packet(src_ip: bytes, dst_ip: bytes, protocol: int, payload: bytes, ident: int) -> bytes:
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
        "type": struct.unpack("!H", frame[12:14])[0],
        "src": frame[6:12],
        "payload": frame[14:],
    }


def parse_arp(payload: bytes):
    if len(payload) < 28:
        return None
    vals = struct.unpack("!HHBBH6s4s6s4s", payload[:28])
    return {
        "oper": vals[4],
        "sha": vals[5],
        "spa": vals[6],
        "tpa": vals[8],
    }


def parse_ipv4(payload: bytes):
    if len(payload) < 20:
        return None
    ihl = (payload[0] & 0x0F) * 4
    if ihl < 20 or len(payload) < ihl:
        return None
    total_len = struct.unpack("!H", payload[2:4])[0]
    if total_len < ihl or len(payload) < total_len:
        return None
    return {
        "protocol": payload[9],
        "src_ip": payload[12:16],
        "dst_ip": payload[16:20],
        "header_len": ihl,
        "total_len": total_len,
        "payload": payload[ihl:total_len],
    }


def parse_tcp(payload: bytes):
    if len(payload) < 20:
        return None
    src_port, dst_port, seq, ack, data_offset, flags, _window, _csum, _urgent = struct.unpack("!HHLLBBHHH", payload[:20])
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


def ipv4_checksum_ok(packet: bytes) -> bool:
    if len(packet) < 20:
        return False
    ihl = (packet[0] & 0x0F) * 4
    if ihl < 20 or len(packet) < ihl:
        return False
    return checksum(packet[:ihl]) == 0


def tcp_checksum_ok(src_ip: bytes, dst_ip: bytes, segment: bytes) -> bool:
    pseudo = struct.pack("!4s4sBBH", src_ip, dst_ip, 0, IP_PROTO_TCP, len(segment))
    return checksum(pseudo + segment) == 0


class Fs5GateProbe:
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
        qemu_log = self.log_dir / "qemu_fs5_gate.log"
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

    def ensure_alive(self):
        if self.qemu_proc is not None and self.qemu_proc.poll() is not None:
            raise RuntimeError(f"QEMU exited early with status {self.qemu_proc.returncode}")

    def send_frame(self, frame: bytes):
        self.sock.sendto(frame, (self.args.peer_host, self.args.qemu_port))

    def recv_frame(self, timeout: float):
        deadline = time.time() + timeout
        while time.time() < deadline:
            self.ensure_alive()
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
                if arp and arp["oper"] == 2 and arp["spa"] == GUEST_IP and arp["tpa"] == HOST_IP:
                    self.guest_mac = arp["sha"]
                    return
        raise RuntimeError("did not receive ARP reply from guest")

    def send_tcp(self, src_port: int, seq: int, ack: int, flags: int, payload: bytes, ident: int):
        segment = tcp_segment(HOST_IP, GUEST_IP, src_port, 80, seq, ack, flags, payload)
        frame = ethernet_frame(self.guest_mac, HOST_MAC, ETH_TYPE_IP, ipv4_packet(HOST_IP, GUEST_IP, IP_PROTO_TCP, segment, ident))
        self.send_frame(frame)

    def read_http_response(self, src_port: int, client_seq: int, guest_seq: int, expected_fragments):
        data = bytearray()
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
            if not ipv4_checksum_ok(eth["payload"][:ip["total_len"]]):
                continue
            if not tcp_checksum_ok(ip["src_ip"], ip["dst_ip"], ip["payload"]):
                continue
            tcp = parse_tcp(ip["payload"])
            if not tcp or tcp["src_port"] != 80 or tcp["dst_port"] != src_port:
                continue
            if tcp["payload"] and tcp["seq"] == guest_seq:
                data.extend(tcp["payload"])
                guest_seq += len(tcp["payload"])
                self.send_tcp(src_port, client_seq, guest_seq, TCP_ACK, b"", 0x3100)
            if (tcp["flags"] & TCP_FIN) != 0 and tcp["seq"] == guest_seq:
                guest_seq += 1
                self.send_tcp(src_port, client_seq, guest_seq, TCP_ACK, b"", 0x3101)
                break
            text = data.decode("utf-8", errors="replace")
            if all(fragment in text for fragment in expected_fragments):
                return text
        text = data.decode("utf-8", errors="replace")
        if all(fragment in text for fragment in expected_fragments):
            return text
        raise RuntimeError(f"did not receive expected HTTP response fragments: {expected_fragments!r}")

    def http_request(self, src_port: int, request_text: str, expected_fragments):
        client_seq = 0x10203040 + src_port
        guest_seq = None
        syn_seq = client_seq
        next_syn_at = 0.0

        deadline = time.time() + self.args.timeout
        while time.time() < deadline:
            now = time.time()
            if now >= next_syn_at:
                self.send_tcp(src_port, syn_seq, 0, TCP_SYN, b"", 0x3001)
                next_syn_at = now + 0.5
            rx = self.recv_frame(0.25)
            if rx is None:
                continue
            eth = parse_ethernet(rx)
            if not eth or eth["type"] != ETH_TYPE_IP:
                continue
            ip = parse_ipv4(eth["payload"])
            if not ip or ip["protocol"] != IP_PROTO_TCP or ip["src_ip"] != GUEST_IP or ip["dst_ip"] != HOST_IP:
                continue
            if not ipv4_checksum_ok(eth["payload"][:ip["total_len"]]):
                continue
            if not tcp_checksum_ok(ip["src_ip"], ip["dst_ip"], ip["payload"]):
                continue
            tcp = parse_tcp(ip["payload"])
            if not tcp or tcp["src_port"] != 80 or tcp["dst_port"] != src_port:
                continue
            if (tcp["flags"] & (TCP_SYN | TCP_ACK)) == (TCP_SYN | TCP_ACK) and tcp["ack"] == (syn_seq + 1):
                guest_seq = tcp["seq"] + 1
                client_seq = syn_seq + 1
                self.send_tcp(src_port, client_seq, guest_seq, TCP_ACK, b"", 0x3002)
                payload = request_text.encode("utf-8")
                self.send_tcp(src_port, client_seq, guest_seq, TCP_PSH | TCP_ACK, payload, 0x3003)
                client_seq += len(payload)
                break
        if guest_seq is None:
            raise RuntimeError("did not receive TCP SYN-ACK from guest")

        text = self.read_http_response(src_port, client_seq, guest_seq, expected_fragments)
        time.sleep(0.2)
        return text

    def run(self):
        self.launch_qemu()
        self.resolve_guest_mac()

        index_text = self.http_request(
            41080,
            "GET / HTTP/1.1\r\nHost: 10.0.2.15\r\nConnection: close\r\n\r\n",
            ["HTTP/1.1 200 OK", "X-CCT-Phase: FS5", "Civitas Freestanding HTTP Server"],
        )
        if "HTML generated by CCT running bare-metal." not in index_text:
            raise RuntimeError("GET / body missing CCT HTML marker")

        status_text = self.http_request(
            41081,
            "GET /status HTTP/1.1\r\nHost: 10.0.2.15\r\nConnection: close\r\n\r\n",
            ["HTTP/1.1 200 OK", "\"service\":\"civitas-fs5\"", "\"ip\":\"10.0.2.15\""],
        )
        if "\"requests\":" not in status_text:
            raise RuntimeError("GET /status missing requests field")

        api_text = self.http_request(
            41082,
            "GET /api/info HTTP/1.1\r\nHost: 10.0.2.15\r\nConnection: close\r\n\r\n",
            ["HTTP/1.1 200 OK", "\"router\":\"prefix\"", "\"phase\":\"FS5\""],
        )
        if "\"module\":\"http_router_fs\"" not in api_text:
            raise RuntimeError("GET /api/info missing module field")

        echo_text = self.http_request(
            41083,
            "POST /echo HTTP/1.1\r\nHost: 10.0.2.15\r\nContent-Type: text/plain\r\nContent-Length: 9\r\nConnection: close\r\n\r\nbaremetal",
            ["HTTP/1.1 200 OK", "echo-ok"],
        )
        if "path=/echo" not in echo_text and "method=POST" not in echo_text:
            raise RuntimeError("POST /echo missing route metadata")

        missing_text = self.http_request(
            41084,
            "GET /missing HTTP/1.1\r\nHost: 10.0.2.15\r\nConnection: close\r\n\r\n",
            ["HTTP/1.1 404 Not Found", "404 Not Found", "Path: /missing"],
        )
        if "</html>" not in missing_text:
            raise RuntimeError("GET /missing missing HTML body")


def parse_args(argv):
    parser = argparse.ArgumentParser(description="Probe the freestanding FS-5 gate over a socket-backed RTL8139 peer.")
    parser.add_argument("--qemu-bin", required=True)
    parser.add_argument("--iso", required=True)
    parser.add_argument("--peer-host", default="127.0.0.1")
    parser.add_argument("--peer-port", type=int, default=19056)
    parser.add_argument("--qemu-port", type=int, default=19055)
    parser.add_argument("--boot-delay", type=float, default=9.0)
    parser.add_argument("--timeout", type=float, default=12.0)
    parser.add_argument("--log-dir", default="tests/.tmp/fs5_gate_probe")
    return parser.parse_args(argv)


def main(argv=None):
    args = parse_args(argv or sys.argv[1:])
    probe = Fs5GateProbe(args)
    try:
        probe.run()
        return 0
    except Exception as exc:
        print(f"[ERROR] {exc}", file=sys.stderr)
        return 1
    finally:
        probe.close()


if __name__ == "__main__":
    raise SystemExit(main())
