import time
import threading

time_to_sleep = 0.5
keep_running = True
counter = 0


# default callback function that prints its name
def default_callback1():
    # print("... default_callback")
    global counter
    print(f'Counter: {counter}')
    counter += 1


def default_callback2():
    with open("output.txt", "a") as f:
        f.write("default_callback2\n")


callback = default_callback1


# function to set the callback
def set_callback(new_callback=None):
    global callback
    if new_callback is None:
        print('Setting the default callback.')
        callback = default_callback2
    else:
        print('Setting new callback.')
        callback = new_callback


# function to call the callback every second
def run():
    global keep_running
    global time_to_sleep
    global counter
    keep_running = True
    while keep_running:
        callback()
        time.sleep(time_to_sleep)


# function to stop the callback loop
def stop():
    global keep_running
    global counter
    keep_running = False
    print(f'Global counter: {counter}')


def start():
    t = threading.Thread(target=run)
    t.start()


# if __name__ == '__main__':
#     pass
#
 
