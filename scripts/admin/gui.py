import dearpygui.dearpygui as dpg

from admin.robot_connection import DEFAULT_IP, RobotConnection
from admin.gui_decl import GuiRobot, GuiConfig
from admin.config import ConfigType, BOT_SERVER_CONFIG

def draw_robot_window():
    global robot_ip_box, robot_mac_text

    GuiRobot.window = dpg.generate_uuid()
    with dpg.window(label="Robot Connection", tag=GuiRobot.window):
        dpg.add_text("Robot Connection Information")
        GuiRobot.ip_box = dpg.add_input_text(label="IP", default_value=DEFAULT_IP)

        def connect_to_robot(sender, app_data, user_data):
            RobotConnection.active = RobotConnection(dpg.get_value(user_data))

        def disconnect_robot(sender, app_data, user_data):
            RobotConnection.active.disconnect()
            RobotConnection.active = None

        GuiRobot.connect_button = dpg.add_button(label="Connect", callback=connect_to_robot, user_data=GuiRobot.ip_box)

        GuiRobot.disconnect_button = dpg.add_button(label="Disconnect", callback=disconnect_robot)

        GuiRobot.mac_text = dpg.add_text("Robot MAC Address: Empty")
        GuiRobot.conn_status_text = dpg.add_text("Connection Status: Empty")

    GuiConfig.window = dpg.generate_uuid()
    with dpg.window(label="Robot Config", tag=GuiConfig.window, autosize=True, pos=(250, 0)):
        for i in BOT_SERVER_CONFIG['botConfigSchema']:
            print('itr', i)
            entry = [i, i['default_value'], dpg.add_input_text(label=i['name'], default_value=i['default_value'])]
            GuiConfig.configs.append(entry)
        
        def sync_new_config(sender, app_data, user_data):
            for index, i in enumerate(GuiConfig.configs):
                if dpg.get_value(i[2]) != i[1]:
                    # Update last set value
                    i[1] = dpg.get_value(i[2])

                    t = i[0]["type"]
                    val = i[1]

                    packet = f'{{"type":"SET_VAR","key":"{index}","val":{val},"var_type":"{t}"}};'
                    print('Sending config update', packet)
                    RobotConnection.active.send(packet)
        
        GuiConfig.sync_button = dpg.add_button(label="Sync Config", callback=sync_new_config)

    with dpg.window(label="Override PWM", autosize=True, pos=(650, 0)):
        left = dpg.add_input_text(label='Left', default_value=5957)
        right = dpg.add_input_text(label='Right', default_value=5957)
        def send(sender, app_data, user_data):
            leftPwm = int(dpg.get_value(left))
            rightPwm = int(dpg.get_value(right))
            if dpg.get_item_label(sender) != 'Send':
                adj = int(dpg.get_item_label(sender))
                dpg.set_value(left, leftPwm + adj)
                dpg.set_value(right, rightPwm + adj)

            packet = f'{{"type":"OVERRIDE","left":{leftPwm},"right":{rightPwm}}};'
            print('Sending config update', packet)
            RobotConnection.active.send(packet)
        dpg.add_button(label="Send", callback=send)

        dpg.add_button(label='-1000', callback=send)
        dpg.add_button(label='+1000', callback=send)
        dpg.add_button(label='-100', callback=send)
        dpg.add_button(label='+100', callback=send)
        dpg.add_button(label='-10', callback=send)
        dpg.add_button(label='+10', callback=send)
        dpg.add_button(label='-1', callback=send)
        dpg.add_button(label='+1', callback=send)