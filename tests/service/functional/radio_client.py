import dbus

from dbus.mainloop.glib import DBusGMainLoop
from time import sleep

import FMRadio


class Client:
    """ This is a client app to test that the fm-radio-service properly
    monitors its clients' lifetime"""
    def __init__(self):
        DBusGMainLoop(set_as_default=True)
        self.bus = dbus.SessionBus()
        self.service = FMRadio.Service(self.bus)
        self.tuner = None

    def open_tuner(self):
        if self.tuner:
            return
        path = self.service.open_tuner()
        self.tuner = FMRadio.Tuner(self.bus, path)

    def run(self):
        while True:
            try:
                line = input()
            except EOFError as error:
                break
            line = line.strip()
            if line == 'start':
                self.open_tuner()
                self.tuner.start()
                print('OK')
            elif line == 'stop':
                self.open_tuner()
                self.tuner.stop()
                print('OK')
            elif line.startswith('steal '):
                path = line[6:]
                self.tuner = FMRadio.Tuner(self.bus, path)
            # Handle other commands here:
            # elif line.startswith(...):


def main():
    client = Client()
    client.run()


if __name__ == "__main__":
    main()
