
import atexit
import argparse
from functools import partial
import pathlib
import pyudev
import re
import os
import sys
import signal
import stat
import struct
import threading
import time

subscriber_list = set()
is_verbose = False

def show_threads():
    print(f'Thread stats: {threading.active_count()}')
    for thread in threading.enumerate():
        s = f' - Thread [{thread.native_id}]'
        if thread.name:
            s += ': ' + thread.name
        print(s)


class ShowThreadsThread(threading.Thread):
    def __init__(self, sleep = 0, *args, **kwargs):
        threading.Thread.__init__(self, *args, **kwargs)
        self.__sleep = sleep

    def run(self):
        if self.__sleep > 0:
            time.sleep(self.__sleep)
        show_threads()


class AltShiftNotify(threading.Thread):
    input_event_template = 'llHHi'
    input_event_length = struct.calcsize(input_event_template)

    def __init__(self, input: str, manager, *args, **kwargs):
        threading.Thread.__init__(self, *args, **kwargs)
        self.name = 'Alt+Shift Notifier: ' + input
        self.daemon = True
        self.__input = input
        self.__manager = manager

    def run(self):
        with open(self.__input, 'rb', buffering=0) as file:
            alt_state = False
            shift_state = False
            fail_state = False
            group_state = False
            while True:
                try:
                    buffer = file.read(AltShiftNotify.input_event_length)
                except:
                    manager.on_thread_exit_notify(self.__input)
                    return
                has_change = False
                other_key = False
                data = struct.unpack(AltShiftNotify.input_event_template, buffer)
                if data[2] == 1:
                    if data[3] == 56:
                        has_change = True
                        if data[4] == 0:
                            alt_state = False
                        else:
                            alt_state = True
                    elif data[3] == 42:
                        has_change = True
                        if data[4] == 0:
                            shift_state = False
                        else:
                            shift_state = True
                    else:
                        other_key = True
                if has_change:
                    if alt_state and shift_state:
                        group_state = True
                    elif group_state:
                        group_state = False
                        if not fail_state:
                            send_notification()
                    if not alt_state and not shift_state:
                        fail_state = False
                elif other_key and group_state:
                    fail_state = True

def send_notification():
    if is_verbose:
        print('Alt+Shift detected')
    for pid in subscriber_list:
        if is_verbose:
            print(f'Sending SIGUSR1 to {pid}')
        try:
            os.kill(pid, signal.SIGUSR1)
        except ProcessLookupError:
            subscriber_list.remove(pid)

def enumate_keyboard_event_devices(context: pyudev.Context):
    devices = []
    for device in context.list_devices():
        if 'ID_INPUT_KEYBOARD' in device:
            m = re_is_event_path.match(device.sys_path)
            if m is not None:
                p = '/dev/input/' + m[2]
                try:
                    s = os.stat(p)
                except FileNotFoundError:
                    continue
                if stat.S_ISCHR(s.st_mode):
                    devices.append(p)
    return devices


class KeyboardThreadManager:
    re_is_event_path = re.compile('^(.*)/(event[0-9]+)$')

    def __init__(self, context: pyudev.Context = pyudev.Context()):
        self._context = context
        self._map = {}
        self._map_lock = threading.RLock()
        self._monitor = pyudev.Monitor.from_netlink(context)
        self._monitor.filter_by('input')
        self._observer = observer = pyudev.MonitorObserver(self._monitor, self._on_device_change, name='DeviceChangeMonitorChange')
        self._observer.start()
        self.update_devices()

    def enumerate_keyboard_event_devices(self):
        devices = []
        for device in self._context.list_devices():
            if 'ID_INPUT_KEYBOARD' in device:
                m = KeyboardThreadManager.re_is_event_path.match(device.sys_path)
                if m is not None:
                    p = '/dev/input/' + m[2]
                    try:
                        s = os.stat(p)
                    except FileNotFoundError:
                        continue
                    if stat.S_ISCHR(s.st_mode):
                        devices.append(p)
        return devices

    def update_devices(self):
        devices = self.enumerate_keyboard_event_devices()
        with self._map_lock:
            for device in self._map.keys():
                if device not in devices:
                    if self._map[device].is_alive():
                        signal.pthread_kill(self._map[device].get_native_id(), signal.SIGKILL)
                        self._map[device].join()
                        del self._map[device]
                        if is_verbose:
                            print(f'Killed Alt+Shift Thread for {device}')
                            show_threads()
            for device in devices:
                if device not in self._map:
                    thread = self._map[device] = AltShiftNotify(device, self)
                    thread.start()
                    if is_verbose:
                        print(f'Started Alt+Shift Thread for {device}')
                        show_threads()

    def _on_device_change(self, *args, **kwargs):
        self.update_devices()

    def on_thread_exit_notify(self, device):
        with self._map_lock:
            if device in self._map:
                del self._map[device]
                if is_verbose:
                    print(f'Notification for exit from {device}')
                    ShowThreadsThread(0.1).start()
        self.update_devices()

pid_file = None

@atexit.register
def on_exit():
    if pid_file is not None:
        try:
            os.remove(pid_file)
        except:
            pass

class PIDFileCreator(threading.Thread):
    def __init__(self, pid_file: pathlib.Path, wait = 0.1, *args, **kwargs):
        threading.Thread.__init__(self, *args, **kwargs)
        self.__pid_file = pid_file
        self.__wait = wait

    def run(self):
        if self.__wait > 0:
            time.sleep(self.__wait)
        self.__pid_file.parent.mkdir(parents=True, exist_ok=True)
        with self.__pid_file.open('w') as file:
            file.write(str(os.getpid()))   
        if is_verbose:     
            print(f'Updated {self.__pid_file} to {os.getpid()}')

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Subscription service for Alt+Shift language switch')
    parser.add_argument('--pid-file', '-p', type=pathlib.Path, default=pathlib.Path('/var/run/alt-shift-notify/service.pid'))
    parser.add_argument('--verbose', dest='verbose', action='store_true')
    args = parser.parse_args()
    if not args.pid_file.is_absolute():
        args.pid_file = pathlib.Path(pathlib.Path.cwd(), args.pid_file)
    pid_file = args.pid_file
    if args.verbose:
        is_verbose = True
    context = pyudev.Context()
    manager = KeyboardThreadManager(context)
    threading.current_thread().name = 'SubscriberThread'
    signal.pthread_sigmask(signal.SIG_BLOCK, [signal.SIGUSR1, signal.SIGUSR2])
    pid_file_creator = PIDFileCreator(pid_file=pid_file)
    pid_file_creator.start()
    while True:
        received = signal.sigwaitinfo([signal.SIGUSR1, signal.SIGUSR2, signal.SIGINT, signal.SIGTERM])
        if received.si_signo == signal.SIGUSR1:
            if is_verbose:
                print(f'Adding subscriber: {received.si_pid}')
            subscriber_list.add(received.si_pid)
        elif received.si_signo == signal.SIGUSR2:
            if is_verbose:
                print(f'Removing subscriber: {received.si_pid}')
            subscriber_list.remove(received.si_pid)
        elif received.si_signo == signal.SIGTERM or received.si_signo == signal.SIGINT:
            sys.exit(0)
