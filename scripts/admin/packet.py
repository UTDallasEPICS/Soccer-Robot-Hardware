import json
from enum import Enum
import datetime

class Packet:
    class Type(Enum):
        DRIVE_TANK = "DRIVE_TANK"
        PING_SEND = "PING_SEND"
        SET_VAR = "SET_VAR"
        QUERY_VAR = "QUERY_VAR"
        SERVER_HELLO = "SERVER_HELLO"
        CLIENT_HELLO = "CLIENT_HELLO"

    sent: datetime.datetime

    type: Type
    data: any

    def __init__(self, line):
        self.sent = datetime.datetime.now()

        if line.endswith(';'):
            line = line[:-1]

        self.data = json.loads(line)

        self.type = self.data['type']