import socket
from typing import List

from admin.packet import Packet

class AsyncSocket:
    s: socket.socket
    partial: str

    def __init__(self, sock):
        self.s = sock
        self.partial = ""

def read_lines(sock: AsyncSocket) -> List[str]:
    chunk = None
    strings: List[str] = []

    # Read chunks until reaching a separator, or blocking
    while True:
        try:
            chunk = None
            chunk = sock.s.recv(1024)
        except BlockingIOError:
            return

        if chunk is not None and len(chunk) != 0:
            print('Got some data: ', chunk.decode('utf-8'))
            
            sock.partial += chunk.decode('utf-8')
            if ';' in sock.partial:
                pos = sock.partial.find(';')
                strings.append(sock.partial[0:pos])
                sock.partial = sock.partial[pos:]

def read_packets(sock: AsyncSocket) -> List[Packet]:
    lines = read_lines(sock)
    packets: List[Packet] = [Packet(line) for line in lines]

    return packets

def send_nonblocking_as_blocking(sock: AsyncSocket, message: str):
    message_bytes = message.encode('utf-8')

    print('Send bytes ', message_bytes);

    # this will do bad things if there is too much data
    sock.s.sendall(message_bytes)