import pip

def import_or_install(package):
    try:
        return __import__(package)
    except ImportError:
        pip.main(['install', package])
        return __import__(package)

dearpygui = import_or_install("dearpygui")
import dearpygui.dearpygui as dpg

pygame = import_or_install("pygame")

import admin.gui as gui
from admin.robot_connection import RobotConnection, ConnStatus

dpg.create_context()

gui.draw_robot_window()

dpg.create_viewport(title='ChessBot Local Admin', width=1300, height=700)
dpg.set_viewport_vsync(True)
dpg.setup_dearpygui()
dpg.show_viewport()

try:
    pygame.init()
    joystick = pygame.joystick.Joystick(0)
    joystick.init()
except:
    joystick = None

# Gamepad input and history
controller = [0, 0]
controller_prev = [0, 0]

frame = 0
while dpg.is_dearpygui_running():
    if RobotConnection.active is not None:
        RobotConnection.active.tick()

    if joystick is not None:
        for event in pygame.event.get():
            if event.type == pygame.JOYAXISMOTION:
                if event.axis == 1: #LY
                    controller[0] = -event.value
                if event.axis == 3: #RY
                    controller[1] = -event.value
        
        controller = [0 if abs(i) < 0.1 else i for i in controller]

        if frame % 15 == 0:
            if controller != controller_prev and RobotConnection.active is not None and RobotConnection.active.status == ConnStatus.CONNECTED:
                RobotConnection.active.send(f'{{"type":"DRIVE_TANK","left":{controller[0]},"right":{controller[1]}}};')
            controller_prev = [controller[0], controller[1]]

    dpg.render_dearpygui_frame()

    frame += 1

dpg.destroy_context()