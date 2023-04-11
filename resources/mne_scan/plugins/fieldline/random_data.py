import random


def generate_data():
    timestamp = 179331926
    num_sensors_per_chassis = 16
    num_chassis = 2

    data_frames = {}

    data_values = [91599] + \
        [round(random.normalvariate(0, 1000))
            for _ in range(1, num_sensors_per_chassis * num_chassis + 1)]

    chassis_labels = [0] + [num for num in range(0, num_chassis) for _ in range(num_sensors_per_chassis)]
    sensor_labels = [0] + (list(range(1, num_sensors_per_chassis + 1))) * num_chassis
    data_type_labels = [0] + ([50] * num_sensors_per_chassis * num_chassis)
    global_labels = [f'{chassis_label:02}:{sensor_label:02}:{data_type:02}'
                     for chassis_label, sensor_label, data_type in zip(chassis_labels, sensor_labels, data_type_labels)]

    for global_l, data_i, sensor_l, chassis_l, data_type_l in \
            zip(global_labels, data_values, sensor_labels, chassis_labels, data_type_labels):
        data_frames[global_l] = \
            {'data': data_i, 'sensor': f'{chassis_l:02}:{sensor_l:02}', 'sensor_id': sensor_l, 'data_type': data_type_l}


    # data_frames = []
    return {'timestamp': timestamp, 'data_frames': data_frames}


data = generate_data()
print(data)
