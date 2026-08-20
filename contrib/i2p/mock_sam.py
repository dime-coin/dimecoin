import socket, threading, sys


def handle(conn):
    f = conn.makefile("rwb", buffering=0)
    while True:
        line = f.readline()
        if not line:
            break
        s = line.decode().strip()
        if s.startswith("HELLO"):
            f.write(b"HELLO REPLY RESULT=OK VERSION=3.0\n")
        elif s.startswith("SESSION CREATE"):
            f.write(
                b"SESSION STATUS RESULT=OK DESTINATION=AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA==\n"
            )
        elif s.startswith("STREAM CONNECT"):
            f.write(b"STREAM STATUS RESULT=OK\n")
            # echo loopback: relay whatever the client sends back
            try:
                for _ in range(5):
                    buf = conn.recv(1)
                    if not buf:
                        break
                    conn.sendall(buf)
            except Exception:
                pass
            break
        elif s.startswith("STREAM ACCEPT"):
            f.write(b"STREAM STATUS RESULT=OK\n")
            break
        else:
            f.write(b"SESSION STATUS RESULT=INVALID\n")
    conn.close()


srv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
srv.bind(("127.0.0.1", 7656))
srv.listen(1)
print("mock SAM listening on 127.0.0.1:7656", flush=True)
while True:
    conn, _ = srv.accept()
    threading.Thread(target=handle, args=(conn,), daemon=True).start()
