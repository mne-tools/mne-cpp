import threading
import time
import random

class FieldLineService:
    class SensorApi:
        def __init__(self):
            self.sleep_mean = 1
            self.sleep_var = .2
            self.prob_success = 0.9
 
    def __init__(self, ip_list, prefix=""):
        """
            callback - required callback class
                       should be of type FieldLineCallback
        """
        print("Initializing FieldLine Service")
        self.is_open = False

        self.num_sensors_per_chassis = 16
        self.num_chassis = 2

        self.ip_list = ip_list
        self.num_chassis = len(self.ip_list) 
        # print("IP list: " + str(self.ip_list))

        self.sensors = {i: list(range(1, self.num_sensors_per_chassis + 1)) for i in range(0, self.num_chassis) }
        self.sensor_api = self.SensorApi()

        self.data_random_mean = 0
        self.data_random_var = 1000
        self.sampling_frequency = 1000
        self.callback_function = self.callback_function_default
        self.continue_data_acquisition = False
        self.data_acquisition_thread = threading.Thread(target=self.data_acquisition)

    def callback_function_default(self, _):
        pass

    def __enter__(self):
        self.open()
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        self.close()

    def open(self):
        self.is_open = True
        print("FieldLineService Open")
        print("Connecting to devices in ip_list: " + str(self.ip_list))

    def close(self):
        self.is_open = False
        print("FieldLineService Close.")
        print("Disconnecting.")

    def load_sensors(self):
        if self.is_open:
            print("Loading sensors.")
            return self.sensors
        else:
            print("Fieldline service closed.")

    def get_chassis_list(self):
        if self.is_open:
            print("Get chassis list")
            return list(range(0, len(self.ip_list)))
        else:
            print("Fieldline service closed.")
            return None

    def generate_data(self):
        timestamp = int(time.time())

        data_frames = {}

        num_sensors = sum(len(sensor_list) for sensor_list in self.sensors.values())

        data_values = [91599] + [round(random.normalvariate(self.data_random_mean, self.data_random_var))
                for _ in range(1, num_sensors + 1)]

        chassis_labels = [0] + [chassis for chassis, sensors in self.sensors.items() for _ in sensors]
        sensor_labels = [0] + [sensor for sensor_list in self.sensors.values() for sensor in sensor_list]
                                
        data_type_labels = [0] + ([50] * num_sensors)
        global_labels = [f'{chassis_label:02}:{sensor_label:02}:{data_type:02}'
                         for chassis_label, sensor_label, data_type in zip(chassis_labels, sensor_labels, data_type_labels)]

        for global_l, data_i, sensor_l, chassis_l, data_type_l in \
                zip(global_labels, data_values, sensor_labels, chassis_labels, data_type_labels):
            data_frames[global_l] = \
                {'data': data_i, 'sensor': f'{chassis_l:02}:{sensor_l:02}', 'sensor_id': sensor_l, 'data_type': data_type_l}

        return {'timestamp': timestamp, 'data_frames': data_frames}

    def data_acquisition(self):
        sampling_period = 1/self.sampling_frequency

        start_time = time.time()
        data = self.generate_data()
        end_time = time.time()
        elapsed_time = end_time - start_time
        time.sleep(sampling_period - elapsed_time)
        end_time = time.time()

        elapsed_time = end_time - start_time
        elapsed_time_diff = (sampling_period - elapsed_time)

        while(self.continue_data_acquisition):
            start_time = time.time()
            data = self.generate_data()
            self.callback_function(data)
            end_time = time.time()
            elapsed_time = end_time - start_time
            # time_to_sleep = max(0, .001 - elapsed_time)
            time.sleep(sampling_period + elapsed_time_diff - 0.6 * elapsed_time)
            # print(f"elapsed_time: {elapsed_time:04}")
        # start_time = time.time()
        # end_time = time.time()
        # time_meas = end_time - start_time
        #
        # start_time = time.time()
        # data = self.generate_data()
        # self.callback_function(data)
        # time.sleep(sampling_period)
        # end_time = time.time()
        #
        # time_diff = end_time - start_time - sampling_period - time_meas
        # time_to_sleep_adjusted = sampling_period - 1.5 * time_diff
        # time_to_sleep_final = 0 if time_to_sleep_adjusted < 0 else time_to_sleep_adjusted
        # print(time_to_sleep_final)
        #
        # while(self.continue_data_acquisition):
        #     data = self.generate_data()
        #     self.callback_function(data)
        #     time.sleep(time_to_sleep_final)

    def read_data(self, data_callback=None):
        if self.is_open:
            if(data_callback is not None):
                self.callback_function = data_callback
                print(f"Data callback set to: {data_callback.__name__}.")
            else:
                self.callback_function = self.callback_function_default
                print(f'Data callback disabled.')
        else:
            print("Fieldline service closed.")
            return None

    def start_adc(self, _):
        """
            Start ADC from chassis

            chassis_id - unique ID of chassis
        """
        if self.is_open:
            self.continue_data_acquisition = True
            self.data_acquisition_thread.start()
            print("Starting data acquisition.")
        else:
            print("Fieldline service closed.")
            return None

    def stop_adc(self, chassis_id):
        """
            Stop ADC from chassis

            chassis_id - unique ID of chassis
        """
        if self.is_open:
            self.continue_data_acquisition = False
            if self.data_acquisition_thread.is_alive():
                self.data_acquisition_thread.join(timeout=5)
                print("Stopping data acquisition.")
            else:
                print("Data acquisition running.")
        else:
            print("Fieldline service closed.")
            return None


    def turn_off_sensors(self, sensor_dict):
        """
            Turn off sensors
            Note: can be performed anytime

            sensor_dict - dictionary of chassis id to list of sensors ex. {0: [0,1], 1:[0,1]}
        """
        print("Turning off sensors.")

    def sensors_api(self, sensor_dict, on_next=None, on_error=None, on_completed=None):
        def sensor_thread(chassis_id: int, sensor_id: int):
            sleep_time = random.normalvariate(self.sensor_api.sleep_mean, self.sensor_api.sleep_var)
            if sleep_time <= 0:
                sleep_time = 0.1
            time.sleep(sleep_time)
            result = random.random()
            if result <= self.sensor_api.prob_success:
                if on_next:
                    on_next(chassis_id, sensor_id)
            else:
                codes = [117, 31337, 2626]
                if on_error:
                    on_error(chassis_id, sensor_id, random.choice(codes))

        threads = []
        for chassis_id, sensor_ids in sensor_dict.items():
            for sensor_id in sensor_ids:
                t = threading.Thread(target=sensor_thread, args=(chassis_id, sensor_id))
                t.start()
                threads.append(t)

        for t in threads:
            t.join()

        if on_completed:
            on_completed()

    def restart_sensors(self, sensor_dict, on_next=None, on_error=None, on_completed=None):
        """
            Restart the sensors
            Note: can be performed anytime

            sensor_dict - dictionary of chassis id to list of sensors ex. {0: [0,1], 1:[0,1]}
            on_next - callback when sensor succeeds a step
            on_error - callback when sensor fails a step
            on_completed - callback when all sensors have either succeeded or failed
        """
        print("Restarting sensors.")
        self.sensors_api(sensor_dict, on_next, on_error, on_completed)


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
        self.sensors_api(sensor_dict, on_next, on_error, on_completed)

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
        self.sensors_api(sensor_dict, on_next, on_error, on_completed)

    def set_bz_wave(self, chassis_id, sensor_id, wave_type, freq=None, amplitude=None):
        """
            Apply a known magnetic field to the BZ coil (e.g. sine wave)

            chassis_id - unique ID of chassis
            sensor_id - unique ID of sensor
            wave_type - FieldLineWaveType (WAVE_OFF, WAVE_RAMP, WAVE_SINE)
            freq - frequency of wave
            amplitude - amplitude of wave (nT)
        """
        print('Setting bz wave')
        print('UuuUUUuuuuu...')
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

