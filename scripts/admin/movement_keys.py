import dearpygui.dearpygui as dpg

KEY_W = 87
KEY_A = 65
KEY_S = 83
KEY_D = 68

is_w_held = None
is_a_held = None
is_s_held = None
is_d_held = None

conn = None

def key_down_callback(sender, app_data, user_data):
    global is_w_held, is_a_held, is_s_held, is_d_held, conn

    if app_data[1] == 0: # when first pressed
        print(f"Key pressed: {app_data}")

        if app_data[0] == KEY_W:
            is_w_held = True
            conn.send('{"type":"DRIVE_TANK","left":1,"right":1};')
        elif app_data[0] == KEY_A:
            is_a_held = True
            conn.send('{"type":"DRIVE_TANK","left":-1,"right":1};')
        elif app_data[0] == KEY_S:
            is_s_held = True
            conn.send('{"type":"DRIVE_TANK","left":-1,"right":-1};')
        elif app_data[0] == KEY_D:
            is_d_held = True
            conn.send('{"type":"DRIVE_TANK","left":1,"right":-1};')

def key_up_callback(sender, app_data, user_data):
    global is_w_held, is_a_held, is_s_held, is_d_held, conn

    print(f"Key pressed: {app_data}")
    if app_data in [KEY_W, KEY_A, KEY_S, KEY_D]:
        if app_data == KEY_W:
            is_w_held = False
        elif app_data == KEY_A:
            is_a_held = False
        elif app_data == KEY_S:
            is_s_held = False
        elif app_data == KEY_D:
            is_d_held = False
        
        if not is_w_held and not is_a_held and not is_s_held and not is_d_held:
            conn.send('{"type":"DRIVE_TANK","left":0,"right":0};')

def bind_movement_keys(robotConn):
    global conn
    conn = robotConn

    with dpg.handler_registry():
        dpg.add_key_down_handler(callback=key_down_callback)
        dpg.add_key_release_handler(callback=key_up_callback)