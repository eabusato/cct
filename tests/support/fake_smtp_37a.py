#!/usr/bin/env python3
import argparse
import base64
import pathlib
import socket
import sys


def recv_line(conn):
    data = bytearray()
    while True:
        chunk = conn.recv(1)
        if not chunk:
            return None
        data.extend(chunk)
        if data.endswith(b"\n"):
            return data.decode("utf-8", errors="replace").rstrip("\r\n")


def send_line(conn, text):
    conn.sendall(text.encode("utf-8") + b"\r\n")


def decode_b64(text):
    return base64.b64decode(text.encode("ascii")).decode("utf-8", errors="replace")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--port-file", required=True)
    parser.add_argument("--out-dir", required=True)
    args = parser.parse_args()

    out_dir = pathlib.Path(args.out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)

    listener = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    listener.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    listener.bind(("127.0.0.1", 0))
    listener.listen(1)

    pathlib.Path(args.port_file).write_text(str(listener.getsockname()[1]), encoding="utf-8")

    conn, _ = listener.accept()
    try:
        send_line(conn, "220 fake-smtp-37a ready")
        mail_from = ""
        rcpts = []
        auth_user = ""
        auth_pass = ""
        data_lines = []

        while True:
            line = recv_line(conn)
            if line is None:
                break

            upper = line.upper()
            if upper.startswith("EHLO") or upper.startswith("HELO"):
                send_line(conn, "250-fake-smtp-37a")
                send_line(conn, "250-AUTH LOGIN PLAIN XOAUTH2")
                send_line(conn, "250 OK")
            elif upper == "AUTH LOGIN":
                send_line(conn, "334 VXNlcm5hbWU6")
                auth_user = decode_b64(recv_line(conn) or "")
                send_line(conn, "334 UGFzc3dvcmQ6")
                auth_pass = decode_b64(recv_line(conn) or "")
                send_line(conn, "235 2.7.0 Authentication successful")
            elif upper.startswith("AUTH PLAIN "):
                raw = base64.b64decode(line.split(" ", 2)[2].encode("ascii"))
                parts = raw.split(b"\x00")
                if len(parts) >= 3:
                    auth_user = parts[1].decode("utf-8", errors="replace")
                    auth_pass = parts[2].decode("utf-8", errors="replace")
                send_line(conn, "235 2.7.0 Authentication successful")
            elif upper.startswith("AUTH XOAUTH2 "):
                raw = base64.b64decode(line.split(" ", 2)[2].encode("ascii")).decode("utf-8", errors="replace")
                for piece in raw.split("\x01"):
                    if piece.startswith("user="):
                        auth_user = piece[5:]
                    elif piece.startswith("auth=Bearer "):
                        auth_pass = piece[12:]
                send_line(conn, "235 2.7.0 Authentication successful")
            elif upper.startswith("MAIL FROM:"):
                mail_from = line[line.find("<") + 1 : line.rfind(">")]
                send_line(conn, "250 2.1.0 OK")
            elif upper.startswith("RCPT TO:"):
                rcpts.append(line[line.find("<") + 1 : line.rfind(">")])
                send_line(conn, "250 2.1.5 OK")
            elif upper == "DATA":
                send_line(conn, "354 End data with <CR><LF>.<CR><LF>")
                while True:
                    data_line = recv_line(conn)
                    if data_line is None:
                        break
                    if data_line == ".":
                        break
                    if data_line.startswith(".."):
                        data_line = data_line[1:]
                    data_lines.append(data_line)
                send_line(conn, "250 2.0.0 queued")
            elif upper == "QUIT":
                send_line(conn, "221 2.0.0 bye")
                break
            else:
                send_line(conn, "500 5.5.1 unsupported")

        (out_dir / "mail_from.txt").write_text(mail_from + "\n", encoding="utf-8")
        (out_dir / "rcpts.txt").write_text("\n".join(rcpts) + ("\n" if rcpts else ""), encoding="utf-8")
        (out_dir / "auth.txt").write_text(f"{auth_user}\n{auth_pass}\n", encoding="utf-8")
        (out_dir / "data.eml").write_text("\r\n".join(data_lines) + ("\r\n" if data_lines else ""), encoding="utf-8")
    finally:
        conn.close()
        listener.close()


if __name__ == "__main__":
    sys.exit(main())
