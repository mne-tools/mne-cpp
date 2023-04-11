import threading
import random_data
import time

class FieldLineService:
    def __init__(self, ip_list, prefix=""):
        """
            callback - required callback class
                       should be of type FieldLineCallback
        """
        self.counter_lock = 0
        self.hardware_state = 0
        print("Initializing FieldLine Service")
        self.prefix = "prefix"
        self.data_source = None
        # interfaces = netifaces.interfaces()
        self.network_interfaces = []
        self.ip_list = ip_list
        self.continue_data_acquisition = False
        self.data_acquisition_thread = None
        # for i in interfaces:
        #     print("interface %s" % i)
        #     if i.startswith('lo'):
        #         continue
        #     iface = netifaces.ifaddresses(i).get(netifaces.AF_INET)
        #     if iface is not None:
        #         for j in iface:
        #             ip_addr = j['addr']
        #             print("address %s" % ip_addr)
        #             self.network_interfaces.append(ip_addr)
        # print("network interfaces: %s" % self.network_interfaces)

    def __enter__(self):
        self.open()
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        self.close()

    def open(self):
        print("connecting to devices in ip_list: " + str(self.ip_list))
        print("connecting to devices")
        print("start monitor")
        print("start data source")

    def close(self):
        print("data source shut down")

    def load_sensors(self):
        print("loading sensors")
        # return self.data_source.load_sensors()
        self.list_of_sensors = {i: list(range(1, 16 + 1)) for i in range(0, 1 + 1)}

        return self.list_of_sensors

    def get_chassis_list(self):
        print("get chassis list")
        # return self.data_source.get_chassis_list()

    def data_acquisition(self):
    
        start_time = time.time()
        data = random_data.generate_data()
        end_time = time.time()
        elapsed_time = end_time - start_time
        time.sleep(0.001 - elapsed_time)
        end_time = time.time()

        elapsed_time = end_time - start_time
        elapsed_time_diff = (0.001 - elapsed_time)

        while(self.continue_data_acquisition):
            start_time = time.time()
            data = random_data.generate_data()
            self.callback_function(data)
            end_time = time.time()
            elapsed_time = end_time - start_time
            # time_to_sleep = max(0, .001 - elapsed_time)
            time.sleep(0.001 + elapsed_time_diff - 0.5 * elapsed_time)
            # print(f"elapsed_time: {elapsed_time:04}")

    def read_data(self, data_callback=None):
        if(data_callback is not None):
            print(f"read_data defined. Callback set to : {data_callback.__name__}")
            self.callback_function = data_callback
        # self.data_source.read_data(data_callback=data_callback)


    def start_adc(self, chassis_id):
        """
            Start ADC from chassis

            chassis_id - unique ID of chassis
        """
        print("start adc")
        self.continue_data_acquisition = True
        self.data_acquisition_thread = threading.Thread(target=self.data_acquisition)
        self.data_acquisition_thread.start()
        # self.data_source.start_adc(chassis_id)

    def stop_adc(self, chassis_id):
        """
            Stop ADC from chassis

            chassis_id - unique ID of chassis
        """
        print("stop adc")
        self.continue_data_acquisition = False
        self.data_acquisition_thread.join(timeout=5)
        # self.data_source.stop_adc(chassis_id)

    def turn_off_sensors(self, sensor_dict):
        """
            Turn off sensors
            Note: can be performed anytime

            sensor_dict - dictionary of chassis id to list of sensors ex. {0: [0,1], 1:[0,1]}
        """
        print("turn off sensors")
        # self.data_source.send_logic_command(sensor_dict, proto.LogicMessage.LOGIC_SENSOR_OFF)

    def restart_sensors(self, sensor_dict, on_next=None, on_error=None, on_completed=None):
        """
            Restart the sensors
            Note: can be performed anytime

            sensor_dict - dictionary of chassis id to list of sensors ex. {0: [0,1], 1:[0,1]}
            on_next - callback when sensor succeeds a step
            on_error - callback when sensor fails a step
            on_completed - callback when all sensors have either succeeded or failed
        """
        print("restart sensors")
        time.sleep(.3)
        on_completed()
        # self.data_source.set_callbacks(on_next=on_next, on_error=on_error, on_completed=on_completed)
        # self.data_source.send_logic_command(sensor_dict, proto.LogicMessage.LOGIC_SENSOR_RESTART)

    def coarse_zero_sensors(self, sensor_dict, on_next=None, on_error=None, on_completed=None):
        """
            Coarse zero sensor
            Note: can only be performed after restart is complete

            sensor_dict - dictionary of chassis id to list of sensors ex. {0: [0,1], 1:[0,1]}
            on_next - callback when sensor succeeds a step
            on_error - callback when sensor fails a step
            on_completed - callback when all sensors have either succeeded or failed
        """
        print("coarse zero sensor")
        time.sleep(.3)
        on_completed()
        # self.data_source.set_callbacks(on_next=on_next, on_error=on_error, on_completed=on_completed)
        # self.data_source.send_logic_command(sensor_dict, proto.LogicMessage.LOGIC_SENSOR_COARSE_ZERO)

    def fine_zero_sensors(self, sensor_dict, on_next=None, on_error=None, on_completed=None):
        """
            Fine zero sensor
            Note: can only be performed after coarse zero is complete

            sensor_dict - dictionary of chassis id to list of sensors ex. {0: [0,1], 1:[0,1]}
            on_next - callback when sensor succeeds a step
            on_error - callback when sensor fails a step
            on_completed - callback when all sensors have either succeeded or failed
        """
        print("fine zero sensors")
        time.sleep(.3)
        on_completed()
        # self.data_source.set_callbacks(on_next=on_next, on_error=on_error, on_completed=on_completed)
        # self.data_source.send_logic_command(sensor_dict, proto.LogicMessage.LOGIC_SENSOR_FINE_ZERO)

    # def set_bz_wave(self, chassis_id, sensor_id, wave_type, freq=None, amplitude=None):
    #     """
    #         Apply a known magnetic field to the BZ coil (e.g. sine wave)
    #
    #         chassis_id - unique ID of chassis
    #         sensor_id - unique ID of sensor
    #         wave_type - FieldLineWaveType (WAVE_OFF, WAVE_RAMP, WAVE_SINE)
    #         freq - frequency of wave
    #         amplitude - amplitude of wave (nT)
    #     """
    #
    #     if wave_type == FieldLineWaveType.WAVE_OFF:
    #         self.data_source.set_wave_off(chassis_id, sensor_id)
    #     elif wave_type == FieldLineWaveType.WAVE_RAMP:
    #         self.data_source.set_wave_ramp(chassis_id, sensor_id, freq, amplitude)
    #     elif wave_type == FieldLineWaveType.WAVE_SINE:
    #         self.data_source.set_wave_sine(chassis_id, sensor_id, freq, amplitude)

    def set_closed_loop(self, enable):
        """
            Turn closed loop on or off

            enable - True or False for on or off
        """
        print("set closed loop")
        # self.data_source.set_closed_loop(enable)

    def get_fields(self, chassis_id, sensor_id):
        """
            Get the fields for a sensor

            chassis_id - unique ID of chassis
            sensor_id - unique ID of sensor
        """
        print("get fields")
        # for s in self.data_source.get_sensors():
        #     if s.chassis_id == chassis_id and s.sensor_id == sensor_id:
        #         return s.fields
        # return None

    def get_serial_numbers(self, chassis_id, sensor_id):
        """
            Get the card and sensor serial number for a sensor

            chassis_id - unique ID of chassis
            sensor_id - unique ID of sensor
        """
        print("get serial numbers")
        # for s in self.data_source.get_sensors():
        #     if s.chassis_id == chassis_id and s.sensor_id == sensor_id:
        #         return (s.card_serial,s.sensor_serial)

    def get_chassis_serial_number(self, chassis_id):
        """
            Get the chassis serial number

            chassis_id - unique ID of chassis
        """
        print("get chassis serial number")
        # return self.data_source.get_chassis_serial(chassis_id)

    def get_version(self, chassis_id):
        """
            Get the build version

            chassis_id - unique ID of chassis
        """
        print("get version")
        # return self.data_source.get_chassis_version(chassis_id)

    def get_sensor_state(self, chassis_id, sensor_id):
        """
            Get the current state of the sensor

            chassis_id - unique ID of chassis
            sensor_id  - ID of sensor
        """
        print("get sensor state")
        # return self.data_source.get_sensor_state(chassis_id, sensor_id)

    def get_calibration_value(self, ch_name):
        """
            Get the calibration value for a channel

            ch_name - channel name in chassis:sensor:datatype format
        """
        print("get calibration value")
        # channel_dict = self.hardware_state.get_channel_dict()
        # return channel_dict[ch_name] if ch_name in channel_dict else 1.0

