import socket
import time

HOST = "127.0.0.1"
PORT = 6667

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

sock.connect((HOST, PORT))

sock.sendall(
    b"NICK reda\r\n"
)

# print("Connceted succesfully to the server")
# sock.sendall(b"NIC")
# print("sent : NIC")
# time.sleep(1)
# sock.sendall(b"K reda\r\n")
# print("sent : K reda\\r\\n")
# time.sleep(1)
sock.close()
print("sock disconnected")
