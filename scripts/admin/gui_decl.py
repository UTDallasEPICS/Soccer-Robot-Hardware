# Robot Window
class GuiRobot:
    window: int
    ip_box: int
    mac_text: int
    status_text: int
    conn_status_text: int
    connect_button: int
    disconnect_button: int

class GuiControl:
    enable_ir_led_button: int
    disable_ir_led_button: int

class GuiConfig:
    window: int
    
    # Tuple of (json config, current value, editable box handle)
    configs: list[(object, object, int)] = []
    sync_button: int