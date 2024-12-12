from __future__ import annotations
from enum import Enum
import socket
import base64
import time
import dearpygui.dearpygui as dpg

from admin.movement_keys import bind_movement_keys
from admin.packet import Packet
from admin.socket_tools import AsyncSocket, read_packets, send_nonblocking_as_blocking
from admin.gui_decl import GuiConfig, GuiRobot

JSON_PORT = 80
DEFAULT_IP = '192.168.133.32'

class ConnStatus(Enum):
    EMPTY = "Empty"
    CONNECTING = "Connecting"
    CONNECTED = "Connected"
    DISCONNECTED = "Disconnected"

class RobotConnection:
    active: RobotConnection = None

    sock: socket.socket
    status: str
    ip: str
    macAddress: None | str
    lastMessageSent: str

    def __init__(self, ip):
        self.ip = ip
        self.status = ConnStatus.EMPTY
        self.macAddress = None
        self.lastMessageSent = ""

        self.connect()

        #todo: allow cancel
        #status = self.sock.s.connect_ex((ip, JSON_PORT))
        #while status != 0:
        #    status = self.sock.s.connect_ex((ip, JSON_PORT))

    def connect(self):
        print('Connecting to', self.ip)

        self.sock = AsyncSocket(socket.socket(socket.AF_INET, socket.SOCK_STREAM))
        self.sock.s.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
        self.sock.s.connect((self.ip, JSON_PORT))
        self.sock.s.setblocking(False)

        self.setStatus(ConnStatus.CONNECTING)

    def disconnect(self):
        self.setStatus(ConnStatus.DISCONNECTED)
        self.s.close()
        print('Closing connection to', self.ip)

    def setStatus(self, newStatus: ConnStatus):
        self.status = newStatus
        dpg.set_value(GuiRobot.conn_status_text, "Connection Status: " + newStatus.value)

    def onHello(self, p: Packet):
        if self.macAddress is not None:
            print('Client sent hello more than once?')

        self.macAddress = p.data['macAddress']
        self.setStatus(ConnStatus.CONNECTED)

        dpg.set_value(GuiRobot.mac_text, "Robot MAC Address: " + self.macAddress)

        self.syncConfig()

        bind_movement_keys(RobotConnection.active)

    def tick(self):
        try:
            packets = read_packets(self.sock)
            if packets is not None:
                for p in packets:
                    print('Packet', p.type, p.data)
                    if p.type == Packet.Type.CLIENT_HELLO:
                        self.onHello(p)
                    elif p.type == Packet.Type.LOG:
                        self.onLog(p)
        
        except ConnectionResetError:
            self.setStatus(ConnStatus.DISCONNECTED)
            time.sleep(1)
            print('Socket connection reset! Reconnecting...')
            self.connect()


    def send(self, msg: str):
        if self.lastMessageSent != msg:
            send_nonblocking_as_blocking(self.sock, msg)
            self.lastMessageSent = msg

    def syncConfig(self):
        for index, i in enumerate(GuiConfig.configs):
            # Update last set value
            i[1] = dpg.get_value(i[2])

            t = i[0]["type"]
            val = i[1]

            packet = f'{{"type":"SET_VAR","key":"{index}","val":{val},"var_type":"{t}"}};'
            self.send(packet)

activeConnection: RobotConnection