import socket

host = "0.0.0.0"  # Listen on all available network interfaces
port = 8080

with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    s.bind((host, port))
    s.listen()

    print(f"Listening on {host}:{port}")

    while True:
        conn, addr = s.accept()
        with conn:
            print(f"Connected by {addr}")
            data = conn.recv(1024).decode()
            print(f"Received data: {data}")