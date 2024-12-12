from enum import Enum
import json

class ConfigType(Enum):
    GPIO = "gpio"
    FLOAT = "float"

BOT_SERVER_CONFIG: dict
with open('bot-server-config.json') as f:
    BOT_SERVER_CONFIG = json.load(f)