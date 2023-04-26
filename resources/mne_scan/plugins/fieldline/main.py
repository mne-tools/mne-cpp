"""This is my super module"""
import sys
from fieldline_api_mock.fieldline_service import FieldLineService 
import time

for path in sys.path:
    print(path)

print("sys.executable: " + str(sys.executable))

print("hello pepe vamos vamos!!!")

ip_list = ["8.8.8.8", "9.9.9.9"]

# fl = FieldLineService(["8.8.8.8", "9.9.9.9"])
# with FieldLineService(ip_list) as service: 
#     done = False
#     # Get dict of all the sensors
#     sensors = service.load_sensors()
#     print(f"Got sensors: {sensors}")
#     # Make sure closed loop is set
#     service.set_closed_loop(True)
#     print("Doing sensor restart")
#     # Do the restart
#     service.restart_sensors(sensors, on_next=lambda c_id, s_id: print(f'sensor {c_id}:{s_id} finished restart'), on_error=lambda c_id, s_id, err: print(f'sensor {c_id}:{s_id} failed with {hex(err)}'), on_completed=lambda: call_done())
#     while not done:
#         time.sleep(0.5)

