import socket
import threading

HOST = "127.0.0.1"
PORT = 6667


def receive_messages(sock):
    while True:
        try:
            data = sock.recv(4096)

            if not data:
                print("\nServer disconnected")
                break

            print("\n< " + data.decode(), end="")
            print("> ", end="", flush=True)

        except OSError:
            break


sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.connect((HOST, PORT))

print("Connected to server")
print("Type commands, or type 'quit' to disconnect")

receiver = threading.Thread(
    target=receive_messages,
    args=(sock,)
)
receiver.daemon = True
receiver.start()

while True:
    try:
        message = input("> ")

        if message == "quit":
            break

        sock.sendall((message + "\r\n").encode())

    except (KeyboardInterrupt, EOFError):
        break

sock.close()
print("Disconnected")