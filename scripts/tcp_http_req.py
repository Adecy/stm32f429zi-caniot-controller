import socket
import time
from random import shuffle

n = 3

req = b"""GET /path/2 HTTP/1.1
Host: 192.168.10.240
User-Agent: python-requests/2.26.0
Accept-Encoding: gzip, deflate
Accept: */*
Connection: keep-alive
Content-Length: 17
Content-Type: application/json
Authorization: Basic bHVjYXM6cGFzc3dvcmQh

{"user": "Lucas"}"""

req.replace(b"\n", b"\r\n")

a = time.time()

sock = [0] * n

for i in range(n):
        sock[i] = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock[i].settimeout(5.0)
        sock[i].connect(("192.168.10.240", 80))

b = time.time()

time.sleep(2.0)

shuffle(sock)

for j in range(3):
        for i in range(n):
                sock[i].send(req)
                data = sock[i].recv(1024)
                print(f"[{len(data)}] {data}")

                time.sleep(1.0)

c = time.time()

for i in range(n):
        sock[i].close()

d = time.time()

print(f"d - a = {d - a: .3f} s / {b - a: .3f} / {c - b: .3f} / {d - c: .3f}")